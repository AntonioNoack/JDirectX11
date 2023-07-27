#pragma once

struct Window {

	HWND hwnd = nullptr;

	ID3D11Device1* device = nullptr;
	ID3D11DeviceContext1* deviceContext = nullptr;
	ID3D11InfoQueue* debugInfoQueue = nullptr;
	ID3D11RenderTargetView* primaryRTV = nullptr;
	IDXGISwapChain1* swapChain = nullptr;

	ID3D11Buffer* nullBuffer = nullptr;

	jobject frameSizeCallback = nullptr;
	jobject focusCallback = nullptr;
	jobject iconifyCallback = nullptr;
	jobject refreshCallback = nullptr;
	jobject contentScaleCallback = nullptr;
	jobject dropCallback = nullptr;
	jobject charModsCallback = nullptr;
	jobject cursorPosCallback = nullptr;
	jobject scrollCallback = nullptr;
	jobject mouseButtonCallback = nullptr;
	jobject keyCallback = nullptr;

	jint width = 0, height = 0;
	jint vsyncInterval = 1;
	jint shouldClose = 0;

};

struct VertexShader {

	uint32_t id;
	ID3DBlob* blob = nullptr;
	ID3D11VertexShader* shader = nullptr;
	ID3D11Buffer* uniforms = nullptr;
	char** attrNames = nullptr;
	uint8_t numAttributes = 0;

};

struct Texture {

	ID3D11Resource* texture = nullptr; // could be texture2d or texture3d
	ID3D11ShaderResourceView* textureView = nullptr;

};