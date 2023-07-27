package org.lwjgl.opengl;

import kotlin.NotImplementedError;
import me.anno.directx11.DirectX;
import me.anno.utils.pooling.ByteBufferPool;
import me.anno.utils.types.Floats;
import org.lwjgl.opengl.custom.Buffer;
import org.lwjgl.opengl.custom.*;
import org.lwjgl.system.MemoryUtil;

import java.nio.*;
import java.util.ArrayList;
import java.util.HashSet;
import java.util.List;

import static org.lwjgl.opengl.GL11.GL_VERSION;
import static org.lwjgl.opengl.GL15.GL_ARRAY_BUFFER;
import static org.lwjgl.opengl.GL15.GL_ELEMENT_ARRAY_BUFFER;
import static org.lwjgl.opengl.GL20.*;
import static org.lwjgl.opengl.GL30.*;
import static org.lwjgl.opengl.GL43.*;
import static org.lwjgl.opengl.GL45.GL_NEGATIVE_ONE_TO_ONE;
import static org.lwjgl.opengl.GL45.GL_ZERO_TO_ONE;

@SuppressWarnings({"unused", "ConstantConditions"})
public class GL11C {

    public static int glGetError() {
        return 0;
    }

    public static int glGetInteger(int key) {
        // query this property
        // todo actually query these properties
        switch (key) {
            case GL_MAX_VERTEX_UNIFORM_COMPONENTS:
            case GL_MAX_FRAGMENT_UNIFORM_COMPONENTS:
                return 4096;
            case GL_MAX_TEXTURE_IMAGE_UNITS:
                // D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT? 16
                return 16;
            case GL_MAX_UNIFORM_LOCATIONS:
                return 65536;
            case GL_MAX_COLOR_ATTACHMENTS:
                return 8;
            case GL_MAX_SAMPLES:
                // todo change back to 8, when we have implemented blitting
                return 1;
            case GL_MAX_TEXTURE_SIZE:
                return 2048;
            case GL_MAX_VERTEX_ATTRIBS:
                return 16; // on my RTX 3070
            default:
                return -1;
        }
    }

    public static void glGetIntegeri_v(int key, int index, int[] dst) {
        if (key != GL_MAX_COMPUTE_WORK_GROUP_COUNT) throw new IllegalArgumentException("Unknown key");
        dst[0] = 1024;
        // todo check this
    }

    public static void glGetIntegerv(int key, int[] dst) {
        if (key != GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS) throw new IllegalArgumentException("Unknown key");
        dst[0] = 1024;
        // todo check this, should be 1024 or 1024 * 1.5 though
    }

    private static final ArrayList<Program> programs = new ArrayList<>();
    private static final ArrayList<Shader> shaders = new ArrayList<>();

    private static final Program nullProgram = new Program();
    private static Program currentProgram = nullProgram;

    static {
        // to avoid returning 0
        programs.add(nullProgram);
        shaders.add(null);
    }

    public static int glCreateProgram() {
        Program p = new Program();
        synchronized (programs) {
            int index = programs.size();
            p.index = index;
            programs.add(p);
            return index;
        }
    }

    public static int glCreateShader(int type) {
        Shader s = new Shader();
        s.type = type;
        synchronized (shaders) {
            int idx = shaders.size();
            shaders.add(s);
            return idx;
        }
    }

    public static void glShaderSource(int shader, CharSequence source) {
        shaders.get(shader).source = source.toString();
    }

    public static void glCompileShader(int shader) {
        Shader s = shaders.get(shader);
        s.translator = new ShaderTranslator(s.type);
        s.translator.translateShader(s.source);
    }

    public static void glAttachShader(int program, int shader) {
        Program p = programs.get(program);
        Shader s = shaders.get(shader);
        switch (s.type) {
            case GL_VERTEX_SHADER:
                p.vertex = s;
                break;
            case GL_FRAGMENT_SHADER:
                p.fragment = s;
                break;
            case GL_COMPUTE_SHADER:
                p.compute = s;
                break;
            default:
                throw new NotImplementedError();
        }
    }

    public static String glGetShaderInfoLog(int shader) {
        return null;
    }

    public static String glGetProgramInfoLog(int program) {
        return null;
    }

    public static int glGetProgrami(int program, int key) {
        Program p = programs.get(program);
        if (key == GL_LINK_STATUS) {
            return (p.fragmentP | p.computeP) != 0 ? 1 : 0;
        }
        throw new NotImplementedError();
    }

    public static void glBindAttribLocation(int program, int location, CharSequence name) {
        Program p = programs.get(program);
        if ((p.fragmentP | p.computeP) != 0L)
            throw new IllegalStateException("Cannot bind attribute locations after compilation");
        p.attributes.put(name.toString(), location);
    }

    public static void glLinkProgram(int program) {
        Program p = programs.get(program);
        if (p.compute != null) throw new NotImplementedError();
        ShaderTranslator vt = p.vertex.translator;
        ShaderTranslator ft = p.fragment.translator;
        HashSet<CharSequence> tokens = joinTokens(vt.getFunctions(), ft.getFunctions());
        join(vt.getUniforms(), ft.getUniforms(), tokens);
        join(vt.getVaryings(), ft.getVaryings(), tokens);
        String fsSource = p.fragment.translator.emitProgramShader(p, 0);
        p.fragmentP = DirectX.compilePixelShader(fsSource);
        if (p.fragmentP == 0L) throw new IllegalStateException("Fragment shader didn't compile properly");
        p.uniformBuffer = ByteBuffer.allocateDirect(p.uniformSize).order(ByteOrder.nativeOrder());
    }

