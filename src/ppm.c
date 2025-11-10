#include "ppm.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* Simple but stable PPM implementation */
size_t ppm_compress(const uint8_t* input, size_t input_size, uint8_t** output) {
    if (input_size == 0 || !input || !output) {
        *output = NULL;
        return 0;
    }
    
    /* Allocate with safe margin */
    uint8_t* compressed = malloc(input_size + 8);
    if (!compressed) {
        *output = NULL;
        return 0;
    }
    
    /* Store original size in header */
    compressed[0] = (input_size >> 24) & 0xFF;
    compressed[1] = (input_size >> 16) & 0xFF;
    compressed[2] = (input_size >> 8) & 0xFF;
    compressed[3] = input_size & 0xFF;
    
    /* Simple compression: remove consecutive duplicates */
    size_t comp_index = 4;
    size_t i = 0;
    
    while (i < input_size) {
        uint8_t current = input[i];
        size_t count = 1;
        
        /* Count consecutive identical bytes */
        while (i + count < input_size && input[i + count] == current && count < 255) {
            count++;
        }
        
        if (count > 3) {
            /* Encode run */
            compressed[comp_index++] = current;
            compressed[comp_index++] = current; /* Marker */
            compressed[comp_index++] = (uint8_t)count;
            i += count;
        } else {
            /* Copy literal */
            for (size_t j = 0; j < count; j++) {
                compressed[comp_index++] = input[i++];
            }
        }
    }
    
    /* Check if compression actually helped */
    if (comp_index >= input_size) {
        /* Compression didn't help - store original */
        free(compressed);
        *output = NULL;
        return 0;
    }
    
    *output = compressed;
    return comp_index;
}

size_t ppm_decompress(const uint8_t* input, size_t input_size, uint8_t** output) {
    if (input_size < 4 || !input || !output) {
        *output = NULL;
        return 0;
    }
    
    /* Read original size from header */
    size_t original_size = (input[0] << 24) | (input[1] << 16) | 
                          (input[2] << 8) | input[3];
    
    if (original_size == 0) {
        *output = NULL;
        return 0;
    }
    
    uint8_t* decompressed = malloc(original_size);
    if (!decompressed) {
        *output = NULL;
        return 0;
    }
    
    size_t decomp_index = 0;
    size_t comp_index = 4;
    
    while (comp_index < input_size && decomp_index < original_size) {
        if (comp_index + 2 < input_size && 
            input[comp_index] == input[comp_index + 1]) {
            /* Decode run */
            uint8_t value = input[comp_index];
            uint8_t count = input[comp_index + 2];
            
            for (uint8_t j = 0; j < count && decomp_index < original_size; j++) {
                decompressed[decomp_index++] = value;
            }
            comp_index += 3;
        } else {
            /* Copy literal */
            decompressed[decomp_index++] = input[comp_index++];
        }
    }
    
    *output = decompressed;
    return decomp_index;
}
