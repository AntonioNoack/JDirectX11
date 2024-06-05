package me.anno.directx11;

import org.lwjgl.opengl.GL11C;
import org.lwjgl.opengl.custom.Buffer;
import org.lwjgl.opengl.custom.Program;
import org.lwjgl.opengl.custom.ProgramVariation;
import org.lwjgl.opengl.custom.VAO;
import org.lwjgl.system.MemoryUtil;

import java.io.File;

public class DirectX {

	// write a function OpenGL driver based on DirectX :)
	// write the main parts in ... ?

	public static native void swapBuffers(long window);

	public static native long createWindow(int width, int height, String name, String iconPath);

	public static native void getFramebufferSize(long window, int[] w, int[] h);

	public static native void makeContextCurrent(long window);

	public static native void clearDepth(long ptr, float depth, int stencil, int dxMask);

	public static native void clearColor(long fb, float red, float green, float blue, float alpha);

	public static void attachDirectX() {
		GL11C.nullTexture.colorRTV = doAttachDirectX();
		GL11C.nullFramebuffer.enabledDrawTargets = 1;
	}

	public static native long doAttachDirectX();

	// todo this data needs to be ThreadLocal if we want to support multiple windows

	static {
		int version = 6;
		File path = new File(System.getProperty("user.home"),
				"Documents/IdeaProjects/JDirectX11/cpp" + version +
						"/DirectX11v" + version +
						"/x64/Release/DirectX11v" + version + ".dll"
		);
		if (!path.exists()) throw new IllegalStateException("File does not exist, " + path);
		System.load(path.getAbsolutePath());
	}

	public static native long compileVertexShader(String source, String[] attributes, int uniformSize);

	public static native long compilePixelShader(String source);

	public static native long createBuffer(int type, long addr, long length, int i1);

	public static native long createBufferI(int type, int[] data, int i1);

	public static native void destroyBuffer(long ptr);

	public static native void destroyTexture(long ptr);

	/**
	 * returns 0 on success
	 */
	public static int bindVAO(Program program, ProgramVariation pv, VAO vao) {
		String[] attrNames = program.attrNames;
		assert attrNames != null;
		for (int i = 0; i < attrNames.length; i++) {
			Buffer buffer = vao.buffers[i];
			vao.bufferPts[i] = buffer == null ? 0 : vao.buffers[i].pointer;
		}
		return doBindVAO(
				pv.vertexP, vao.channels, vao.types, vao.strides,
				vao.offsets, vao.bufferPts, vao.perInstance, vao.normalized, vao.enabled
		);
	}

	public static int calculateProgramVariation(VAO vao) {
		return vao.perInstance;
	}

	public static native int doBindVAO(
			long vertexShader, // required in DirectX
			byte[] channels, int[] types, int[] strides, int[] offsets,
			long[] buffers, int divisors, int normalized, int enabled
	);

	public static native void drawArrays(int mode, int start, int length);

	public static native void drawArraysInstanced(int mode, int start, int length, int instanceCount);

	public static native void drawElements(int mode, int length, int elementsType, long elementsPtr);

	public static native void drawElementsInstanced(int mode, int length, int elementsType, long elementsPtr, int instanceCount);

	public static native void setShaders(long vertexShader, long fragmentShader);

	public static native void setRenderTargets(int numRTVs, long[] colorRTVs, long depthRTV);

	public static native void setWindowTitle(long window, String title);

	public static native void updateViewport(float vx, float vy, float vw, float vh, float zMin, float zMax);

	public static native void updateScissor(float sx, float sy, float sw, float sh);

	public static native long createTexture2DMS(int samples, int internalFormat, int width, int height);

	public static native long createTexture2D(int samples, int internalFormat, int width, int height,
											  int srcFormat, int srcType, long ptr);

	public static native long createTexture2Di(int samples, int internalFormat, int width, int height,
											   int srcFormat, int srcType, int[] data);

	public static native void updateTexture2D(long pointer, int x0, int y0, int dx, int dy,
											  int srcFormat, int srcType, long ptr, long ptrLength);

	public static native long createTexture3DMS(int type, int samples, int internalFormat, int width, int height, int depth);

	public static native long createTexture3Di(int type, int samples, int internalFormat, int width, int height, int depth,
											   int srcFormat, int srcType, int[] ptr);

	public static native long createTexture3Dp(int type, int samples, int internalFormat, int width, int height, int depth,
											   int srcFormat, int srcType, long ptr, long length);

	public static native void bindTextures(int numTextures, long texturePtr);

	public static void bindUniforms(Program currProgram, ProgramVariation pv) {
		assert currProgram.uniformBuffer != null;
		doBindUniforms(pv.vertexP, MemoryUtil.memAddress(currProgram.uniformBuffer), currProgram.uniformSize);
	}

	public static native void doBindUniforms(long vertexShader, long dataPointer, int dataLength);

	public static native void setPipelineState(int blendState, int depthState, int stencilState, int cullScissorState);

	public static native void generateMipmaps(long ptr);

	public static native long createColorRTV(long tex);

	public static native long createDepthRTV(long tex);

	public static native void destroyColorRTV(long rtv);

	public static native void destroyDepthRTV(long rtv);

	public static native long waitEvents();

	public static native void setMouseButtonCallback(long window, Object callback);

	public static native void setKeyCallback(long window, Object callback);

	public static native void setCharModsCallback(long window, Object callback);

	public static native void setScrollCallback(long window, Object callback);

	public static native void setCursorPosCallback(long window, Object callback);

	public static native void setFrameSizeCallback(long window, Object callback);

	public static native void setFocusCallback(long window, Object callback);

	public static native void setIconifyCallback(long window, Object callback);

	public static native void setPosCallback(long window, Object callback);

	public static native boolean shouldWindowClose(long window);

	public static native void setShouldWindowClose(long window, boolean shouldClose);

	public static native void destroyWindow(long window);

	public static native void updateBuffer(long pointer, long offset, long memAddress, int remaining);

	public static native void setVsyncInterval(int interval);

	public static native void getCursorPos(long window, double[] x, double[] y);

	public static native void readPixelsF32(int x, int y, int w, int h, int format, int type, int[] dst, long tex);

	public static native void readPixelsI32(int x, int y, int w, int h, int format, int type, float[] dst, long tex);
}
