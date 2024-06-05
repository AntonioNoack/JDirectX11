package org.lwjgl.opengl.custom;

public class Texture {
    public int index;
    public int format;
    public int width, height, depth;
    public long pointer;
    public long sampler = -1;
    public int wrapS, wrapT, wrapU;
    public int minFilter, magFilter;
    public int depthMode = 0;
    // border color
    public float br, bg, bb, ba;
    public long colorRTV, depthStencilRTV;
}