    private static ProgramVariation getVariation(Program p, int pvi) {
        ProgramVariation pv = p.variations.get(pvi);
        if (pv == null) {
            pv = new ProgramVariation();
            pv.index = pvi;
            String vsSource = p.vertex.translator.emitProgramShader(p, pvi);
            if (p.variations.isEmpty()) { // only needs to be done once
                p.attrNames = p.attributes1.stream().map((it) -> it.name).toArray(String[]::new);
            }
            pv.vertexP = DirectX.compileVertexShader(vsSource, p.attrNames, p.uniformSize);
            if (pv.vertexP == 0L) throw new IllegalStateException("Vertex shader didn't compile properly");
            p.variations.put(pvi, pv);
        }
        return pv;
    }

    private static void join(
            ArrayList<Variable> a, ArrayList<Variable> b,
            HashSet<CharSequence> tokens0
    ) {
        a.removeIf((s) -> !tokens0.contains(s.getName()));
        b.removeIf((s) -> !tokens0.contains(s.getName()));
        HashSet<CharSequence> tokens = new HashSet<>(a.size());
        for (Variable ai : a) {
            tokens.add(ai.getName());
        }
        b.removeIf((bi) -> tokens.contains(bi.getName()));
        a.addAll(b);
        b.clear();
        b.addAll(a);
    }

    private static HashSet<CharSequence> joinTokens(List<List<CharSequence>> t0, List<List<CharSequence>> t1) {
        HashSet<CharSequence> tokens = new HashSet<>(64);
        for (List<CharSequence> tl : t0) tokens.addAll(tl);
        for (List<CharSequence> tl : t1) tokens.addAll(tl);
        return tokens;
    }

    public static void glUseProgram(int program) {
        if (program != currentProgram.index) {
            currentProgram = programs.get(program);
            hasValidVAO = false; // DirectX-specific
        }
    }

    public static void glDeleteProgram(int program) {
        // do this?
    }

    public static int glGetUniformLocation(int program, CharSequence name) {
        Program p = programs.get(program);
        Integer idx = p.uniforms.get(name.toString());
        if (printUniforms) System.out.println("Uniform[" + program + ", " + name + "]=" + idx);
        return idx == null ? -1 : idx;
    }

    public static void glEnable(int mode) {
        // todo store state for rendering
        switch (mode) {
            case GL_BLEND:
                blendState |= 2;
                break;
            case GL_MULTISAMPLE:
                break;
            case GL_LINE_SMOOTH:
                break;
            case GL_CULL_FACE:
                break;
            case GL_DEPTH_TEST:
                break;
            case GL_STENCIL_TEST:
                break;
            case GL_SCISSOR_TEST:
                break;
            default:
                System.out.println("Unknown enable/disable mode");
        }
    }

    public static void glDisable(int mode) {
        // todo store state for rendering
        switch (mode) {
            case GL_BLEND:
                blendState &= ~2;
                break;
            case GL_MULTISAMPLE:
                break;
            case GL_LINE_SMOOTH:
                break;
            case GL_CULL_FACE:
                break;
            case GL_DEPTH_TEST:
                break;
            case GL_STENCIL_TEST:
                break;
            case GL_SCISSOR_TEST:
                break;
            default:
                System.out.println("Unknown enable/disable mode");
        }
    }

    private static int blendState;
    private static final int[] blendStates = new int[8];

    private static int blendOp(int gl) {
        switch (gl) {
            case GL_FUNC_ADD:
                return 1;
            case GL_FUNC_SUBTRACT:
                return 2;
            case GL_FUNC_REVERSE_SUBTRACT:
                return 3;
            case GL_MIN:
                return 4;
            case GL_MAX:
                return 5;
            default:
                throw new NotImplementedError("Unknown type");
        }
    }

    private static int blend(int op) {
        switch (op) {
            case GL_ZERO:
                return 1;
            case GL_ONE:
                return 2;
            case GL_SRC_COLOR:
                return 3;
            case GL_ONE_MINUS_SRC_COLOR:
                return 4;
            case GL_SRC_ALPHA:
                return 5;
            case GL_ONE_MINUS_SRC_ALPHA:
                return 6;
            case GL_DST_ALPHA:
                return 7;
            case GL_ONE_MINUS_DST_ALPHA:
                return 8;
            case GL_DST_COLOR:
                return 9;
            case GL_ONE_MINUS_DST_COLOR:
                return 10;
            case GL_SRC_ALPHA_SATURATE:
                return 11;
            case GL_CONSTANT_COLOR:
                return 14;
            case GL_ONE_MINUS_CONSTANT_COLOR:
                return 15;
            case GL_SRC1_COLOR:
                return 16;
            case GL_ONE_MINUS_SRC1_COLOR:
                return 17;
            case GL_SRC1_ALPHA:
                return 18;
            case GL_ONE_MINUS_SRC1_ALPHA:
                return 19;
            default:
                throw new NotImplementedError("Unknown type");
        }
    }

    public static void glBlendEquationSeparate(int color, int alpha) {
        blendState = (blendState & ~0b11111100) | (blendOp(color) << 2) | (blendOp(alpha) << 5);
    }

    public static void glBlendFuncSeparate(int srcColor, int dstColor, int srcAlpha, int dstAlpha) {
        blendState = (blendState & ~(0xfffff << 8)) | // 20 bits
                (blend(srcColor) << 8) | (blend(srcAlpha) << 13) |
                (blend(dstColor) << 18) | (blend(dstAlpha) << 23);
    }

    private static int vx, vy, vw = 256, vh = 256;
    private static float zMin = 0, zMax = 1;

    public static void glViewport(int x, int y, int w, int h) {
        vx = x;
        vy = y;
        vw = w;
        vh = h;
    }

    public static void glDepthRange(double zMin0, double zMax0) {
        zMin = (float) zMin0;
        zMax = (float) zMax0;
    }

