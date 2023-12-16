
// Compile: Strg+B

#include "pch.h"
#include "Includes.hpp"
#include "JavaInterface.hpp"
#include "Structs.hpp"
#include "SharedAPI.hpp"

thread_local Window* currentWindow = nullptr;
bool global_windowDidResize = false;
JavaVM* jvm = nullptr;

#include <vector>
std::vector<Window*> windows;

#pragma region Debugging

void CheckLastError(const char* callName) {
	auto lastError = GetLastError();
	if (lastError) {
		std::cerr << "Last Error: " << lastError << " by " << callName << std::endl;
		SetLastError(0);
	}
}

void PrintDebugMessages() {
	if (!currentWindow->debugInfoQueue) return;
	UINT64 message_count = currentWindow->debugInfoQueue->GetNumStoredMessages();
	std::cout << "Checking for messages... " << message_count << std::endl;
	for (UINT64 i = 0; i < message_count; i++) {
		SIZE_T message_size = 0;
		currentWindow->debugInfoQueue->GetMessage(i, nullptr, &message_size); //get the size of the message
		D3D11_MESSAGE* message = (D3D11_MESSAGE*)malloc(message_size); //allocate enough space
		if (!message) break;
		HRESULT result = currentWindow->debugInfoQueue->GetMessage(i, message, &message_size); //get the actual message
		if (FAILED(result)) PrintError(result, "PrintDebugMessages");
		printf("Message: %.*s", (int) message->DescriptionByteLength, message->pDescription);
		free(message);
	}
	currentWindow->debugInfoQueue->ClearStoredMessages();
}

#include "dxerr.h"
void PrintError(HRESULT hr, const char* callName) {
	CheckLastError(callName);
	static WCHAR WStr[1000];
	DXGetErrorDescriptionW(hr, WStr, sizeof(WStr) / sizeof(WCHAR));
	static char ASCIIStr[1000];
	size_t out;
	wcstombs_s(&out, ASCIIStr, WStr, sizeof(WStr) / sizeof(WCHAR) - 1);
	ASCIIStr[sizeof(ASCIIStr) / sizeof(char) - 1] = 0; // ensure null-termination
	size_t asciiLen = strlen(ASCIIStr); // remove linebreak at end; nobody asked for it
	if (asciiLen > 0 && ASCIIStr[asciiLen - 1] == '\n') {
		ASCIIStr[--asciiLen] = 0;
		if (asciiLen > 0 && ASCIIStr[asciiLen - 1] == '\r') {
			ASCIIStr[--asciiLen] = 0;
		}
	}
	std::cerr << "Error in " << callName << "(#" << std::hex << hr << std::dec << "): " << ASCIIStr << std::endl;
	if (hr == 0x887a0005) {
		// #887a0006 = DXGI_ERROR_DEVICE_HUNG
		hr = currentWindow->device->GetDeviceRemovedReason();
		if (hr != 0x887a0005) PrintError(hr, callName);
		else std::cerr << "Device-Removed-Reason: #" << std::hex << hr << std::dec << std::endl;
	}
	PrintDebugMessages();
	ExitProcess(hr);
}

#pragma endregion Debugging

#pragma region Context

JNIEXPORT void JNICALL Java_me_anno_directx11_DirectX_makeContextCurrent
(JNIEnv*, jclass, jlong handle) {
	currentWindow = (Window*)handle;
}

jint ParamToScancode(WPARAM p) {
	if ((p >= 0x30 && p <= 0x39) || (p >= 0x41 && p <= 0x5a)) { // 0-9, A-Z
		return p;
	}
	else if (p >= 0x60 && p <= 0x69) { // numpad 0-9
		return p + 320 - 0x60;
	}
	else if (p >= 0x70 && p <= 0x87) { // function keys 1-24
		return p + 290 - 0x70;
	}
	else {
		switch (p) {
			// mouse buttons
		case VK_LBUTTON: return 0;
		case VK_RBUTTON: return 1;
		case VK_MBUTTON: return 2;
			// correct???
		case VK_XBUTTON1: return 4;
		case VK_XBUTTON2: return 5;
			// others
		case VK_ESCAPE: return 256;
		case VK_RETURN: return 257;
		case VK_TAB: return 258;
		case VK_BACK: return 259;
		case VK_INSERT: return 260;
		case VK_DELETE: return 261;
		case VK_RIGHT: return 262;
		case VK_LEFT: return 263;
		case VK_DOWN: return 264;
		case VK_UP: return 265;
			// page up/down
		case VK_PRIOR: return 266;
		case VK_NEXT: return 267;
		case VK_HOME: return 268;
		case VK_END: return 269;
		case VK_CAPITAL: return 280;
		case VK_SCROLL: return 281;
		case VK_NUMLOCK: return 282;
		case VK_SNAPSHOT: return 283;
		case VK_PAUSE: return 284;
			// super keys
		case VK_LSHIFT: return 340;
		case VK_LCONTROL: return 341;
		case VK_LMENU: return 342;
		case VK_LWIN: return 343;
		case VK_RSHIFT: return 344;
		case VK_RCONTROL: return 345;
		case VK_RMENU: return 346;
		case VK_RWIN: return 347;
		case VK_APPS: return 348; // correct??
		case VK_SPACE: return 32;
		case VK_OEM_COMMA: return 44;
		case VK_OEM_MINUS: return 45;
		case VK_OEM_PERIOD: return 46;
		default: return p + 512;
		}
	}
}

bool isKeyPressed(int virtualKeyCode) {
	return (GetAsyncKeyState(virtualKeyCode) & 0x8000) != 0;
}

jint GetKeyMods() {
	jint code = 0;
	if (isKeyPressed(VK_SHIFT)) code |= 0x01;
	if (isKeyPressed(VK_CONTROL)) code |= 0x02;
	if (isKeyPressed(VK_MENU)) code |= 0x04;
	if (isKeyPressed(VK_LWIN) || isKeyPressed(VK_RWIN)) code |= 0x08;
	if (isKeyPressed(VK_CAPITAL)) code |= 0x10;
	if (isKeyPressed(VK_NUMLOCK)) code |= 0x20;
	return code;
}

void keyCallback(Window* window, WPARAM wparam, jint action) {
	if (window && window->keyCallback) {
		JNIEnv* env = nullptr;
		jvm->AttachCurrentThread((void**)(&env), NULL);
		jclass cbClass = env->FindClass("org/lwjgl/glfw/GLFWKeyCallbackI");
		if (cbClass) {
			jmethodID method = env->GetMethodID(cbClass, "invoke", "(JIIII)V");
			if (method) {
				std::cout << "Calling key callback" << std::endl;
				jint key = ParamToScancode(wparam), scancode = key;
				jint mods = GetKeyMods();
				env->CallVoidMethod(window->keyCallback, method,
									(jlong)window, key, scancode, action, mods);
			}
		}
		jvm->DetachCurrentThread();
	}
}

void mouseButtonCallback(Window* window, jint button, jint action) {
	if (window && window->mouseButtonCallback) {
		std::cout << "Handling key" << std::endl;
		JNIEnv* env = nullptr;
		jvm->AttachCurrentThread((void**)(&env), NULL);
		jclass cbClass = env->FindClass("org/lwjgl/glfw/GLFWMouseButtonCallbackI");
		if (cbClass) {
			jmethodID method = env->GetMethodID(cbClass, "invoke", "(JIII)V");
			if (method) {
				std::cout << "Calling mouse callback" << std::endl;
				jint mods = GetKeyMods();
				env->CallVoidMethod(window->mouseButtonCallback, method,
									(jlong)window, button, action, mods);
			}
		}
		jvm->DetachCurrentThread();
	}
}



const char* GetEventType(UINT msg) {
	switch (msg) {
	case WM_CREATE: return "WM_CREATE";
	case WM_KEYDOWN: return "WM_KEYDOWN";
	case WM_KEYUP: return "WM_KEYUP";
	case WM_MOUSEMOVE: return "WM_MOUSEMOVE";
	case WM_MOUSEWHEEL: return "WM_MOUSEWHEEL";
	case WM_LBUTTONDOWN: return "WM_LBUTTONDOWN";
	case WM_LBUTTONUP: return "WM_LBUTTONUP";
	case WM_LBUTTONDBLCLK: return "WM_LBUTTONDBLCLK";
	case WM_CHAR: return "WM_CHAR";
	case WM_RBUTTONDOWN: return "WM_RBUTTONDOWN";
	case WM_RBUTTONUP: return "WM_RBUTTONUP";
	case WM_MBUTTONDOWN: return "WM_MBUTTONDOWN";
	case WM_MBUTTONUP: return "WM_MBUTTONUP";
	case WM_UNICHAR: return "WM_UNICHAR";
	case WM_DESTROY: return "WM_DESTROY";
	case WM_SIZE: return "WM_SIZE";
	case WM_MOVE: return "WM_MOVE";
	case WM_CLOSE: return "WM_CLOSE";
	case WM_WINDOWPOSCHANGED: return "WM_WINDOWPOSCHANGED";
	case WM_WINDOWPOSCHANGING: return "WM_WINDOWPOSCHANGING";
	case WM_ERASEBKGND: return "WM_ERASEBKGND";
	case WM_SETCURSOR: return "WM_SETCURSOR";
	case WM_NCHITTEST: return "WM_NCHITTEST";
	case WM_NCMOUSELEAVE: return "WM_NCMOUSELEAVE";
	case WM_GETICON: return "WM_GETICON";
	case WM_SETICON: return "WM_SETICON";
	case WM_GETMINMAXINFO: return "WM_GETMINMAXINFO";
	case WM_PAINT: return "WM_PAINT";
	case WM_ACTIVATE: return "WM_ACTIVATE";
	case WM_ACTIVATEAPP: return "WM_ACTIVATEAPP";
	case WM_SETFOCUS: return "WM_SETFOCUS";
	case WM_KILLFOCUS: return "WM_KILLFOCUS";
	case WM_SHOWWINDOW: return "WM_SHOWWINDOW";
	case WM_NCACTIVATE: return "WM_NCACTIVATE";
	case WM_NCCALCSIZE: return "WM_NCCALCSIZE";
	case WM_NCLBUTTONDOWN: return "WM_NCLBUTTONDOWN";
	case WM_NCLBUTTONUP: return "WM_NCLBUTTONUP";
	case WM_NCCREATE: return "WM_NCCREATE";
	case WM_NCPAINT: return "WM_NCPAINT";
	case WM_IME_SETCONTEXT: return "WM_IME_SETCONTEXT";
	case WM_IME_NOTIFY: return "WM_IME_NOTIFY";
	case WM_DWMNCRENDERINGCHANGED: return "WM_DWMNCRENDERINGCHANGED";
	case SPI_SETSNAPTODEFBUTTON: return "SPI_SETSNAPTODEFBUTTON";
	case WM_TIMER: return "WM_TIMER";
	case WM_NCMOUSEMOVE: return "WM_NCMOUSEMOVE";
	case WM_SIZING: return "WM_SIZING";
	case WM_CAPTURECHANGED: return "WM_CAPTURECHANGED";
	case WM_ENTERSIZEMOVE: return "WM_ENTERSIZEMOVE";
	case WM_EXITSIZEMOVE: return "WM_EXITSIZEMOVE";
	case WM_SYSCOMMAND: return "WM_SYSCOMMAND";
	case WM_SETTEXT: return "WM_SETTEXT";
	case WM_NCDESTROY: return "WM_NCDESTROY";
	case SPI_GETDOCKMOVING: return "SPI_GETDOCKMOVING";
	default: return "?";
	}
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	static int eventDepth = 0;
	for (int i = 0; i < eventDepth; i++) std::cout << "  ";
	const char* type = GetEventType(msg);
	std::cout << "Event " << type;
	if (type == "?") std::cout << "(" << msg << ")";
	std::cout << ", " << wparam << ", " << lparam << std::endl;
	eventDepth++;
	
	LRESULT result = 0;
	Window* window = nullptr;
	for (Window* candidate : windows) { // finding the used window
		if (candidate->hwnd == hwnd) {
			window = candidate;
			break;
		}
	}

	// std::cerr << "Received WndProc event: " << msg << ", " << wparam << ", " << lparam << std::endl;
	switch (msg)
	{
	case WM_KEYDOWN: {
		keyCallback(window, wparam, 1);
		break;
	}
	case WM_KEYUP: {
		keyCallback(window, wparam, 0);
		break;
	}
	case WM_LBUTTONDOWN: {
		mouseButtonCallback(window, 0, 1);
		break;
	}
	case WM_LBUTTONUP: {
		mouseButtonCallback(window, 0, 0);
		break;
	}
	case WM_RBUTTONDOWN: {
		mouseButtonCallback(window, 1, 1);
		break;
	}
	case WM_RBUTTONUP: {
		mouseButtonCallback(window, 1, 0);
		break;
	}
	case WM_MBUTTONDOWN: {
		mouseButtonCallback(window, 2, 1);
		break;
	}
	case WM_MBUTTONUP: {
		mouseButtonCallback(window, 2, 0);
		break;
	}
	case WM_MOUSEMOVE: {
		if (window && window->cursorPosCallback) {
			JNIEnv* env = nullptr;
			jvm->AttachCurrentThread((void**)(&env), NULL);
			jclass cbClass = env->FindClass("org/lwjgl/glfw/GLFWCursorPosCallbackI");
			if (cbClass) {
				jmethodID method = env->GetMethodID(cbClass, "invoke", "(JDD)V");
				if (method) {
					jdouble x = (double) LOWORD(lparam), y = (double) HIWORD(lparam);
					env->CallVoidMethod(window->cursorPosCallback, method, (jlong)window, x, y);
				}
			}
			jvm->DetachCurrentThread();
		}
		break;
	}
	case WM_MOUSEWHEEL:{
		if (window && window->scrollCallback) {
			JNIEnv* env = nullptr;
			jvm->AttachCurrentThread((void**)(&env), NULL);
			jclass cbClass = env->FindClass("org/lwjgl/glfw/GLFWScrollCallbackI");
			if (cbClass) {
				jmethodID method = env->GetMethodID(cbClass, "invoke", "(JDD)V");
				if (method) {
					jdouble dy = ((int16_t)HIWORD(wparam)) / 120.0, dx = 0;
					env->CallVoidMethod(window->scrollCallback, method, (jlong)window, dx, dy);
				}
			}
			jvm->DetachCurrentThread();
		}
		break;
	}
	case WM_CHAR:
	case WM_UNICHAR: { // todo what is deadchar?
		if (wparam == UNICODE_NOCHAR || wparam < 32) result = true; // requirement
		else if (window && window->charModsCallback) {
			JNIEnv* env = nullptr;
			jvm->AttachCurrentThread((void**)(&env), NULL);
			jclass cbClass = env->FindClass("org/lwjgl/glfw/GLFWCharModsCallbackI");
			if (cbClass) {
				jmethodID method = env->GetMethodID(cbClass, "invoke", "(JII)V");
				if (method) {
					jint codepoint = (jint) wparam, mods = GetKeyMods();
					env->CallVoidMethod(window->charModsCallback, method,
										(jlong)window, codepoint, mods);
				}
			}
			jvm->DetachCurrentThread();
		}
		break;
	}
	case WM_SETFOCUS: {
		if (window && window->focusCallback) {
			JNIEnv* env = nullptr;
			jvm->AttachCurrentThread((void**)(&env), NULL);
			jclass cbClass = env->FindClass("org/lwjgl/glfw/GLFWWindowFocusCallbackI");
			if (cbClass) {
				jmethodID method = env->GetMethodID(cbClass, "invoke", "(JZ)V");
				if (method) env->CallVoidMethod(window->focusCallback, method, (jlong)window, true);
			}
			jvm->DetachCurrentThread();
		}
		break;
	}
	case WM_KILLFOCUS: {
		if (window && window->focusCallback) {
			JNIEnv* env = nullptr;
			jvm->AttachCurrentThread((void**)(&env), NULL);
			jclass cbClass = env->FindClass("org/lwjgl/glfw/GLFWWindowFocusCallbackI");
			if (cbClass) {
				jmethodID method = env->GetMethodID(cbClass, "invoke", "(JZ)V");
				if (method) env->CallVoidMethod(window->focusCallback, method, (jlong)window, false);
			}
			jvm->DetachCurrentThread();
		}
		break;
	}
	case WM_DESTROY:
	{
		if(window) window->shouldClose = true;
		PostQuitMessage(0);
		break;
	}
	case WM_CLOSE: {
		if (window) window->shouldClose = true;
		else DestroyWindow(hwnd);
		break;
	}
	case WM_SIZE: {
		if (window && window->hwnd && (window->frameSizeCallback || window->iconifyCallback)) {
			JNIEnv* env = nullptr;
			jvm->AttachCurrentThread((void**)(&env), NULL);
			RECT winRect;
			GetClientRect(window->hwnd, &winRect);
			jint width = window->width = winRect.right - winRect.left;
			jint height = window->height = winRect.bottom - winRect.top;
			if (window->frameSizeCallback) {
				jclass cbClass = env->FindClass("org/lwjgl/glfw/GLFWFramebufferSizeCallbackI");
				if (cbClass) {
					jmethodID method = env->GetMethodID(cbClass, "invoke", "(JII)V");
					if (method) {
						env->CallVoidMethod(window->frameSizeCallback, method, (jlong)window, width, height);
					}
				}
			}
			if (window->iconifyCallback) {
				jclass cbClass = env->FindClass("org/lwjgl/glfw/GLFWWindowIconifyCallbackI");
				if (cbClass) {
					jmethodID method = env->GetMethodID(cbClass, "invoke", "(JZ)V");
					if (method) {
						bool isMinimized = width == 0 && height == 0;
						env->CallVoidMethod(window->iconifyCallback, method, (jlong)window, isMinimized);
					}
				}
			}
			jvm->DetachCurrentThread();
		}
		global_windowDidResize = true;
		break;
	}
	default:
		result = DefWindowProcW(hwnd, msg, wparam, lparam);
	}

	eventDepth--;
	return result;
}

