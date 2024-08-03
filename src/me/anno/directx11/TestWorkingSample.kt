package me.anno.directx11

import me.anno.directx11.DirectX.*
import me.anno.gpu.RenderDoc.forceLoadRenderDoc
import me.anno.utils.Color.b01
import me.anno.utils.Color.g01
import me.anno.utils.Color.r01
import org.lwjgl.opengl.GL11.*
import org.lwjgl.system.MemoryUtil
import java.io.File
import java.nio.ByteBuffer
import java.nio.ByteOrder
import kotlin.math.cos
import kotlin.math.sin

fun main() {

    forceLoadRenderDoc()

    val w = 800
    val h = 600

    val window = createWindow(w, h, "Window", "")

    makeContextCurrent(window)
    doAttachDirectX()

    val assets = File(System.getProperty("user.home"), "Documents/IdeaProjects/JDirectX11/assets")
    val shaderText = File(assets, "shaders.hlsl").readText()
    val vs = compileVertexShader(shaderText, arrayOf("coords", "instData", "color0x", "color1x"), 8)
    val ps = compilePixelShader(shaderText)

    val vertexData = floatArrayOf(
        -0.5f, 0.5f,
        +0.5f, -0.5f,
        -0.5f, -0.5f,
        -0.5f, 0.5f,
        +0.5f, 0.5f,
        +0.5f, -0.5f
    )

    val vertexStride = 4 * 2
    val vertexData1 = ByteBuffer.allocateDirect(vertexData.size * 4)
        .order(ByteOrder.nativeOrder())
        .asFloatBuffer()
        .put(vertexData).flip()

    val vertexBuffer = createBuffer(1, MemoryUtil.memAddress(vertexData1), vertexData1.remaining() * 4L, 1)

    val red = 0xff0000
    val green = 0x00ff00
    val blue = 0x0000ff
    val black = 0x000000
    val white = 0xffffff
    val instanceData = arrayOf(
        arrayOf(0.0f, 0.0f, 0.0f, red, black),
        arrayOf(0.5f, 1.0f, 0.0f, green, black),
        arrayOf(1.0f, 2.0f, 0.0f, blue, red),
        arrayOf(1.5f, 3.0f, 0.0f, green, white),
        arrayOf(2.0f, -3.0f, 0.0f, red, white),
        arrayOf(2.5f, -2.0f, 0.0f, blue, white),
        arrayOf(3.0f, -1.0f, 0.0f, black, white)
    )

    val instanceStride = 4 * 3 + 4 * 3 * 2
    val instanceData1 = ByteBuffer.allocateDirect(instanceData.size * instanceStride)
        .order(ByteOrder.nativeOrder())
    for (v in instanceData) {
        for (vi in v) {
            when (vi) {
                is Float -> instanceData1.putFloat(vi)
                is Int -> {
                    instanceData1.putFloat(vi.r01())
                    instanceData1.putFloat(vi.g01())
                    instanceData1.putFloat(vi.b01())
                }
            }
        }
    }
    instanceData1.flip()

    val instanceBuffer = createBuffer(2, MemoryUtil.memAddress(instanceData1), instanceData1.remaining().toLong(), 1)

    val vaoChannels = byteArrayOf(2, 3, 3, 3)
    val vaoTypes = intArrayOf(GL_FLOAT, GL_FLOAT, GL_FLOAT, GL_FLOAT)
    val vaoStrides = intArrayOf(vertexStride, instanceStride, instanceStride, instanceStride)
    val vaoOffsets = intArrayOf(0, 0, 12, 24)
    val vaoBuffers = longArrayOf(vertexBuffer, instanceBuffer, instanceBuffer, instanceBuffer)
    val perInstance = 0b1110
    val normalized = 0

    val texData = ByteBuffer.allocateDirect(16 * 4)
        .order(ByteOrder.nativeOrder())
        .put(
            byteArrayOf(
                0, 0, 0, 0, -1, -1, -1, -1, -1, -1, -1, -1, 0, 0, 0, 0,
                -1, -1, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0, -1, -1, -1, -1,
                -1, -1, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0, -1, -1, -1, -1,
                0, 0, 0, 0, -1, -1, -1, -1, -1, -1, -1, -1, 0, 0, 0, 0,
            )
        )
        .flip()
    val tex = createTexture2D(
        1, GL_RGBA8, 4, 4, GL_RGBA, GL_UNSIGNED_BYTE,
        MemoryUtil.memAddress(texData)
    )

    val texPtrs = ByteBuffer.allocateDirect(8 * 2)
        .order(ByteOrder.nativeOrder())
    texPtrs.putLong(0, tex)

    val ws = IntArray(1)
    val hs = IntArray(1)

    val uniformData = ByteBuffer.allocateDirect(8)
        .order(ByteOrder.nativeOrder())

    setVsyncInterval(0)

    var time = 0f
    val rtvs = LongArray(1)
    while (!shouldWindowClose(window)) {
        val primRTV = waitEvents()

        getFramebufferSize(window, ws, hs)
        updateViewport(0f, 0f, ws[0].toFloat(), hs[0].toFloat(), 0f, 1f)

        uniformData.putFloat(0, sin(time)).putFloat(4, cos(time))
        doBindUniforms(vs, MemoryUtil.memAddress(uniformData), uniformData.remaining())

        clearColor(primRTV, 0.1f, 0.2f, 0.6f, 1f)
        rtvs[0] = primRTV
        setRenderTargets(1, rtvs, 0)

        setShaders(vs, ps)
        bindTextures(1, MemoryUtil.memAddress(texPtrs))

        doBindVAO(
            vs, vaoChannels, vaoTypes,
            vaoStrides, vaoOffsets, vaoBuffers,
            perInstance, normalized
        )

        drawArraysInstanced(4, 0, vertexData.size / 2, instanceData.size)
        swapBuffers(window)

        time += 1f / 60f
    }

}