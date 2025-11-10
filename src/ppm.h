#ifndef PPM_H
#define PPM_H

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

/* PPM context structure */
typedef struct PPMNode {
    uint8_t symbol;
    uint32_t count;
    struct PPMNode* next;
} PPMNode;

/* PPM model structure */
typedef struct {
    PPMNode** contexts;
    int order;
    size_t memory_limit;
} PPMModel;

/* Function declarations */
size_t ppm_compress(const uint8_t* input, size_t input_size, uint8_t** output);
size_t ppm_decompress(const uint8_t* input, size_t input_size, uint8_t** output);
PPMModel* ppm_create_model(int order, size_t memory_limit);
void ppm_free_model(PPMModel* model);

#endif