    public static void glClipControl(int corner, int depthRange) {
        if (corner != GL_LOWER_LEFT) throw new IllegalArgumentException("Unsupported option " + corner);
        if (depthRange == GL_ZERO_TO_ONE) {
            zMin = 0f;
            zMax = 1f;
        } else if (depthRange == GL_NEGATIVE_ONE_TO_ONE) {
            zMin = -1f;
            zMax = 1f;
        } else throw new IllegalArgumentException("Unsupported option " + depthRange);
    }

    private static float cr, cg, cb, ca, cd;
    private static int cs;

    public static void glClearColor(float r, float g, float b, float a) {
        cr = r;
        cg = g;
        cb = b;
        ca = a;
    }

    public static void glClearDepth(double depth) {
        cd = (float) depth;
    }

    public static void glClear(int mask) {
        if ((mask & GL_COLOR_BUFFER_BIT) != 0) {
            int index = Integer.numberOfTrailingZeros(currentFramebuffer.enabledColorTargets);
            Texture tex = currentFramebuffer.colors[index];
            long ptr = tex == null ? 0 : tex.colorRTV;
            if (ptr != 0) DirectX.clearColor(ptr, cr, cg, cb, ca);
        }
        if ((mask & (GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT)) != 0) {
            Texture tex = currentFramebuffer.depth;
            long ptr = tex == null ? 0 : tex.depthStencilRTV;
            if (ptr != 0) DirectX.clearDepth(ptr, cd, cs,
                    ((mask & GL_DEPTH_BUFFER_BIT) >> 8) + // depth is 256, becomes 1
                            ((mask & GL_STENCIL_BUFFER_BIT) >> 9) // stencil is 1024, becomes 2
            );
        }
    }

    private static final ArrayList<org.lwjgl.opengl.custom.Buffer> buffers = new ArrayList<>();
    private static final Buffer nullBuffer = new Buffer();

    static {
        buffers.add(nullBuffer);
    }

    public static int glGenBuffers() {
        org.lwjgl.opengl.custom.Buffer buffer = new org.lwjgl.opengl.custom.Buffer();
        synchronized (buffers) {
            int index = buffers.size();
            buffer.index = index;
            buffers.add(buffer);
            return index;
        }
    }

    public static void glDeleteBuffers(int id) {
        Buffer buffer = buffers.get(id);
        if (buffer.pointer != 0) {
            DirectX.destroyBuffer(buffer.pointer);
            buffer.pointer = 0;
        }
    }

    public static Buffer currABuffer = nullBuffer, currEBuffer = nullBuffer;

    public static void glBindBuffer(int target, int buffer) {
        if (target == GL_ARRAY_BUFFER) currABuffer = buffers.get(buffer);
        else if (target == GL_ELEMENT_ARRAY_BUFFER) currEBuffer = buffers.get(buffer);
        else if (target == GL_PIXEL_UNPACK_BUFFER) {
            // idk... we don't use this
        } else throw new IllegalArgumentException("Unexpected buffer type " + buffer);
    }

    private static final byte[] tmp = new byte[2048];

    private static void glBufferData(int target, long addr, long length, int usage) {
        if (target != GL_ARRAY_BUFFER && target != GL_ELEMENT_ARRAY_BUFFER) throw new NotImplementedError();
        Buffer buffer = target == GL_ARRAY_BUFFER ? currABuffer : currEBuffer;
        if (buffer.pointer != 0) DirectX.destroyBuffer(buffer.pointer);
        int dx11Usage = usage == GL_STATIC_DRAW ? 0 : // gpu read/write
                usage == GL_DYNAMIC_DRAW ? 2 : // gpu read, cpu write
                        0;
        if (dx11Usage != 0) {
            if (usageFlagWarn) System.err.println("Ignored usage flag, because that caused crashes: " + dx11Usage);
            usageFlagWarn = false;
            dx11Usage = 0; // default usage
        }
        int dx11Type = target == GL_ARRAY_BUFFER ? 1 : 2; // D3D11_BIND_VERTEX_BUFFER or D3D11_BIND_INDEX_BUFFER
        buffer.pointer = length > 0 ? DirectX.createBuffer(dx11Type, addr, length, dx11Usage) : 0;
        // System.out.println("Created buffer, located at #" + Long.toHexString(buffer.pointer));
    }

    private static boolean usageFlagWarn = true;

    public static void glBufferData(int target, ByteBuffer data, int usage) {
        long addr = MemoryUtil.memAddress(data);
        long length = data.remaining();
        glBufferData(target, addr, length, usage);
    }

    public static void glBufferData(int target, int[] data, int usage) {
        if (target != GL_ARRAY_BUFFER && target != GL_ELEMENT_ARRAY_BUFFER) throw new NotImplementedError();
        Buffer buffer = target == GL_ARRAY_BUFFER ? currABuffer : currEBuffer;
        if (buffer.pointer != 0) DirectX.destroyBuffer(buffer.pointer);
        int dx11Usage = usage == GL_STATIC_DRAW ? 0 : // gpu read/write
                usage == GL_DYNAMIC_DRAW ? 2 : // gpu read, cpu write
                        0;
        if (dx11Usage != 0) {
            if (usageFlagWarn) System.err.println("Ignored usage flag, because that caused crashes: " + dx11Usage);
            usageFlagWarn = false;
            dx11Usage = 0; // default usage
        }
        int dx11Type = target == GL_ARRAY_BUFFER ? 1 : 2; // D3D11_BIND_VERTEX_BUFFER or D3D11_BIND_INDEX_BUFFER
        buffer.pointer = data.length > 0 ? DirectX.createBufferI(dx11Type, data, dx11Usage) : 0;
        // System.out.println("Created buffer, located at #" + Long.toHexString(buffer.pointer));
    }

    public static void glBufferData(int target, ShortBuffer data, int usage) {
        long addr = MemoryUtil.memAddress(data);
        long length = (long) data.remaining() << 1;
        glBufferData(target, addr, length, usage);
    }

