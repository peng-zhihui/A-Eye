#!/usr/bin/python

import sys
from PIL import Image

if len(sys.argv) != 2:
    print "Usage: convert.py <file>"
    sys.exit(1)

print "Reading: " + sys.argv[1] + ", save to src/icon_generate.h"
im = Image.open(sys.argv[1])

if im.mode == "RGB":
    pixelSize = 3
elif im.mode == "RGBA":
    pixelSize = 4
else:
    sys.exit('not supported pixel mode: "%s"' % (im.mode))

out = open("src/icon_generate.h", "wb")

pixels = im.tobytes()
pixelsArr = []
for i in range(1, len(pixels), pixelSize):
    pixelsVal = '{:02X}'.format(ord(pixels[i + 1]) >> 3 | (ord(pixels[i]) << 3 & 0xe0))
    pixelsVal += '{:02X}'.format(ord(pixels[i-1]) & 0xf8 | (ord(pixels[i]) >> 5 & 0x07))
    pixelsArr.append(pixelsVal)

pixelsArrMerge = []
for i in range(0, len(pixelsArr), 2):
    pixelsArrMerge.append("0x" + pixelsArr[i] + pixelsArr[i + 1] + ",")
    if (i % 16) == 14:
        pixelsArrMerge.append("\n\t")
    else:
        pixelsArrMerge.append(" ")

out.write("#ifndef _ICON_GENERATE_H_\n")
out.write("#define _ICON_GENERATE_H_\n")

out.write("#include <stdint.h>\n")

out.write("#define IMAGE_WIDTH "+str(im.width)+"\n")
out.write("#define IMAGE_HEIGHT "+str(im.height)+"\n")

out.write("uint32_t rgb_image[] __attribute__((aligned(64))) = {\n")
out.write("\t"+"".join(pixelsArrMerge))

out.write("\n};\n")
out.write("#endif // _ICON_GENERATE_H_\n")


out.close()
