/* DO NOT EDIT THIS FILE - it is machine generated */
#include <jni.h>
/* Header for class me_anno_directx11_DirectX */

#ifndef _Included_me_anno_directx11_DirectX
#define _Included_me_anno_directx11_DirectX
#ifdef __cplusplus
extern "C" {
#endif
	/*
	 * Class:     me_anno_directx11_DirectX
	 * Method:    swapBuffers
	 * Signature: (J)V
	 */
	JNIEXPORT void JNICALL Java_me_anno_directx11_DirectX_swapBuffers
	(JNIEnv*, jclass, jlong);

	/*
	 * Class:     me_anno_directx11_DirectX
	 * Method:    createWindow
	 * Signature: (IILjava/lang/String;Ljava/lang/String;)J
	 */
	JNIEXPORT jlong JNICALL Java_me_anno_directx11_DirectX_createWindow
	(JNIEnv*, jclass, jint, jint, jstring, jstring);

	/*
	 * Class:     me_anno_directx11_DirectX
	 * Method:    getFramebufferSize
	 * Signature: (J[I[I)V
	 */
	JNIEXPORT void JNICALL Java_me_anno_directx11_DirectX_getFramebufferSize
	(JNIEnv*, jclass, jlong, jintArray, jintArray);

	/*
	 * Class:     me_anno_directx11_DirectX
	 * Method:    makeContextCurrent
	 * Signature: (J)V
	 */
	JNIEXPORT void JNICALL Java_me_anno_directx11_DirectX_makeContextCurrent
	(JNIEnv*, jclass, jlong);

	/*
	 * Class:     me_anno_directx11_DirectX
	 * Method:    clearDepth
	 * Signature: (JFII)V
	 */
	JNIEXPORT void JNICALL Java_me_anno_directx11_DirectX_clearDepth
	(JNIEnv*, jclass, jlong, jfloat, jint, jint);

	/*
	 * Class:     me_anno_directx11_DirectX
	 * Method:    clearColor
	 * Signature: (JFFFF)V
	 */
	JNIEXPORT void JNICALL Java_me_anno_directx11_DirectX_clearColor
	(JNIEnv*, jclass, jlong, jfloat, jfloat, jfloat, jfloat);

	/*
	 * Class:     me_anno_directx11_DirectX
	 * Method:    doAttachDirectX
	 * Signature: ()J
	 */
	JNIEXPORT jlong JNICALL Java_me_anno_directx11_DirectX_doAttachDirectX
	(JNIEnv*, jclass);

	/*
	 * Class:     me_anno_directx11_DirectX
	 * Method:    compileVertexShader
	 * Signature: (Ljava/lang/String;[Ljava/lang/String;I)J
	 */
	JNIEXPORT jlong JNICALL Java_me_anno_directx11_DirectX_compileVertexShader
	(JNIEnv*, jclass, jstring, jobjectArray, jint);

	/*
	 * Class:     me_anno_directx11_DirectX
	 * Method:    compilePixelShader
	 * Signature: (Ljava/lang/String;)J
	 */
	JNIEXPORT jlong JNICALL Java_me_anno_directx11_DirectX_compilePixelShader
	(JNIEnv*, jclass, jstring);

	/*
	 * Class:     me_anno_directx11_DirectX
	 * Method:    createBuffer
	 * Signature: (IJJI)J
	 */
	JNIEXPORT jlong JNICALL Java_me_anno_directx11_DirectX_createBuffer
	(JNIEnv*, jclass, jint, jlong, jlong, jint);

	/*
	 * Class:     me_anno_directx11_DirectX
	 * Method:    createBufferI
	 * Signature: (I[II)J
	 */
	JNIEXPORT jlong JNICALL Java_me_anno_directx11_DirectX_createBufferI
	(JNIEnv*, jclass, jint, jintArray, jint);

	/*
	 * Class:     me_anno_directx11_DirectX
	 * Method:    destroyBuffer
	 * Signature: (J)V
	 */
	JNIEXPORT void JNICALL Java_me_anno_directx11_DirectX_destroyBuffer
	(JNIEnv*, jclass, jlong);

	/*
	 * Class:     me_anno_directx11_DirectX
	 * Method:    destroyTexture
	 * Signature: (J)V
	 */
	JNIEXPORT void JNICALL Java_me_anno_directx11_DirectX_destroyTexture
	(JNIEnv*, jclass, jlong);

	/*
	 * Class:     me_anno_directx11_DirectX
	 * Method:    doBindVAO
	 * Signature: (J[B[I[I[I[JIII)I
	 */
	JNIEXPORT jint JNICALL Java_me_anno_directx11_DirectX_doBindVAO
	(JNIEnv*, jclass, jlong, jbyteArray, jintArray, jintArray, jintArray, jlongArray, jint, jint, jint);

	/*
	 * Class:     me_anno_directx11_DirectX
	 * Method:    drawArrays
	 * Signature: (III)V
	 */
	JNIEXPORT void JNICALL Java_me_anno_directx11_DirectX_drawArrays
	(JNIEnv*, jclass, jint, jint, jint);

	/*
	 * Class:     me_anno_directx11_DirectX
	 * Method:    drawArraysInstanced
	 * Signature: (IIII)V
	 */
	JNIEXPORT void JNICALL Java_me_anno_directx11_DirectX_drawArraysInstanced
	(JNIEnv*, jclass, jint, jint, jint, jint);

	/*
	 * Class:     me_anno_directx11_DirectX
	 * Method:    drawElements
	 * Signature: (IIIJ)V
	 */
	JNIEXPORT void JNICALL Java_me_anno_directx11_DirectX_drawElements
	(JNIEnv*, jclass, jint, jint, jint, jlong);

	/*
	 * Class:     me_anno_directx11_DirectX
	 * Method:    drawElementsInstanced
	 * Signature: (IIIJI)V
	 */
	JNIEXPORT void JNICALL Java_me_anno_directx11_DirectX_drawElementsInstanced
	(JNIEnv*, jclass, jint, jint, jint, jlong, jint);

	/*
	 * Class:     me_anno_directx11_DirectX
	 * Method:    setShaders
	 * Signature: (JJ)V
	 */
	JNIEXPORT void JNICALL Java_me_anno_directx11_DirectX_setShaders
	(JNIEnv*, jclass, jlong, jlong);

	/*
	 * Class:     me_anno_directx11_DirectX
	 * Method:    setRenderTargets
	 * Signature: (I[JJ)V
	 */
	JNIEXPORT void JNICALL Java_me_anno_directx11_DirectX_setRenderTargets
	(JNIEnv*, jclass, jint, jlongArray, jlong);

	/*
	 * Class:     me_anno_directx11_DirectX
	 * Method:    setWindowTitle
	 * Signature: (JLjava/lang/String;)V
	 */
	JNIEXPORT void JNICALL Java_me_anno_directx11_DirectX_setWindowTitle
	(JNIEnv*, jclass, jlong, jstring);

	/*
	 * Class:     me_anno_directx11_DirectX
	 * Method:    setWindowIcon
	 * Signature: (JIIJ)V
	 */
	JNIEXPORT void JNICALL Java_me_anno_directx11_DirectX_setWindowIcon
	(JNIEnv*, jclass, jlong, jint, jint, jlong);

	/*
	 * Class:     me_anno_directx11_DirectX
	 * Method:    updateViewport
	 * Signature: (FFFFFF)V
	 */
	JNIEXPORT void JNICALL Java_me_anno_directx11_DirectX_updateViewport
	(JNIEnv*, jclass, jfloat, jfloat, jfloat, jfloat, jfloat, jfloat);

	/*
	 * Class:     me_anno_directx11_DirectX
	 * Method:    updateScissor
	 * Signature: (FFFF)V
	 */
	JNIEXPORT void JNICALL Java_me_anno_directx11_DirectX_updateScissor
	(JNIEnv*, jclass, jfloat, jfloat, jfloat, jfloat);

	/*
	 * Class:     me_anno_directx11_DirectX
	 * Method:    createTexture2DMS
	 * Signature: (IIII)J
	 */
	JNIEXPORT jlong JNICALL Java_me_anno_directx11_DirectX_createTexture2DMS
	(JNIEnv*, jclass, jint, jint, jint, jint);

	/*
	 * Class:     me_anno_directx11_DirectX
	 * Method:    createTexture2D
	 * Signature: (IIIIIIJ)J
	 */
	JNIEXPORT jlong JNICALL Java_me_anno_directx11_DirectX_createTexture2D
	(JNIEnv*, jclass, jint, jint, jint, jint, jint, jint, jlong);

	/*
	 * Class:     me_anno_directx11_DirectX
	 * Method:    createTexture2Di
	 * Signature: (IIIIII[I)J
	 */
	JNIEXPORT jlong JNICALL Java_me_anno_directx11_DirectX_createTexture2Di
	(JNIEnv*, jclass, jint, jint, jint, jint, jint, jint, jintArray);

	/*
	 * Class:     me_anno_directx11_DirectX
	 * Method:    updateTexture2D
	 * Signature: (JIIIIIIJJ)V
	 */
	JNIEXPORT void JNICALL Java_me_anno_directx11_DirectX_updateTexture2D
	(JNIEnv*, jclass, jlong, jint, jint, jint, jint, jint, jint, jlong, jlong);

	/*
	 * Class:     me_anno_directx11_DirectX
	 * Method:    createTexture3DMS
	 * Signature: (IIIIII)J
	 */
	JNIEXPORT jlong JNICALL Java_me_anno_directx11_DirectX_createTexture3DMS
	(JNIEnv*, jclass, jint, jint, jint, jint, jint, jint);

	/*
	 * Class:     me_anno_directx11_DirectX
	 * Method:    createTexture3Di
	 * Signature: (IIIIIIII[I)J
	 */
	JNIEXPORT jlong JNICALL Java_me_anno_directx11_DirectX_createTexture3Di
	(JNIEnv*, jclass, jint, jint, jint, jint, jint, jint, jint, jint, jintArray);

	/*
	 * Class:     me_anno_directx11_DirectX
	 * Method:    createTexture3Dp
	 * Signature: (IIIIIIIIJJ)J
	 */
	JNIEXPORT jlong JNICALL Java_me_anno_directx11_DirectX_createTexture3Dp
	(JNIEnv*, jclass, jint, jint, jint, jint, jint, jint, jint, jint, jlong, jlong);

	/*
	 * Class:     me_anno_directx11_DirectX
	 * Method:    bindTextures
	 * Signature: (IJ)V
	 */
	JNIEXPORT void JNICALL Java_me_anno_directx11_DirectX_bindTextures
	(JNIEnv*, jclass, jint, jlong);

	/*
	 * Class:     me_anno_directx11_DirectX
	 * Method:    doBindUniforms
	 * Signature: (JJI)V
	 */
	JNIEXPORT void JNICALL Java_me_anno_directx11_DirectX_doBindUniforms
	(JNIEnv*, jclass, jlong, jlong, jint);

	/*
	 * Class:     me_anno_directx11_DirectX
	 * Method:    setPipelineState
	 * Signature: (IIII)V
	 */
	JNIEXPORT void JNICALL Java_me_anno_directx11_DirectX_setPipelineState
	(JNIEnv*, jclass, jint, jint, jint, jint);

	/*
	 * Class:     me_anno_directx11_DirectX
	 * Method:    generateMipmaps
	 * Signature: (J)V
	 */
	JNIEXPORT void JNICALL Java_me_anno_directx11_DirectX_generateMipmaps
	(JNIEnv*, jclass, jlong);

	/*
	 * Class:     me_anno_directx11_DirectX
	 * Method:    createColorRTV
	 * Signature: (J)J
	 */
	JNIEXPORT jlong JNICALL Java_me_anno_directx11_DirectX_createColorRTV
	(JNIEnv*, jclass, jlong);

	/*
	 * Class:     me_anno_directx11_DirectX
	 * Method:    createDepthRTV
	 * Signature: (J)J
	 */
	JNIEXPORT jlong JNICALL Java_me_anno_directx11_DirectX_createDepthRTV
	(JNIEnv*, jclass, jlong);

	/*
	 * Class:     me_anno_directx11_DirectX
	 * Method:    destroyColorRTV
	 * Signature: (J)V
	 */
	JNIEXPORT void JNICALL Java_me_anno_directx11_DirectX_destroyColorRTV
	(JNIEnv*, jclass, jlong);

	/*
	 * Class:     me_anno_directx11_DirectX
	 * Method:    destroyDepthRTV
	 * Signature: (J)V
	 */
	JNIEXPORT void JNICALL Java_me_anno_directx11_DirectX_destroyDepthRTV
	(JNIEnv*, jclass, jlong);

	/*
	 * Class:     me_anno_directx11_DirectX
	 * Method:    waitEvents
	 * Signature: ()J
	 */
	JNIEXPORT jlong JNICALL Java_me_anno_directx11_DirectX_waitEvents
	(JNIEnv*, jclass);

	/*
	 * Class:     me_anno_directx11_DirectX
	 * Method:    setMouseButtonCallback
	 * Signature: (JLjava/lang/Object;)V
	 */
	JNIEXPORT void JNICALL Java_me_anno_directx11_DirectX_setMouseButtonCallback
	(JNIEnv*, jclass, jlong, jobject);

	/*
	 * Class:     me_anno_directx11_DirectX
	 * Method:    setKeyCallback
	 * Signature: (JLjava/lang/Object;)V
	 */
	JNIEXPORT void JNICALL Java_me_anno_directx11_DirectX_setKeyCallback
	(JNIEnv*, jclass, jlong, jobject);

	/*
	 * Class:     me_anno_directx11_DirectX
	 * Method:    setCharModsCallback
	 * Signature: (JLjava/lang/Object;)V
	 */
	JNIEXPORT void JNICALL Java_me_anno_directx11_DirectX_setCharModsCallback
	(JNIEnv*, jclass, jlong, jobject);

	/*
	 * Class:     me_anno_directx11_DirectX
	 * Method:    setScrollCallback
	 * Signature: (JLjava/lang/Object;)V
	 */
	JNIEXPORT void JNICALL Java_me_anno_directx11_DirectX_setScrollCallback
	(JNIEnv*, jclass, jlong, jobject);

	/*
	 * Class:     me_anno_directx11_DirectX
	 * Method:    setCursorPosCallback
	 * Signature: (JLjava/lang/Object;)V
	 */
	JNIEXPORT void JNICALL Java_me_anno_directx11_DirectX_setCursorPosCallback
	(JNIEnv*, jclass, jlong, jobject);

	/*
	 * Class:     me_anno_directx11_DirectX
	 * Method:    setFrameSizeCallback
	 * Signature: (JLjava/lang/Object;)V
	 */
	JNIEXPORT void JNICALL Java_me_anno_directx11_DirectX_setFrameSizeCallback
	(JNIEnv*, jclass, jlong, jobject);

	/*
	 * Class:     me_anno_directx11_DirectX
	 * Method:    setFocusCallback
	 * Signature: (JLjava/lang/Object;)V
	 */
	JNIEXPORT void JNICALL Java_me_anno_directx11_DirectX_setFocusCallback
	(JNIEnv*, jclass, jlong, jobject);

	/*
	 * Class:     me_anno_directx11_DirectX
	 * Method:    setIconifyCallback
	 * Signature: (JLjava/lang/Object;)V
	 */
	JNIEXPORT void JNICALL Java_me_anno_directx11_DirectX_setIconifyCallback
	(JNIEnv*, jclass, jlong, jobject);

	/*
	 * Class:     me_anno_directx11_DirectX
	 * Method:    setPosCallback
	 * Signature: (JLjava/lang/Object;)V
	 */
	JNIEXPORT void JNICALL Java_me_anno_directx11_DirectX_setPosCallback
	(JNIEnv*, jclass, jlong, jobject);

	/*
	 * Class:     me_anno_directx11_DirectX
	 * Method:    shouldWindowClose
	 * Signature: (J)Z
	 */
	JNIEXPORT jboolean JNICALL Java_me_anno_directx11_DirectX_shouldWindowClose
	(JNIEnv*, jclass, jlong);

	/*
	 * Class:     me_anno_directx11_DirectX
	 * Method:    setShouldWindowClose
	 * Signature: (JZ)V
	 */
	JNIEXPORT void JNICALL Java_me_anno_directx11_DirectX_setShouldWindowClose
	(JNIEnv*, jclass, jlong, jboolean);

	/*
	 * Class:     me_anno_directx11_DirectX
	 * Method:    destroyWindow
	 * Signature: (J)V
	 */
	JNIEXPORT void JNICALL Java_me_anno_directx11_DirectX_destroyWindow
	(JNIEnv*, jclass, jlong);

	/*
	 * Class:     me_anno_directx11_DirectX
	 * Method:    updateBuffer
	 * Signature: (JJJI)V
	 */
	JNIEXPORT void JNICALL Java_me_anno_directx11_DirectX_updateBuffer
	(JNIEnv*, jclass, jlong, jlong, jlong, jint);

	/*
	 * Class:     me_anno_directx11_DirectX
	 * Method:    setVsyncInterval
	 * Signature: (I)V
	 */
	JNIEXPORT void JNICALL Java_me_anno_directx11_DirectX_setVsyncInterval
	(JNIEnv*, jclass, jint);

	/*
	 * Class:     me_anno_directx11_DirectX
	 * Method:    getCursorPos
	 * Signature: (J[D[D)V
	 */
	JNIEXPORT void JNICALL Java_me_anno_directx11_DirectX_getCursorPos
	(JNIEnv*, jclass, jlong, jdoubleArray, jdoubleArray);

	/*
	 * Class:     me_anno_directx11_DirectX
	 * Method:    readPixelsI32
	 * Signature: (IIIIII[IJ)V
	 */
	JNIEXPORT void JNICALL Java_me_anno_directx11_DirectX_readPixelsI32
	(JNIEnv*, jclass, jint, jint, jint, jint, jint, jint, jintArray, jlong tex);

	/*
	 * Class:     me_anno_directx11_DirectX
	 * Method:    readPixelsF32
	 * Signature: (IIIIII[FJ)V
	 */
	JNIEXPORT void JNICALL Java_me_anno_directx11_DirectX_readPixelsF32
	(JNIEnv*, jclass, jint, jint, jint, jint, jint, jint, jfloatArray, jlong tex);

#ifdef __cplusplus
}
#endif
#endif