    public static void glBufferData(int target, FloatBuffer data, int usage) {
        long addr = MemoryUtil.memAddress(data);
        long length = (long) data.remaining() << 2;
        glBufferData(target, addr, length, usage);
    }

    public static void glBufferSubData(int target, long offset, ByteBuffer buffer) {
        /*if (target != GL_ARRAY_BUFFER && target != GL_ELEMENT_ARRAY_BUFFER) throw new NotImplementedError();
        Buffer buffer1 = target == GL_ARRAY_BUFFER ? currABuffer : currEBuffer;
        DirectX.updateBuffer(buffer1.pointer, offset, MemoryUtil.memAddress(buffer), buffer.remaining());*/
        // this is broken :/, crashes with segfault
        glBufferData(target, buffer, GL_DYNAMIC_DRAW);
    }

    public static void glBindVertexArray(int va) {
        currentVAO = vaos.get(va);
    }

    public static final int MAX_ATTRIBUTES = 16; // 16 on RTX 3070, 32 by DX11

    private static final ArrayList<VAO> vaos = new ArrayList<>();
    private static VAO currentVAO = new VAO();

    static {
        vaos.add(currentVAO);
    }

    static boolean hasValidVAO = false;

    public static void glVertexAttribPointer(int index, int channels, int type, boolean norm, int stride, long ptr) {
        VAO vao = currentVAO;
        vao.channels[index] = (byte) channels;
        vao.types[index] = type;
        vao.normalized = setFlag(vao.normalized, norm, 1 << index);
        vao.strides[index] = stride;
        if (ptr > Integer.MAX_VALUE) throw new IllegalArgumentException("Offset must be i32");
        vao.offsets[index] = (int) ptr;
        vao.buffers[index] = currABuffer;
        hasValidVAO = false;
    }

    public static void glVertexAttrib1f(int index, float value) {
        if (value != 0f) throw new IllegalArgumentException("Only zero is supported");
    }

    private static long setFlag(long l, boolean value, long mask) {
        return value ? l | mask : l & ~mask;
    }

    private static int setFlag(int l, boolean value, int mask) {
        return value ? l | mask : l & ~mask;
    }

    public static void glVertexAttribIPointer(int index, int channels, int type, int stride, long ptr) {
        glVertexAttribPointer(index, channels, ~type, false, stride, ptr);
    }

    public static void glVertexAttribDivisor(int index, int divisor) {
        currentVAO.perInstance = setFlag(currentVAO.perInstance, divisor > 0, 1 << index);
        hasValidVAO = false; // not really necessary, as it would be very unlikely that only this property is changing
    }

    public static void glEnableVertexAttribArray(int index) {
        currentVAO.enabled |= 1L << index;
        hasValidVAO = false;
    }

    public static void glDisableVertexAttribArray(int index) {
        currentVAO.enabled &= ~(1L << index);
        hasValidVAO = false;
    }

    private static final LongBuffer texBuffer =
            ByteBuffer.allocateDirect(8 * 32)
                    .order(ByteOrder.nativeOrder())
                    .asLongBuffer();

    private static final long[] colorRTVs = new long[16];

    private static boolean bindBeforeDrawing() {

        int totalH = (int) currentFramebuffer.getSize();
        DirectX.updateViewport(vx, totalH - (vy + vh), vw, vh, zMin, zMax);

        VAO vao = currentVAO;
        Program p = currentProgram;
        int pvi = DirectX.calculateProgramVariation(vao);
        ProgramVariation pv = getVariation(p, pvi);

        int vaoRet = DirectX.bindVAO(p, pv, vao);
        hasValidVAO = vaoRet == 0;
        if (!hasValidVAO) {
            if (vaoRet != -7) System.err.println("Invalid VAO! " + vaoRet);
            return false;
        }

        DirectX.bindUniforms(p, pv);
        // bind textures and samplers
        int numTextures = Math.min(p.texMap.length, 16);
        for (int i = 0; i < numTextures; i++) {
            Texture tex = boundTextures[p.texMap[i]];
            texBuffer.put(i, tex.pointer);
            if (tex.sampler == -1) {
                // todo recalculate sampler
                tex.sampler = 0;
            }
            texBuffer.put(i + numTextures, tex.sampler);
        }

        DirectX.setBlendState(blendState);

        DirectX.bindTextures(numTextures, MemoryUtil.memAddress(texBuffer));
        DirectX.setShaders(pv.vertexP, p.fragmentP);

        Framebuffer fb = currentFramebuffer;
        // todo what about disabled RTVs??? set them null???
        int numRTVs = Integer.bitCount(fb.enabledColorTargets);
        for (int i = 0; i < numRTVs; i++) {
            Texture tex = fb.colors[i];
            colorRTVs[i] = tex == null ? 0 : tex.colorRTV;
        }
        Texture dTex = fb.depth;
        long depthStencilRTV = dTex == null ? 0 : dTex.depthStencilRTV;
        DirectX.setRenderTargets(numRTVs, colorRTVs, depthStencilRTV);
        return true;
    }

    public static void glDrawArrays(int mode, int start, int length) {
        if (bindBeforeDrawing()) {
            DirectX.drawArrays(mode, start, length);
        } else System.err.println("Skipped rendering " + length + " elements");
    }

    public static void glDrawArraysInstanced(int mode, int start, int length, int instanceCount) {
        if (bindBeforeDrawing()) {
            DirectX.drawArraysInstanced(mode, start, length, instanceCount);
        } else System.err.println("Skipped rendering " + length + " elements");
    }

    public static void glDrawElements(int mode, int length, int type, long ptr) {
        if (ptr != 0) throw new IllegalArgumentException();
        if (bindBeforeDrawing()) {
            DirectX.drawElements(mode, length, type, currEBuffer.pointer);
        } else System.err.println("Skipped rendering " + length + " elements");
    }

