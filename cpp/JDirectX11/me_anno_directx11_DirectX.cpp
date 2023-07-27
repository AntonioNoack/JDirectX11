
#include <jni.h>

#include <array>
#include <cstdio>
#include <fstream>
#include <memory>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include <windows.h>
#include "pch.hpp"


#include "../../src/me_anno_directx11_DirectX.h"

// compile: control + shift + B

std::string jstring2cpp(JNIEnv* env, jstring jStr) {
    const char* cstr = env->GetStringUTFChars(jStr, NULL);
    std::string str = std::string(cstr);
    env->ReleaseStringUTFChars(jStr, cstr);
    return str;
}

void callWinMain();

void Java_me_anno_directx11_DirectX_init(JNIEnv* env, jclass clazz, jstring name) {
	std::cout << jstring2cpp(env, name) << std::endl;
    callWinMain();
}

std::vector<uint8_t> readBytes(std::string path) {
    std::ifstream input(path, std::ios::binary);
    std::vector<uint8_t> bytes(
        (std::istreambuf_iterator<char>(input)),
        (std::istreambuf_iterator<char>()));
    input.close();
    return bytes;
}

void callWinMain() {
    CoreApplication::Run(winrt::make<App>());
}