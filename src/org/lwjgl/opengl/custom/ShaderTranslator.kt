package org.lwjgl.opengl.custom

import me.anno.utils.OS
import me.anno.utils.strings.StringHelper.indexOf2
import me.anno.utils.structures.lists.Lists.indexOf2
import me.anno.utils.structures.lists.Lists.sortedByTopology
import org.anarres.cpp.Feature
import org.anarres.cpp.Preprocessor
import org.anarres.cpp.StringLexerSource
import org.anarres.cpp.Token
import org.lwjgl.opengl.GL20

class ShaderTranslator(val type: Int) {

    companion object {

        var debug = false
        var write = true

        val texMap = hashMapOf(
            // todo shadow types are just normal -> sample() return type is wrong :(
            "sampler2D" to "Texture2D",
            "sampler3D" to "Texture3D",
            "sampler2DMS" to "Texture2D",
            "sampler2DShadow" to "Texture2D",
            "sampler3DShadow" to "Texture3D",
            "isampler2D" to "Texture2D",
            "isampler3D" to "Texture3D",
            "usampler2D" to "Texture2D",
            "usampler3D" to "Texture3D",
            "samplerCube" to "TextureCube",
            "samplerCubeShadow" to "TextureCube",
            "sampler2DArray" to "Texture2DArray"
        )

        val typeMap = hashMapOf(
            "float" to "float",
            "vec2" to "float2",
            "vec3" to "float3",
            "vec4" to "float4",
            "int" to "int",
            "ivec2" to "int2",
            "ivec3" to "int3",
            "ivec4" to "int4",
            "bool" to "bool",
            "bvec2" to "bool2",
            "bvec3" to "bool3",
            "bvec4" to "bool4", // todo they might have to become int, because of uniform1i
            "uint" to "uint",
            "uvec2" to "uint2",
            "uvec3" to "uint3",
            "uvec4" to "uint4",
            "double" to "double",
            "dvec2" to "double2",
            "dvec3" to "double3",
            "dvec4" to "double4",
            "void" to "void",
            "mat2" to "float2x2",
            "mat2x2" to "float2x2",
            "mat3" to "float3x3",
            "mat3x3" to "float3x3",
            "mat4" to "float4x4",
            "mat4x3" to "float3x4", // is mirrored
        ) + texMap

        val typeSizeMap = hashMapOf(
            "float" to 4, "vec2" to 8, "vec3" to 12, "vec4" to 16,
            "int" to 4, "ivec2" to 8, "ivec3" to 12, "ivec4" to 16,
            "bool" to 4, "bvec2" to 8, "bvec3" to 12, "bvec4" to 16, // a little wasteful...
            "uint" to 4, "uvec2" to 8, "uvec3" to 12, "uvec4" to 16,
            "void" to -1,
            "mat2" to 16, "mat3" to 36, "mat4" to 64, "mat4x3" to 64, "mat3x4" to 48,
        )

        val typeAlignmentMap = hashMapOf(
            "float" to 4, "vec2" to 8, "vec3" to 16, "vec4" to 16,
            "int" to 4, "ivec2" to 8, "ivec3" to 16, "ivec4" to 16,
            "bool" to 4, "bvec2" to 8, "bvec3" to 16, "bvec4" to 16, // a little wasteful...
            "uint" to 4, "uvec2" to 8, "uvec3" to 16, "uvec4" to 16,
            "void" to -1,
            "mat2" to 16, "mat3" to 48, "mat4" to 64, "mat4x3" to 64, "mat3x4" to 48,
        )

        val wordMap = typeMap + hashMapOf(
            "mix" to "lerp",
            "fract" to "frac",
            "dFdx" to "ddx",
            "dFdy" to "ddy"
        )
    }

    val i0 = IntArray(0)