    public static void glDrawElementsInstanced(int mode, int length, int type, long ptr, int instanceCount) {
        if (ptr != 0) throw new IllegalArgumentException();
        if (bindBeforeDrawing()) {
            DirectX.drawElementsInstanced(mode, length, type, currEBuffer.pointer, instanceCount);
        } else System.err.println("Skipped rendering " + length + " elements");
    }

    public static void glDebugMessageCallback(GLDebugMessageCallbackI callback, long window) {
    }

    public static String glGetString(int mode) {
        switch (mode) {
            case GL_VERSION:
                return "DirectX11";
            case GL_SHADING_LANGUAGE_VERSION:
                return "Shader Model <?>";
            default:
                return null;
        }
    }

    public static void glPixelStorei(int a, int b) {
        // pixel alignment... potentially not useful
    }

    private static int fb;

    private static final ArrayList<Framebuffer> framebuffers = new ArrayList<>();
    public static final Framebuffer nullFramebuffer = new Framebuffer();

    static {
        framebuffers.add(nullFramebuffer);
    }

    private static Framebuffer currentFramebuffer = nullFramebuffer;

    public static void glBindFramebuffer(int target, int fb) {
        currentFramebuffer = framebuffers.get(fb);
    }

    public static int glGenFramebuffers() {
        Framebuffer fb = new Framebuffer();
        synchronized (framebuffers) {
            int index = framebuffers.size();
            fb.index = index;
            framebuffers.add(fb);
            return index;
        }
    }

    private static int activeSlot = 0;

    public static void glGenTextures(int[] dst) {
        for (int i = 0, l = dst.length; i < l; i++) {
            dst[i] = glGenTexture();
        }
    }

    public static int glGenTexture() {
        synchronized (deletedTextures) {
            int size = deletedTextures.size();
            if (size > 0) {
                return deletedTextures.remove(size - 1).index;
            }
        }
        Texture texture = new Texture();
        synchronized (textures) {
            int index = textures.size();
            textures.add(texture);
            texture.index = index;
            return index;
        }
    }

    private static final ArrayList<Texture> textures = new ArrayList<>();
    private static final ArrayList<Texture> deletedTextures = new ArrayList<>();

    public static final Texture nullTexture = new Texture();

    static {
        nullFramebuffer.colors[0] = nullTexture;
        textures.add(nullTexture);
    }

    private static final Texture[] boundTextures = new Texture[32];
    public static Texture currentTexture = nullTexture;
    private static final int[] textureTypes = new int[32];

    public static void glBindTexture(int type, int tex) {
        currentTexture = boundTextures[activeSlot] = textures.get(tex);
        textureTypes[activeSlot] = type;
    }

    public static void glActiveTexture(int slot) {
        activeSlot = slot - GL_TEXTURE0;
    }

    public static void glTexImage2DMultisample(int target, int samples, int iFormat, int width, int height, boolean fixedSampleLocations) {
        if (target != GL_TEXTURE_2D_MULTISAMPLE) throw new IllegalArgumentException();
        Texture tex = currentTexture;
        // blitting multisampled -> single sampled: https://learn.microsoft.com/en-us/windows/win32/api/d3d11/nf-d3d11-id3d11devicecontext-resolvesubresource?redirectedfrom=MSDN
        destroyExistingTexture(tex);
        tex.format = iFormat;
        tex.width = width;
        tex.height = height;
        tex.pointer = DirectX.createTexture2DMS(samples, iFormat, width, height);
    }

    private static void destroyExistingTexture(Texture tex) {
        if (tex.colorRTV != 0) {
            DirectX.destroyColorRTV(tex.colorRTV);
            tex.colorRTV = 0;
        }
        if (tex.depthStencilRTV != 0) {
            DirectX.destroyDepthRTV(tex.depthStencilRTV);
            tex.depthStencilRTV = 0;
        }
        if (tex.pointer != 0) {
            DirectX.destroyTexture(tex.pointer);
            tex.pointer = 0;
            tex.format = 0;
        }
    }

    public static void glFramebufferTexture2D(int target, int attachment, int texTarget, int texture, int level) {
        Framebuffer fb = currentFramebuffer;
        Texture tex = textures.get(texture);
        glFramebufferTexture2D(fb, attachment, tex, level);
    }

    private static void glFramebufferTexture2D(Framebuffer fb, int attachment, Texture tex, int level) {
        int colorIndex = attachment - GL_COLOR_ATTACHMENT0;
        if (colorIndex >= 0 && colorIndex < 8) {
            fb.colors[colorIndex] = tex;
            if (tex.colorRTV != 0) DirectX.destroyColorRTV(tex.colorRTV);
            tex.colorRTV = DirectX.createColorRTV(tex.pointer);
        } else switch (attachment) {
            case GL_DEPTH_ATTACHMENT:
                fb.depth = tex;
                if (tex.depthStencilRTV != 0) DirectX.destroyDepthRTV(tex.depthStencilRTV);
                tex.depthStencilRTV = DirectX.createDepthRTV(tex.pointer);
                break;
            case GL_DEPTH_STENCIL_ATTACHMENT:
                fb.depth = fb.stencil = tex;
                if (tex.depthStencilRTV != 0) DirectX.destroyDepthRTV(tex.depthStencilRTV);
                tex.depthStencilRTV = DirectX.createDepthRTV(tex.pointer);
                break;
            default:
                throw new NotImplementedError("Unknown attachment point " + attachment);
        }
    }

    public static void glFramebufferRenderbuffer(int target, int attachment, int rbTarget, int renderbuffer) {
        glFramebufferTexture2D(currentFramebuffer, attachment, renderbuffers.get(renderbuffer), 0);
    }

    public static void glDrawBuffer(int attachment) {
        int colorIndex = attachment - GL_COLOR_ATTACHMENT0;
        currentFramebuffer.enabledColorTargets = attachment == GL_NONE ? 0 : 1 << colorIndex;
    }

