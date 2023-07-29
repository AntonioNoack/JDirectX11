package me.anno.directx11

import me.anno.Engine
import me.anno.engine.RemsEngine
import me.anno.gpu.GFXBase
import me.anno.gpu.shader.OpenGLShader.Companion.UniformCacheSize
import kotlin.system.exitProcess

fun main() {
    runDX11()
}

fun runDX11() {
    try {
        // we have our own cache
        UniformCacheSize = 0
        GFXBase.useSeparateGLFWThread = false
        RemsEngine().run()
    } catch (e: Throwable) {
        e.printStackTrace()
    }
    Engine.requestShutdown()
    exitProcess(0)
}
