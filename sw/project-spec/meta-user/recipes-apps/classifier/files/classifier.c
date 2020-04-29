#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <pthread.h>
#include "dma-proxy.h"
#include <string.h>
#include "stdint.h"
#include "memory.h"
#include <endian.h>
#include "devices.h"
#include "interface.h"
#include <byteswap.h>

static dma_object tx_dma, rx_dma;
static int rx_size, tx_size;
static int num_pixels;
static int config_reg_fd;

static int data_endianness = DATA_BIG_ENDIAN;
static int num_values_in_pixel;
hw_object hardware_desc;

// profiling variables
static clock_t total_start = 0;
static clock_t total_end = 0;

// Thread which is responsible for initiating the DMA tx
static pthread_t tid_tx;
void *tx_thread(void *arg)
{
    int dummy;

    // Perform the DMA transfer and the check the status after it completes
    // as the call blocks til the transfer is done.
    ioctl(tx_dma.fd, 0, &dummy);

    if (tx_dma.proxyInterface->status != PROXY_NO_ERROR) {
        printf("Proxy tx transfer error (code %d)\n", tx_dma.proxyInterface->status);
    }
}

// Function that receives data from DMA, blocks until the DMA transaction is complete
void receive() {
    int dummy;

    // Perform the DMA transfer and the check the status after it completes
    // as the call blocks til the transfer is done.
    ioctl(rx_dma.fd, 0, &dummy);

    if (rx_dma.proxyInterface->status != PROXY_NO_ERROR) {
        printf("Proxy rx transfer error (code %d)\n", rx_dma.proxyInterface->status);
    }
}

// Opens the specified data file, and derives the transmission size parameters from it
void open_data_file(FILE **datafile, char *path) {
    // Attempt to open the data file
    *datafile = fopen(path, "rb");
    if (!*datafile) {
        printf("Opening file (%s) failed\n", path);
        exit(EXIT_FAILURE);
    }

    // Get the size of the file
    fseek(*datafile, 0L, SEEK_END);
    uint32_t fsize = ftell(*datafile);
    rewind(*datafile);

    printf("File size: %d bytes\n", fsize);
    num_pixels = fsize/(num_values_in_pixel*hardware_desc.data_element_size_bytes);
    printf("Number of pixels in file: %d\n", num_pixels);
    tx_size = num_pixels * hardware_desc.data_element_size_bytes *hardware_desc.values_used_per_pixel;
    rx_size = num_pixels * OUTPUT_SIZE_BYTES;
    printf("tx size: %d bytes, rx size: %d bytes\n", tx_size, rx_size);
    printf("\n");

    if (tx_size < 1 || rx_size < 1) {
        printf("The file size/format is incorrect\n");
        exit(EXIT_FAILURE);
    }

    // Make sure all of the data to be sent can be fit in to the pre-allocated DMA buffer
    if (tx_size > TEST_SIZE) {
        printf("The file size is bigger then what is supported by the DMA setup\n");
        exit(EXIT_FAILURE);
    }
}

