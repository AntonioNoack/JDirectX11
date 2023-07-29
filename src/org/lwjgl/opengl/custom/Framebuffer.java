package org.lwjgl.opengl.custom;

import org.jetbrains.annotations.Nullable;

public class Framebuffer {
    public int index;
    public final Texture[] colors = new Texture[8]; // limit on RTX 3070, and by DirectX11
    @Nullable
    public Texture depth, stencil;
    @Nullable
    public Texture[] cubeSides;// = new Texture[6];
    public int enabledDrawTargets = 1;
    public int enabledReadTargets = 1;

    public long getSize() {
        for (Texture tex : colors) {
            if (tex != null && tex.width != 0) {
                return getSize(tex);
            }
        }
        if (depth != null && depth.width != 0) return getSize(depth);
        if (stencil != null && stencil.width != 0) return getSize(stencil);
        return 0;
    }

    public long getSize(Texture tex) {
        return ((long) tex.width << 32) | tex.height;
    }
}