JNIEXPORT jlong JNICALL Java_me_anno_directx11_DirectX_createWindow
(JNIEnv* env, jclass, jint width, jint height, jstring name, jstring iconPath) {

	// Open a window
	HWND hwnd;
	HINSTANCE hInstance = GetModuleHandle(NULL);
	Window* window = new Window();
	windows.push_back(window);

	jboolean name1Copy;
	int length = env && name ? env->GetStringLength(name) : 0;
	const jchar* name1 = env && name ? env->GetStringChars(name, &name1Copy) : nullptr;
	LPCWSTR name2 = (LPCWSTR) calloc(1, sizeof(WCHAR) * (length + 1));
	if(name1 && name2) memcpy((void*) name2, (void*) name1, sizeof(WCHAR) * length);

	const jchar* iconPath1 = env->GetStringChars(iconPath, nullptr);
	std::wstring iconPath2((wchar_t*) iconPath1, env->GetStringLength(iconPath));

	{
		WNDCLASSEXW winClass = {};
		winClass.cbSize = sizeof(WNDCLASSEXW);
		winClass.style = CS_HREDRAW | CS_VREDRAW;
		winClass.lpfnWndProc = &WndProc;
		winClass.hInstance = hInstance;
		HICON icon = (HICON)LoadImageW(0, iconPath2.c_str(), IMAGE_ICON, 0, 0, LR_LOADFROMFILE);
		winClass.hIcon = icon;
		winClass.hCursor = LoadCursorW(0, IDC_ARROW);
		winClass.lpszClassName = L"RemsEngineWindow";
		winClass.hIconSm = icon;

		if (!RegisterClassExW(&winClass)) {
			MessageBoxA(0, "RegisterClassEx failed", "Fatal Error", MB_OK);
			return GetLastError();
		}

		RECT initialRect = { 0, 0, width, height };
		AdjustWindowRectEx(&initialRect, WS_OVERLAPPEDWINDOW, FALSE, WS_EX_OVERLAPPEDWINDOW);
		LONG initialWidth = initialRect.right - initialRect.left;
		LONG initialHeight = initialRect.bottom - initialRect.top;

		hwnd = CreateWindowExW(WS_EX_OVERLAPPEDWINDOW,
							   winClass.lpszClassName,
							   name2 ? name2 : L"Rem's Engine",
							   WS_OVERLAPPEDWINDOW | WS_VISIBLE,
							   CW_USEDEFAULT, CW_USEDEFAULT,
							   initialWidth,
							   initialHeight,
							   0, 0, hInstance, 0);

		window->hwnd = hwnd;

		RECT winRect;
		GetClientRect(window->hwnd, &winRect);
		window->width = winRect.right - winRect.left;
		window->height = winRect.bottom - winRect.top;

		std::cout << "Resize: " << window->width << " x " << window->height << std::endl;

		if (!hwnd) {
			MessageBoxA(0, "CreateWindowEx failed", "Fatal Error", MB_OK);
			return GetLastError();
		}
	}

	if (env) env->ReleaseStringChars(name, name1);
	return (jlong)window;
}

JNIEXPORT void JNICALL Java_me_anno_directx11_DirectX_setWindowTitle
(JNIEnv* env, jclass, jlong handle, jstring title) {
	if (!handle || !title) return;
	Window* window = (Window*)handle;
	jboolean copy;
	jsize length = env->GetStringUTFLength(title);
	const char* src = env->GetStringUTFChars(title, &copy);
	char* dst = new char[length + 1];
	memcpy(dst, src, length);
	dst[length] = 0;
	SetWindowTextA(window->hwnd, dst);
	env->ReleaseStringUTFChars(title, src);
	delete[] dst;
}


#include <codecvt>
std::wstring StrToWStr(std::string src) {
	std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
	return converter.from_bytes(src);
}

JNIEXPORT void JNICALL Java_me_anno_directx11_DirectX_setWindowIcon
(JNIEnv*, jclass, jlong windowPtr, jint width, jint height, jlong pixels) {

	// didn't work :/
	/*if (!windowPtr || !pixels || width <= 0 || height <= 0) return;
	Window* window = (Window*)windowPtr;
	if (!window->hwnd) return;*/

	// even with loading from a path, the result is transparent :/
	/*std::cout << L"Icon path: " << (char*) pixels << std::endl;

	std::wstring path = StrToWStr((char*) pixels);
	HICON handle = (HICON)LoadImageW(NULL, path.c_str(), IMAGE_ICON, 0, 0, LR_LOADFROMFILE);

	if (handle) {
		std::cout << "Setting icon :)" << std::endl;
		SendMessageW(window->hwnd, WM_SETICON, ICON_BIG, (LPARAM)handle);
		SendMessageW(window->hwnd, WM_SETICON, ICON_SMALL, (LPARAM)handle);
		DestroyIcon(handle);
	}
	else {
		std::cout << "Failed setting icon :/" << std::endl;
	}*/
}

JNIEXPORT jlong JNICALL Java_me_anno_directx11_DirectX_doAttachDirectX
(JNIEnv*, jclass) {
	Window* window = currentWindow;
	if (!window) return 0;
	CreateDeviceAndContext(*window);
	if (!window->device) return 0;
	SetupDebugging(*window);
	CreateSwapChain(*window);
	CreateRenderTargetView(*window);
	CheckLastError("AttachDirectX");

	D3D11_BUFFER_DESC bufferDesc = {};
	bufferDesc.ByteWidth = (UINT)(16 << 20); // 16MB... and this whole thing will GPU-segfault, if we have more than 4M unbound elements
	bufferDesc.Usage = D3D11_USAGE_IMMUTABLE; // D3D11_USAGE_IMMUTABLE;
	bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER; // D3D11_BIND_VERTEX_BUFFER or D3D11_BIND_INDEX_BUFFER;

	D3D11_SUBRESOURCE_DATA subresourceData = {};
	void* zeroData = calloc(1, bufferDesc.ByteWidth);
	subresourceData.pSysMem = (void*) zeroData;

	HRESULT hResult = window->device->CreateBuffer(&bufferDesc, &subresourceData, &window->nullBuffer);
	free(zeroData);
	if (FAILED(hResult)) {
		std::cerr << "CreateBuffer#0" << std::endl;
		PrintError(hResult, "CreateBuffer");
		return 0;
	}

	return (jlong)window->primaryRTV;
}

void CreateDeviceAndContext(Window& window) {

	ID3D11Device* baseDevice;
	ID3D11DeviceContext* baseDeviceContext;
	D3D_FEATURE_LEVEL featureLevels[] = { D3D_FEATURE_LEVEL_11_0 };
	UINT creationFlags = 0;

	HRESULT hResult = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE,
										nullptr, creationFlags,
										featureLevels, ARRAYSIZE(featureLevels),
										D3D11_SDK_VERSION, &baseDevice,
										0, &baseDeviceContext);
	if (FAILED(hResult)) {
		std::cerr << "First D3D11CreateDevice failed" << std::endl;
		hResult = D3D11CreateDevice(
			nullptr, D3D_DRIVER_TYPE_WARP,
			nullptr, creationFlags,
			featureLevels, ARRAYSIZE(featureLevels),
			D3D11_SDK_VERSION, &baseDevice,
			nullptr, &baseDeviceContext
		);
		if (FAILED(hResult)) {
			PrintError(hResult, "D3D11CreateDevice");
			window.device = nullptr;
			window.deviceContext = nullptr;
			return;
		}
	}

	// Get 1.1 interface of D3D11 Device and Context
	hResult = baseDevice->QueryInterface(__uuidof(ID3D11Device1), (void**)&window.device);
	assert(SUCCEEDED(hResult));
	baseDevice->Release();

	hResult = baseDeviceContext->QueryInterface(__uuidof(ID3D11DeviceContext1), (void**)&window.deviceContext);
	assert(SUCCEEDED(hResult));
	baseDeviceContext->Release();

}

void CreateSwapChain(Window& window) {
	HWND hwnd = window.hwnd;
	{
		// Get DXGI Factory (needed to create SwapChain)
		IDXGIFactory2* dxgiFactory = nullptr;
		{
			IDXGIDevice1* dxgiDevice = nullptr;
			HRESULT hResult = currentWindow->device->QueryInterface(__uuidof(IDXGIDevice1), (void**)&dxgiDevice);
			assert(SUCCEEDED(hResult));

			IDXGIAdapter* dxgiAdapter = nullptr;
			hResult = dxgiDevice->GetAdapter(&dxgiAdapter);
			assert(SUCCEEDED(hResult));
			dxgiDevice->Release();

			DXGI_ADAPTER_DESC adapterDesc;
			dxgiAdapter->GetDesc(&adapterDesc);

			OutputDebugStringA("Graphics Device: ");
			OutputDebugStringW(adapterDesc.Description);

			hResult = dxgiAdapter->GetParent(__uuidof(IDXGIFactory2), (void**)&dxgiFactory);
			assert(SUCCEEDED(hResult));
			dxgiAdapter->Release();
		}

		DXGI_SWAP_CHAIN_DESC1 d3d11SwapChainDesc = {};
		d3d11SwapChainDesc.Width = 0; // use window width
		d3d11SwapChainDesc.Height = 0; // use window height
		d3d11SwapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		d3d11SwapChainDesc.SampleDesc.Count = 1;
		d3d11SwapChainDesc.SampleDesc.Quality = 0;
		d3d11SwapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		d3d11SwapChainDesc.BufferCount = 2;
		d3d11SwapChainDesc.Scaling = DXGI_SCALING_STRETCH;
		d3d11SwapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
		d3d11SwapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
		d3d11SwapChainDesc.Flags = 0;

		HRESULT hResult = dxgiFactory->CreateSwapChainForHwnd(currentWindow->device, hwnd, &d3d11SwapChainDesc, 0, 0, &window.swapChain);
		assert(SUCCEEDED(hResult));

		dxgiFactory->Release();
	}
}

void CreateRenderTargetView(Window& window) {

	ID3D11Texture2D* fb = nullptr;
	HRESULT hResult = window.swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&fb);
	if (FAILED(hResult)) {
		PrintError(hResult, "CreateWindowRTV/1");
		return;
	}
	assert(fb != nullptr);

	hResult = window.device->CreateRenderTargetView(fb, 0, &window.primaryRTV);
	if (FAILED(hResult)) {
		PrintError(hResult, "CreateWindowRTV/2");
		return;
	}
	fb->Release();

}

