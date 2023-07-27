package me.anno.directx11

import me.anno.gpu.GFXBase
import me.anno.maths.Maths.hasFlag
import me.anno.utils.Color.b01
import me.anno.utils.Color.g01
import me.anno.utils.Color.r01
import org.lwjgl.glfw.GLFW.*
import org.lwjgl.opengl.GL
import org.lwjgl.opengl.GL11C.*
import org.lwjgl.opengl.GL30
import java.io.File
import java.nio.ByteBuffer
import java.nio.ByteOrder
import kotlin.concurrent.thread
import kotlin.math.cos
import kotlin.math.sin

fun main() {

    GFXBase.forceLoadRenderDoc()

    val w = 800
    val h = 600

    val window = glfwCreateWindow(w, h, "Window", 0, 0)

    glfwMakeContextCurrent(window)
    GL.createCapabilities()

    val assets = File(System.getProperty("user.home"), "Documents/IdeaProjects/JDirectX11/assets")

    val vss = glCreateShader(GL30.GL_VERTEX_SHADER)
    val fss = glCreateShader(GL30.GL_FRAGMENT_SHADER)
    glShaderSource(vss, File(assets, "vs.glsl").readText())
    glShaderSource(fss, File(assets, "fs.glsl").readText())
    glCompileShader(vss)
    glCompileShader(fss)
    val program = glCreateProgram()
    glAttachShader(program, vss)
    glAttachShader(program, fss)
    glLinkProgram(program)
    glUseProgram(program)

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
        .put(vertexData)
    vertexData1.flip()

    val vertexBuffer0 = glGenBuffers()
    glBindBuffer(GL30.GL_ARRAY_BUFFER, vertexBuffer0)
    glBufferData(GL30.GL_ARRAY_BUFFER, vertexData1, GL30.GL_STATIC_DRAW)
    val vertexBuffer = currABuffer.pointer

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

    val instanceBuffer0 = glGenBuffers()
    glBindBuffer(GL30.GL_ARRAY_BUFFER, instanceBuffer0)
    glBufferData(GL30.GL_ARRAY_BUFFER, instanceData1, GL30.GL_STATIC_DRAW)
    val instanceBuffer = currABuffer.pointer

    val vaoChannels = byteArrayOf(2, 3, 3, 3)
    val vaoTypes = intArrayOf(GL30.GL_FLOAT, GL30.GL_FLOAT, GL30.GL_FLOAT, GL30.GL_FLOAT)
    val vaoStrides = intArrayOf(vertexStride, instanceStride, instanceStride, instanceStride)
    val vaoOffsets = intArrayOf(0, 0, 12, 24)
    val vaoBuffers = longArrayOf(vertexBuffer, instanceBuffer, instanceBuffer, instanceBuffer)
    val perInstance = 0b1110
    val normalized = 0
    // val enabled = 0b1111

    for (i in 0 until 4) {
        val flag = 1 shl i
        glBindBuffer(GL30.GL_ARRAY_BUFFER, if (vaoBuffers[i] == vertexBuffer) vertexBuffer0 else instanceBuffer0)
        glVertexAttribPointer(
            i, vaoChannels[i].toInt(), vaoTypes[i],
            normalized.hasFlag(flag), vaoStrides[i],
            vaoOffsets[i].toLong()
        )
        glVertexAttribDivisor(i, if (perInstance.hasFlag(flag)) 1 else 0)
        glEnableVertexAttribArray(i)
    }

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
    texData.flip()

    val tex0 = glGenTexture()
    glBindTexture(GL30.GL_TEXTURE_2D, tex0)
    glTexImage2D(
        GL30.GL_TEXTURE_2D, 0, GL30.GL_RGBA8, 4, 4, 0,
        GL30.GL_RGBA, GL30.GL_UNSIGNED_BYTE, texData
    )

    val ws = IntArray(1)
    val hs = IntArray(1)

    glfwSwapInterval(4)
    glBindFramebuffer(GL30.GL_FRAMEBUFFER, 0)

    // async doesn't work :/
    if (false) thread {
        glfwMakeContextCurrent(window)
        while (!glfwWindowShouldClose(window)) {
            glfwWaitEventsTimeout(0.0)
            Thread.sleep(5)
        }
    }

    var time = 0f
    val offsetLoc = glGetUniformLocation(program, "offset")
    while (!glfwWindowShouldClose(window)) {
        glfwWaitEventsTimeout(0.0)

        glfwGetFramebufferSize(window, ws, hs)
        glViewport(0, 0, ws[0], hs[0])
        glDepthRange(0.0, 1.0)

        glUniform2f(offsetLoc, sin(time), cos(time))

        glClearColor(0.1f, 0.2f, 0.6f, 1f)
        glClear(GL30.GL_COLOR_BUFFER_BIT)

        glDrawArraysInstanced(4, 0, vertexData.size / 2, instanceData.size)
        glfwSwapBuffers(window)

        time += 1f / 60f
    }

}