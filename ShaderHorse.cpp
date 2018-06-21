#include "ShaderHorse.h"
#include <algorithm>
#include <fstream>
#include <sstream>

extern LOGERRPROC errorLog;

namespace {

std::string ShaderString(GLsizei count, const GLchar **string, const GLint *length) {
  std::string result;
  if (!string)
    return result;

  for (GLsizei i = 0; i < count; ++i) {
    if (length)
      result += std::string((const char*)string[i], length[i]);
    else
      result += std::string((const char*)string[i]);
  }
  return result;
}

typedef void (GLAPIENTRY *glGetivT) (GLuint, GLenum, GLint*);
typedef void (GLAPIENTRY *glGetInfoLogT) (GLuint, GLsizei, GLsizei*, GLchar*);

const char *InfoLog(glGetivT glGetiv, glGetInfoLogT glGetInfoLog, GLuint name) {
  GLint infologLength = 0;
  glGetiv(name, GL_INFO_LOG_LENGTH, &infologLength);

  if (infologLength > 0) {
    static GLchar infoLog[512];
    GLsizei charsWritten = 0;
    #undef min
    glGetInfoLog(name, std::min(infologLength, (GLsizei)512), &charsWritten, infoLog);
    if (charsWritten != 0) {
      return (const char*)infoLog;
    }
  }
  return nullptr;
}

}

ShaderHorse::ShaderHorse(InterceptPluginCallbacks *gli_callbacks)
  : gli_callbacks_(gli_callbacks) {}

ShaderHorse::~ShaderHorse() {}