int main(int argc, char *argv[])
{
    FILE *fp_source;
    char *filepath;
    int *datapoint_indexes;
    int *comparision_values;

    if (argc == 2 && strcmp(argv[1], "--help") == 0) {
        print_usage();
        exit(EXIT_FAILURE);
    }
    if (argc < 5) {
        printf("Invalid arguments, use classifier --help for usage info\n");
        exit(EXIT_FAILURE);
    }

    // Memory maps the configuration register for the decision trees
    map_config_registers(&config_reg_fd, &hardware_desc.confregs, hardware_desc.num_comparision_registers);
    // Read the current hardware configuration
    read_hardware_config(&hardware_desc);

    // Allocate memory for the selected datapoint indexes, and the specified comparision values
    datapoint_indexes = malloc(hardware_desc.values_used_per_pixel * sizeof(int));
    if (datapoint_indexes == NULL) {
        printf("Malloc fail for datapoint indexes\n");
        exit(EXIT_FAILURE);
    }
    comparision_values = malloc(hardware_desc.num_comparision_registers * sizeof(int));
    if (comparision_values == NULL) {
        printf("Malloc fail for comparision values\n");
        exit(EXIT_FAILURE);
    }

    // Parse the provided command line arguments, and configure the system based on them
    parse_args(argc, argv, &hardware_desc, &num_values_in_pixel, &data_endianness, datapoint_indexes, comparision_values, &filepath);

    // Re-map the registers to the correct size, now that we know that amount of comparision registers available
    unmap_config_registers(&config_reg_fd, &hardware_desc.confregs, hardware_desc.num_comparision_registers);
    map_config_registers(&config_reg_fd, &hardware_desc.confregs, hardware_desc.num_comparision_registers);

    // Initialize the dma proxy character devices
    open_dma_devices(&tx_dma, &rx_dma);

    total_start = clock();

    // Open the data file that is to be processed
    open_data_file(&fp_source, filepath);

    printf("Reading the data file...\n");
    // Temporary buffer to which we can read a single pixels data at once
    uint8_t tempBuffer[num_values_in_pixel*hardware_desc.data_element_size_bytes];
    // Loop over the pixel data contained in the file, and read the data points for a single pixel each iteration
    int data_buffer_index = 0;
    for (int pixel_index = 0; pixel_index < num_pixels; pixel_index++) {
        // Helper variable for fseek which tracks the current distance from beginning of file (in bytes)
        int pixels_read_already = pixel_index*num_values_in_pixel*hardware_desc.data_element_size_bytes;

        // Fseek to the first value of the current pixel
        fseek(fp_source, pixels_read_already, SEEK_SET);
        // Read all of the values for a single pixel in to a temporary array (since spamming memcpy is faster than fseek)
        fread(&tempBuffer[0], hardware_desc.data_element_size_bytes, num_values_in_pixel, fp_source);
        // Memcpy the selected data elements to the DMA tx buffer from the temp buffer
        for (int dp_i = 0; dp_i < hardware_desc.values_used_per_pixel; dp_i++) {
            memcpy(&tx_dma.proxyInterface->buffer[data_buffer_index], &tempBuffer[(datapoint_indexes[dp_i]-1)*hardware_desc.data_element_size_bytes], hardware_desc.data_element_size_bytes);
            data_buffer_index += hardware_desc.data_element_size_bytes;
        }
    }

    // Close the source file pointer since we no longer need it
    fclose(fp_source);

    printf("Configuring registers...\n");
    // Write the comparision constants to the config registers
    for (int i = INFO_REG_COUNT; i < hardware_desc.num_comparision_registers+INFO_REG_COUNT; i++) {
        // If we are dealing with big endian data, swap the comparator value endianness to make the HW function correctly
        if (data_endianness == DATA_BIG_ENDIAN) {
            if (hardware_desc.data_element_size_bytes == 2)
                hardware_desc.confregs[i] = bswap_16((uint16_t) comparision_values[i-INFO_REG_COUNT]);
            else if (hardware_desc.data_element_size_bytes == 4) {
                hardware_desc.confregs[i] = bswap_32((uint32_t) comparision_values[i-INFO_REG_COUNT]);
            } else {
                hardware_desc.confregs[i] = comparision_values[i-INFO_REG_COUNT];
            }
        } else {
            hardware_desc.confregs[i] = comparision_values[i - INFO_REG_COUNT];
        }
    }

    printf("Starting the DMA transmit...\n");
	// Create the thread for the transmit processing
    tx_dma.proxyInterface->length = tx_size;
	pthread_create(&tid_tx, NULL, tx_thread, NULL);

	printf("Starting the DMA reception...\n");
	// Setup the rx processing
    rx_dma.proxyInterface->length = rx_size;
    receive();

    total_end = clock();

    printf("\n");
    printf("total time spent (clock cycles): %d\n", total_end-total_start);
    printf("\n");

    // Write the rx data to a file called outfile.bin
    FILE *datafile_out = fopen("./outfile.bin", "wb");
    if (!datafile_out) {
        printf("Opening/creating output file failed\n");
        exit(EXIT_FAILURE);
    }
    fwrite((uint8_t *)rx_dma.proxyInterface->buffer, 1, rx_size, datafile_out);
    fclose(datafile_out);
    printf("Output written to file (outfile.bin)\n");

    // Unmap the config registers
    unmap_config_registers(&config_reg_fd, &hardware_desc.confregs, hardware_desc.num_comparision_registers);
    // Close the dma devices
    close_dma_devices(&tx_dma, &rx_dma);

    free(datapoint_indexes);
    free(comparision_values);
	return 0;
}