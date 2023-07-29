package org.lwjgl.opengl.custom;

import org.jetbrains.annotations.Nullable;

import java.nio.ByteBuffer;
import java.util.ArrayList;
import java.util.HashMap;

public class Program {

    // todo instanced attributes are special
    //  -> creation 2^attributes1 variations of this shader for compensation

    public int index;
    @Nullable
    public Shader vertex, fragment, compute;
    public final HashMap<String, Integer> attributes = new HashMap<>();
    public final ArrayList<Attribute> attributes1 = new ArrayList<>();
    public final HashMap<String, Integer> uniforms = new HashMap<>();
    public final HashMap<Integer, Integer> uniformSizes = new HashMap<>();
    public int[] texMap;

    public final HashMap<Integer, ProgramVariation> variations = new HashMap<>();

    public long fragmentP, computeP;

    @Nullable
    public String[] attrNames;

    @Nullable
    public ByteBuffer uniformBuffer;
    public int uniformSize;

    public static class Attribute {
        public String dataType, hlslType, name;

        public Attribute(String dataType, String hlslType, String name) {
            this.dataType = dataType;
            this.hlslType = hlslType;
            this.name = name;
        }
    }
}