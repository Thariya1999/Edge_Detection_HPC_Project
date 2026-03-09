#!/bin/bash

# Usage: ./convert_photo.sh "path/to/photo.jpg"

if [ $# -eq 0 ]; then
    echo "=============================="
    echo "  Photo Converter for OpenMP  "
    echo "=============================="
    echo ""
    echo "USAGE:"
    echo "  ./convert_photo.sh photo.jpg"
    echo "  ./convert_photo.sh /mnt/c/Users/lahir/Downloads/photo.jpg"
    echo ""
    echo "PHOTOS IN images/ FOLDER:"
    ls images/*.pgm 2>/dev/null || echo "  No photos yet"
    exit 0
fi

# Create images folder
mkdir -p images

INPUT="$1"
FILENAME=$(basename "$INPUT")
NAME="${FILENAME%.*}"
OUTPUT="images/${NAME}.pgm"

echo ""
echo "Converting: $INPUT"
echo "Output:     $OUTPUT"

convert "$INPUT" -colorspace Gray -resize 512x512 "$OUTPUT"

if [ $? -eq 0 ]; then
    echo ""
    echo "✅ Success! Now run:"
    echo "   ./omp_edge $OUTPUT"
else
    echo ""
    echo "❌ Failed! Check if imagemagick is installed:"
    echo "   sudo apt install imagemagick"
fi
