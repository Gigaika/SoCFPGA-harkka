//
// Created by diggoo on 4/28/20.
//

#ifndef CLASSIFIER_CLASSIFIER_H
#define CLASSIFIER_CLASSIFIER_H

#include "stdint.h"

#define INFO_REG_COUNT 3
#define VALUES_IN_USE_REG_INDEX 1
#define COMP_REG_COUNT_REG_INDEX 3
#define ELEMENT_SIZE_BYTES_REG_INDEX 2

#define CONFIG_REG_WIDTH_BYTES 4
#define OUTPUT_SIZE_BYTES 1

#define DATA_LITTLE_ENDIAN 0
#define DATA_BIG_ENDIAN 1

typedef struct dma_struct {
    int fd;
    struct dma_proxy_channel_interface *proxyInterface;
} dma_object;

typedef struct hw_config {
    int data_element_size_bytes;
    int values_used_per_pixel;
    int num_comparision_registers;
    uint32_t *confregs;
} hw_object;

#endif //CLASSIFIER_CLASSIFIER_H
