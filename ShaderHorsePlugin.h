#ifndef SHADER_HORSE_PLUGIN_H
#define SHADER_HORSE_PLUGIN_H

#include <memory>
#include "../../MainLib/InterceptPluginInterface.h"

class ShaderHorse;

class ShaderHorsePlugin : public InterceptPluginInterface {
public:
  ShaderHorsePlugin(InterceptPluginCallbacks *callBacks);
  virtual ~ShaderHorsePlugin();

  virtual void GLIAPI GLFunctionPre(uint updateID, const char *funcName,
    uint funcIndex, const FunctionArgs &args) override;

  virtual void GLIAPI GLFunctionPost(uint updateID, const char *funcName,
    uint funcIndex, const FunctionRetValue &retVal) override;

  virtual void GLIAPI GLFrameEndPre(const char *funcName, uint funcIndex,
    const FunctionArgs &args) override;

  virtual void GLIAPI GLFrameEndPost(const char *funcName, uint funcIndex,
    const FunctionRetValue &retVal) override;

  virtual void GLIAPI GLRenderPre(const char *funcName, uint funcIndex,
    const FunctionArgs &args) override;

  virtual void GLIAPI GLRenderPost(const char *funcName, uint funcIndex,
    const FunctionRetValue &retVal) override;

  virtual void GLIAPI OnGLError(const char *funcName, uint funcIndex) override;

  virtual void GLIAPI OnGLContextCreate(HGLRC rcHandle) override;

  virtual void GLIAPI OnGLContextDelete(HGLRC rcHandle) override;

  virtual void GLIAPI OnGLContextSet(HGLRC oldRCHandle, HGLRC newRCHandle) override;

  virtual void GLIAPI OnGLContextShareLists(HGLRC srcHandle, HGLRC dstHandle) override;

  virtual void GLIAPI Destroy() override;

protected:
  const InputUtils *input_utils_;
  std::unique_ptr<ShaderHorse> horse_;
};

#endif
