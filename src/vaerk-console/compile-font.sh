#!/bin/bash

# based on https://github.com/AsahiLinux/m1n1/blob/main/font/makefont.sh
# Copyright The Asahi Linux Contributors

width=$1
height=$2
size=$3
fontfile=$4
outfile=$5
shift 5

baseline=${BASELINE:-$((height * 3 / 4))}

tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT

: > "$outfile"
for ord in $(seq 32 126); do
    printf "\\x$(printf %x $ord)" > "$tmp/c"
    magick \
        -size ${width}x${height} xc:black \
        -fill white \
        -antialias \
        -font "$fontfile" \
        -density 72 \
        -pointsize $size \
        "$@" \
        -annotate +0+$baseline "@$tmp/c" \
        -depth 8 \
        gray:- >> "$outfile"
done

od -An -v -tx1 "$outfile" | sed -E 's/ ([0-9a-f]{2})/0x\1,/g' > "${outfile%.bin}.inc"