    val functions = ArrayList<List<CharSequence>>()
    val uniforms = ArrayList<Variable>()
    val attributes = ArrayList<Variable>()
    val varyings = ArrayList<Variable>()
    val constants = ArrayList<Pair<Variable, String>>()
    val layers = ArrayList<Variable>()
    val variables = ArrayList<Variable>()

    val isVertex = type == GL20.GL_VERTEX_SHADER
    val isFragment = type == GL20.GL_FRAGMENT_SHADER

    fun parseVariable(list: List<CharSequence>): Variable {
        val type = list[0].toString()
        return if (list.size == 2) Variable(type, list[1].toString(), i0)
        else {
            val sl = list.subList(1, list.size)
            if (sl.count { it[0] !in '0'..'9' && it[0] !in "[]" } == 1) {
                // there is only one name
                val name = sl.first { it[0] !in '0'..'9' && it[0] !in "[]" }.toString()
                val dimensions = sl.mapNotNull { it.toString().toIntOrNull() }.toIntArray()
                Variable(type, name, dimensions)
            } else throw NotImplementedError(list.joinToString(" "))
        }
    }

    fun translateShader(src0: String) {

        if (debug) {
            println(src0)
            println(type)
        }

        val src1 = if ("define" in src0) {

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

            tmp.toString()
        } else src0

        val tokens = tokenize(src1)
        if (debug) println(tokens)

        parse(tokens)

        if (debug) {
            println(functions)
            println(variables)
            println(uniforms)
            println(attributes)
            println(layers)
        }

    }

