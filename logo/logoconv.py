#!/usr/bin/env python3
"""
Convert the GIPS logo into a compressed representation.

Two methods are implemented here:

CompressBlockMap - split the image into 4x4 or 8x8 pixel blocks and try to
                   create a tilemapped representation, consisting of a number
                   of tile sprites and a "nametable" that references those
                   sprites, with optional vertical and/or horizontal flip

CompressRLE - encode runs of perfectly white or black pixels (as 'A'...'Z'
              and 'a'...'z', respectively), and encode all other values directly
              (as '0'...'9')

Only CompressRLE is used, as this is the much simpler and also more efficient
algorithm in this case.
"""
from PIL import Image
import sys
import os

################################################################################

def GetImageSize(img):
    return (len(img[0]), len(img))

def QuantizeImage(img, levels):
    return [[(x * (levels - 1) + 127) // 255 for x in row] for row in img]

def DequantizeImage(img, levels):
    radj = (levels - 1) >> 1
    return [[(x * 255 + radj) // (levels - 1) for x in row] for row in img]

def ConvertImage(img):
    a = bytearray()
    for row in img: a += bytearray(row)
    return Image.frombytes('L', GetImageSize(img), bytes(a))

################################################################################

def _hflip(block, size):
    return tuple(block[y*size + size-1-x] for y in range(size) for x in range(size))
def _vflip(block, size):
    return tuple(block[(size-1-y)*size + x] for y in range(size) for x in range(size))

def CompressBlockMap(img, blocksize, diffthresh=0):
    w, h = GetImageSize(img)
    assert not(w % blocksize)
    assert not(h % blocksize)

    # search for identical blocks (with reflections)
    block2key = {}
    key2block = {}
    nametable = []
    nblocks = 0
    for y in range(0, h, blocksize):
        for x in range(0, w, blocksize):
            block = []
            for i in range(blocksize): block.extend(img[y+i][x:x+blocksize])
            block = tuple(block)
            try:
                nametable.append(block2key[block])
                continue
            except KeyError:
                pass
            nametable.append((nblocks, 0))
            block2 = _vflip(block,  blocksize)
            block1 = _hflip(block,  blocksize)
            block3 = _hflip(block2, blocksize)
            block2key[block3] = (nblocks, 3);  key2block[(nblocks, 3)] = block3
            block2key[block2] = (nblocks, 2);  key2block[(nblocks, 2)] = block2
            block2key[block1] = (nblocks, 1);  key2block[(nblocks, 1)] = block1
            block2key[block]  = (nblocks, 0);  key2block[(nblocks, 0)] = block
            nblocks += 1
    print(f"blocksize {blocksize},  pre-merge: {len(nametable)} blocks, {len(set(nametable))} unique, {nblocks} bitmaps, {nblocks * blocksize*blocksize} pixels")

    # merge similar blocks
    remap = {}
    for na in range(0, nblocks):
        if na in remap: continue
        ba = key2block[(na, 0)]
        for nb in range(na+1, nblocks):
            if nb in remap: continue
            for r in range(4):
                bb = key2block[(nb, r)]
                diff = sum((xa-xb)*(xa-xb) for xa,xb in zip(ba,bb))
                if diff <= diffthresh:
                    remap[nb] = (na, r)
                    break

    # apply remapping
    nametable = [(n, r1^r2) for (n,r1),r2 in ((remap.get(n,(n,0)),r) for n,r in nametable)]
    nblocks -= len(remap)
    print(f"blocksize {blocksize}, post-merge: {len(nametable)} blocks, {len(set(nametable))} unique, {nblocks} bitmaps, {nblocks * blocksize*blocksize} pixels")

    # create additional mapping to remove holes
    defined = {n for n,r in key2block}
    used = {n for n,r in nametable}
    unused = defined - used
    assert len(unused) == len(remap)
    remap = {}
    for new, old in enumerate(sorted(used)):
        remap[old] = new

    # apply remapping
    nametable = [(remap[n],r) for n,r in nametable]
    key2block = {(remap[n],r):b for (n,r),b in key2block.items() if n in remap}
    nblocks = len(remap)

    defbytes = nblocks * blocksize*blocksize + len(nametable) + sum((r>0) for n,r in nametable)
    print(f"blocksize {blocksize},      final: {defbytes} defbytes")

    # re-synthesize image
    img = []
    stride = w // blocksize
    for by in range(h // blocksize):
        for sy in range(blocksize):
            row = []
            for bx in range(stride):
                row.extend(key2block[nametable[by * stride + bx]][sy*blocksize : (sy+1)*blocksize])
            img.append(row)
    return img

################################################################################

def CompressRLE(img, runlimit=0):
    runlevs = {0, max(map(max, img))}
    data = []
    for row in img:
        last = -1
        run = 0
        for curr in row + [-1]:
            if (curr == last) and (curr in runlevs) and (not(runlimit) or (run < runlimit)):
                run += 1
                continue
            if run:
                data.append((last, run))
            last = curr
            run = 1
    print(f"RLE, runlimit {runlimit}: {max(r for v,r in data)} maxrun, {len(data)} defbytes")

    # re-synthesize image
    diter = data[::-1]
    w = len(img[0])
    img = []
    while diter:
        row = []
        while len(row) < w:
            v,r = diter.pop()
            row.extend([v] * r)
        img.append(row)
    return img, b''.join(
        (run * bytes([lev + 47]))
        if not(lev in runlevs)
        else bytes([run + (96 if lev else 64)])
        for lev, run in data)

################################################################################

if __name__ == "__main__":
    # load image and convert to 2D integer array
    os.chdir(os.path.dirname(sys.argv[0]))
    img = Image.open("gips_logo.png").convert('L')
    w, h = img.size
    img = list(img.tobytes())
    img = [img[o : o+w] for o in range(0, len(img), w)]

    img = [row[4:] for row in img]  # cut away the 4 unused rows on the left
    levels = 12
    img = QuantizeImage(img, levels)
    #img2 = CompressBlockMap(img, 4, 3)
    #img2 = CompressBlockMap([[0]*4 + row for row in img], 8, 2)
    img2, data = CompressRLE(img, 26)

    with open("../src/gips_logo.h", "w") as f:
        f.write( "// this file was auto-generated by logoconv.py; do not edit\n"
                f"static constexpr int LogoWidth = {len(img[0])};\n"
                f"static constexpr int LogoHeight = {len(img)};\n"
                f"static const char* LogoData =  // {len(data)} bytes of compressed data")
        W = 76
        for o in range(0, len(data), W):
            f.write("\n\"" + data[o:o+W].decode() + "\"")
        f.write(";\n")

    if 0:
        img2 = DequantizeImage(img2, levels)
        ConvertImage(img2).show()
