/*
 * Program: Gray Level Reduction
 * Description: Reduces the number of gray levels in an image from 256 to 2,
 *              in integer powers of 2.
 * Input: BMP image file and desired number of gray levels (2, 4, 8, 16, 32, 64, 128, or 256)
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#pragma pack(push, 1)
typedef struct {
    unsigned short type;
    unsigned int size;
    unsigned short reserved1;
    unsigned short reserved2;
    unsigned int offset;
} BMPFileHeader;

typedef struct {
    unsigned int size;
    int width;
    int height;
    unsigned short planes;
    unsigned short bitsPerPixel;
    unsigned int compression;
    unsigned int imageSize;
    int xPixelsPerMeter;
    int yPixelsPerMeter;
    unsigned int colorsUsed;
    unsigned int colorsImportant;
} BMPInfoHeader;
#pragma pack(pop)

// Check if a number is a power of 2
int isPowerOfTwo(int n) {
    return (n > 0) && ((n & (n - 1)) == 0);
}

// Reduce gray levels
unsigned char reduceGrayLevel(unsigned char pixel, int numLevels) {
    // Calculate the step size (256 / numLevels)
    int step = 256 / numLevels;

    // Quantize the pixel value
    // new_value = (pixel / step) * step + step/2 - 0.5
    // Simplified: new_value = (int)(pixel / step) * step + (step - 1) / 2
    int quantized = (pixel / step) * step + (step - 1) / 2;

    // Ensure the value is within [0, 255]
    if (quantized > 255) quantized = 255;

    return (unsigned char)quantized;
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        printf("Usage: %s <input.bmp> <output.bmp> <num_gray_levels>\n", argv[0]);
        printf("num_gray_levels must be a power of 2 (2, 4, 8, 16, 32, 64, 128, or 256)\n");
        return 1;
    }

    int numLevels = atoi(argv[3]);

    // Validate that numLevels is a power of 2 and between 2 and 256
    if (!isPowerOfTwo(numLevels) || numLevels < 2 || numLevels > 256) {
        printf("Error: Number of gray levels must be a power of 2 between 2 and 256.\n");
        printf("Valid values: 2, 4, 8, 16, 32, 64, 128, 256\n");
        return 1;
    }

    // Open input file
    FILE *inputFile = fopen(argv[1], "rb");
    if (inputFile == NULL) {
        printf("Error: Cannot open input file %s\n", argv[1]);
        return 1;
    }

    // Read BMP file header
    BMPFileHeader fileHeader;
    if (fread(&fileHeader, sizeof(BMPFileHeader), 1, inputFile) != 1) {
        printf("Error: Cannot read BMP file header\n");
        fclose(inputFile);
        return 1;
    }

    // Check if it's a BMP file
    if (fileHeader.type != 0x4D42) {
        printf("Error: Not a valid BMP file\n");
        fclose(inputFile);
        return 1;
    }

    // Read BMP info header
    BMPInfoHeader infoHeader;
    if (fread(&infoHeader, sizeof(BMPInfoHeader), 1, inputFile) != 1) {
        printf("Error: Cannot read BMP info header\n");
        fclose(inputFile);
        return 1;
    }

    // Check if it's an 8-bit grayscale or 24-bit image
    if (infoHeader.bitsPerPixel != 8 && infoHeader.bitsPerPixel != 24) {
        printf("Error: Only 8-bit grayscale or 24-bit BMP files are supported\n");
        fclose(inputFile);
        return 1;
    }

    printf("Image Information:\n");
    printf("  Width: %d pixels\n", infoHeader.width);
    printf("  Height: %d pixels\n", infoHeader.height);
    printf("  Bits per pixel: %d\n", infoHeader.bitsPerPixel);
    printf("  Original gray levels: 256\n");
    printf("  Target gray levels: %d\n\n", numLevels);

    // Open output file
    FILE *outputFile = fopen(argv[2], "wb");
    if (outputFile == NULL) {
        printf("Error: Cannot create output file %s\n", argv[2]);
        fclose(inputFile);
        return 1;
    }

    // Write file header and info header to output file
    fwrite(&fileHeader, sizeof(BMPFileHeader), 1, outputFile);
    fwrite(&infoHeader, sizeof(BMPInfoHeader), 1, outputFile);

    // If 8-bit image, copy the color palette
    if (infoHeader.bitsPerPixel == 8) {
        // Read and write color palette (256 colors * 4 bytes each)
        unsigned char palette[1024];
        if (fread(palette, 1, 1024, inputFile) != 1024) {
            printf("Error reading color palette\n");
            fclose(inputFile);
            fclose(outputFile);
            return 1;
        }
        fwrite(palette, 1, 1024, outputFile);
    }

    // Calculate row size (BMP rows are padded to 4-byte boundaries)
    int rowSize;
    if (infoHeader.bitsPerPixel == 8) {
        rowSize = ((infoHeader.width + 3) / 4) * 4;
    } else {
        rowSize = ((infoHeader.width * 3 + 3) / 4) * 4;
    }

    // Allocate memory for one row
    unsigned char *row = (unsigned char *)malloc(rowSize);

    // Process each row
    for (int y = 0; y < abs(infoHeader.height); y++) {
        if (fread(row, 1, rowSize, inputFile) != rowSize) {
            printf("Error reading image data\n");
            free(row);
            fclose(inputFile);
            fclose(outputFile);
            return 1;
        }

        // Process each pixel in the row
        if (infoHeader.bitsPerPixel == 8) {
            // 8-bit grayscale
            for (int x = 0; x < infoHeader.width; x++) {
                row[x] = reduceGrayLevel(row[x], numLevels);
            }
        } else {
            // 24-bit color (convert to grayscale first, then reduce)
            for (int x = 0; x < infoHeader.width; x++) {
                int idx = x * 3;
                // Convert to grayscale using luminance formula
                unsigned char gray = (unsigned char)(0.299 * row[idx+2] +
                                                      0.587 * row[idx+1] +
                                                      0.114 * row[idx]);
                // Reduce gray level
                unsigned char newGray = reduceGrayLevel(gray, numLevels);
                // Set all channels to the same value
                row[idx] = newGray;     // Blue
                row[idx+1] = newGray;   // Green
                row[idx+2] = newGray;   // Red
            }
        }

        // Write the processed row to output file
        fwrite(row, 1, rowSize, outputFile);
    }

    // Clean up
    free(row);
    fclose(inputFile);
    fclose(outputFile);

    printf("Successfully reduced gray levels from 256 to %d\n", numLevels);
    printf("Output saved to: %s\n", argv[2]);

    return 0;
}