void SetupDebugging(Window& window) {

	ID3D11Device1* device = window.device;
#ifdef DEBUG_BUILD
	// Set up debug layer to break on D3D11 errors
	ID3D11Debug* d3dDebug = nullptr;
	device->QueryInterface(__uuidof(ID3D11Debug), (void**)&d3dDebug);
	device->QueryInterface(__uuidof(ID3D11InfoQueue), (void**)&window.debugInfoQueue);
	if (window.debugInfoQueue) window.debugInfoQueue->PushEmptyStorageFilter();
	if (d3dDebug)
	{
		ID3D11InfoQueue* d3dInfoQueue = nullptr;
		if (SUCCEEDED(d3dDebug->QueryInterface(__uuidof(ID3D11InfoQueue), (void**)&d3dInfoQueue)))
		{
			// d3dInfoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_WARNING, true);
			d3dInfoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_CORRUPTION, true);
			d3dInfoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_ERROR, true);
			d3dInfoQueue->Release();
		}
		d3dDebug->Release();
		std::cerr << "Attached debugger to halt on error" << std::endl;
	}
	else std::cerr << "Debugging not supported" << std::endl;

	// check for debug layer:
	// todo this is not working :(
	/*for (int i = 0; i < 10; i++) {
		d3d11Device->AddRef();
	}
	PrintDebugMessages();*/
#else
	std::cerr << "Debugging flag is not enabled" << std::endl;
#endif
}

#pragma endregion Context

#pragma region DepthStencil

ID3D11DepthStencilState* CreateDepthStencilState(uint8_t depthBits) {
	D3D11_DEPTH_STENCIL_DESC desc;

	// Depth test parameters
	desc.DepthWriteMask = (D3D11_DEPTH_WRITE_MASK)((depthBits >> 3) & 1); // | 2
	desc.DepthFunc = (D3D11_COMPARISON_FUNC)((depthBits & 7) + 1); // | 8
	desc.DepthEnable = desc.DepthFunc != D3D11_COMPARISON_ALWAYS;
	/*
	D3D11_COMPARISON_NEVER	= 1,
	D3D11_COMPARISON_LESS	= 2,
	D3D11_COMPARISON_EQUAL	= 3,
	D3D11_COMPARISON_LESS_EQUAL	= 4,
	D3D11_COMPARISON_GREATER	= 5,
	D3D11_COMPARISON_NOT_EQUAL	= 6,
	D3D11_COMPARISON_GREATER_EQUAL	= 7,
	D3D11_COMPARISON_ALWAYS	= 8
	*/

	// Stencil test parameters
	desc.StencilEnable = false;						// | 2
	desc.StencilReadMask = 0xFF;						// | 256
	desc.StencilWriteMask = 0xFF;						// | 256

	/* can OpenGL do this, too? -> yes xD
	D3D11_STENCIL_OP_KEEP	= 1,
	D3D11_STENCIL_OP_ZERO	= 2,
	D3D11_STENCIL_OP_REPLACE	= 3,
	D3D11_STENCIL_OP_INCR_SAT	= 4,
	D3D11_STENCIL_OP_DECR_SAT	= 5,
	D3D11_STENCIL_OP_INVERT	= 6,
	D3D11_STENCIL_OP_INCR	= 7,
	D3D11_STENCIL_OP_DECR	= 8
	*/
	// Stencil operations if pixel is front-facing
	desc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	desc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_INCR;
	desc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	desc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

	// Stencil operations if pixel is back-facing
	desc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	desc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_DECR;
	desc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	desc.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

	// Create depth stencil state
	ID3D11DepthStencilState* state;
	HRESULT hr = currentWindow->device->CreateDepthStencilState(&desc, &state);
	if (FAILED(hr)) {
		PrintError(hr, "CreateDepthStencilState");
		return 0;
	}
	return state;
}

#pragma endregion DepthStencil

#pragma region Blending

ID3D11BlendState* CreateSimpleBlendState(jint id) {
	if (!currentWindow || !currentWindow->device) return nullptr;
	D3D11_BLEND_DESC desc{};

	// flags 2 -> 2
	desc.AlphaToCoverageEnable = (id & 1) != 0;   //				 | 2
	desc.IndependentBlendEnable = false; //							 | 1, if disabled, all targets will use 0th value
	desc.RenderTarget[0].BlendEnable = (id & 2) != 0; //	         | 1?, only needs one special value

	// blend op 6 -> 8
	desc.RenderTarget[0].BlendOp = (D3D11_BLEND_OP)((id >> 2) & 7); //      | 5, +, -, rev-, min, max
	desc.RenderTarget[0].BlendOpAlpha = (D3D11_BLEND_OP)((id >> 5) & 7); // | 5, +, -, rev-, min, max

	// blend values 4 * 5 -> 26
	/* D3D11_BLEND_ZERO	= 1,
		D3D11_BLEND_ONE	= 2,
		D3D11_BLEND_SRC_COLOR	= 3,
		D3D11_BLEND_INV_SRC_COLOR	= 4,
		D3D11_BLEND_SRC_ALPHA	= 5,
		D3D11_BLEND_INV_SRC_ALPHA	= 6,
		D3D11_BLEND_DEST_ALPHA	= 7,
		D3D11_BLEND_INV_DEST_ALPHA	= 8,
		D3D11_BLEND_DEST_COLOR	= 9,
		D3D11_BLEND_INV_DEST_COLOR	= 10,
		D3D11_BLEND_SRC_ALPHA_SAT	= 11,
		D3D11_BLEND_BLEND_FACTOR	= 14,
		D3D11_BLEND_INV_BLEND_FACTOR	= 15,
		D3D11_BLEND_SRC1_COLOR	= 16,
		D3D11_BLEND_INV_SRC1_COLOR	= 17,
		D3D11_BLEND_SRC1_ALPHA	= 18,
		D3D11_BLEND_INV_SRC1_ALPHA	= 19 */
	desc.RenderTarget[0].SrcBlend = (D3D11_BLEND)((id >> 8) & 31); //       | 19
	desc.RenderTarget[0].SrcBlendAlpha = (D3D11_BLEND)((id >> 13) & 31); // | 19
	desc.RenderTarget[0].DestBlend = (D3D11_BLEND)((id >> 18) & 31); //		| 19
	desc.RenderTarget[0].DestBlendAlpha = (D3D11_BLEND)((id >> 23) & 31);// | 19

	// mask, unused
	desc.RenderTarget[0].RenderTargetWriteMask = (UINT8) ((id >> 28) & 15); // rgba bitmask, r=1,g=2,b=4,a=8
	// possibilities: 19^4 * 100 = 12M, or upto ^8, if separate blending is used

	ID3D11BlendState* result;
	currentWindow->device->CreateBlendState(&desc, &result);
	return result;
}

ID3D11BlendState* CreateComplexBlendState(JNIEnv* env, jintArray ids) {
	if (!currentWindow || !currentWindow->device) return nullptr;
	D3D11_BLEND_DESC desc{};

	jint id = 0;
	env->GetIntArrayRegion(ids, 0, 1, &id);

	// flags 1 -> 1
	desc.AlphaToCoverageEnable = (id & 1) != 0;   //				 | 2
	desc.IndependentBlendEnable = true; //							 | 1, if disabled, all targets will use 0th value

	for (int i = 0; i < 8; i++) {
		env->GetIntArrayRegion(ids, 0, i, &id);

		desc.RenderTarget[i].BlendEnable = id != 0; //					 | 1?, only needs one special value

		// blend op 6 -> 7
		desc.RenderTarget[i].BlendOp = (D3D11_BLEND_OP)((id >> 1) & 7); //      | 5, +, -, rev-, min, max
		desc.RenderTarget[i].BlendOpAlpha = (D3D11_BLEND_OP)((id >> 4) & 7); // | 5, +, -, rev-, min, max

		// blend values 4 * 5 -> 26
		/* D3D11_BLEND_ZERO	= 1,
			D3D11_BLEND_ONE	= 2,
			D3D11_BLEND_SRC_COLOR	= 3,
			D3D11_BLEND_INV_SRC_COLOR	= 4,
			D3D11_BLEND_SRC_ALPHA	= 5,
			D3D11_BLEND_INV_SRC_ALPHA	= 6,
			D3D11_BLEND_DEST_ALPHA	= 7,
			D3D11_BLEND_INV_DEST_ALPHA	= 8,
			D3D11_BLEND_DEST_COLOR	= 9,
			D3D11_BLEND_INV_DEST_COLOR	= 10,
			D3D11_BLEND_SRC_ALPHA_SAT	= 11,
			D3D11_BLEND_BLEND_FACTOR	= 14,
			D3D11_BLEND_INV_BLEND_FACTOR	= 15,
			D3D11_BLEND_SRC1_COLOR	= 16,
			D3D11_BLEND_INV_SRC1_COLOR	= 17,
			D3D11_BLEND_SRC1_ALPHA	= 18,
			D3D11_BLEND_INV_SRC1_ALPHA	= 19 */
		desc.RenderTarget[i].SrcBlend = (D3D11_BLEND)((id >> 7) & 31); //      | 19
		desc.RenderTarget[i].SrcBlendAlpha = (D3D11_BLEND)((id >> 12) & 31); // | 19
		desc.RenderTarget[i].DestBlend = (D3D11_BLEND)((id >> 17) & 31); //		| 19
		desc.RenderTarget[i].DestBlendAlpha = (D3D11_BLEND)((id >> 21) & 31);// | 19

		// mask, unused
		desc.RenderTarget[i].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL; // rgba bitmask, r=1,g=2,b=4,a=8
		// possibilities: 19^4 * 100 = 12M, or upto ^8, if separate blending is used
	}

	ID3D11BlendState* result;
	currentWindow->device->CreateBlendState(&desc, &result);
	return result;
}

#include <unordered_map>

ID3D11RasterizerState* CreateCullState(Window* window, jint culling, jboolean scissor) {
	D3D11_RASTERIZER_DESC desc = {
		D3D11_FILL_SOLID, // fill-mode
		(D3D11_CULL_MODE) (culling + 1), // disable culling			| 3
		true, // FrontCounterClockwise, we have to test this...
		0, // depth-bias,
		0.0f, 0.0f, // depth-bias clamp, slope-scaled-depth-bias,
		true, // depth-clip-enable									| 2
		scissor, // scissor enable,									| 2
		false, // multi-sample enable,								| 2
		false, // anti-aliased lines enable							| 2
	};

	ID3D11RasterizerState* state;
	HRESULT hResult = window->device->CreateRasterizerState(&desc, &state);
	if (FAILED(hResult)) {
		PrintError(hResult, "CreateRasterizerState");
		return 0;
	}
	return state;
}

JNIEXPORT void JNICALL Java_me_anno_directx11_DirectX_setPipelineState
(JNIEnv*, jclass, jint blendState, jint depthState, jint stencilState, jint cullScissorState) {
	static std::unordered_map<jint, ID3D11BlendState*> blendStates = {};
	static ID3D11DepthStencilState* depthStates[16] = {};
	static ID3D11RasterizerState* cullStates[8] = {};

	static jint lastBlendState = -1, lastDepthState = -1, lastCulling = -1;

	Window* window = currentWindow;
	if (!window || !window->device || !window->deviceContext) return;

	// blending
	if(lastBlendState != blendState) {
		ID3D11BlendState* blendState1 = blendStates[blendState];
		if (!blendState1) {
			blendStates[blendState] = blendState1 = CreateSimpleBlendState(blendState);
		}
		FLOAT blendFactor = 0;
		currentWindow->deviceContext->OMSetBlendState(blendState1, &blendFactor, -1);
		lastBlendState = blendState;
	}

	// depth and stencil
	if(lastDepthState != depthState) {
		ID3D11DepthStencilState* depthState1 = depthStates[depthState];
		if (!depthState1) {
			depthStates[depthState] = depthState1 = CreateDepthStencilState(depthState);
		}
		UINT stencilRef = 0;// stencil comparsion value
		currentWindow->deviceContext->OMSetDepthStencilState(depthState1, stencilRef);
		lastDepthState = depthState;
	}

	// culling
	if(lastCulling != cullScissorState) {
		ID3D11RasterizerState* cullState1 = cullStates[cullScissorState];
		if (!cullState1) {
			jint culling = cullScissorState & 3;
			jboolean scissor = (cullScissorState & 4) != 0;
			cullStates[cullScissorState] = cullState1 = CreateCullState(window, culling, scissor);
		}
		window->deviceContext->RSSetState(cullState1);
		lastCulling = cullScissorState;
	}

}

#pragma endregion Blending

#pragma region Framebuffers

JNIEXPORT void JNICALL Java_me_anno_directx11_DirectX_swapBuffers(JNIEnv*, jclass, jlong handle) {
	Window* window = (Window*)handle;
	if (window && window->swapChain) {
		HRESULT hr = window->swapChain->Present(window->vsyncInterval, 0);
		if (FAILED(hr)) PrintError(hr, "Present"); 
	}
}

JNIEXPORT void JNICALL Java_me_anno_directx11_DirectX_setVsyncInterval(JNIEnv*, jclass, jint interval) {
	Window* window = currentWindow;
	if(window) window->vsyncInterval = std::min<jint>(std::max<jint>(interval, 0), 4);
}

JNIEXPORT void JNICALL Java_me_anno_directx11_DirectX_updateViewport
(JNIEnv*, jclass, jfloat x, jfloat y, jfloat w, jfloat h, jfloat z0, jfloat z1) {
	if (!currentWindow || !currentWindow->deviceContext) return;
	D3D11_VIEWPORT viewport = { x, y, w, h, z0, z1 };
	// RECT winRect;
	// GetClientRect(currentWindow->hwnd, &winRect);
	// jint width = winRect.right - winRect.left;
	// jint height = winRect.bottom - winRect.top;
	// std::cout << "Drawing " << x << "," << y << " += " << w << " x " << h << " / " << width << " x " << height << std::endl;
	currentWindow->deviceContext->RSSetViewports(1, &viewport);
	// CheckLastError("SetViewport");
}

JNIEXPORT void JNICALL Java_me_anno_directx11_DirectX_updateScissor
(JNIEnv*, jclass, jfloat x, jfloat y, jfloat w, jfloat h) {
	if (!currentWindow || !currentWindow->deviceContext) return;
	D3D11_RECT rect = { x, y, w, h };
	currentWindow->deviceContext->RSSetScissorRects(1, &rect);
}

JNIEXPORT void JNICALL Java_me_anno_directx11_DirectX_getFramebufferSize(JNIEnv* env, jclass, jlong handle, jintArray x, jintArray y) {
	if (!handle) return;
	Window* window = (Window*)handle;
	if (x && env->GetArrayLength(x) > 0) env->SetIntArrayRegion(x, 0, 1, &window->width);
	if (y && env->GetArrayLength(y) > 0) env->SetIntArrayRegion(y, 0, 1, &window->height);
	CheckLastError("GetFramebufferSize");
}