    fun emitProgramShader(p: Program, pvi: Int): String {

        val r = StringBuilder()
        val inputs = ArrayList<List<CharSequence>>()
        val outputs = ArrayList<List<CharSequence>>()

        r.append("cbuffer constants : register(b0) { // uniforms\n")
        // sort uniforms by type and name
        // assign them their correct byte offsets
        var pos = 0
        for (v in uniforms
            .filter { v -> texMap[v.type] == null }
            .sortedByDescending { v ->
                typeSizeMap[v.type] ?: throw IllegalStateException("Missing type size for $v")
            }) {
            val size = typeSizeMap[v.type]!!
            val alignment = typeAlignmentMap[v.type]!!
            val padding = alignment - (pos % alignment)
            if (padding != alignment) {
                // add soo many bytes as padding
                for (i in 0 until (padding ushr 2)) {
                    r.append("  float __pad").append(pos).append(";\n")
                    pos += 4
                }
            }
            val type = typeMap[v.type] ?: continue
            val numElements = v.appendDeclaration(type, r)
            r.append(";\n")
            // r.append("; // ").append(pos).append(", align ").append(alignment).append("\n")
            p.uniforms[v.name] = pos
            p.uniformSizes[pos] = size
            // special cases:
            pos += size * numElements
        }
        p.uniformSize = (pos + 15).ushr(4).shl(4)
        r.append("};\n")

        if (isVertex) {
            r.append("struct VS_Input { // attributes\n")
            r.append("  uint gl_VertexID : SV_VertexID;\n")
            r.append("  uint gl_InstanceID : SV_InstanceID;\n")
            inputs.add(listOf("VS_Input", "uint", "gl_VertexID"))
            inputs.add(listOf("VS_Input", "uint", "gl_InstanceID"))
            // todo respect attribute indices
            val addAttr = p.attributes1.isEmpty()
            for (j in attributes.indices) {
                val v = attributes[j]
                val type = typeMap[v.type]!!
                val name = v.name
                if (addAttr) p.attributes1.add(Program.Attribute(v.type, type, name + "_"))
                if ((pvi and (1 shl j)) != 0) continue
                inputs.add(v.listDeclaration("VS_Input", type))
                v.appendDeclaration(type, r)
                r.append(" : ").append(name).append("_;\n")
            }
            r.append("};\n")
            if (pvi != 0) {
                r.append("struct VS_InstInput { // instanced attributes\n")
                for (j in attributes.indices) {
                    if ((pvi and (1 shl j)) == 0) continue
                    val v = attributes[j]
                    val type = typeMap[v.type]!!
                    val name = v.name
                    inputs.add(v.listDeclaration("VS_InstInput", type))
                    r.append("  ").append(type).append(' ').append(name)
                    for (ai in v.dimensions) {
                        r.append('[').append(ai).append(']')
                    }
                    r.append(" : ").append(name).append("_;\n")
                }
                r.append("};\n")
            }
        }

        val vsOutputList = if (isVertex) outputs else inputs
        r.append("struct VS_Output { // varyings\n")
        for (j in varyings.indices) {
            val v = varyings[j]
            val type = typeMap[v.type]!!
            vsOutputList.add(v.listDeclaration("VS_Output", type))
            v.appendDeclaration(type, r)
            r.append(" : V").append(j)
            r.append(";\n")
        }
        if (isVertex) {
            r.append("  float4 gl_Position : SV_POSITION;\n")
            vsOutputList.add(listOf("VS_Output", "float4", "gl_Position"))
        } else if (isFragment) {
            r.append("  float4 gl_FragCoord : SV_POSITION;\n")
            vsOutputList.add(listOf("VS_Output", "float4", "gl_FragCoord"))
            r.append("  uint gl_InstanceID : SV_InstanceID;\n")
            vsOutputList.add(listOf("VS_Output", "uint", "gl_InstanceID"))
            // this property needs to be inverted (except if not flippedY)
            r.append("  bool gl_FrontFacing0 : SV_IsFrontFace;\n")
            r.append("#define gl_FrontFacing (gl_FrontFacing0 != (gl_FlippedY<0.0))\n")
            vsOutputList.add(listOf("VS_Output", "bool", "gl_FrontFacing0"))
        }
        r.append("};\n")

        // append all textures and samplers
        var numTextures = 0
        for (v in uniforms) {
            texMap[v.type] ?: continue
            numTextures++
        }
        p.texMap = IntArray(numTextures) // by default, everything is bound to slot 0
        var ti = 0
        for (v in uniforms) {
            val kw = texMap[v.type] ?: continue
            val name = v.name
            r.append(kw).append(' ')
                .append(name).append(" : register(t").append(ti).append(");\n")
            r.append("SamplerState ")
                .append(name).append("_s : register(s").append(ti).append(");\n")
            p.uniforms[name] = 1_000_000 + ti
            ti++
        }

        val isWritingFragDepth = isFragment && functions.any { "gl_FragDepth" in it }
        if (isFragment) {
            r.append("struct PS_Output {\n")
            for (j in layers.indices) {
                val v = layers[j]
                val type = typeMap[v.type]!!
                outputs.add(v.listDeclaration("PS_Output", type))
                v.appendDeclaration(type, r)
                r.append(" : COLOR").append(j)
                r.append(";\n")
            }
            // if shader uses gl_FragDepth=..., then add : DEPTH
            if (isWritingFragDepth) {
                outputs.add(listOf("PS_Output", "float4", "gl_FragDepth"))
                r.append("float4 gl_FragDepth : DEPTH;\n")
            }
            r.append("};\n")
        }

        for ((const, value) in constants) {
            r.append("static const ")
            const.appendDeclaration(typeMap[const.type]!!, r)
            r.append(" = ").append(value).append(";\n")
        }

        var oldMain: List<CharSequence>? = null
        var oldMainMod: List<CharSequence>? = null
        var newMain: List<CharSequence>? = null

        for (j in functions.indices) {
            val mainFunc = functions[j]
            if (mainFunc[0] == "void" &&
                mainFunc[1] == "main" &&
                mainFunc[2] == "("
            ) {

                val canDiscard = mainFunc.contains("discard")

                val newFunc = ArrayList<CharSequence>()
                when {
                    isVertex -> {
                        newFunc.add("VS_Output vs_main(VS_Input input")
                        if (pvi != 0) newFunc.add(", VS_InstInput instInput")
                        newFunc.add(") {\n")
                        newFunc.add("  VS_Output output = (VS_Output) 0;\n")
                    }

                    isFragment -> {
                        newFunc.add("PS_Output ps_main(VS_Output input) : SV_Target {\n")
                        newFunc.add("  PS_Output output = (PS_Output) 0;\n")
                    }

                    else -> throw NotImplementedError()
                }
                // add function parameters
                val extra = ArrayList<CharSequence>()
                if (canDiscard) {
                    newFunc.add("  if(main")
                    newFunc.add("(")
                    extra.add("bool")
                    extra.add("main")
                    extra.add("(")
                } else {
                    newFunc.add("  main")
                    newFunc.add("(")
                    extra.add("void")
                    extra.add("main")
                    extra.add("(")
                }
                var first = true
                fun next() {
                    if (first) first = false
                    else {
                        newFunc.add(",")
                        extra.add(",")
                    }
                }
                for (ji in inputs.indices) {
                    val v = inputs[ji]
                    next()
                    val normalInput = v[0] != "VS_InstInput"
                    newFunc.add(if (normalInput) "input" else "instInput")
                    newFunc.add(".")
                    newFunc.add(v.last())
                    extra.add("in")
                    extra.addAll(v.subList(1, v.size))
                }
                for (v in outputs) {
                    next()
                    newFunc.add("output")
                    newFunc.add(".")
                    newFunc.add(v.last())
                    extra.add("inout") // out would be enough, but sometimes values may be uninitialized -> initialize them using inout
                    extra.addAll(v.subList(1, v.size))
                }
                if (canDiscard) {
                    newFunc.add(")) discard;\n")
                } else {
                    newFunc.add(");\n")
                }
                if (isVertex) {
                    newFunc.add("output.gl_Position.y *= gl_FlippedY;\n")
                }
                newFunc.add(" return output;\n")
                newFunc.add("}")

                oldMain = mainFunc
                oldMainMod = extra + if (canDiscard) {
                    mainFunc.subList(3, mainFunc.size - 1).map {
                        when (it) {
                            "discard" -> "return true"
                            "return" -> "return false"
                            else -> it
                        }
                    } + listOf("return false", ";", "}")
                } else {
                    mainFunc.subList(3, mainFunc.size)
                }
                newMain = newFunc
                break
            }
        }

        for (func in functions) {
            if (func !== oldMain) {
                appendFunc(func, r)
            }
        }
        if (oldMainMod != null) appendFunc(oldMainMod, r)
        if (newMain != null) appendFunc(newMain, r)

        var str = r.trim().toString()
            .replace(" . ", ".")
            .replace("float2(", "makeFloat2(")
            .replace("float3(", "makeFloat3(")
            .replace("float4(", "makeFloat4(")
            .replace("texture(", "sampleTexture(")
            .replace("textureLod(", "sampleTextureLod(")

        str = "" +
                "float2 makeFloat2(float x) { return float2(x,x); }\n" +
                "float2 makeFloat2(float x, float y) { return float2(x,y); }\n" +
                "float2 makeFloat2(int2 x) { return float2(x.x,x.y); }\n" +
                "float3 makeFloat3(float x) { return float3(x,x,x); }\n" +
                "float3 makeFloat3(float x, float y, float z) { return float3(x,y,z); }\n" +
                "float3 makeFloat3(float x, float2 yz) { return float3(x,yz); }\n" +
                "float3 makeFloat3(float2 xy, float z) { return float3(xy,z); }\n" +
                "float4 makeFloat4(float x) { return float4(x,x,x,x); }\n" +
                "float4 makeFloat4(float2 xy, float z, float w) { return float4(xy,z,w); }\n" +
                "float4 makeFloat4(float x, float2 yz, float w) { return float4(x,yz,w); }\n" +
                "float4 makeFloat4(float x, float y, float2 zw) { return float4(x,y,zw); }\n" +
                "float4 makeFloat4(float2 xy, float2 zw) { return float4(xy,zw); }\n" +
                "float4 makeFloat4(float3 xyz, float w) { return float4(xyz,w); }\n" +
                "float4 makeFloat4(float x, float3 yzw) { return float4(x,yzw); }\n" +
                "float4 makeFloat4(float x, float y, float z, float w) { return float4(x,y,z,w); }\n" +
                "#define lessThan(a,b) ((a)<(b))\n" +
                "#define greaterThan(a,b) ((a)>(b))\n" +
                "#define sampleTexture(tex, uv) tex.Sample(tex##_s, uv)\n" +
                // "#define textureGather(tex, uv, loc) tex.Gather(tex##_s, uv, loc)\n" +
                "#define textureOffset(tex, uv, offset) tex.Sample(tex##_s, uv, offset)\n" +
                "#define sampleTextureLod(tex, uv, lod) tex.SampleLevel(tex##_s, uv, lod)\n" +
                "float4 texelFetch(Texture2D tex, int2 coords, int layer){\n" +
                "   return tex.Load(int3(coords,layer));\n" +
                "}\n" +
                "int2 textureSize(TextureCube tex, int lod) {\n" +
                "   uint sx,sy;\n" +
                "   tex.GetDimensions(sx,sy);\n" +
                "   return int2(sx,sy);\n" +
                "}\n" +
                "int2 textureSize(Texture2D tex, uint lod) {\n" +
                "   uint sx,sy;\n" +
                "   tex.GetDimensions(sx,sy);\n" +
                "   return int2(sx,sy);\n" +
                "}\n" +
                "int3 textureSize(Texture3D tex, uint lod) {\n" +
                "   uint sx,sy,sz;\n" +
                "   tex.GetDimensions(sx,sy,sz);\n" +
                "   return int3(sx,sy,sz);\n" +
                "}\n" +
                "uint floatBitsToUint(float v) { return asuint(v); }\n" +
                "uint2 floatBitsToUint(float2 v) { return uint2(asuint(v.x),asuint(v.y)); }\n" +
                "uint3 floatBitsToUint(float3 v) { return uint3(asuint(v.x),asuint(v.y),asuint(v.z)); }\n" +
                "uint4 floatBitsToUint(float4 v) { return uint4(asuint(v.x),asuint(v.y),asuint(v.z),asuint(v.w)); }\n" +
                "float uintBitsToFloat(uint v) { return asfloat(v); }\n" +
                "float2 uintBitsToFloat(uint2 v) { return float2(asfloat(v.x),asfloat(v.y)); }\n" +
                "float3 uintBitsToFloat(uint3 v) { return float3(asfloat(v.x),asfloat(v.y),asfloat(v.z)); }\n" +
                "float4 uintBitsToFloat(uint4 v) { return float4(asfloat(v.x),asfloat(v.y),asfloat(v.z),asfloat(v.w)); }\n" +
                "float atan(float x, float y){ return atan2(x,y); }\n" + // luckily works :)
                str

        if (debug) {
            // print str with line numbers
            println(str.split('\n')
                .withIndex().joinToString("\n") {
                    "${(it.index + 1).toString().padStart(4)}: ${it.value}"
                })
        }

        if (write) {
            val symbol = if (isVertex) "v" else if (isFragment) "f" else "c"
            val folder = OS.documents.getChild("IdeaProjects/JDirectX11/debug")
            if (!folder.exists) folder.tryMkdirs()
            val file = folder.getChild("${p.index}${if (pvi != 0) "-$pvi" else ""}.${symbol}s.hlsl")
            file.writeText(str)
        }

        return str
    }

