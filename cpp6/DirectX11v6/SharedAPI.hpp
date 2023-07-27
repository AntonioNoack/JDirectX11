
#pragma once
#include "Includes.hpp"
#include "Structs.hpp"

void CreateDeviceAndContext(Window& window);
void CreateSwapChain(Window& window);
void CreateRenderTargetView(Window& window);
void SetupDebugging(Window& window);

void PrintError(HRESULT hResult, const char* callName);
void HandleShaderError(HRESULT hResult, const char* src, ID3DBlob* shaderCompileErrorsBlob);
