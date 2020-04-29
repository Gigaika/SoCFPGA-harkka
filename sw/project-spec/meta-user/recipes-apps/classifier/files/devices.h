#ifndef CLASSIFIER_DEVICES_H
#define CLASSIFIER_DEVICES_H

#include "classifier.h"

void open_dma_devices(dma_object *tx, dma_object *rx);
void close_dma_devices(dma_object *tx, dma_object *rx);
void map_config_registers(int *fd, uint32_t **ptr, int n_cr);
void unmap_config_registers(int *fd, uint32_t **ptr, int n_cr);

#endif //CLASSIFIER_DEVICES_H
