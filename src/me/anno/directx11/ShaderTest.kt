package me.anno.directx11

import me.anno.utils.OS.documents

fun main() {
    val window = DirectX.createWindow(10, 10, "Test", "")
    DirectX.makeContextCurrent(window)
    DirectX.attachDirectX()
    val folder = documents.getChild("IdeaProjects/JDirectX11/debug")
    DirectX.compilePixelShader(folder.getChild("test.1.fs.hlsl").readTextSync())
}