JNIEXPORT void JNICALL Java_me_anno_directx11_DirectX_getCursorPos
(JNIEnv* env, jclass, jlong handle, jdoubleArray x, jdoubleArray y) {
	if (!handle) return;
	Window* window = (Window*)handle;
	POINT cursorPos;
	GetCursorPos(&cursorPos);  // Get the mouse position relative to the screen coordinates
	ScreenToClient(window->hwnd, &cursorPos); // Convert the screen coordinates to client coordinates
	jdouble vx = cursorPos.x, vy = cursorPos.y;
	if (x && env->GetArrayLength(x) > 0) env->SetDoubleArrayRegion(x, 0, 1, &vx);
	if (y && env->GetArrayLength(y) > 0) env->SetDoubleArrayRegion(y, 0, 1, &vy);
	CheckLastError("GetCursorPos");
}

JNIEXPORT void JNICALL Java_me_anno_directx11_DirectX_clearColor(JNIEnv*, jclass, jlong fb, jfloat r, jfloat g, jfloat b, jfloat a) {
	Window* window = currentWindow;
	ID3D11DeviceContext1* ctx = window ? window->deviceContext : nullptr;
	if (ctx && fb) {
		FLOAT backgroundColor[4] = { r, g, b, a };
		ctx->ClearRenderTargetView((ID3D11RenderTargetView*)fb, backgroundColor);
		CheckLastError("ClearColor");
	}
}

JNIEXPORT void JNICALL Java_me_anno_directx11_DirectX_clearDepth(JNIEnv*, jclass, jlong fb, jfloat depth, jint stencil, jint dxMask) {
	Window* window = currentWindow;
	ID3D11DeviceContext1* ctx = window ? window->deviceContext : nullptr;
	if (ctx && fb) {
		ctx->ClearDepthStencilView((ID3D11DepthStencilView*)fb, dxMask, depth, (UINT8) stencil);
		CheckLastError("ClearDepth");
	}
}

#pragma endregion Framebuffers

#pragma region Textures

JNIEXPORT jlong JNICALL Java_me_anno_directx11_DirectX_createColorRTV
(JNIEnv*, jclass, jlong tex) {
	Window* window = currentWindow;
	ID3D11Device1* device = window ? window->device : nullptr;
	if (device && tex) {
		ID3D11RenderTargetView* rtv = nullptr;
		Texture* tex1 = (Texture*)tex;
		if (!tex1->texture) return 0;
		HRESULT hr = device->CreateRenderTargetView(tex1->texture, nullptr, &rtv);
		if (FAILED(hr)) {
			PrintError(hr, "CreateRenderTargetView");
			return 0;
		}
		return (jlong) rtv;
	}
	else return 0;
}

JNIEXPORT jlong JNICALL Java_me_anno_directx11_DirectX_createDepthRTV
(JNIEnv*, jclass, jlong tex) {
	Window* window = currentWindow;
	ID3D11Device1* device = window ? window->device : nullptr;
	if (device && tex) {
		ID3D11DepthStencilView* rtv = nullptr;
		Texture* tex1 = (Texture*)tex;
		if (!tex1->texture) return 0;
		D3D11_DEPTH_STENCIL_VIEW_DESC desc{};
		desc.Flags = 0;
		desc.Format = tex1->depthFormat;// DXGI_FORMAT_D32_FLOAT;
		desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D; // use D3D11_DSV_DIMENSION_TEXTURE2DMS for multisampled targets?
		HRESULT hr = device->CreateDepthStencilView(tex1->texture, &desc, &rtv);
		if (FAILED(hr)) {
			PrintError(hr, "CreateDepthStencilView");
			return 0;
		}
		return (jlong)rtv;
	}
	else return 0;
}

JNIEXPORT void JNICALL Java_me_anno_directx11_DirectX_destroyColorRTV
(JNIEnv*, jclass, jlong rtv1) {
	ID3D11RenderTargetView* rtv = (ID3D11RenderTargetView*)rtv1;
	rtv->Release();
}

JNIEXPORT void JNICALL Java_me_anno_directx11_DirectX_destroyDepthRTV
(JNIEnv*, jclass, jlong rtv1) {
	ID3D11DepthStencilView* rtv = (ID3D11DepthStencilView*)rtv1;
	rtv->Release();
}

#include <algorithm>
#include <unordered_map>
ID3D11SamplerState* simpleSamplers[512];
std::unordered_map<uint64_t, ID3D11SamplerState*> coloredSamplers;

D3D11_FILTER simpleFilters[18] = {
	D3D11_FILTER_MIN_MAG_MIP_POINT,
	D3D11_FILTER_MIN_MAG_POINT_MIP_LINEAR,
	D3D11_FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT,
	D3D11_FILTER_MIN_POINT_MAG_MIP_LINEAR,
	D3D11_FILTER_MIN_LINEAR_MAG_MIP_POINT,
	D3D11_FILTER_MIN_LINEAR_MAG_POINT_MIP_LINEAR,
	D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT,
	D3D11_FILTER_MIN_MAG_MIP_LINEAR,
	D3D11_FILTER_ANISOTROPIC,
	D3D11_FILTER_COMPARISON_MIN_MAG_MIP_POINT,
	D3D11_FILTER_COMPARISON_MIN_MAG_POINT_MIP_LINEAR,
	D3D11_FILTER_COMPARISON_MIN_POINT_MAG_LINEAR_MIP_POINT,
	D3D11_FILTER_COMPARISON_MIN_POINT_MAG_MIP_LINEAR,
	D3D11_FILTER_COMPARISON_MIN_LINEAR_MAG_MIP_POINT,
	D3D11_FILTER_COMPARISON_MIN_LINEAR_MAG_POINT_MIP_LINEAR,
	D3D11_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT,
	D3D11_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR,
	D3D11_FILTER_COMPARISON_ANISOTROPIC,
};

ID3D11SamplerState* getSampler(Window* window, uint64_t code) {
	if (code < 512) {
		ID3D11SamplerState* ptr = simpleSamplers[code];
		if (!ptr) {

			// create sampler
			int compIdx = (code >> 6) & 3;
			int filterIdx = std::min<int>(code & 15, 8);
			D3D11_SAMPLER_DESC samplerDesc = {};
			samplerDesc.Filter = simpleFilters[filterIdx + (compIdx == 0 ? 0 : 9)];
			D3D11_TEXTURE_ADDRESS_MODE addrMode = (D3D11_TEXTURE_ADDRESS_MODE)(((code >> 4) & 3) + 1);
			samplerDesc.AddressU = addrMode;
			samplerDesc.AddressV = addrMode;
			samplerDesc.AddressW = addrMode;
			samplerDesc.ComparisonFunc = (D3D11_COMPARISON_FUNC)(compIdx + 1);

			std::cout << "Creating sampler " << code << std::endl;

			HRESULT hr = window->device->CreateSamplerState(&samplerDesc, &ptr);
			if (FAILED(hr)) {
				PrintError(hr, "CreateSamplerState1");
				return 0;
			}

			simpleSamplers[code] = ptr;
		}
		return ptr;
	}
	else {
		ID3D11SamplerState* ptr = coloredSamplers[code];
		if (!ptr) {
			// todo create sampler
			std::cerr << "Todo: create sampler " << code << std::endl;
			coloredSamplers[code] = ptr;
		}
		return ptr;
	}
}

// 16 is the internal limit for DX11
ID3D11ShaderResourceView* textureViews[16];
ID3D11SamplerState* samplerStates[16];
JNIEXPORT void JNICALL Java_me_anno_directx11_DirectX_bindTextures
(JNIEnv* env, jclass, jint numTextures, jlong texturePtrs) {
	Window* window = currentWindow;
	ID3D11DeviceContext1* ctx = window ? window->deviceContext : nullptr;
	if (!ctx || !texturePtrs || numTextures < 0 || numTextures > 16) return;

	Texture** textures = (Texture**)texturePtrs;
	uint64_t* samplers = (uint64_t*)texturePtrs;
	for (uint32_t i = 0; i < (uint32_t) numTextures; i++) {
		textureViews[i] = textures[i]->textureView;
		samplerStates[i] = getSampler(window, samplers[i + numTextures]);
		// std::cerr << "Binding[" << i << "] " << std::hex << textureViews[i] << "/" << samplerStates[i] << std::dec << std::endl;
	}

	ctx->PSSetShaderResources(0, numTextures, textureViews);
	ctx->PSSetSamplers(0, numTextures, samplerStates);
	CheckLastError("BindTextures");
}

DXGI_FORMAT mapTextureFormat(jint format) {
	// map texture formats
	switch (format) {
	case 0x8051: // rgb8
	case 0x8058: // rgba8
		return DXGI_FORMAT_R8G8B8A8_UNORM;
	case 0x8059: // rgb10a2
	case 0x805a: // rgba12
	case 0x805b: // rgba16
		return DXGI_FORMAT_R16G16B16A16_UNORM;
	case 0x8229:
		return DXGI_FORMAT_R8_UNORM;
	case 0x822a:
		return DXGI_FORMAT_R16_UNORM;
	case 0x822b: // rg8
	case 0x8227: // rg
		return DXGI_FORMAT_R8G8_UNORM;
	case 0x822c:
		return DXGI_FORMAT_R16G16_UNORM;
	case 0x822d: // r16f
		return DXGI_FORMAT_R16_FLOAT;
	case 0x822e: // r32f
		return DXGI_FORMAT_R32_FLOAT;
	case 0x822f: // rg16f
		return DXGI_FORMAT_R16G16_FLOAT;
	case 0x8230: // rg32f
		return DXGI_FORMAT_R32G32_FLOAT;
	case 0x8231:
		return DXGI_FORMAT_R8_SINT;
	case 0x8232:
		return DXGI_FORMAT_R8_UINT;
	case 0x8233:
		return DXGI_FORMAT_R16_SINT;
	case 0x8234:
		return DXGI_FORMAT_R16_UINT;
	case 0x8235:
		return DXGI_FORMAT_R32_SINT;
	case 0x8236:
		return DXGI_FORMAT_R32_UINT;
	case 0x8237:
		return DXGI_FORMAT_R8G8_SINT;
	case 0x8238:
		return DXGI_FORMAT_R8G8_UINT;
	case 0x8239:
		return DXGI_FORMAT_R16G16_SINT;
	case 0x823a:
		return DXGI_FORMAT_R16G16_UINT;
	case 0x823b:
		return DXGI_FORMAT_R32G32_SINT;
	case 0x823c:
		return DXGI_FORMAT_R32G32_UINT;
	case 0x8814:
		return DXGI_FORMAT_R32G32B32A32_FLOAT;
	case 0x8815:
		return DXGI_FORMAT_R32G32B32_FLOAT;
	case 0x881a:
		return DXGI_FORMAT_R16G16B16A16_FLOAT;
	case 0x881b: // rgb16f, doesn't exist
		return DXGI_FORMAT_R16G16B16A16_FLOAT;
		// depth formats: ? https://gamedev.stackexchange.com/questions/26687/what-are-the-valid-depthbuffer-texture-formats-in-directx-11-and-which-are-also
	case 0x81a5: // depth 16
		return DXGI_FORMAT_D16_UNORM;
	case 0x81a6: // depth 24
		return DXGI_FORMAT_D24_UNORM_S8_UINT;
	case 0x81a7: // depth 32
		return DXGI_FORMAT_D32_FLOAT;
	case 0x8cac: // depth 32f
		return DXGI_FORMAT_D32_FLOAT;
	case 0x8cad: // depth 32f, stencil 8
		return DXGI_FORMAT_D32_FLOAT_S8X24_UINT; // what is this X? unused?
	default:
		std::cerr << "Unsupported texture format: " << format << std::endl;
		return DXGI_FORMAT_UNKNOWN;
	}
}
jint mapTextureFormatSize(jint format, jint type) {
	// map texture formats
	jint channels = 0;
	switch (format) {
	case 0x1903: // red
	case 0x1902: // depth
	case 0x8D94: // red integer
		channels = 1;
		break;
	case 0x8227: // rg
	case 0x8228: // rg integer
		channels = 2;
		break;
	case 0x1907: // rgb
	case 0x8D98: // rgb integer
	case 0x80E0: // bgr
	case 0x8D9A: // bgr integer
		channels = 3;
		break;
	case 0x1908: // rgba
	case 0x8D99: // rgba integer
	case 0x8D9B: // bgra integer
		channels = 4;
		break;
	default:
		std::cerr << "Unknown format: " << format << std::endl;
		break;
	}
	jint perChannel = 0;
	switch (type-0x1400) {
	case 0x0: // byte
	case 0x1: // ubyte
		perChannel = 1;
		break;
	case 0x2: // short
	case 0x3: // ushort
	case 0xb: // half float
		perChannel = 2;
		break;
	case 0x4: // int
	case 0x5: // uint
	case 0x6: // float
		perChannel = 4;
		break;
	case 0xa: // double
		perChannel = 8;
		break;
	default:
		std::cerr << "Unknown type: " << type << std::endl;
		break;
	}
	return channels * perChannel;
}

bool isDepthFormat(jint format) {
	// map texture formats
	switch (format) {
		// depth formats: ? https://gamedev.stackexchange.com/questions/26687/what-are-the-valid-depthbuffer-texture-formats-in-directx-11-and-which-are-also
	case 0x81a5: // depth 16
	case 0x81a6: // depth 24
	case 0x81a7: // depth 32
	case 0x8cac: // depth 32f
	case 0x8cad: // depth 32f, stencil 8
		return true;
	default:
		return false;
	}
}

void mapDepthFormat(jint format, DXGI_FORMAT& dstTexFormat, DXGI_FORMAT& dstDscFormat, DXGI_FORMAT& dstRTVFormat) {
	switch (format) {
	case 0x81a5: // depth 16
		dstTexFormat = DXGI_FORMAT_R16_TYPELESS;
		dstDscFormat = DXGI_FORMAT_R16_UNORM;
		dstRTVFormat = DXGI_FORMAT_D16_UNORM;
		break;
	case 0x81a6: // depth 24
		dstTexFormat = DXGI_FORMAT_R24G8_TYPELESS;
		dstDscFormat = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
		dstRTVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
		break;
	case 0x81a7: // depth 32
	case 0x8cac: // depth 32f
		dstTexFormat = DXGI_FORMAT_R32_TYPELESS;
		dstDscFormat = DXGI_FORMAT_R32_FLOAT;
		dstRTVFormat = DXGI_FORMAT_D32_FLOAT;
		break;
	case 0x8cad: // depth 32f, stencil 8
		dstTexFormat = DXGI_FORMAT_R32G8X24_TYPELESS;
		dstDscFormat = DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS;
		dstRTVFormat = DXGI_FORMAT_D32_FLOAT_S8X24_UINT; // what is this X? unused?
		break;
	default:
		std::cerr << "Unsupported texture format: " << format << std::endl;
		dstTexFormat = dstDscFormat = dstRTVFormat = DXGI_FORMAT_UNKNOWN;
	}
}