    public static void glDrawBuffers(int[] attachments) {
        // we know what is put into here, so we even could just read the length, and assume
        int sum = 0;
        for (int attachment : attachments) {
            int colorIndex = attachment - GL_COLOR_ATTACHMENT0;
            sum |= 1 << colorIndex;
        }
        currentFramebuffer.enabledColorTargets = sum;
    }

    public static void glBindImageTexture(int a, int b, int c, boolean d, int e, int f, int g) {
        throw new NotImplementedError();
    }

    public static void glDispatchCompute(int gcx, int gcy, int gcz) {
        throw new NotImplementedError();
    }

    public static void glMemoryBarrier(int mode) {
        // todo do we have to do this? how is it done?
        throw new NotImplementedError();
    }

    public static int glCheckFramebufferStatus(int sth) {
        return GL_FRAMEBUFFER_COMPLETE;
    }

    public static void glBlitFramebuffer(
            int x0, int y0, int w0, int h0,
            int x1, int y1, int w1, int h1,
            int mask, int filtering
    ) {
        // todo run copy shader ...
        // todo if is multi-sampled -> single-sampled, ResolveSubresource can be used
        throw new NotImplementedError();
    }

    public static void glDeleteFramebuffers(int fbi) {
        // delete color and depth rtvs
    }

    public static void glDeleteTextures(int[] textures) {
        for (int texture : textures) {
            glDeleteTextures(texture);
        }
    }

    public static void glDeleteTextures(int index) {
        Texture texture = textures.get(index);
        destroyExistingTexture(texture);
        synchronized (deletedTextures) {
            if (deletedTextures.contains(texture))
                throw new IllegalStateException("Cannot delete texture twice");
            deletedTextures.add(texture);
        }
    }

    public static void glTexImage2D(
            int target, int level, int iFormat, int w, int h, int border,
            int dFormat, int dType, ByteBuffer dataOrNull) {
        Texture tex = currentTexture;
        destroyExistingTexture(tex);
        if (dataOrNull != null && iFormat == GL_RGB8) {
            ByteBuffer tmp = ByteBufferPool.allocateDirect(w * h * 4)
                    .order(ByteOrder.nativeOrder());
            for (int i = 0, s = w * h; i < s; i++) {
                int i4 = i * 4, i3 = i * 3;
                tmp.put(i4, dataOrNull.get(i3));
                tmp.put(i4 + 1, dataOrNull.get(i3 + 1));
                tmp.put(i4 + 2, dataOrNull.get(i3 + 2));
                tmp.put(i4 + 3, (byte) -1);
            }
            tmp.position(0).limit(w * h * 4);
            glTexImage2D(target, level, GL_RGBA8, w, h, border, GL_RGBA, GL_UNSIGNED_BYTE, tmp);
            ByteBufferPool.free(tmp);
            return;
        }
        tex.format = iFormat;
        tex.width = w;
        tex.height = h;
        tex.pointer = dataOrNull == null ?
                DirectX.createTexture2DMS(1, iFormat, w, h) :
                DirectX.createTexture2D(1, iFormat, w, h, dFormat, dType, MemoryUtil.memAddress(dataOrNull));
    }

    public static void glTexImage2D(
            int target, int level, int iFormat, int w, int h, int border,
            int dFormat, int dType, IntBuffer dataOrNull) {
        Texture tex = currentTexture;
        destroyExistingTexture(tex);
        tex.format = iFormat;
        tex.width = w;
        tex.height = h;
        if (dataOrNull != null && iFormat == GL_RGB8) throw new NotImplementedError("Insert alpha");
        tex.pointer = dataOrNull == null ?
                DirectX.createTexture2DMS(1, iFormat, w, h) :
                DirectX.createTexture2D(1, iFormat, w, h, dFormat, dType, MemoryUtil.memAddress(dataOrNull));
    }

    public static void glTexImage2D(int target, int level, int iFormat, int w, int h, int border,
                                    int dFormat, int dType, int[] dataOrNull) {
        Texture tex = currentTexture;
        destroyExistingTexture(tex);
        tex.format = iFormat;
        tex.width = w;
        tex.height = h;
        tex.pointer = dataOrNull == null ?
                DirectX.createTexture2DMS(1, iFormat, w, h) :
                DirectX.createTexture2Di(1, iFormat, w, h, dFormat, dType, dataOrNull);
    }

    private static boolean isFP16Format(int iFormat) {
        return iFormat == GL_R16F || iFormat == GL_RG16F || iFormat == GL_RGB16F || iFormat == GL_RGBA16F;
    }

    public static void glTexImage2D(int target, int level, int iFormat, int w, int h, int border,
                                    int dFormat, int dType, FloatBuffer dataOrNull) {
        Texture tex = currentTexture;
        destroyExistingTexture(tex);
        tex.width = w;
        tex.height = h;
        tex.format = iFormat;
        if (isFP16Format(iFormat) && dType == GL_FLOAT && dataOrNull != null) {
            ByteBuffer tmpBytes = ByteBufferPool.allocateDirect(dataOrNull.remaining() * 2);
            tmpBytes.order(ByteOrder.nativeOrder()).position(0);
            ShortBuffer tmp = tmpBytes.asShortBuffer();
            for (int i = 0, r = dataOrNull.remaining(), p = dataOrNull.position(); i < r; i++) {
                tmp.put(i, (short) Floats.float32ToFloat16(dataOrNull.get(p + i)));
            }
            dType = GL_HALF_FLOAT;
            tex.pointer = DirectX.createTexture2D(1, iFormat, w, h, dFormat, dType, MemoryUtil.memAddress(tmp));
            ByteBufferPool.free(tmpBytes);
        } else {
            if (dataOrNull != null && iFormat == GL_RGB8) throw new NotImplementedError("Insert alpha");
            tex.pointer = dataOrNull == null ?
                    DirectX.createTexture2DMS(1, iFormat, w, h) :
                    DirectX.createTexture2D(1, iFormat, w, h, dFormat, dType, MemoryUtil.memAddress(dataOrNull));
        }
    }

