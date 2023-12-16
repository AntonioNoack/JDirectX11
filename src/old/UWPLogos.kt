package old

import me.anno.image.Image
import me.anno.image.ImageCache
import me.anno.image.raw.IntImage
import me.anno.io.files.FileReference
import me.anno.utils.LOGGER
import java.io.IOException
import kotlin.math.max
import kotlin.math.min

fun buildLogos(src: FileReference, dst: FileReference) {
    val srcImage0 = ImageCache[src, false] ?: throw IOException("Could not load $src as image")
    val maxScale = 2048 / max(srcImage0.width, srcImage0.height)
    val srcImage1 = srcImage0.scaleUp(maxScale, maxScale)
    for (file in dst.listChildren()!!) {
        buildLogo(srcImage1, file)
    }
}

fun buildLogo(srcImage: Image, dst: FileReference) {
    val dstImage = ImageCache[dst, false] ?: return
    val newSize = min(dstImage.width, dstImage.height)
    var newImage = srcImage.resized(newSize, newSize, false)
    if (newSize != dstImage.width) {
        val newImage2 = IntImage(dstImage.width, dstImage.height, true)
        // add left/right border
        val dw = dstImage.width - newSize
        val dx0 = dw / 2
        val dx1 = dw - dx0
        var i0 = 0
        for (y in 0 until dstImage.height) {
            newImage2.data.fill(newImage.getRGB(0, y), i0, i0 + dx0)
            i0 += dx0
            for (x in 0 until newImage.width) {
                newImage2.data[i0++] = newImage.getRGB(x, y)
            }
            newImage2.data.fill(newImage.getRGB(newImage.width - 1, y), i0, i0 + dx1)
            i0 += dx1
        }
        newImage = newImage2
    }
    if (newImage.width != dstImage.width || newImage.height != dstImage.height) {
        LOGGER.warn("Incorrect sizes! $dstImage x $srcImage -> $newImage")
    } else {
        newImage.write(dst)
    }
}