    private fun appendFunc(func: List<CharSequence>, r: StringBuilder) {
        r.append('\n')
        for (f in func) {
            when (f) {
                "(", "[" -> {
                    if (r.endsWith(" ")) {
                        r.setLength(r.length - 1)
                    }
                    r.append(f)
                }

                ",", ")", "]" -> {
                    if (r.endsWith(" ")) {
                        r.setLength(r.length - 1)
                    }
                    r.append(f).append(' ')
                }

                "&", "|" -> {
                    if (r.endsWith("& ") || r.endsWith("| ")) {
                        r.setLength(r.length - 1)
                    }
                    r.append(f).append(' ')
                }

                "{" -> r.append(f).append("\n  ")
                ";" -> {
                    if (r.endsWith(" ")) {
                        r.setLength(r.length - 1)
                    }
                    r.append(f).append("\n  ")
                }

                "}" -> {
                    if (r.endsWith("  ")) {
                        r.setLength(r.length - 2)
                    }
                    r.append(f).append("\n  ")
                }

                "=", "+", "-", ">", "<" -> {
                    if (r.endsWith("> ") ||
                        r.endsWith("< ") ||
                        r.endsWith("! ") ||
                        r.endsWith("= ") ||
                        r.endsWith("- ") ||
                        r.endsWith("+ ") ||
                        r.endsWith("* ") ||
                        r.endsWith("/ ") ||
                        r.endsWith("% ")
                    ) r.setLength(r.length - 1)
                    r.append(f)
                }

                else -> {
                    if (f.startsWith("#")) {
                        r.append('\n').append(f).append('\n')
                    } else {
                        r.append(wordMap[f.toString()] ?: f).append(' ')
                    }
                }
            }
        }
    }