    public static void glTexSubImage2D(
            int target, int level, int x0, int y0, int dx, int dy, int format, int type, long buffer,
            long length
    ) {
        if (level != 0) throw new NotImplementedError();
        Texture tex = currentTexture;
        if (format == GL_RGB && type == GL_UNSIGNED_BYTE) throw new NotImplementedError("Insert alpha");
        DirectX.updateTexture2D(tex.pointer, x0, y0, dx, dy, format, type, buffer, length);
    }

    public static void glTexSubImage2D(
            int target, int level, int x0, int y0, int dx, int dy,
            int format, int type, IntBuffer buffer
    ) {
        glTexSubImage2D(target, level, x0, y0, dx, dy, format, type,
                MemoryUtil.memAddressSafe(buffer), (long) buffer.remaining() << 2);
    }

    public static void glTexSubImage2D(
            int target, int level, int x0, int y0, int dx, int dy,
            int dFormat, int dType, FloatBuffer dataOrNull
    ) {
        if (dataOrNull != null && dType == GL_FLOAT && isFP16Format(currentTexture.format)) {
            ByteBuffer tmpBytes = ByteBufferPool.allocateDirect(dataOrNull.remaining() << 1);
            tmpBytes.order(ByteOrder.nativeOrder()).position(0);
            ShortBuffer tmp = tmpBytes.asShortBuffer();
            for (int i = 0, r = dataOrNull.remaining(), p = dataOrNull.position(); i < r; i++) {
                tmp.put(i, (short) Floats.float32ToFloat16(dataOrNull.get(p + i)));
            }
            dType = GL_HALF_FLOAT;
            glTexSubImage2D(target, level, x0, y0, dx, dy, dFormat, dType,
                    MemoryUtil.memAddress(tmpBytes), tmpBytes.remaining());
            ByteBufferPool.free(tmpBytes);
        } else {
            glTexSubImage2D(target, level, x0, y0, dx, dy, dFormat, dType,
                    MemoryUtil.memAddressSafe(dataOrNull), (long) dataOrNull.remaining() << 2);
        }
    }

    public static void glTexParameteri(int target, int key, int value) {
        switch (key) {
            case GL_TEXTURE_MIN_FILTER:
                currentTexture.minFilter = value;
                break;
            case GL_TEXTURE_MAG_FILTER:
                currentTexture.magFilter = value;
                break;
            case GL_TEXTURE_WRAP_S:
                currentTexture.wrapS = value;
                break;
            case GL_TEXTURE_WRAP_T:
                currentTexture.wrapT = value;
                break;
            case GL_TEXTURE_WRAP_R:
                currentTexture.wrapU = value;
                break;
            case GL_GENERATE_MIPMAP:
                // todo is there such a flag in DirectX?
                break;
            default:
                throw new NotImplementedError("glTexParameteri(" + target + ", " + key + ", " + value + ")");
        }
    }

    public static void glTexParameterfv(int target, int key, float[] value) {
        if (key != GL_TEXTURE_BORDER_COLOR) throw new NotImplementedError();
        if (value != null && value.length >= 4) {
            currentTexture.br = value[0];
            currentTexture.bg = value[1];
            currentTexture.bb = value[2];
            currentTexture.ba = value[3];
        }
    }

    public static void glTexParameteriv(int target, int key, int[] value) {
        if (key != GL_TEXTURE_SWIZZLE_RGBA) throw new IllegalArgumentException();
        // do this (used for mono-textures, I think)
        // throw new NotImplementedError();
        System.err.println("Mono-texture-swizzling isn't supported yet");
    }

    public static void glTexImage3D(int target, int level, int iFormat, int w, int h, int d, int border,
                                    int dFormat, int dType, int[] dataOrNull) {
        Texture tex = currentTexture;
        destroyExistingTexture(tex);
        tex.format = iFormat;
        tex.width = w;
        tex.height = h;
        tex.depth = d;
        if (target != GL_TEXTURE_3D && target != GL_TEXTURE_2D_ARRAY) throw new NotImplementedError();
        tex.pointer = dataOrNull == null ?
                DirectX.createTexture3DMS(target == GL_TEXTURE_3D ? 0 : 1, 1, iFormat, w, h, d) :
                DirectX.createTexture3Di(target == GL_TEXTURE_3D ? 0 : 1, 1, iFormat, w, h, d, dFormat, dType, dataOrNull);
    }

    public static void glGenerateMipmap(int target) {
        DirectX.generateMipmaps(currentTexture.pointer);
    }

    // todo bind these
    //  https://learn.microsoft.com/en-us/windows/win32/direct3d11/d3d10-graphics-programming-guide-depth-stencil
    private static int depthFunc;
    private static boolean depthMask;
    private static int cullMode = 0;

    public static void glDepthFunc(int func) {
        depthFunc = func;
    }

    public static void glDepthMask(boolean enabled) {
        depthMask = enabled;
    }

    public static void glCullFace(int mode) {
        cullMode = mode;
    }

    public static void glFinish() {
        // do something? only latency or read-out related
    }

    public static boolean printUniforms = false;

    public static void glUniform1f(int pos, float x) {
        if (printUniforms) System.out.println("U[" + currentProgram.index + ", " + pos + "]=float(" + x + ")");
        ByteBuffer dst = currentProgram.uniformBuffer;
        dst.putFloat(pos, x);
    }