JNIEXPORT jlong JNICALL Java_me_anno_directx11_DirectX_createTexture2DMS
(JNIEnv*, jclass, jint samples, jint format, jint width, jint height) {

	// std::cerr << "Creating " << width << "x" << height << " texture with " << format << " format and " << samples << " samples" << std::endl;
	bool isDF = isDepthFormat(format);
	DXGI_FORMAT texFormat = DXGI_FORMAT_UNKNOWN, dscFormat = DXGI_FORMAT_UNKNOWN, rtvFormat = DXGI_FORMAT_UNKNOWN;
	if (isDF) {
		mapDepthFormat(format, texFormat, dscFormat, rtvFormat);
	}
	else {
		texFormat = mapTextureFormat(format);
	}
	if (texFormat == DXGI_FORMAT_UNKNOWN) return 0;
	// std::cerr << "Mapped texture format from " << format << " to " << texFormat << ", depth? " << isDF << std::endl;

	UINT maxSampleLevels = 1;
	HRESULT hResult = 0;
	if (samples > 1) {
		hResult = currentWindow->device->CheckMultisampleQualityLevels(texFormat, (UINT)samples, &maxSampleLevels);
		if (FAILED(hResult)) maxSampleLevels = 1;
		std::cerr << "Max sample levels for format: " << maxSampleLevels << std::endl;
	}

	// Create Texture
	D3D11_TEXTURE2D_DESC textureDesc = {};
	textureDesc.Width = width;
	textureDesc.Height = height;
	textureDesc.MipLevels = 1; // todo generate mip levels?; 0 = auto :3
	textureDesc.ArraySize = 1;
	textureDesc.Format = texFormat;
	textureDesc.SampleDesc.Count = samples;
	textureDesc.SampleDesc.Quality = maxSampleLevels - 1;
	textureDesc.Usage = D3D11_USAGE_DEFAULT;
	textureDesc.BindFlags = (isDF ? D3D11_BIND_DEPTH_STENCIL : D3D11_BIND_RENDER_TARGET) | D3D11_BIND_SHADER_RESOURCE;

	// std::cout << "Creating texture" << std::endl;
	ID3D11Texture2D* texture;
	hResult = currentWindow->device->CreateTexture2D(&textureDesc, nullptr, &texture);
	if (FAILED(hResult)) {
		PrintError(hResult, "CreateTexture2D");
		return 0;
	}

	ID3D11ShaderResourceView* textureView = nullptr;
	D3D11_SHADER_RESOURCE_VIEW_DESC* descPtr = nullptr;
	D3D11_SHADER_RESOURCE_VIEW_DESC desc{};
	if (isDF) {
		desc.Format = dscFormat;
		if (samples > 1) {
			desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DMS;
			// nothing to do
		}
		else {
			desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
			desc.Texture2D.MostDetailedMip = 0;
			desc.Texture2D.MipLevels = -1;// all
		}
		descPtr = &desc;
	}
	
	// std::cout << "Creating texture view" << std::endl;
	hResult = currentWindow->device->CreateShaderResourceView(texture, descPtr, &textureView);
	if (FAILED(hResult)) {
		PrintError(hResult, "CreateShaderResourceView");
		return 0;
	}

	CheckLastError("CreateTex2dMS");
	Texture* tex = new Texture { texture, textureView, (UINT) samples, rtvFormat };
	// std::cerr << "Created texture " << std::hex << tex << " -> {" << texture << ", " << textureView << "}" << std::dec << std::endl;
	return (jlong) tex;
}

JNIEXPORT jlong JNICALL Java_me_anno_directx11_DirectX_createTexture2Di
(JNIEnv* env, jclass, jint samples, jint format, jint width, jint height, jint srcFormat, jint srcType, jintArray data) {

	// std::cerr << "Creating " << width << "x" << height << " texture with " << format << " format and " << samples << " samples" << std::endl;

	DXGI_FORMAT format1 = mapTextureFormat(format);
	if (format1 == DXGI_FORMAT_UNKNOWN) return 0;
	// std::cerr << "Mapped texture2di format from " << format << " to " << format1 << std::endl;

	UINT maxSampleLevels = 1;
	HRESULT hResult = 0;
	if (samples > 1) {
		hResult = currentWindow->device->CheckMultisampleQualityLevels(format1, (UINT)samples, &maxSampleLevels);
		if (FAILED(hResult)) maxSampleLevels = 1;
		std::cerr << "Max sample levels for format: " << maxSampleLevels << std::endl;
	}

	// Create Texture
	D3D11_TEXTURE2D_DESC textureDesc = {};
	textureDesc.Width = width;
	textureDesc.Height = height;
	textureDesc.MipLevels = 1; // todo generate mip levels?; 0 = auto :3
	textureDesc.ArraySize = 1;
	textureDesc.Format = format1;
	textureDesc.SampleDesc.Count = samples;
	textureDesc.SampleDesc.Quality = maxSampleLevels - 1;
	textureDesc.Usage = D3D11_USAGE_DEFAULT;
	textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;

	jint* data1 = env->GetIntArrayElements(data, nullptr);
	D3D11_SUBRESOURCE_DATA textureSubresourceData = {};
	textureSubresourceData.pSysMem = data1;
	textureSubresourceData.SysMemPitch = width * sizeof(jint);

	// std::cerr << "Creating texture" << std::endl;
	ID3D11Texture2D* texture;
	hResult = currentWindow->device->CreateTexture2D(&textureDesc, &textureSubresourceData, &texture);
	env->ReleaseIntArrayElements(data, data1, 0); // todo what is this mode???
	if (FAILED(hResult)) {
		PrintError(hResult, "CreateTexture2Di");
		return 0;
	}

	// std::cerr << "Creating texture view" << std::endl;
	ID3D11ShaderResourceView* textureView;
	hResult = currentWindow->device->CreateShaderResourceView(texture, nullptr, &textureView);
	if (FAILED(hResult)) {
		PrintError(hResult, "CreateShaderResourceView");
		return 0;
	}

	CheckLastError("CreateTex2di");
	Texture* tex = new Texture { texture, textureView, (UINT) samples };
	// std::cerr << "Created texture " << std::hex << tex << " -> {" << texture << ", " << textureView << "}" << std::dec << std::endl;
	return (jlong)tex;

}

JNIEXPORT jlong JNICALL Java_me_anno_directx11_DirectX_createTexture2D
(JNIEnv*, jclass, jint samples, jint format, jint width, jint height, jint srcFormat, jint srcType, jlong data) {

	// std::cerr << "Creating " << width << "x" << height << " texture with " << format << " format and " << samples << " samples" << std::endl;

	DXGI_FORMAT format1 = mapTextureFormat(format);
	if (format1 == DXGI_FORMAT_UNKNOWN) return 0;
	// std::cerr << "Mapped texture format from " << format << " to " << format1 << std::endl;

	UINT maxSampleLevels = 1;
	HRESULT hResult = 0;
	if (samples > 1) {
		hResult = currentWindow->device->CheckMultisampleQualityLevels(format1, (UINT)samples, &maxSampleLevels);
		if (FAILED(hResult)) maxSampleLevels = 1;
		std::cerr << "Max sample levels for format: " << maxSampleLevels << std::endl;
	}

	// Create Texture
	D3D11_TEXTURE2D_DESC textureDesc = {};
	textureDesc.Width = width;
	textureDesc.Height = height;
	textureDesc.MipLevels = 1; // todo generate mip levels?; 0 = auto :3
	textureDesc.ArraySize = 1;
	textureDesc.Format = format1;
	textureDesc.SampleDesc.Count = samples;
	textureDesc.SampleDesc.Quality = maxSampleLevels - 1;
	textureDesc.Usage = D3D11_USAGE_DEFAULT;
	textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;

	size_t elementSize = mapTextureFormatSize(srcFormat, srcType);
	if (elementSize == 0) return 0;

	D3D11_SUBRESOURCE_DATA textureSubresourceData = {};
	textureSubresourceData.pSysMem = (void*) data;
	textureSubresourceData.SysMemPitch = (UINT) (width * elementSize);

	// std::cerr << "Creating texture" << std::endl;
	ID3D11Texture2D* texture;
	hResult = currentWindow->device->CreateTexture2D(&textureDesc, &textureSubresourceData, &texture);
	if (FAILED(hResult)) {
		PrintError(hResult, "CreateTexture2Di");
		return 0;
	}

	// std::cerr << "Creating texture view" << std::endl;
	ID3D11ShaderResourceView* textureView;
	hResult = currentWindow->device->CreateShaderResourceView(texture, nullptr, &textureView);
	if (FAILED(hResult)) {
		PrintError(hResult, "CreateShaderResourceView");
		return 0;
	}

	CheckLastError("CreateTex2d");
	Texture* tex = new Texture{ texture, textureView, (UINT) samples };
	// std::cerr << "Created texture " << std::hex << tex << " -> {" << texture << ", " << textureView << "}" << std::dec << std::endl;
	return (jlong)tex;

}

JNIEXPORT void JNICALL Java_me_anno_directx11_DirectX_updateTexture2D
(JNIEnv*, jclass, jlong texturePtr, jint x0, jint y0, jint dx, jint dy, jint srcFormat, jint srcType, jlong ptr, jlong ptrLength) {

	Window* window = currentWindow;
	ID3D11DeviceContext1* ctx = window ? window->deviceContext : nullptr;
	if (!ctx || !texturePtr || !ptr) return;
	
	if(false) std::cerr << "UpdateTexture2D " << std::hex << texturePtr << std::dec << ", " <<
		x0 << ", " << y0 << ", " << dx << " x " << dy << ", " << srcFormat << ", " << srcType <<
		", data: *" << std::hex << ptr << std::dec << std::endl;

	Texture* texture0 = (Texture*)texturePtr;
	ID3D11Texture2D* texture = (ID3D11Texture2D*) texture0->texture;
	if (!texture) return;

	D3D11_TEXTURE2D_DESC desc;
	texture->GetDesc(&desc);

	// Create a D3D11_BOX structure to define the subregion
	D3D11_BOX box;
	box.left = x0;
	box.top = y0;
	box.right = x0 + dx;
	box.bottom = y0 + dy;
	box.front = 0;
	box.back = 1;

	size_t elementSize = mapTextureFormatSize(srcFormat, srcType);
	if (!elementSize) return;

	// Prepare the updated data (assuming you have a buffer with the new data)
	const void* newData = (void*) ptr; // Pointer to the new data buffer
	UINT newDataRowPitch = (UINT) (dx * elementSize); // Row pitch of the new data
	if ((size_t)(newDataRowPitch * dy) > (size_t) ptrLength) {
		std::cerr << "Not enough data!" << std::endl;
		return;
	}

	// std::cerr << "Expected size: " << newDataRowPitch * dy << std::endl;

	// Update the subresource of the texture
	ctx->UpdateSubresource(texture, 0, &box, newData, newDataRowPitch, 0);

	// std::cerr << "Survived UpdateSubresource" << std::endl;

}

JNIEXPORT jlong JNICALL Java_me_anno_directx11_DirectX_createTexture3Di
(JNIEnv* env, jclass, jint type, jint samples, jint format, jint width, jint height, jint depth, jint srcFormat, jint srcType, jintArray data) {

	// types: 0 = 3d, 1 = tex2d[]

	std::cerr << "Creating " << width << "x" << height << "x" << depth << " texture with " << format << " format and " <<
		samples << " samples" << std::endl;

	DXGI_FORMAT format1 = mapTextureFormat(format);
	if (format1 == DXGI_FORMAT_UNKNOWN) return 0;
	std::cerr << "Mapped texture format from " << format << " to " << format1 << std::endl;

	UINT maxSampleLevels = 1;
	HRESULT hResult = 0;
	if (samples > 1) {
		hResult = currentWindow->device->CheckMultisampleQualityLevels(format1, (UINT)samples, &maxSampleLevels);
		if (FAILED(hResult)) maxSampleLevels = 1;
		std::cerr << "Max sample levels for format: " << maxSampleLevels << std::endl;
	}

	if (type == 0) {

		// Create Texture
		D3D11_TEXTURE3D_DESC textureDesc = {};
		textureDesc.Width = width;
		textureDesc.Height = height;
		textureDesc.Depth = depth;
		textureDesc.MipLevels = 1; // todo generate mip levels?; 0 = auto :3
		// textureDesc.ArraySize = 1;
		textureDesc.Format = format1;
		// textureDesc.SampleDesc.Count = samples;
		// textureDesc.SampleDesc.Quality = maxSampleLevels - 1;
		textureDesc.Usage = D3D11_USAGE_DEFAULT;
		textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;

		jint* data1 = env->GetIntArrayElements(data, nullptr);
		D3D11_SUBRESOURCE_DATA textureSubresourceData = {};
		textureSubresourceData.pSysMem = data1;
		textureSubresourceData.SysMemPitch = width * sizeof(jint);
		textureSubresourceData.SysMemSlicePitch = (size_t)width * height * sizeof(jint);

		ID3D11Texture3D* texture;
		hResult = currentWindow->device->CreateTexture3D(&textureDesc, &textureSubresourceData, &texture);
		env->ReleaseIntArrayElements(data, data1, 0); // todo what is this mode???
		if (FAILED(hResult)) {
			PrintError(hResult, "CreateTexture3D");
			return 0;
		}

		ID3D11ShaderResourceView* textureView;
		hResult = currentWindow->device->CreateShaderResourceView(texture, nullptr, &textureView);
		if (FAILED(hResult)) {
			PrintError(hResult, "CreateShaderResourceView");
			return 0;
		}

		CheckLastError("CreateTex3di");
		Texture* tex = new Texture{ texture, textureView };
		return (jlong)tex;

	}
	else if (type == 1) {

		// Create Texture
		D3D11_TEXTURE2D_DESC textureDesc = {};
		textureDesc.Width = width;
		textureDesc.Height = height;
		textureDesc.ArraySize = depth;
		textureDesc.MipLevels = 1; // todo generate mip levels?; 0 = auto :3
		textureDesc.Format = format1;
		textureDesc.SampleDesc.Count = samples;
		textureDesc.SampleDesc.Quality = maxSampleLevels - 1;
		textureDesc.Usage = D3D11_USAGE_DEFAULT;
		textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;

		jint* data1 = env->GetIntArrayElements(data, nullptr);
		// we could keep the highest value to save allocations...
		D3D11_SUBRESOURCE_DATA* textureSubresourceData = (D3D11_SUBRESOURCE_DATA*) calloc(depth, sizeof(D3D11_SUBRESOURCE_DATA));
		if (!textureSubresourceData) {
			env->ReleaseIntArrayElements(data, data1, 0);
			return -1;
		}
		
		size_t slicePitch = (size_t)width * height * sizeof(jint);
		for (int i = 0; i < depth; i++) {
			textureSubresourceData[i].pSysMem = (UINT8*) data1 + i * slicePitch;
			textureSubresourceData[i].SysMemPitch = width * sizeof(jint);
			textureSubresourceData[i].SysMemSlicePitch = slicePitch;
		}

		ID3D11Texture2D* texture;
		hResult = currentWindow->device->CreateTexture2D(&textureDesc, textureSubresourceData, &texture);
		free(textureSubresourceData);
		env->ReleaseIntArrayElements(data, data1, 0); // todo what is this mode???
		if (FAILED(hResult)) {
			PrintError(hResult, "CreateTexture2D[]");
			return 0;
		}

		ID3D11ShaderResourceView* textureView;
		hResult = currentWindow->device->CreateShaderResourceView(texture, nullptr, &textureView);
		if (FAILED(hResult)) {
			PrintError(hResult, "CreateShaderResourceView");
			return 0;
		}

		CheckLastError("CreateTex2d[]i");
		Texture* tex = new Texture{ texture, textureView, (UINT) samples };
		return (jlong)tex;

	}
	else {
		return 0;
	}
}