    private fun parse(tokens: ArrayList<CharSequence>) {
        var i = 0

        tokens.removeIf { it.startsWith("/*") || it.startsWith("//") }

        val tk0 = tokens[i]
        if (tk0.startsWith("#version")) i++

        val inList = when {
            isVertex -> attributes
            isFragment -> varyings
            else -> throw NotImplementedError()
        }

        val outList = when {
            isVertex -> varyings
            isFragment -> layers
            else -> throw NotImplementedError()
        }

        while (i < tokens.size) {
            fun readVariables(list: MutableList<Variable>) {
                val j = tokens.indexOf2(";", i, false) + 1
                var k = tokens.indexOf2(",", i, false) + 1
                if (k < j) {
                    list.add(parseVariable(tokens.subList(i, k - 1)))
                    val common = tokens.subList(i, k - 2)
                    i = k
                    k = tokens.indexOf2(",", i, false) + 1
                    while (k < j) {
                        list.add(parseVariable(common + tokens.subList(i, k - 1)))
                        i = k
                        k = tokens.indexOf2(",", i, false) + 1
                    }
                    list.add(parseVariable(common + tokens.subList(i, j - 1)))
                    i = j
                } else {
                    list.add(parseVariable(tokens.subList(i, j - 1)))
                    i = j
                }
            }
            when (val tk = tokens[i++]) {
                "precision" -> {
                    // value, type, semicolon
                    i = tokens.indexOf2(";", i, false) + 1
                }

                "in" -> readVariables(inList)
                "out" -> readVariables(outList)
                "uniform" -> readVariables(uniforms)
                "const" -> {
                    val j = tokens.indexOf2(";", i, false) + 1
                    val k = tokens.indexOf2("=", i, false)
                    if (k > j) throw IllegalStateException()
                    val variable = parseVariable(tokens.subList(i, k))
                    val value = tokens.subList(k + 1, j - 1).joinToString(" ") { wordMap[it] ?: it }
                    constants.add(variable to value)
                    i = j
                }

                "layout" -> {
                    // layout(std140, shared, binding = 0)
                    if (tokens[i++] != "(") throw IllegalStateException("Expected (")
                    while (tokens[i] != ")") {
                        i++
                    }
                    i++ // skip )
                    // todo use this data?
                }

                "flat" -> {
                    // todo implement this...
                }
                // list all types
                in typeMap -> {
                    val i0 = i - 1
                    // read global variable or function
                    val j = tokens.indexOf2(";", i, false)
                    val k = tokens.indexOf2("(", i, false)
                    if (j < k) {
                        // variable
                        variables.add(parseVariable(tokens.subList(i0, j)))
                        i = j + 1
                    } else {
                        // function
                        val p = tokens.indexOf2("{", i, false) + 1
                        val q = tokens.indexOf2(";", i, false) + 1
                        if (p < q) {
                            // normal function
                            i = p
                            var depth = 1
                            while (depth > 0) {
                                when (tokens[i++]) {
                                    "{" -> depth++
                                    "}" -> depth--
                                }
                            }
                            functions.add(tokens.subList(i0, i))
                        } else {
                            // just declared function
                            // -> not supported, and we have to sort the functions ourselves by their dependencies
                            i = q
                            // functions.add(tokens.subList(i0, i))
                        }
                    }
                }

                else -> throw NotImplementedError(tk.toString())
            }
        }

        // order functions by dependencies
        // for each function, find all dependencies
        val funcByName = functions.groupBy {
            it[it.indexOf("(") - 1]
        }

        val dependencies = funcByName.entries.associate { (name, functions) ->
            name to functions.map { list ->
                val startIndex = list.indexOf(")")
                list.withIndex().filter {
                    val c0 = it.value[0]
                    if (it.value != name && it.index > startIndex && (c0 in 'A'..'Z' || c0 in 'a'..'z') && it.index + 2 < list.size) {
                        list[it.index + 1] == "("
                    } else false
                }.map { it.value }
            }.flatten().toSet().toList() // toSet().toList() needed?
        }

        val funcOrder = funcByName.keys
            .sortedByTopology { name -> dependencies[name]!! }
            .withIndex()
            .associate { it.value to it.index }

        functions.sortBy {
            val name = it[it.indexOf("(") - 1]
            funcOrder[name]!!
        }

    }

