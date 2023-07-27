package me.anno.directx11

import me.anno.Engine
import me.anno.config.DefaultConfig.style
import me.anno.gpu.GFXBase
import me.anno.gpu.shader.OpenGLShader.Companion.UniformCacheSize
import me.anno.studio.StudioBase
import me.anno.ui.debug.TestStudio.Companion.testUI
import me.anno.ui.editor.files.FileExplorer
import me.anno.utils.OS.pictures
import kotlin.system.exitProcess

fun main() {
    runDX11()
}

fun runDX11() {
    try {
        // we have our own cache
        UniformCacheSize = 0
        GFXBase.useSeparateGLFWThread = false
        testUI("Engine in DirectX11") {
            StudioBase.instance?.enableVSync = false
            FileExplorer(pictures.getChild("Anime/anim"), style)
        }
    } catch (e: Throwable) {
        e.printStackTrace()
    }
    Engine.requestShutdown()
    exitProcess(0)
}