JNIEXPORT void JNICALL Java_me_anno_directx11_DirectX_generateMipmaps
(JNIEnv*, jclass, jlong tex) {
	// why is this called on the context, not the device?
	Texture* tex1 = (Texture*) tex; 
	ID3D11ShaderResourceView* srv = tex1 ? tex1->textureView : nullptr;
	Window* window = currentWindow;
	ID3D11DeviceContext1* ctx = window ? window->deviceContext : nullptr;
	if(ctx && srv) ctx->GenerateMips(srv);
	CheckLastError("GenerateMips");
}

#pragma endregion Textures

#pragma region ReadPixels

void readPixels(jint x, jint y, jint w, jint h, void* dst, size_t length, jlong tex, DXGI_FORMAT format) {

	// TODO we need information about the texture, which format is to be used...
	// TODO and we need to verify we don't access memory OOB

	Window* window = currentWindow;
	Texture* tex1 = (Texture*)tex;
	if (!window || !tex1) return;
	ID3D11Device1* device = window->device;
	ID3D11DeviceContext1* deviceContext = window->deviceContext;
	ID3D11Resource* tex2 = tex1->texture;
	if (!device || !deviceContext || !tex2) return;

	// Create a staging texture to hold the pixel data.
	D3D11_TEXTURE2D_DESC desc{};
	desc.Width = w;
	desc.Height = h;
	desc.MipLevels = 1;
	desc.ArraySize = 1;
	desc.Format = format;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.Usage = D3D11_USAGE_STAGING;
	desc.BindFlags = 0;
	desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
	desc.MiscFlags = 0;

	ID3D11Texture2D* stagingTexture = nullptr;
	device->CreateTexture2D(&desc, nullptr, &stagingTexture);
	if (!stagingTexture) return;

	// Copy the contents of the back buffer to the staging texture.
	D3D11_BOX box{ x, y, 0, x + w, y + h, 1 };
	deviceContext->CopySubresourceRegion(
		stagingTexture, 0, 0, 0, 0,
		tex2, 0, &box);

	// Map the staging texture to access its data.
	D3D11_MAPPED_SUBRESOURCE mappedResource{};
	deviceContext->Map(stagingTexture, 0, D3D11_MAP_READ, 0, &mappedResource);

	// Now, you can access the pixel data in the mappedResource.pData pointer.
	memcpy(dst, mappedResource.pData, length);

	// After you're done, unmap the staging texture and release it.
	deviceContext->Unmap(stagingTexture, 0);
	stagingTexture->Release();
}

JNIEXPORT void JNICALL Java_me_anno_directx11_DirectX_readPixelsI32
(JNIEnv* env, jclass, jint x, jint y, jint w, jint h, jint format, jint type, jintArray dst, jlong texture) {
	if (!dst || !texture || !currentWindow) return;
	jint* dst1 = env->GetIntArrayElements(dst, nullptr);
	readPixels(x, y, w, h, dst1, (size_t) env->GetArrayLength(dst) << 2, texture, DXGI_FORMAT_R8G8B8A8_UNORM);
	env->ReleaseIntArrayElements(dst, dst1, 0);
}

JNIEXPORT void JNICALL Java_me_anno_directx11_DirectX_readPixelsF32
(JNIEnv* env, jclass, jint x, jint y, jint w, jint h, jint format, jint type, jfloatArray dst, jlong texture) {
	if (!dst || !texture || !currentWindow) return;
	jfloat* dst1 = env->GetFloatArrayElements(dst, nullptr);
	readPixels(x, y, w, h, dst1, (size_t)env->GetArrayLength(dst) << 2, texture, DXGI_FORMAT_R32_FLOAT);
	env->ReleaseFloatArrayElements(dst, dst1, 0);
}

#pragma endregion ReadPixels

#pragma region Buffers

JNIEXPORT jlong JNICALL Java_me_anno_directx11_DirectX_createBuffer
(JNIEnv*, jclass, jint type, jlong ptr, jlong length, jint usage) {

	Window* window = currentWindow;
	if (!window) return 0;
	ID3D11Device1* device = window->device;
	if (!device) return 0;
	ID3D11Buffer* buffer;
	{
		if (length >= (uint64_t) 1 << 32) {
			std::cerr << "Buffer too large: " << length << std::endl;
			return 0;
		}

		D3D11_BUFFER_DESC bufferDesc = {};
		bufferDesc.ByteWidth = (UINT) length;
		bufferDesc.Usage = (D3D11_USAGE)usage; // D3D11_USAGE_IMMUTABLE;
		bufferDesc.BindFlags = type; // D3D11_BIND_VERTEX_BUFFER or D3D11_BIND_INDEX_BUFFER;
		
		D3D11_SUBRESOURCE_DATA subresourceData = {};
		subresourceData.pSysMem = (void*) ptr;

		HRESULT hResult = device->CreateBuffer(&bufferDesc, &subresourceData, &buffer);
		if (FAILED(hResult)) {
			std::cerr << "CreateBuffer {type: " << type <<
				", ptr: #" << std::hex << ptr << std::dec <<
				", length: " << length << ", usage: " << usage << "}" << std::endl;
			PrintError(hResult, "CreateBuffer");
			return 0;
		}
	}
	CheckLastError("CreateBuffer");
	/*std::cerr << "CreateBuffer() -> #" << std::hex << buffer << std::dec << 
		" from " << std::hex << ptr << std::dec << " += " << length << ", usage: " << usage << std::endl;*/
	return (jlong)buffer;
}

JNIEXPORT jlong JNICALL Java_me_anno_directx11_DirectX_createBufferI
(JNIEnv* env, jclass, jint type, jintArray data, jint usage) {
	Window* window = currentWindow;
	if (!window) return 0;
	ID3D11Device1* device = window->device;
	if (!device) return 0;
	ID3D11Buffer* buffer;
	uint64_t length = (uint64_t) env->GetArrayLength(data) << 2;
	{
		if (length >= (uint64_t)1 << 32) {
			std::cerr << "Buffer too large: " << length << std::endl;
			return 0;
		}

		D3D11_BUFFER_DESC bufferDesc = {};
		bufferDesc.ByteWidth = (UINT)length;
		bufferDesc.Usage = (D3D11_USAGE)usage; // D3D11_USAGE_IMMUTABLE;
		bufferDesc.BindFlags = type; // D3D11_BIND_VERTEX_BUFFER or D3D11_BIND_INDEX_BUFFER;

		D3D11_SUBRESOURCE_DATA subresourceData = {};
		jint* arrayElements = env->GetIntArrayElements(data, nullptr);
		subresourceData.pSysMem = (void*)arrayElements;

		HRESULT hResult = device->CreateBuffer(&bufferDesc, &subresourceData, &buffer);
		env->ReleaseIntArrayElements(data, arrayElements, 0);
		if (FAILED(hResult)) {
			std::cerr << "CreateBuffer {type: " << type <<
				", ptr: #" << std::hex << arrayElements << std::dec <<
				", length: " << length << ", usage: " << usage << "}" << std::endl;
			PrintError(hResult, "CreateBuffer");
			return 0;
		}
	}
	CheckLastError("CreateBuffer");
	/*std::cerr << "CreateBuffer() -> #" << std::hex << buffer << std::dec <<
		" from " << std::hex << ptr << std::dec << " += " << length << ", usage: " << usage << std::endl;*/
	return (jlong)buffer;
}


JNIEXPORT void JNICALL Java_me_anno_directx11_DirectX_updateBuffer
(JNIEnv*, jclass, jlong bufferPtr, jlong offset, jlong dataPtr, jint length) {

	Window* window = currentWindow;
	ID3D11DeviceContext1* ctx = window ? window->deviceContext : nullptr;
	if (!ctx || !bufferPtr || !dataPtr) return;

	D3D11_BOX box = {};
	box.left =(UINT) offset;
	box.right = (UINT) (offset + length);
	box.top = 0;
	box.bottom = 1;
	box.front = 0;
	box.back = 1;

	std::cerr << "Updating subbuffer: " << std::hex << bufferPtr << std::dec <<
		", start: " << offset << 
		", data: " << std::hex << dataPtr << std::dec <<
		", length: " << length << std::endl;

	// discard is only sometimes valid... -> in our engine, it's always valid xD
	bool discard = false;
	D3D11_MAPPED_SUBRESOURCE mappedResource = {};
	ID3D11Buffer* buffer = (ID3D11Buffer*) bufferPtr;
	ctx->Map(buffer, 0, discard ? D3D11_MAP_WRITE_DISCARD : D3D11_MAP_WRITE, 0, &mappedResource);
	void* dst = (UINT8*) mappedResource.pData + offset;
	std::memcpy(dst, (void*) dataPtr, (size_t) length);
	ctx->Unmap(buffer, 0);
	ctx->UpdateSubresource(buffer, 0, &box, mappedResource.pData, 0, 0);
	CheckLastError("BufferSubData");

}

JNIEXPORT void JNICALL Java_me_anno_directx11_DirectX_destroyBuffer
(JNIEnv*, jclass, jlong bufferPtr) {
	Window* window = currentWindow;
	ID3D11Device1* device = window ? window->device : nullptr;
	if (!device || !bufferPtr) return;
	ID3D11Buffer* buffer = (ID3D11Buffer*) bufferPtr;
	buffer->Release();
}

JNIEXPORT void JNICALL Java_me_anno_directx11_DirectX_destroyTexture
(JNIEnv*, jclass, jlong texturePtr) {
	Texture* texture = (Texture*)texturePtr;
	if (!texture) return;
	ID3D11Resource* tex = texture->texture;
	if(tex) tex->Release();
	ID3D11ShaderResourceView* srv = texture->textureView;
	if (srv) srv->Release();
	texture->texture = nullptr;
	texture->textureView = nullptr;
	CheckLastError("DestroyTextures");
	delete texture;
}

const DXGI_FORMAT formatMap[7 * 4 * 2] = {
	// types: byte, ubyte, short, ushort, int, uint, float
	// channels: 1-4
	// normalized: false/true

	DXGI_FORMAT_R8_SINT, // byte 1 false
	DXGI_FORMAT_R8_SNORM, // byte 1 true
	DXGI_FORMAT_R8G8_SINT, // byte 2 false
	DXGI_FORMAT_R8G8_SNORM, // byte 2 true
	DXGI_FORMAT_UNKNOWN, // byte 3 false
	DXGI_FORMAT_UNKNOWN, // byte 3 true
	DXGI_FORMAT_R8G8B8A8_SINT, // byte 4 false
	DXGI_FORMAT_R8G8B8A8_SNORM, // byte 4 true

	DXGI_FORMAT_R8_UINT, // ubyte 1 false
	DXGI_FORMAT_R8_UNORM, // ubyte 1 true
	DXGI_FORMAT_R8G8_UINT, // ubyte 2 false
	DXGI_FORMAT_R8G8_UNORM, // ubyte 2 true
	DXGI_FORMAT_UNKNOWN, // ubyte 3 false
	DXGI_FORMAT_UNKNOWN, // ubyte 3 true
	DXGI_FORMAT_R8G8B8A8_UINT, // ubyte 4 false
	DXGI_FORMAT_R8G8B8A8_UNORM, // ubyte 4 true

	DXGI_FORMAT_R16_SINT, // short 1 false
	DXGI_FORMAT_R16_SNORM, // short 1 true
	DXGI_FORMAT_R16G16_SINT, // short 2 false
	DXGI_FORMAT_R16G16_SNORM, // short 2 true
	DXGI_FORMAT_UNKNOWN, // short 3 false
	DXGI_FORMAT_UNKNOWN, // short 3 true
	DXGI_FORMAT_R16G16B16A16_SINT, // short 4 false
	DXGI_FORMAT_R16G16B16A16_SNORM, // short 4 true

	DXGI_FORMAT_R16_UINT, // ushort 1 false
	DXGI_FORMAT_R16_UNORM, // ushort 1 true
	DXGI_FORMAT_R16G16_UINT, // ushort 2 false
	DXGI_FORMAT_R16G16_UNORM, // ushort 2 true
	DXGI_FORMAT_UNKNOWN, // ushort 3 false
	DXGI_FORMAT_UNKNOWN, // ushort 3 true
	DXGI_FORMAT_R16G16B16A16_UINT, // ushort 4 false
	DXGI_FORMAT_R16G16B16A16_UNORM, // ushort 4 true

	DXGI_FORMAT_R32_SINT, // int 1 false
	DXGI_FORMAT_UNKNOWN, // int 1 true
	DXGI_FORMAT_R32G32_SINT, // int 2 false
	DXGI_FORMAT_UNKNOWN, // int 2 true
	DXGI_FORMAT_R32G32B32_SINT, // int 3 false
	DXGI_FORMAT_UNKNOWN, // int 3 true
	DXGI_FORMAT_R32G32B32A32_SINT, // int 4 false
	DXGI_FORMAT_UNKNOWN, // int 4 true

	DXGI_FORMAT_R32_UINT, // uint 1 false
	DXGI_FORMAT_UNKNOWN, // uint 1 true
	DXGI_FORMAT_R32G32_UINT, // uint 2 false
	DXGI_FORMAT_UNKNOWN, // uint 2 true
	DXGI_FORMAT_R32G32B32_UINT, // uint 3 false
	DXGI_FORMAT_UNKNOWN, // uint 3 true
	DXGI_FORMAT_R32G32B32A32_UINT, // uint 4 false
	DXGI_FORMAT_UNKNOWN, // uint 4 true

	DXGI_FORMAT_R32_FLOAT, // float 1 false
	DXGI_FORMAT_R32_FLOAT, // float 1 true
	DXGI_FORMAT_R32G32_FLOAT, // float 2 false
	DXGI_FORMAT_R32G32_FLOAT, // float 2 true
	DXGI_FORMAT_R32G32B32_FLOAT, // float 3 false
	DXGI_FORMAT_R32G32B32_FLOAT, // float 3 true
	DXGI_FORMAT_R32G32B32A32_FLOAT, // float 4 false
	DXGI_FORMAT_R32G32B32A32_FLOAT, // float 4 true

};