void ShaderHorse::LoadFunctions() {
  valid_ = false;
  if (!gli_callbacks_)
    return;

#define GetGLIFunction(name) \
  i##name = (decltype(i##name))gli_callbacks_->GetGLFunction(#name);\
  if (!i##name) {\
    errorLog("ShaderHorse: Cannot get "#name);\
    return;\
  }

  GetGLIFunction(glCreateShader);
  GetGLIFunction(glShaderSource)
  GetGLIFunction(glCompileShader)
  GetGLIFunction(glAttachShader)
  GetGLIFunction(glDetachShader)
  GetGLIFunction(glLinkProgram)
  GetGLIFunction(glDeleteShader)
  GetGLIFunction(glIsProgram)
  GetGLIFunction(glGetShaderiv)
  GetGLIFunction(glGetShaderInfoLog)
  GetGLIFunction(glGetProgramiv)
  GetGLIFunction(glGetProgramInfoLog)

  #undef GetGLIFunction

  valid_ = true;
}

void ShaderHorse::glCreateShaderPre(GLenum shaderType) {
  if (!valid_)
    return;

  if (state_.shader_creation_state != GL_NONE) {
    errorLog("ShaderHorse: Wrong shader creation state in glCreateShaderPre");
    return;
  }

  // modify state
  state_.shader_creation_state = shaderType;
}

void ShaderHorse::glCreateShaderPost(GLuint shader) {
  if (!valid_ || !shader)
    return;

  if (state_.shader_creation_state == GL_NONE) {
    errorLog("ShaderHorse: Wrong shader creation state in glCreateShaderPost");
    return;
  }

  // modify state
  state_.shader_state[shader] = {state_.shader_creation_state, "", ""};
  state_.shader_creation_state = GL_NONE;
}

void ShaderHorse::glShaderSource(GLuint shader, GLsizei count, const GLchar **string,
  const GLint *length) {
  if (!valid_)
    return;

  // modify state
  auto it = state_.shader_state.find(shader);
  if (it != state_.shader_state.end())
    it->second.source = ShaderString(count, string, length);
}

void ShaderHorse::glCompileShader(GLuint shader) {
  if (!valid_)
    return;

  // modify state
  auto it = state_.shader_state.find(shader);
  if (it != state_.shader_state.end())
    it->second.compiled_source = it->second.source;
}

void ShaderHorse::glAttachShader(GLuint program, GLuint shader) {
  if (!valid_)
    return;

  // modify state
  auto it = state_.program_attach_state.find(program);
  if (it == state_.program_attach_state.end()) {
    state_.program_attach_state[program] = {shader};
    return;
  }

  for (auto s : it->second)
    if (s == shader)
      return;
  it->second.push_back(shader);
}

void ShaderHorse::glDetachShader(GLuint program, GLuint shader) {
  if (!valid_)
    return;

  // modify state
  auto it = state_.program_attach_state.find(program);
  if (it == state_.program_attach_state.end())
    return;

  auto pos = std::find(it->second.begin(), it->second.end(), shader);
  if (pos != it->second.end())
    it->second.erase(pos);
}

void ShaderHorse::glLinkProgram(GLuint program) {
  if (!valid_)
    return;

  // find all shader names currently atatched to this shader
  auto it = state_.program_attach_state.find(program);
  if (it == state_.program_attach_state.end())
    return;
  auto &shader_names = it->second;

  // create soldiers for shaders attached to this program
  std::vector<ShaderSoldier> soldiers;
  for (auto shader : shader_names) {
    auto it = state_.shader_state.find(shader);
    if (it == state_.shader_state.end()) {
      errorLog("ShaderHorse: Cannot find shader %d", (int)shader);
      return;
    }

    soldiers.emplace_back(program, it->second);

    // save to disk for editing
    soldiers.back().Save();
  }

  soldiers_[program] = std::move(soldiers);
}

void ShaderHorse::Deliver() {
  if (!valid_)
    return;

  for (auto it = soldiers_.begin(); it != soldiers_.end(); ++it) {
    GLuint program = it->first;
    auto &soldiers = it->second;

    // reload shaders from file
    for (auto &soldier : soldiers)
      if (!soldier.Load())
        errorLog("ShaderHorse: Failed to load shader for program %d", (int)program);

    Deliver(program, soldiers);
  }
}

void ShaderHorse::Deliver(GLuint program, const std::vector<ShaderSoldier> &soldiers) const {
  errorLog("Linking program %d", (int)program);

  if (!iglIsProgram(program)) {
    errorLog("ShaderHorse: %d is not a program", (int)program);
    return;
  }

  auto it = state_.program_attach_state.find(program);
  if (it == state_.program_attach_state.end()) {
    errorLog("ShaderHorse: Cannot find attachment state of program %d", (int)program);
    return;
  }

  // create and compile new shaders
  std::vector<GLuint> new_shaders;
  for (auto &soldier : soldiers) {
    GLuint new_shader = iglCreateShader(soldier.type);
    if (!new_shader) {
      errorLog("ShaderHorse: Failed to create shader of type %d", (int)soldier.type);
      goto cleanup;
    }

    new_shaders.push_back(new_shader);

    const char *source = soldier.source.c_str();
    iglShaderSource(new_shader, 1, (const GLchar**)&source, nullptr);
    iglCompileShader(new_shader);
    const char *info = InfoLog(iglGetShaderiv, iglGetShaderInfoLog, new_shader);
    if (info) {
      errorLog("ShaderHorse: Failed to compile shader:\n%s", info);
      goto cleanup;
    }
  }

  // detach current shaders and attach new shaders
  for (auto shader : it->second)
    iglDetachShader(program, shader);
  for (auto shader : new_shaders)
    iglAttachShader(program, shader);

  // link program
  iglLinkProgram(program);
  const char *info = InfoLog(iglGetProgramiv, iglGetProgramInfoLog, program);
  if (info)
    errorLog("ShaderHorse: Failed to link program %d:\n%s", program, info);

  // resume shader attachment and delete new shaders
  for (auto shader : new_shaders)
    iglDetachShader(program, shader);
  for (auto shader : it->second)
    iglAttachShader(program, shader);
cleanup:
  for (auto shader : new_shaders)
    iglDeleteShader(shader);
}

GLuint ShaderHorse::ShaderSoldier::id_ = 0;

ShaderHorse::ShaderSoldier::ShaderSoldier(GLuint program, const ShaderState &state) :
  type(state.type),
  source(state.compiled_source),
  program_(program) {
  unique_id_ = id_++;
}

ShaderHorse::ShaderSoldier::ShaderSoldier(ShaderSoldier &&other) :
  type(other.type),
  source(std::move(other.source)),
  program_(other.program_),
  unique_id_(other.unique_id_) {
  other.type = GL_NONE;
  other.program_ = 0;
}

ShaderHorse::ShaderSoldier::~ShaderSoldier() {
  std::remove(FileName().c_str());
}

void ShaderHorse::ShaderSoldier::Save() const {
  auto filename = FileName();
  std::ofstream out(filename);
  if (!out.good()) {
    errorLog("ShaderHorse: Cannot open file %s", filename.c_str());
    return;
  }

  out << source;
  out.close();
}

bool ShaderHorse::ShaderSoldier::Load() {
  auto filename = FileName();
  std::ifstream in(filename);
  if (!in.good()) {
    errorLog("ShaderHorse: Cannot open file %s", filename.c_str());
    return false;
  }

  source.assign(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
  in.close();
  return true;
}

std::string ShaderHorse::ShaderSoldier::FileName() const {
  const char *extension = "shader";
  switch (type) {
  case GL_COMPUTE_SHADER:
    extension = "comp";
    break;
  case GL_VERTEX_SHADER:
    extension = "vert";
    break;
  case GL_TESS_CONTROL_SHADER:
    extension = "ctrl";
    break;
  case GL_TESS_EVALUATION_SHADER:
    extension = "eval";
    break;
  case GL_GEOMETRY_SHADER:
    extension = "geom";
    break;
  case GL_FRAGMENT_SHADER:
    extension = "frag";
    break;
  }

  return std::to_string(program_) + "." + std::to_string(unique_id_) + "." + extension;
}
