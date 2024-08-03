package org.lwjgl.opengl.custom;

import static org.lwjgl.opengl.GL11C.MAX_ATTRIBUTES;

public class VAO {
    public final byte[] channels = new byte[MAX_ATTRIBUTES];
    public final int[] types = new int[MAX_ATTRIBUTES];
    public final int[] strides = new int[MAX_ATTRIBUTES];
    public final int[] offsets = new int[MAX_ATTRIBUTES];
    public final Buffer[] buffers = new Buffer[MAX_ATTRIBUTES];
    public int enabled = 0;
    public int normalized = 0;
    public int perInstance = 0;
}