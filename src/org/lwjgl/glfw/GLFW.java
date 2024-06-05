package org.lwjgl.glfw;

import me.anno.directx11.DirectX;
import org.jetbrains.annotations.Nullable;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;

import static me.anno.io.files.Reference.getReference;
import static org.lwjgl.opengl.GL11C.nullTexture;

@SuppressWarnings("unused")
public class GLFW {

    public static boolean glfwInit() {
        return true;
    }

    public static void glfwDefaultWindowHints() {
    }

    public static void glfwWindowHint(int key, int value) {
    }

    public static long glfwCreateWindow(int width, int height, @Nullable CharSequence name, long monitor, long sharedCtx) throws IOException {
        File tmp = File.createTempFile("icon", ".ico");
        try (InputStream str = getReference("res://icon.ico").inputStreamSync()) {
            FileOutputStream fos = new FileOutputStream(tmp);
            byte[] tmp1 = new byte[1024];
            while (true) {
                int len = str.read(tmp1);
                if (len < 0) break;
                fos.write(tmp1, 0, len);
            }
            fos.close();
        }
        long window = DirectX.createWindow(width, height, name == null ? null : name.toString(), tmp.getAbsolutePath());
        tmp.deleteOnExit();
        nullTexture.width = width;
        nullTexture.height = height;
        return window;
    }

    @Nullable
    public static GLFWKeyCallback glfwSetKeyCallback(long window, GLFWKeyCallbackI callback) {
        DirectX.setKeyCallback(window, callback);
        return null;
    }

    @Nullable
    public static GLFWFramebufferSizeCallback glfwSetFramebufferSizeCallback(long window, GLFWFramebufferSizeCallbackI callback) {
        DirectX.setFrameSizeCallback(window, (GLFWFramebufferSizeCallbackI) (window1, width, height) -> {
            nullTexture.width = width;
            nullTexture.height = height;
            callback.invoke(window1, width, height);
        });
        return null;
    }

    @Nullable
    public static GLFWWindowFocusCallback glfwSetWindowFocusCallback(long window, GLFWWindowFocusCallbackI callback) {
        DirectX.setFocusCallback(window, callback);
        callback.invoke(window, true);
        return null;
    }

    @Nullable
    public static GLFWWindowIconifyCallback glfwSetWindowIconifyCallback(long window, GLFWWindowIconifyCallbackI callback) {
        DirectX.setIconifyCallback(window, callback);
        return null;
    }

    @Nullable
    public static GLFWWindowPosCallback glfwSetWindowPosCallback(long window, GLFWWindowPosCallbackI callback) {
        DirectX.setPosCallback(window, callback);
        return null;
    }

    @Nullable
    public static GLFWWindowRefreshCallback glfwSetWindowRefreshCallback(long window, GLFWWindowRefreshCallbackI callback) {
        return null;
    }

    @Nullable
    public static GLFWWindowContentScaleCallback glfwSetWindowContentScaleCallback(long window, GLFWWindowContentScaleCallbackI callback) {
        return null;
    }

    @Nullable
    public static GLFWDropCallback glfwSetDropCallback(long window, GLFWDropCallbackI callback) {
        return null;
    }

    @Nullable
    public static GLFWCharModsCallback glfwSetCharModsCallback(long window, GLFWCharModsCallbackI callback) {
        DirectX.setCharModsCallback(window, callback);
        return null;
    }

    @Nullable
    public static GLFWCursorPosCallback glfwSetCursorPosCallback(long window, GLFWCursorPosCallbackI callback) {
        DirectX.setCursorPosCallback(window, callback);
        return null;
    }

    public static void glfwSetCursor(long window, long cursor) {
        // todo do this...
    }

    @Nullable
    public static GLFWMouseButtonCallback glfwSetMouseButtonCallback(long window, GLFWMouseButtonCallbackI callback) {
        DirectX.setMouseButtonCallback(window, callback);
        return null;
    }

    @Nullable
    public static GLFWScrollCallback glfwSetScrollCallback(long window, GLFWScrollCallbackI callback) {
        DirectX.setScrollCallback(window, callback);
        return null;
    }

    public static long glfwGetPrimaryMonitor() {
        return 0;
    }

    @Nullable
    public static GLFWVidMode glfwGetVideoMode(long monitor) {
        return null;
    }

    public static void glfwGetWindowContentScale(long window, float[] x, float[] y) {
        x[0] = 1f;
        y[0] = 1f;
    }

    public static void glfwGetFramebufferSize(long window, int[] w, int[] h) {
        DirectX.getFramebufferSize(window, w, h);
        nullTexture.width = w[0];
        nullTexture.height = h[0];
    }

    public static void glfwSetWindowTitle(long window, CharSequence title) {
        DirectX.setWindowTitle(window, title.toString());
    }

    public static void glfwShowWindow(long window) {
    } // not needed, we show it anyway

    public static void glfwSetWindowIcon(long window, GLFWImage.Buffer buffer) {
        // didn't work :/
    }

    public static void glfwGetCursorPos(long window, double[] x, double[] y) {
        DirectX.getCursorPos(window, x, y);
    }

    public static void glfwMakeContextCurrent(long window) {
        DirectX.makeContextCurrent(window);
    }

    public static boolean glfwWindowShouldClose(long window) {
        return DirectX.shouldWindowClose(window);
    }

    public static void glfwSetWindowShouldClose(long window, boolean shouldClose) {
        DirectX.setShouldWindowClose(window, shouldClose);
    }

    public static void glfwDestroyWindow(long window) {
        DirectX.destroyWindow(window);
    }

    public static void glfwSwapInterval(int interval) {
        DirectX.setVsyncInterval(interval);
    }

    public static void glfwWaitEventsTimeout(double timeout) {
        // work is done in swapBuffers instead, because that's on the correct thread
    }

    public static void glfwTerminate() {
        // implement?
        // todo close all windows
    }

    public static GLFWErrorCallback glfwSetErrorCallback(GLFWErrorCallbackI callback) {
        return null;
    }

    public static void glfwSwapBuffers(long window) {
        DirectX.swapBuffers(window);
        nullTexture.colorRTV = DirectX.waitEvents();
    }

    private static long cursors;

    public static long glfwCreateStandardCursor(int i) {
        return ++cursors;
    }

    public static boolean glfwJoystickPresent(int i) {
        return false;
    }
}
