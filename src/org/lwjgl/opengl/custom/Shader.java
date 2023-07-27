package org.lwjgl.opengl.custom;

import org.jetbrains.annotations.Nullable;

public class Shader {
    public int type;
    @Nullable
    public String source;
    @Nullable
    public ShaderTranslator translator;
}