package org.lwjgl.opengl.custom

data class Variable(val type: String, val name: String, val dimensions: IntArray) {

    override fun equals(other: Any?): Boolean {
        if (this === other) return true
        if (javaClass != other?.javaClass) return false

        other as Variable

        if (type != other.type) return false
        if (name != other.name) return false
        return dimensions.contentEquals(other.dimensions)
    }

    override fun hashCode(): Int {
        var result = type.hashCode()
        result = 31 * result + name.hashCode()
        result = 31 * result + dimensions.contentHashCode()
        return result
    }

    fun appendDeclaration(type: String, r: StringBuilder): Int {
        r.append("  ").append(type).append(' ').append(name)
        var numElements = 1
        for (ai in dimensions) {
            r.append('[').append(ai).append(']')
            numElements *= ai
        }
        return numElements
    }

    fun listDeclaration(type0: String, type: String): List<CharSequence> {
        return listOf(type0, type, name) + dimensions.map { "[$it]" }
    }

}