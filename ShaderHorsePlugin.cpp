#include "ShaderHorsePlugin.h"
#include <PluginCommon.cpp>
#include "ShaderHorse.h"

InterceptPluginInterface* GLIAPI CreateFunctionLogPlugin(const char *pluginName,
  InterceptPluginCallbacks *callBacks) {
  //If no call backs, return NULL
  if (callBacks == NULL) {
    return NULL;
  }

  //Assign the error logger
  if (errorLog == NULL) {
    errorLog = callBacks->GetLogErrorFunction();
  }

  string cmpPluginName(pluginName);

  //Test for the function status plugin
  if (cmpPluginName == "ShaderHorse") {
    return new ShaderHorsePlugin(callBacks);
  }

  return NULL;
}

ShaderHorsePlugin::ShaderHorsePlugin(InterceptPluginCallbacks *callBacks) : input_utils_(nullptr) {
  if (callBacks) {
    callBacks->RegisterGLFunction("glCreateShader");
    callBacks->RegisterGLFunction("glShaderSource");
    callBacks->RegisterGLFunction("glCompileShader");
    callBacks->RegisterGLFunction("glAttachShader");
    callBacks->RegisterGLFunction("glDetachShader");
    callBacks->RegisterGLFunction("glLinkProgram");

    input_utils_ = callBacks->GetInputUtils();
    if (!input_utils_)
      errorLog("ShaderHorse: Input utils is null");
  } else {
    errorLog("ShaderHorse: GLI callbacks is null");
  }

  horse_ = std::make_unique<ShaderHorse>(callBacks);
}

ShaderHorsePlugin::~ShaderHorsePlugin() {}

void ShaderHorsePlugin::GLFunctionPre(uint updateID, const char *funcName,
    uint funcIndex, const FunctionArgs &args) {
  FunctionArgs args_(args);

  if (strcmp(funcName, "glCreateShader") == 0) {
    GLenum shaderType; args_.Get(shaderType);
    horse_->glCreateShaderPre(shaderType);
  } else if (strcmp(funcName, "glShaderSource") == 0) {
    GLuint shader; args_.Get(shader);
    GLsizei count; args_.Get(count);
    void *string; args_.Get(string);
    void *length; args_.Get(length);
    horse_->glShaderSource(shader, count, (const GLchar**)string, (const GLint*)length);
  } else if (strcmp(funcName, "glCompileShader") == 0) {
    GLuint shader; args_.Get(shader);
    horse_->glCompileShader(shader);
  } else if (strcmp(funcName, "glAttachShader") == 0) {
    GLuint program; args_.Get(program);
    GLuint shader; args_.Get(shader);
    horse_->glAttachShader(program, shader);
  } else if (strcmp(funcName, "glDetachShader") == 0) {
    GLuint program; args_.Get(program);
    GLuint shader; args_.Get(shader);
    horse_->glDetachShader(program, shader);
  } else if (strcmp(funcName, "glLinkProgram") == 0) {
    GLuint program; args_.Get(program);
    horse_->glLinkProgram(program);
  }
}

void ShaderHorsePlugin::GLFunctionPost(uint updateID, const char *funcName,
    uint funcIndex, const FunctionRetValue &retVal) {
  FunctionRetValue ret_val(retVal);
  if (strcmp(funcName, "glCreateShader") == 0) {
    GLuint shader; ret_val.Get(shader);
    horse_->glCreateShaderPost(shader);
  }
}

void ShaderHorsePlugin::GLFrameEndPre(const char *funcName, uint funcIndex,
    const FunctionArgs &args) {
  if (input_utils_ && input_utils_->IsKeyDown(InputUtils::GetKeyCode("t"))) {
    horse_->Deliver();
  }
}

void ShaderHorsePlugin::GLFrameEndPost(const char *funcName, uint funcIndex,
    const FunctionRetValue &retVal) {}

void ShaderHorsePlugin::GLRenderPre(const char *funcName, uint funcIndex,
    const FunctionArgs &args) {}

void ShaderHorsePlugin::GLRenderPost(const char *funcName, uint funcIndex,
    const FunctionRetValue &retVal) {}

void ShaderHorsePlugin::OnGLError(const char *funcName, uint funcIndex) {}

void ShaderHorsePlugin::OnGLContextCreate(HGLRC rcHandle) {}

void ShaderHorsePlugin::OnGLContextDelete(HGLRC rcHandle) {}

void ShaderHorsePlugin::OnGLContextSet(HGLRC oldRCHandle, HGLRC newRCHandle) {
  if (newRCHandle)
    horse_->LoadFunctions();
}

void ShaderHorsePlugin::OnGLContextShareLists(HGLRC srcHandle, HGLRC dstHandle) {}

void ShaderHorsePlugin::Destroy() {
  delete this;
}
