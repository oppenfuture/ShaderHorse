#ifndef SHADER_HORSE_H
#define SHADER_HORSE_H

#include <unordered_map>
#include "../../MainLib/InterceptPluginInterface.h"

class ShaderHorse {
public:
  ShaderHorse(InterceptPluginCallbacks *gli_callbacks);
  ~ShaderHorse();

  void LoadFunctions();

  void glCreateShaderPre(GLenum shaderType);
  void glCreateShaderPost(GLuint shader);

  void glShaderSource(GLuint shader, GLsizei count, const GLchar **string, const GLint *length);
  void glCompileShader(GLuint shader);
  void glAttachShader(GLuint program, GLuint shader);
  void glDetachShader(GLuint program, GLuint shader);
  void glLinkProgram(GLuint program);

  void Deliver();

private:
  InterceptPluginCallbacks *gli_callbacks_{nullptr};
  GLuint (GLAPIENTRY *iglCreateShader) (GLenum);
  void (GLAPIENTRY *iglShaderSource) (GLuint, GLsizei, const GLchar**, const GLint*);
  void (GLAPIENTRY *iglCompileShader) (GLuint);
  void (GLAPIENTRY *iglAttachShader) (GLuint, GLuint);
  void (GLAPIENTRY *iglDetachShader) (GLuint, GLuint);
  void (GLAPIENTRY *iglLinkProgram) (GLuint);
  void (GLAPIENTRY *iglDeleteShader) (GLuint);
  GLboolean (GLAPIENTRY *iglIsProgram) (GLuint);
  void (GLAPIENTRY *iglGetShaderiv) (GLuint, GLenum, GLint*);
  void (GLAPIENTRY *iglGetShaderInfoLog) (GLuint, GLsizei, GLsizei*, GLchar*);
  void (GLAPIENTRY *iglGetProgramiv) (GLuint, GLenum, GLint*);
  void (GLAPIENTRY *iglGetProgramInfoLog) (GLuint, GLsizei, GLsizei*, GLchar*);

  bool valid_{false};

  struct ShaderState {
    GLenum type{GL_NONE};
    std::string source;
    std::string compiled_source;
  };

  struct GLState {
    GLenum shader_creation_state{GL_NONE};
    std::unordered_map<GLuint, ShaderState> shader_state;
    std::unordered_map<GLuint, std::vector<GLuint>> program_attach_state;
  };
  GLState state_;

  class ShaderSoldier {
  public:
    ShaderSoldier(GLuint program, const ShaderState &state);
    ShaderSoldier(const ShaderSoldier&) = delete;
    ShaderSoldier& operator=(const ShaderSoldier&) = delete;
    ShaderSoldier(ShaderSoldier &&other);
    ~ShaderSoldier();

    GLenum type{GL_NONE};
    std::string source;

    void Save() const;
    bool Load();

  private:

    GLuint program_;
    GLuint unique_id_;
    static GLuint id_;
    std::string FileName() const;
  };

  std::unordered_map<GLuint, std::vector<ShaderSoldier>> soldiers_;

  void Deliver(GLuint program, const std::vector<ShaderSoldier> &soldiers) const;
};

#endif
