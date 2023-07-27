
#include "pch.h"

#include <fstream>

#include "Includes.hpp"
#include "Structs.hpp"
#include "JavaInterface.hpp"
#include "SharedAPI.hpp"

#define WinMain MyWinMain

int WinMain(JNIEnv* env);

JNIEXPORT jint JNICALL Java_me_anno_directx11_DirectX_runNativeSample
(JNIEnv* env, jclass) {
	return WinMain(env);
}

struct float2 {
	float x, y;
};

struct float4 {
	float x, y, z, w;
};

// Create Constant Buffer
struct Constants {
	float2 pos;
	float2 paddingUnused; // color (below) needs to be 16-byte aligned! 
	float4 color;
};

int WINAPI WinMain(JNIEnv* env)
{
	// Open a window
	jlong window = Java_me_anno_directx11_DirectX_createWindow(env, nullptr, 1024, 768, nullptr, 0, 0);
	if (!window) return -1;

	Java_me_anno_directx11_DirectX_makeContextCurrent(env, nullptr, window);
	Java_me_anno_directx11_DirectX_doAttachDirectX(env, nullptr);

	// Create Vertex Shader
	std::ifstream vsIn("C:/Users/Antonio/Documents/IdeaProjects/JDirectX11/assets/shaders.hlsl");
	std::string shaderText((std::istreambuf_iterator<char>(vsIn)), std::istreambuf_iterator<char>());

	jstring shaderTextStr = env->NewStringUTF(shaderText.c_str());
	jobjectArray attr = env->NewObjectArray(4, env->GetObjectClass(shaderTextStr), nullptr);
	env->SetObjectArrayElement(attr, 0, env->NewStringUTF("coords"));
	env->SetObjectArrayElement(attr, 1, env->NewStringUTF("instData"));
	env->SetObjectArrayElement(attr, 2, env->NewStringUTF("color0x"));
	env->SetObjectArrayElement(attr, 3, env->NewStringUTF("color1x"));

	jlong vs = Java_me_anno_directx11_DirectX_compileVertexShader(env, nullptr, shaderTextStr, attr, sizeof(Constants));
	if (!vs) return -17;

	// Create Pixel Shader
	jlong ps = Java_me_anno_directx11_DirectX_compilePixelShader(env, nullptr, shaderTextStr);
	if (!ps) return -18;
	
	struct Vertex {
		float x, y;
	};

	// Create Vertex Buffer
	jlong vertexBuffer;
	UINT vertexCount, vertexStride = sizeof(Vertex);
	std::cout << "VertexStride: " << vertexStride << std::endl;
	{
		Vertex vertexData[] = {
			{ -0.5f,  0.5f },
			{ +0.5f, -0.5f },
			{ -0.5f, -0.5f },
			{ -0.5f,  0.5f },
			{ +0.5f,  0.5f },
			{ +0.5f, -0.5f }
		};
		vertexStride = sizeof(Vertex);
		vertexCount = ARRAYSIZE(vertexData);
		vertexBuffer = Java_me_anno_directx11_DirectX_createBuffer(
			env, nullptr,
			D3D11_BIND_VERTEX_BUFFER, (jlong)vertexData, sizeof(vertexData), D3D11_USAGE_IMMUTABLE
		);
		if (!vertexBuffer) return -19;
	}

	struct Float3 {
		float r, g, b;
	};

	struct Instance {
		float px, py, pz;
		Float3 color0;
		Float3 color1;
	};

	jlong instanceBuffer;
	UINT instanceCount = 7;
	UINT instanceStride = sizeof(Instance);
	std::cout << "InstanceStride: " << instanceStride << std::endl;
	{
		Float3 red{ 1, 0, 0 }, green{ 0, 1, 0 }, blue{ 0, 0, 1 }, black{ 0, 0, 0 }, white{ 1, 1, 1 };
		Instance instanceData[] = {
			{ 0.0f, 0.0f, 0.0f, red, black },
			{ 0.5f, 1.0f, 0.0f, green, black },
			{ 1.0f, 2.0f, 0.0f, blue, red },
			{ 1.5f, 3.0f, 0.0f, green, white },
			{ 2.0f,-3.0f, 0.0f, red, white },
			{ 2.5f,-2.0f, 0.0f, blue, white },
			{ 3.0f,-1.0f, 0.0f, black, white },
		};
		instanceBuffer = Java_me_anno_directx11_DirectX_createBuffer(
			env, nullptr, 
			D3D11_BIND_INDEX_BUFFER, (jlong) instanceData, sizeof(instanceData), D3D11_USAGE_IMMUTABLE
		);
		if (!instanceBuffer) return -20;
	}

	jlongArray primRTVArray = env->NewLongArray(1);
	Constants constants{};

	jbyteArray vaoChannels = env->NewByteArray(4);
	jbyte vaoChannelData[4] = { 2, 3, 3, 3 };
	env->SetByteArrayRegion(vaoChannels, 0, 4, vaoChannelData);

	jintArray vaoTypes = env->NewIntArray(4);
	jint vaoTypeData[4] = { 0x1406, 0x1406, 0x1406, 0x1406 }; // 4x GL_FLOAT
	env->SetIntArrayRegion(vaoTypes, 0, 4, vaoTypeData);

	jintArray vaoStrides = env->NewIntArray(4);
	jint vaoStrideData[4] = { vertexStride, instanceStride, instanceStride, instanceStride };
	env->SetIntArrayRegion(vaoStrides, 0, 4, vaoStrideData);

	jintArray vaoOffsets = env->NewIntArray(4);
	jint vaoOffsetData[4] = { 0, 0, 12, 24 };
	env->SetIntArrayRegion(vaoOffsets, 0, 4, vaoOffsetData);

	jlongArray vaoBuffers = env->NewLongArray(4);
	jlong vaoBufferData[4] = { vertexBuffer, instanceBuffer, instanceBuffer, instanceBuffer };
	env->SetLongArrayRegion(vaoBuffers, 0, 4, vaoBufferData);

	jint perInstance = 0b1110;
	jint normalized = 0;
	jint enabled = 0b1111;

	unsigned char texData[] = {
		0, 0, 0, 0, 255, 255, 255, 255, 255, 255, 255, 255, 0, 0, 0, 0,
		255, 255, 255, 255, 0, 0, 0, 0, 0, 0, 0, 0, 255, 255, 255, 255,
		255, 255, 255, 255, 0, 0, 0, 0, 0, 0, 0, 0, 255, 255, 255, 255,
		0, 0, 0, 0, 255, 255, 255, 255, 255, 255, 255, 255, 0, 0, 0, 0,
	};
	jlong tex = Java_me_anno_directx11_DirectX_createTexture2D(
		env, nullptr, 1, 0x8058 /* rgba8 */, 
		4, 4, 0x1908 /* rgba */, 0x1401 /* ubyte */,
		(jlong) texData);
	if (!tex) return -23;

	jlongArray texPtrs = env->NewLongArray(2);
	jlong texPtrData[2] = { tex, 0 };

	// Main Loop
	float time = 0.0;
	jlong primRTV = 0;
	jint w = 0, h = 0;
	jintArray ws = env->NewIntArray(1);
	jintArray hs = env->NewIntArray(1);

	while (!Java_me_anno_directx11_DirectX_shouldWindowClose(env, nullptr, window)) {

		primRTV = Java_me_anno_directx11_DirectX_waitEvents(env, nullptr);
		Java_me_anno_directx11_DirectX_getFramebufferSize(env, nullptr, window, ws, hs);

		env->GetIntArrayRegion(ws, 0, 1, &w);
		env->GetIntArrayRegion(hs, 0, 1, &h);
		
		Java_me_anno_directx11_DirectX_updateViewport(env, nullptr, 0, 0, w, h, 0, 1);

		constants.pos = { sinf(time), cosf(time) };
		constants.color = { 0.7f, 0.65f, 0.1f, 1.f }; 
		Java_me_anno_directx11_DirectX_doBindUniforms(env, nullptr, vs, (jlong) &constants, sizeof(Constants));

		Java_me_anno_directx11_DirectX_clearColor(env, nullptr, primRTV, 0.1f, 0.2f, 0.6f, 1.0f);
		env->SetLongArrayRegion(primRTVArray, 0, 1, &primRTV); // can change -> must be set every frame
		Java_me_anno_directx11_DirectX_setRenderTargets(env, nullptr, 1, primRTVArray, 0);
		
		Java_me_anno_directx11_DirectX_setShaders(env, nullptr, vs, ps);
		Java_me_anno_directx11_DirectX_bindTextures(env, nullptr, 1, (jlong) texPtrData);
		
		int code = Java_me_anno_directx11_DirectX_doBindVAO(
			env, nullptr, vs,
			vaoChannels, vaoTypes, vaoStrides, vaoOffsets,
			vaoBuffers, perInstance, normalized, enabled);
		if (code != 0) return -22;

		Java_me_anno_directx11_DirectX_drawArraysInstanced(env, nullptr, 4, 0, vertexCount, instanceCount);

		Java_me_anno_directx11_DirectX_swapBuffers(env, nullptr, window);

		time += (float)(1.0 / 60.0);
	}

	return 0;
}

