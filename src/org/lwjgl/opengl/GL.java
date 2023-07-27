package org.lwjgl.opengl;

import me.anno.directx11.DirectX;

@SuppressWarnings("unused")
public class GL {
    public static void initialize() {
    }

    private static GLCapabilities capabilities;
    public static GLCapabilities createCapabilities() {
        DirectX.attachDirectX();
        capabilities = new GLCapabilities();
        return capabilities;
    }

    public static GLCapabilities getCapabilities() {
        return capabilities;
    }
}
