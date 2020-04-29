#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include "stdint.h"
#include "memory.h"
#include "devices.h"
#include "dma-proxy.h"

static int config_read = 0;

// Opens and memory maps the dma proxy character devices
void open_dma_devices(dma_object *tx, dma_object *rx) {
    // Attempt to create file descriptors for the character devices
    tx->fd = open("/dev/dma_proxy_tx", O_RDWR);
    if (tx->fd < 1) {
        printf("Unable to open DMA proxy device file, make sure /dev/dma_proxy_tx exists\n");
        exit(EXIT_FAILURE);
    }
    rx->fd = open("/dev/dma_proxy_rx", O_RDWR);
    if (rx->fd < 1) {
        printf("Unable to open DMA proxy device file, make sure /dev/dma_proxy_rx exists\n");
        exit(EXIT_FAILURE);
    }

    // Map the transmit and receive channels memory into user space so it's accessible
    tx->proxyInterface = (struct dma_proxy_channel_interface *)mmap(NULL, sizeof(struct dma_proxy_channel_interface),
                                                                      PROT_READ | PROT_WRITE, MAP_SHARED, tx->fd, 0);

    rx->proxyInterface = (struct dma_proxy_channel_interface *)mmap(NULL, sizeof(struct dma_proxy_channel_interface),
                                                                      PROT_READ | PROT_WRITE, MAP_SHARED, rx->fd, 0);
    // Ensure that the memory maps succeeded
    if ((tx->proxyInterface == MAP_FAILED) || (rx->proxyInterface == MAP_FAILED)) {
        printf("Failed to mmap\n");
        exit(EXIT_FAILURE);
    }
}

// Closes and unmaps the dma proxy character devices
void close_dma_devices(dma_object *tx, dma_object *rx) {
    munmap(tx->proxyInterface, sizeof(struct dma_proxy_channel_interface));
    munmap(rx->proxyInterface, sizeof(struct dma_proxy_channel_interface));
    close(tx->fd);
    close(rx->fd);
}

// Memory maps the configuration registers residing in the PL
void map_config_registers(int *fd, uint32_t **ptr, int n_cr) {
    // Attempt to create a file descriptor for the uio device
    *fd = open("/dev/uio0", O_RDWR);
    if (*fd < 1) {
        printf("Unable to open config register UIO device, make sure /dev/uio0 exists\n");
        exit(EXIT_FAILURE);
    }

    int reg_count = config_read == 0 ? 1 : n_cr;

    // Map the registers to the register array
    *ptr = (uint32_t *)mmap(NULL, (reg_count + INFO_REG_COUNT)*CONFIG_REG_WIDTH_BYTES, PROT_READ | PROT_WRITE, MAP_SHARED, *fd, 0);

    // Ensure that the memory maps succeeded
    if (*ptr == MAP_FAILED) {
        printf("Failed to mmap config registers\n");
        exit(EXIT_FAILURE);
    }
}

// Closes and unmaps the config uio device
void unmap_config_registers(int *fd, uint32_t **ptr, int n_cr) {
    int reg_count = config_read == 0 ? 1 : n_cr;
    munmap(*ptr, (reg_count + INFO_REG_COUNT)*CONFIG_REG_WIDTH_BYTES);
    close(*fd);
    config_read = 1;
}