package me.anno.directx11

import me.anno.utils.OS.desktop
import org.anarres.cpp.Feature
import org.anarres.cpp.Preprocessor
import org.anarres.cpp.StringLexerSource
import org.anarres.cpp.Token

fun main(){
    val src0 = desktop.getChild("warn1690464137068.txt").readTextSync()
    val pp = Preprocessor()
    pp.addFeature(Feature.CSYNTAX)
    pp.addFeature(Feature.KEEPCOMMENTS)
    pp.addInput(StringLexerSource("#define HLSL\n"))
    pp.addInput(StringLexerSource(src0))
    pp.addFeature(Feature.DEBUG)
    val tmp = StringBuilder()
    while (true) {
        val tk = pp.token()
        if (tk.type == Token.EOF) break
        tmp.append(tk.text)
    }
    println(tmp.toString())
}