    public static void glUniform1i(int pos, int x) {
        if (printUniforms) System.out.println("U[" + currentProgram.index + ", " + pos + "]=int(" + x + ")");
        if (pos < 1_000_000) {
            currentProgram.uniformBuffer.putInt(pos, x);
        } else {
            // binding texture to respective slot
            currentProgram.texMap[pos - 1_000_000] = x;
        }
    }

    public static void glUniform2f(int pos, float x, float y) {
        ByteBuffer dst = currentProgram.uniformBuffer;
        if (printUniforms) System.out.println("U[" + currentProgram.index + ", " + pos + "]=vec2(" + x + "," + y + ")");
        dst.putFloat(pos, x);
        dst.putFloat(pos + 4, y);
    }

    public static void glUniform2i(int pos, int x, int y) {
        ByteBuffer dst = currentProgram.uniformBuffer;
        if (printUniforms)
            System.out.println("U[" + currentProgram.index + ", " + pos + "]=ivec2(" + x + "," + y + ")");
        dst.putInt(pos, x);
        dst.putInt(pos + 4, y);
    }

    public static void glUniform3f(int pos, float x, float y, float z) {
        ByteBuffer dst = currentProgram.uniformBuffer;
        if (printUniforms)
            System.out.println("[" + currentProgram.index + "," + pos + "]=vec3(" + x + "," + y + "," + z + ")");
        dst.putFloat(pos, x);
        dst.putFloat(pos + 4, y);
        dst.putFloat(pos + 8, z);
    }

    public static void glUniform3i(int pos, int x, int y, int z) {
        ByteBuffer dst = currentProgram.uniformBuffer;
        if (printUniforms)
            System.out.println("[" + currentProgram.index + "," + pos + "]=ivec3(" + x + "," + y + "," + z + ")");
        dst.putInt(pos, x);
        dst.putInt(pos + 4, y);
        dst.putInt(pos + 8, z);
    }

    public static void glUniform4f(int pos, float x, float y, float z, float w) {
        ByteBuffer dst = currentProgram.uniformBuffer;
        if (printUniforms)
            System.out.println("[" + currentProgram.index + "," + pos + "]=vec4(" + x + "," + y + "," + z + "," + w + ")");
        dst.putFloat(pos, x);
        dst.putFloat(pos + 4, y);
        dst.putFloat(pos + 8, z);
        dst.putFloat(pos + 12, w);
    }

    public static void glUniform4fv(int pos, FloatBuffer buffer) {
        ByteBuffer dst = currentProgram.uniformBuffer;
        if (printUniforms)
            System.out.println("[" + currentProgram.index + "," + pos + "]=vec4[...]");
        for (int i = 0, p = buffer.position(), r = buffer.remaining(); i < r; i++) {
            dst.putFloat(pos, buffer.get(p + i));
            pos += 4;
        }
    }

    public static void glUniform4i(int pos, int x, int y, int z, int w) {
        ByteBuffer dst = currentProgram.uniformBuffer;
        if (printUniforms)
            System.out.println("[" + currentProgram.index + "," + pos + "]=ivec4(" + x + "," + y + "," + z + "," + w + ")");
        dst.putInt(pos, x);
        dst.putInt(pos + 4, y);
        dst.putInt(pos + 8, z);
        dst.putInt(pos + 12, w);
    }

    public static void glUniformMatrix3fv(int pos, boolean transpose, FloatBuffer src) {
        // todo is this supported in HLSL?
        if (transpose) throw new IllegalArgumentException();
        ByteBuffer dst = currentProgram.uniformBuffer;
        if (printUniforms) System.out.println("[" + currentProgram.index + "," + pos + "]=mat3(...)");
        for (int i = pos, j = src.position(), i1 = pos + 36; i < i1; i += 4, j++) {
            dst.putFloat(i, src.get(j));
        }
    }

    public static void glUniformMatrix4fv(int pos, boolean transpose, FloatBuffer src) {
        if (transpose) throw new IllegalArgumentException();
        ByteBuffer dst = currentProgram.uniformBuffer;
        if (printUniforms) System.out.println("[" + currentProgram.index + "," + pos + "]=mat4(...)");
        for (int i = pos, j = src.position(), i1 = pos + 64; i < i1; i += 4, j++) {
            dst.putFloat(i, src.get(j));
        }
    }

    public static void glUniformMatrix4x3fv(int pos, boolean transpose, FloatBuffer src) {
        // todo is this supported in HLSL?
        if (transpose) throw new IllegalArgumentException();
        ByteBuffer dst = currentProgram.uniformBuffer;
        if (printUniforms) System.out.println("[" + currentProgram.index + "," + pos + "]=mat4x3(...)");
        for (int i = pos, j = src.position(), i1 = pos + 48; i < i1; i += 4, j++) {
            dst.putFloat(i, src.get(j));
        }
    }

    public static void glObjectLabel(int type, int object, CharSequence name) {
    }

    private static final ArrayList<Renderbuffer> renderbuffers = new ArrayList<>();
    private static Renderbuffer currentRenderbuffer = null;

    static {
        renderbuffers.add(null);
    }

    public static int glGenRenderbuffers() {
        Renderbuffer buffer = new Renderbuffer();
        synchronized (renderbuffers) {
            int index = renderbuffers.size();
            renderbuffers.add(buffer);
            return index;
        }
    }

    public static void glDeleteRenderbuffers(int id) {
        Renderbuffer renderbuffer = renderbuffers.get(id);
        destroyExistingTexture(renderbuffer);
    }

    public static void glBindRenderbuffer(int target, int index) {
        if (target != GL_RENDERBUFFER) throw new IllegalArgumentException();
        currentRenderbuffer = renderbuffers.get(index);
    }

    public static void glRenderbufferStorage(int target, int format, int width, int height) {
        Texture old = currentTexture;
        currentTexture = currentRenderbuffer;
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, 0, 0, (ByteBuffer) null);
        currentTexture = old;
    }

}