#define MAX_ATTRIBUTES 16
// byte[] channels, int[] types, int[] strides, long[] pointers,
// long[] buffers, int[] divisors, long normalized, long enabled
D3D11_INPUT_ELEMENT_DESC layoutElements[MAX_ATTRIBUTES];
ID3D11Buffer* layoutBuffers[MAX_ATTRIBUTES];
UINT layoutStrides[MAX_ATTRIBUTES];
UINT layoutOffsets[MAX_ATTRIBUTES];

#include <chrono>

struct LayoutKey {
	uint32_t shaderId = 0;
	uint16_t bindings[MAX_ATTRIBUTES] = { };

	bool operator==(const LayoutKey& other) const {
		return memcmp(this, &other, sizeof(LayoutKey)) == 0;
	}
};

template<typename CustomStruct>
struct CustomStructHash {
	size_t operator()(const CustomStruct& obj) const {
		// Cast the struct to a byte array
		const char* data = reinterpret_cast<const char*>(&obj);

		// Calculate the hash using a simple byte-wise hash combining algorithm
		uint64_t hash = 0;
		uint64_t len = sizeof(CustomStruct);
		const uint64_t seed = 31;

		const uint64_t m = 0xc6a4a7935bd1e995ULL;
		const int r = 47;

		while (len >= 8) {
			uint64_t k = *((uint64_t*)data);
			k *= m;
			k ^= k >> r;
			k *= m;

			hash ^= k;
			hash *= m;

			data += 8;
			len -= 8;
		}

		if (len >= 4) {
			hash ^= (uint64_t)(*((uint32_t*)data)) * m;
			data += 4;
			len -= 4;
		}

		switch (len) {
		case 3: hash ^= (uint64_t)(data[2]) << 16;
		case 2: hash ^= (uint64_t)(data[1]) << 8;
		case 1: hash ^= (uint64_t)(data[0]);
			hash *= m;
		};

		hash ^= hash >> r;
		hash *= m;
		hash ^= hash >> r;

		return hash;
	}
};

namespace std {
	template<>
	struct hash<LayoutKey> {
		size_t operator()(const LayoutKey& obj) const {
			return CustomStructHash<LayoutKey>()(obj);
		}
	};
}

#include <unordered_map>
#include <string>
static std::unordered_map<LayoutKey, ID3D11InputLayout*> inputLayouts{};

JNIEXPORT jint JNICALL Java_me_anno_directx11_DirectX_doBindVAO
(JNIEnv* env, jclass, jlong vertexShaderHandle,
 jbyteArray channels, jintArray types, jintArray strides, jintArray offsets,
 jlongArray buffers, jint perInstance, jint normalized, jint enabled) {

	// auto t0 = std::chrono::high_resolution_clock::now();

	VertexShader* vertexShader = (VertexShader*)vertexShaderHandle;
	ID3DBlob* vsBlob = vertexShader->blob;
	Window* window = currentWindow;
	if (!window || !window->device || !window->deviceContext) return -7;
	ID3D11Device* device = window->device;
	ID3D11DeviceContext1* ctx = window->deviceContext;

	LayoutKey key { vertexShader->id };
	
	UINT numAttr = vertexShader->numAttributes;
	for (UINT i = 0; i < numAttr; i++) {
		const char* attrName = vertexShader->attrNames[i];
		int32_t mask = 1 << i;
		// todo if not enabled, but still valid in shader, bind 0 buffer...
		// todo what happens with not-matching channel counts?
		bool isPerInstance = (perInstance & mask) != 0;
		UINT inputSlot = isPerInstance ? 1u : 0u;
		jlong buffer = 0;
		env->GetLongArrayRegion(buffers, i, 1, &buffer);
		if (((enabled & mask) != 0) && (buffer != 0)) {

			jint stride = 0;
			jint offset = 0;
			jbyte channel = 0;
			jint type = 0;

			env->GetIntArrayRegion(types, i, 1, &type);
			env->GetIntArrayRegion(strides, i, 1, &stride);
			env->GetByteArrayRegion(channels, i, 1, &channel);
			env->GetIntArrayRegion(offsets, i, 1, &offset);

			bool normalized1 = (normalized & mask) != 0;
			if (type < 0x1400 || type > 0x1406 || channel < 1 || channel > 4) {
				std::cerr << "Unsupported attribute format: {name: " << attrName << ", type: " << type << ", ch: " << (int) channel << ", norm: " << normalized1 << "}" << std::endl;
				return -4;
			}

			int formatIndex = ((type - 0x1400) << 3) + ((channel - 1) << 1) + (normalized1 ? 1 : 0);
			DXGI_FORMAT format = formatMap[formatIndex];
			if (format == DXGI_FORMAT_UNKNOWN) {
				std::cerr << "Unsupported attribute format: {name: " << attrName << ", type: " << type << ", ch: " << (int) channel << ", norm: " << normalized1 << "}" << std::endl;
				return -5;
			}

			key.bindings[i] = (offset * 151) | (format << 1) | (isPerInstance ? 1 : 0);
			layoutElements[i] = D3D11_INPUT_ELEMENT_DESC {
				attrName,
				0, // semantic index, only needed for matrices
				format, inputSlot,
				(UINT)offset,
				isPerInstance ? D3D11_INPUT_PER_INSTANCE_DATA : D3D11_INPUT_PER_VERTEX_DATA,
				isPerInstance ? 1u : 0u
			};

			layoutStrides[i] = stride;
			layoutOffsets[i] = 0;
			layoutBuffers[i] = (ID3D11Buffer*)buffer;
		}
		else {
			key.bindings[i] = 0;
			// bind layout to zero element
			// for that, we may need to extend buffer data or create a shader variation, because we can't bind multiple buffers
			layoutElements[i] = {
				attrName, 0, // semantic index, only needed for matrices
				DXGI_FORMAT_R8G8B8A8_UINT, inputSlot,
				0, // offset
				isPerInstance ? D3D11_INPUT_PER_INSTANCE_DATA : D3D11_INPUT_PER_VERTEX_DATA, // todo this will break, if the attribute is not per instance :/
				isPerInstance ? 0xffffffff - 16 : 0u // maximum value minus some margin
			};
			layoutStrides[i] = 4;
			layoutOffsets[i] = 0;
			layoutBuffers[i] = window->nullBuffer;
			std::cerr << "Missing buffer for attribute " << attrName << ", using " << std::hex << window->nullBuffer << std::dec << std::endl;
		}

		if (true) {
			int format = layoutElements[i].Format;
			std::string formatNum = std::to_string(format);
			std::cerr << "layoutElements[" << i << "] = D3D11_INPUT_ELEMENT_DESC { \"" <<
				layoutElements[i].SemanticName << "\", " <<
				layoutElements[i].SemanticIndex << ", " <<
				(format == DXGI_FORMAT_R32G32B32_FLOAT ? "DXGI_FORMAT_R32G32B32_FLOAT" :
				 format == DXGI_FORMAT_R32_FLOAT ? "DXGI_FORMAT_R32_FLOAT" :
				 format == DXGI_FORMAT_R32G32_FLOAT ? "DXGI_FORMAT_R32G32_FLOAT" :
				 format == DXGI_FORMAT_R16_FLOAT ? "DXGI_FORMAT_R16_FLOAT" :
				 format == DXGI_FORMAT_R16G16_FLOAT ? "DXGI_FORMAT_R16G16_FLOAT" :
				 format == DXGI_FORMAT_R8_UNORM ? "DXGI_FORMAT_R8_UNORM" :
				 format == DXGI_FORMAT_R8G8_UNORM ? "DXGI_FORMAT_R8G8_UNORM" :
				 format == DXGI_FORMAT_R8G8B8A8_UNORM ? "DXGI_FORMAT_R8G8B8A8_UNORM" :
				 format == DXGI_FORMAT_R8G8B8A8_UINT ? "DXGI_FORMAT_R8G8B8A8_UINT" :
				 format == DXGI_FORMAT_R8G8B8A8_SNORM ? "DXGI_FORMAT_R8G8B8A8_SNORM" :
				 formatNum.c_str()) << ", " <<
				layoutElements[i].InputSlot << ", " <<
				layoutElements[i].AlignedByteOffset << ", " <<
				(layoutElements[i].InputSlotClass == D3D11_INPUT_PER_INSTANCE_DATA ? "D3D11_INPUT_PER_INSTANCE_DATA" : "D3D11_INPUT_PER_VERTEX_DATA") << ", " <<
				layoutElements[i].InstanceDataStepRate << " }; // buffer #" << std::hex << layoutBuffers[i] << std::dec << std::endl;
		}
	}

	// auto t1 = std::chrono::high_resolution_clock::now();

	ID3D11InputLayout* inputLayout = inputLayouts[key];
	if (!inputLayout) {
		HRESULT hResult = device->CreateInputLayout(
			layoutElements, numAttr,
			vsBlob->GetBufferPointer(),
			vsBlob->GetBufferSize(),
			&inputLayout
		);
		if (FAILED(hResult)) {
			PrintError(hResult, "CreateInputLayout");
			return -2;
		}
		inputLayouts[key] = inputLayout;
	}

	ctx->IASetInputLayout(inputLayout);
	ctx->IASetVertexBuffers(0, numAttr, layoutBuffers, layoutStrides, layoutOffsets);
	CheckLastError("BindVAO");

	/*auto t2 = std::chrono::high_resolution_clock::now();
	auto d1 = 1e6 * std::chrono::duration<double>(t1 - t0).count();
	auto d2 = 1e6 * std::chrono::duration<double>(t2 - t1).count();*/

	// std::cout << "Prepare/CreateSet: " << d1 << " / " << d2 << std::endl;

	return 0;
}

#pragma endregion Buffers

#pragma region Drawing

const D3D_PRIMITIVE_TOPOLOGY topologyMap[8] = {
	D3D_PRIMITIVE_TOPOLOGY_POINTLIST,
	D3D_PRIMITIVE_TOPOLOGY_LINELIST,
	D3D_PRIMITIVE_TOPOLOGY_UNDEFINED, // line loop
	D3D_PRIMITIVE_TOPOLOGY_LINESTRIP,
	D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST,
	D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP,
	D3D_PRIMITIVE_TOPOLOGY_UNDEFINED, // triangle fan
	D3D_PRIMITIVE_TOPOLOGY_UNDEFINED, // quads
};

JNIEXPORT void JNICALL Java_me_anno_directx11_DirectX_setShaders
(JNIEnv*, jclass, jlong vs, jlong fs) {
	Window* window = currentWindow;
	ID3D11DeviceContext1* ctx = window ? window->deviceContext : nullptr;
	if (ctx && vs && fs) {
		ctx->VSSetShader(((VertexShader*)vs)->shader, nullptr, 0);
		ctx->PSSetShader((ID3D11PixelShader*)fs, nullptr, 0);
	}
}

// maximum by DirectX11
constexpr jint MAX_COLOR_RTVS = 8;
jlong colorRTVs[MAX_COLOR_RTVS];
JNIEXPORT void JNICALL Java_me_anno_directx11_DirectX_setRenderTargets
(JNIEnv* env, jclass, jint numColorRTVs, jlongArray colorRTVs0, jlong depthRTV) {
	Window* window = currentWindow;
	ID3D11DeviceContext1* ctx = window ? window->deviceContext : nullptr;
	if (numColorRTVs < 0) numColorRTVs = 0;
	if (numColorRTVs > MAX_COLOR_RTVS) numColorRTVs = MAX_COLOR_RTVS;
	if (colorRTVs0) env->GetLongArrayRegion(colorRTVs0, 0, numColorRTVs, colorRTVs);
	if (ctx) ctx->OMSetRenderTargets(numColorRTVs, (ID3D11RenderTargetView**) colorRTVs, (ID3D11DepthStencilView*) depthRTV);
	/*std::cerr << "Setting rendertargets to be " << numColorRTVs << std::hex << "x: [" << 
		colorRTVs[0] << ", " <<
		colorRTVs[1] << ", " <<
		colorRTVs[2] << ", " <<
		colorRTVs[3] << ", " <<
		colorRTVs[4] << ", " <<
		colorRTVs[5] << ", " <<
		colorRTVs[6] << ", " <<
		colorRTVs[7] << "], depth: " << depthRTV << std::dec << std::endl;*/
}

JNIEXPORT void JNICALL Java_me_anno_directx11_DirectX_drawArrays
(JNIEnv*, jclass, jint topo, jint start, jint length) {
	if (topo < 0 || topo >= 8 || topologyMap[topo] == 0) {
		std::cerr << "Unsupported topology " << topo << "!" << std::endl;
		return;
	}
	Window* window = currentWindow;
	ID3D11DeviceContext1* ctx = window ? window->deviceContext : nullptr;
	if (ctx) {
		// std::cerr << "Drawing " << start << " += " << length << " with topo " << topologyMap[topo] << std::endl;
		ctx->IASetPrimitiveTopology(topologyMap[topo]);
		ctx->Draw(length, start);
	}
}

