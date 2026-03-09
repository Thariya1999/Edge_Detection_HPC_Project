#!/bin/bash

echo "╔══════════════════════════════════════════╗"
echo "║        Bulk Photo Converter              ║"
echo "╚══════════════════════════════════════════╝"

if [ $# -eq 0 ]; then
    echo ""
    echo "USAGE:"
    echo "  ./convert_bulk.sh /mnt/c/Users/ACER/Downloads/myfolder/"
    echo ""
    echo "CURRENT IMAGES IN images/ FOLDER:"
    ls images/*.pgm 2>/dev/null || echo "  No images yet"
    exit 0
fi

FOLDER="$1"

# Check folder exists
if [ ! -d "$FOLDER" ]; then
    echo "ERROR: Folder not found: $FOLDER"
    exit 1
fi

mkdir -p images

echo ""
echo "Source folder: $FOLDER"
echo "Converting all JPG/PNG/JPEG images..."
echo ""

COUNT=0
FAIL=0

# Loop through all image files in folder
for IMG in "$FOLDER"*.jpg "$FOLDER"*.jpeg "$FOLDER"*.png \
           "$FOLDER"*.JPG "$FOLDER"*.JPEG "$FOLDER"*.PNG; do

    # Skip if no files match
    [ -f "$IMG" ] || continue

    # Get clean filename
    FILENAME=$(basename "$IMG")
    NAME="${FILENAME%.*}"

    # Remove spaces from name
    CLEAN_NAME=$(echo "$NAME" | tr ' ' '_')
    OUTPUT="images/${CLEAN_NAME}.pgm"

    echo -n "  Converting: $FILENAME → $CLEAN_NAME.pgm ... "

    convert "$IMG" -colorspace Gray -resize 512x512 "$OUTPUT" 2>/dev/null

    if [ $? -eq 0 ]; then
        echo "OK"
        COUNT=$((COUNT + 1))
    else
        echo "FAILED"
        FAIL=$((FAIL + 1))
    fi
done

echo ""
echo "=============================="
echo "  Done! Converted: $COUNT images"
if [ $FAIL -gt 0 ]; then
    echo "  Failed:    $FAIL images"
fi
echo "=============================="
echo ""
echo "Now run:"
echo "  ./omp_edge images/"