    private fun tokenize(src: String): ArrayList<CharSequence> {
        var i = 0
        val tokens = ArrayList<CharSequence>()
        fun readNumber() {
            val j = i++
            while (src[i] in '0'..'9') i++
            if (src[i] in "xX") {
                i++
                while (src[i] in "0123456789ABCDEFabcdef") i++
            }
            if (src[i] == '.') {
                i++
                while (src[i] in '0'..'9') i++
            }
            if (src[i] in "eE") {
                i++
                if (src[i] in "+-") i++
                while (src[i] in '0'..'9') i++
            }
            if (src[i] in "uU") {
                i++
            }
            tokens.add(src.subSequence(j, i))
        }
        while (i < src.length) {
            when (src[i]) {
                in " \t\r\n" -> i++
                in 'A'..'Z', in 'a'..'z' -> {
                    val j = i++
                    while (i < src.length) {
                        when (src[i++]) {
                            in 'A'..'Z', in 'a'..'z', in '0'..'9', in "_" -> {
                                // fine
                            }

                            else -> break
                        }
                    }
                    tokens.add(src.subSequence(j, --i))
                }

                '+', '-' -> {
                    if (src[i + 1] in '0'..'9' || src[i + 1] == '.') {
                        readNumber()
                    } else tokens.add(src.subSequence(i, ++i))
                }

                in '0'..'9' -> readNumber()
                '#' -> {
                    // skip until linebreak
                    val idx = src.indexOf2('\n', i)
                    tokens.add(src.subSequence(i, idx))
                    i = idx + 1
                }

                '/' -> {
                    if (src[i + 1] == '/') {
                        val idx = src.indexOf2('\n', i + 2)
                        tokens.add(src.subSequence(i, idx))
                        i = idx + 1
                    } else if (src[i + 1] == '*') {
                        val idx = src.indexOf2("*/", i + 2)
                        tokens.add(src.subSequence(i, idx))
                        i = idx + 1
                    } else {
                        tokens.add(src.subSequence(i, ++i))
                    }
                }

                else -> tokens.add(src.subSequence(i, ++i))
            }
        }
        return tokens
    }

}