JNIEXPORT void JNICALL Java_me_anno_directx11_DirectX_drawArraysInstanced
(JNIEnv*, jclass, jint topo, jint start, jint length, jint instanceCount) {
	if (topo < 0 || topo >= 8 || topologyMap[topo] == 0) {
		std::cerr << "Unsupported topology " << topo << "!" << std::endl;
		return;
	}
	Window* window = currentWindow;
	ID3D11DeviceContext1* ctx = window ? window->deviceContext : nullptr;
	if (ctx) {
		// std::cerr << "Drawing " << start << " += " << length << " with topo " << topologyMap[topo] << " x " << instanceCount << std::endl;
		ctx->IASetPrimitiveTopology(topologyMap[topo]);
		ctx->DrawInstanced(length, instanceCount, start, 0);
	}
}

const DXGI_FORMAT elementFormats[] = {
	DXGI_FORMAT_R8_UINT,
	DXGI_FORMAT_R16_UINT,
	DXGI_FORMAT_R32_UINT,
};

JNIEXPORT void JNICALL Java_me_anno_directx11_DirectX_drawElements
(JNIEnv*, jclass, jint topo, jint length, jint elementsType, jlong indexBuffer) {
	if (topo < 0 || topo >= 8 || topologyMap[topo] == 0) {
		std::cerr << "Unsupported topology " << topo << "!" << std::endl;
		return;
	}
	Window* window = currentWindow;
	ID3D11DeviceContext1* ctx = window ? window->deviceContext : nullptr;
	int eTypeIdx = (elementsType - 0x1400) >> 1;
	if (eTypeIdx < 0 || eTypeIdx > 2) return;
	if (ctx && indexBuffer) {
		// std::cerr << "Drawing elements " << length << " with topo " << topologyMap[topo] << std::endl;
		ctx->IASetPrimitiveTopology(topologyMap[topo]);
		ctx->IASetIndexBuffer((ID3D11Buffer*) indexBuffer, elementFormats[eTypeIdx], 0);
		ctx->DrawIndexed(length, 0, 0);
	}
}

JNIEXPORT void JNICALL Java_me_anno_directx11_DirectX_drawElementsInstanced
(JNIEnv*, jclass, jint topo, jint length, jint elementsType, jlong indexBuffer, jint instanceCount) {
	if (topo < 0 || topo >= 8 || topologyMap[topo] == 0) {
		std::cerr << "Unsupported topology " << topo << "!" << std::endl;
		return;
	}
	Window* window = currentWindow;
	ID3D11DeviceContext1* ctx = window ? window->deviceContext : nullptr;
	int eTypeIdx = (elementsType - 0x1400) >> 1;
	if (eTypeIdx < 0 || eTypeIdx > 2) return;
	if (ctx && indexBuffer) {
		// std::cerr << "Drawing elements " << length << " with topo " << topologyMap[topo] << " x " << instanceCount << std::endl;
		ctx->IASetPrimitiveTopology(topologyMap[topo]);
		ctx->IASetIndexBuffer((ID3D11Buffer*)indexBuffer, elementFormats[eTypeIdx], 0);
		ctx->DrawIndexedInstanced(length, instanceCount, 0, 0, 0);
	}
}


#pragma endregion Drawing

#pragma region Shaders

#include <string>
#include <vector>
#include <sstream>
void HandleShaderError(HRESULT hResult, const char* src, ID3DBlob* shaderCompileErrorsBlob) {
	const char* errorString = NULL;
	if (hResult == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND)) {
		errorString = "Could not compile shader; file not found";
	}
	else if (shaderCompileErrorsBlob) {
		errorString = (const char*)shaderCompileErrorsBlob->GetBufferPointer();
		shaderCompileErrorsBlob->Release();
	}
	std::cerr << errorString << std::endl;
	std::stringstream srcLines(src);
	std::string line;

	int i = 0;
	while (std::getline(srcLines, line, '\n')) {
		std::cerr << (++i) << ": " << line << std::endl;
	}

	ExitProcess(-1);
}

JNIEXPORT jlong JNICALL Java_me_anno_directx11_DirectX_compileVertexShader
(JNIEnv* env, jclass, jstring source, jobjectArray attributes, jint uniformSize) {
	jboolean srcCopy;
	const char* chars = env->GetStringUTFChars(source, &srcCopy);

	ID3D11VertexShader* vertexShader;

	ID3DBlob* shaderBlob;
	ID3DBlob* errorBlob;
	HRESULT hResult = D3DCompile(chars, env->GetStringUTFLength(source), nullptr, nullptr, nullptr, "vs_main", "vs_5_0", 0, 0, &shaderBlob, &errorBlob);
	if (FAILED(hResult)) {
		HandleShaderError(hResult, chars, errorBlob);
		env->ReleaseStringUTFChars(source, chars);
		return 0;
	}

	hResult = currentWindow->device->CreateVertexShader(shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize(), nullptr, &vertexShader);
	if (FAILED(hResult)) {
		PrintError(hResult, "CreateVertexShader");
		return 0;
	}

	env->ReleaseStringUTFChars(source, chars);

	ID3D11Buffer* uniforms;
	{
		D3D11_BUFFER_DESC desc = {};
		// ByteWidth must be a multiple of 16, per the docs
		desc.ByteWidth = (uniformSize + 0xf) & 0xfffffff0;
		desc.Usage = D3D11_USAGE_DYNAMIC;
		desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

		HRESULT hResult = currentWindow->device->CreateBuffer(&desc, nullptr, &uniforms);
		if (FAILED(hResult)) {
			PrintError(hResult, "CreateBuffer(uniforms)");
			return 0;
		}
	}
	assert(uniforms);

	jsize numAttr = env->GetArrayLength(attributes);
	if (numAttr > 16) numAttr = 16;

	// copy attribute names
	char** attrNames = new char*[numAttr];
	for (jsize i = 0; i < numAttr; i++) {
		jstring str = (jstring) env->GetObjectArrayElement(attributes, i);
		size_t len = env->GetStringUTFLength(str);
		char* attrName = (char*) calloc(len + 1, sizeof(char));
		if (attrName == nullptr) { // memory leak, but whatever, shouldn't happen
			std::cerr << "Out of memory :(, cancelling shader creation" << std::endl;
			return 0;
		}
		const char* src = env->GetStringUTFChars(str, nullptr);
		memcpy(attrName, src, len);
		env->ReleaseStringUTFChars(str, src);
		attrNames[i] = attrName;
	}

	static uint32_t nextId = 0;
	VertexShader* vs1 = new VertexShader{ nextId++, shaderBlob, vertexShader, uniforms, attrNames, (uint8_t)numAttr };
	return (jlong)vs1;
}

JNIEXPORT jlong JNICALL Java_me_anno_directx11_DirectX_compilePixelShader
(JNIEnv* env, jclass, jstring source) {

	jboolean srcCopy;
	const char* chars = env->GetStringUTFChars(source, &srcCopy);

	ID3D11PixelShader* pixelShader;

	ID3DBlob* shaderBlob;
	ID3DBlob* errorBlob;
	HRESULT hResult = D3DCompile(chars, env->GetStringUTFLength(source), nullptr, nullptr, nullptr, "ps_main", "ps_5_0", 0, 0, &shaderBlob, &errorBlob);
	if (FAILED(hResult)) {
		HandleShaderError(hResult, chars, errorBlob);
		env->ReleaseStringUTFChars(source, chars);
		return 0;
	}
	hResult = currentWindow->device->CreatePixelShader(shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize(), nullptr, &pixelShader);
	shaderBlob->Release();
	if (FAILED(hResult)) {
		PrintError(hResult, "CreatePixelShader");
		return 0;
	}

	env->ReleaseStringUTFChars(source, chars);
	return (jlong)pixelShader;
}

JNIEXPORT void JNICALL Java_me_anno_directx11_DirectX_doBindUniforms
(JNIEnv*, jclass, jlong vsPtr, jlong dataPtr, jint dataLen) {

	Window* window = currentWindow;
	if (!window) return;
	ID3D11DeviceContext1* ctx = window->deviceContext;
	if (!ctx || !vsPtr) return;

	VertexShader* vs = (VertexShader*)vsPtr;
	ID3D11Buffer* uniforms = vs->uniforms;

	D3D11_MAPPED_SUBRESOURCE mappedSubresource;
	HRESULT hResult = ctx->Map(uniforms, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedSubresource);
	if (FAILED(hResult)) PrintError(hResult, "CreatePixelShader");
	void* dstData = mappedSubresource.pData;
	memcpy(dstData, (void*)dataPtr, dataLen);
	ctx->Unmap(uniforms, 0);

	ctx->VSSetConstantBuffers(0, 1, &uniforms);
	ctx->PSSetConstantBuffers(0, 1, &uniforms);

	CheckLastError("BindUniforms");
}

#pragma endregion Shaders

#pragma region Listeners

JNIEXPORT jlong JNICALL Java_me_anno_directx11_DirectX_waitEvents
(JNIEnv*, jclass) {

	CheckLastError("WaitEvents");

	Window* window = currentWindow;
	if (!window || !window->swapChain || !window->deviceContext) {
		std::cout << "Waiting for events - but no window is defined :/" << std::endl;
		return 0;
	}

	MSG msg = {};
	while (PeekMessageW(&msg, 0, 0, 0, PM_REMOVE)) {
		if (msg.message == WM_QUIT) window->shouldClose = true;
		TranslateMessage(&msg);
		DispatchMessageW(&msg);
	}

	if (global_windowDidResize) {

		window->deviceContext->OMSetRenderTargets(0, 0, 0);
		window->primaryRTV->Release();

		HRESULT res = window->swapChain->ResizeBuffers(0, 0, 0, DXGI_FORMAT_UNKNOWN, 0);
		assert(SUCCEEDED(res));

		ID3D11Texture2D* fbColor0Tex = nullptr;
		res = window->swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&fbColor0Tex);
		assert(SUCCEEDED(res));
		if (!fbColor0Tex) return -15;

		D3D11_TEXTURE2D_DESC desc;
		fbColor0Tex->GetDesc(&desc);
		std::cout << "Updated backbuffer to size " << desc.Width << " x " << desc.Height << std::endl;

		res = window->device->CreateRenderTargetView(fbColor0Tex, nullptr, &window->primaryRTV);
		assert(SUCCEEDED(res));
		fbColor0Tex->Release();
		if (!window->primaryRTV) return -11;

		RECT winRect;
		GetClientRect(window->hwnd, &winRect);
		window->width = winRect.right - winRect.left;
		window->height = winRect.bottom - winRect.top;

		std::cout << "Resized: " << window->width << " x " << window->height << std::endl;

		global_windowDidResize = false;
	}

	return (jlong) window->primaryRTV;
}

JNIEXPORT jboolean JNICALL Java_me_anno_directx11_DirectX_shouldWindowClose
(JNIEnv*, jclass, jlong window) {
	if (!window) return false;
	return ((Window*)window)->shouldClose != 0;
}

JNIEXPORT void JNICALL Java_me_anno_directx11_DirectX_destroyWindow
(JNIEnv*, jclass, jlong handle) {
	if (!handle) return;
	Window* window = (Window*)handle;
	if (window->swapChain) {
		window->swapChain->Release();
		window->swapChain = nullptr;
	}
	if (window->deviceContext) {
		window->deviceContext->Release();
		window->deviceContext = nullptr;
	}
	if (window->device) {
		window->device->Release();
		window->device = nullptr;
	}
	if (window->hwnd) {
		DestroyWindow(window->hwnd);
		window->hwnd = nullptr;
	}
}

JNIEXPORT void JNICALL Java_me_anno_directx11_DirectX_setShouldWindowClose
(JNIEnv* env, jclass, jlong handle, jboolean value) {
	if (!handle) return;
	Window* window = (Window*)handle;
	window->shouldClose = value;
	if (value) {
		// it should close -> close it
		Java_me_anno_directx11_DirectX_destroyWindow(env, nullptr, handle);
	}
	// else it should not close
	// can't really handle that here
}

void UnbindOldListener(JNIEnv* env, jobject old) {
	if (old) env->DeleteGlobalRef(old);
}

void SetListener(JNIEnv* env, jobject* dst, jobject listener) {
	if (!dst) return;
	if (!jvm) env->GetJavaVM(&jvm);
	UnbindOldListener(env, *dst);
	*dst = env->NewGlobalRef(listener);
}

JNIEXPORT void JNICALL Java_me_anno_directx11_DirectX_setMouseButtonCallback
(JNIEnv* env, jclass, jlong windowPtr, jobject listener) {
	SetListener(env, windowPtr ? &((Window*)windowPtr)->mouseButtonCallback : nullptr, listener);
}

JNIEXPORT void JNICALL Java_me_anno_directx11_DirectX_setFocusCallback
(JNIEnv* env, jclass, jlong windowPtr, jobject listener) {
	SetListener(env, windowPtr ? &((Window*)windowPtr)->focusCallback : nullptr, listener);
}

JNIEXPORT void JNICALL Java_me_anno_directx11_DirectX_setIconifyCallback
(JNIEnv* env, jclass, jlong windowPtr, jobject listener) {
	SetListener(env, windowPtr ? &((Window*)windowPtr)->iconifyCallback : nullptr, listener);
}

JNIEXPORT void JNICALL Java_me_anno_directx11_DirectX_setKeyCallback
(JNIEnv* env, jclass, jlong windowPtr, jobject listener) {
	SetListener(env, windowPtr ? &((Window*)windowPtr)->keyCallback : nullptr, listener);
}

JNIEXPORT void JNICALL Java_me_anno_directx11_DirectX_setScrollCallback
(JNIEnv* env, jclass, jlong windowPtr, jobject listener) {
	SetListener(env, windowPtr ? &((Window*)windowPtr)->scrollCallback : nullptr, listener);
}
JNIEXPORT void JNICALL Java_me_anno_directx11_DirectX_setCursorPosCallback
(JNIEnv* env, jclass, jlong windowPtr, jobject listener) {
	SetListener(env, windowPtr ? &((Window*)windowPtr)->cursorPosCallback : nullptr, listener);
}

JNIEXPORT void JNICALL Java_me_anno_directx11_DirectX_setFrameSizeCallback
(JNIEnv* env, jclass, jlong windowPtr, jobject listener) {
	SetListener(env, windowPtr ? &((Window*)windowPtr)->frameSizeCallback : nullptr, listener);
}

JNIEXPORT void JNICALL Java_me_anno_directx11_DirectX_setCharModsCallback
(JNIEnv* env, jclass, jlong windowPtr, jobject listener) {
	SetListener(env, windowPtr ? &((Window*)windowPtr)->charModsCallback : nullptr, listener);
}

#pragma endregion Listeners
