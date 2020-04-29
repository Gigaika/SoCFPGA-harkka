#include "interface.h"
#include <string.h>
#include "stdint.h"
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>

void print_usage() {
    printf("\n");
    printf("Usage: classifier -f [filename] -d [datapoints per pixel in the file] -i [selected datapoint indexes] -c [comparision values]\n");
    printf("\n");
    printf("The file is interpreted as big endian by default, this can be changed to little endian by using the optional -l flag\n");
    printf("For list arguments -i and -c, pass the list of values as a double quoted string with values separated by ';'\n");
    printf("The amount of list arguments passed needs to match the configuration of the hardware\n");
    printf("\n");
    printf("Example usage: classifier -f ./data.bin -d 10 -i \"2;3;4;5\" -c \"5;4;3;2\" -l\n");
    printf("\n");
}

void parse_args(int argc, char *argv[], hw_object *hwd, int *values_per_pixel, int *endianness, int *dp_i, int *cp_v, char **fp) {
    // Flags for determining which arguments were parsed
    int file_read, dpc_read, dpi_read, cv_read = 0;
    char *token;
    int value;
    int current_datapoint_index = 0;
    int current_comparision_index = 0;

    int opt;
    while ((opt = getopt(argc, argv, ":f:d:i:c:l")) != -1) {
        switch(opt) {
            case 'f':
                *fp = optarg;
                file_read = 1;
                break;
            case 'l':
                *endianness = DATA_LITTLE_ENDIAN;
                break;
            case 'd':
                *values_per_pixel = atoi(optarg);
                if (*values_per_pixel < 0) {
                    printf("Invalid argument specified for number of values per pixel in the file\n");
                    exit(EXIT_FAILURE);
                }
                dpc_read = 1;
                break;
            case 'i':
                // Interpret the following argument as a string to allow passing a list in a single argument
                token = strtok(optarg, ";");
                while( token != NULL ) {
                    value = atoi(token);

                    if (value < 0) {
                        printf("Invalid index argument: %s\n", token);
                        exit(EXIT_FAILURE);
                    }

                    if (current_datapoint_index == hwd->values_used_per_pixel) {
                        printf("Too many index arguments\n");
                        exit(EXIT_FAILURE);
                    }

                    dp_i[current_datapoint_index] = value;
                    current_datapoint_index++;

                    token = strtok(NULL, ";");
                }
                dpi_read = 1;
                break;
            case 'c':
                // Interpret the following argument as a string to allow passing a list as a single argument
                token = strtok(optarg, ";");
                while( token != NULL ) {
                    value = atoi(token);

                    if (value < 0) {
                        printf("Invalid comparision value argument: %s\n", token);
                        exit(EXIT_FAILURE);
                    }

                    if (current_comparision_index == hwd->num_comparision_registers) {
                        printf("Too many comparision arguments\n");
                        exit(EXIT_FAILURE);
                    }

                    cp_v[current_comparision_index] = value;
                    current_comparision_index++;

                    token = strtok(NULL, ";");
                }
                cv_read = 1;
                break;
            default:
                printf("Unknown long option: %s\n", argv[optind]);
                exit(EXIT_FAILURE);
        }
    }

    if (!(file_read && dpc_read && dpi_read && cv_read)) {
        printf("Missing or incorrect arguments\n");
        exit(EXIT_FAILURE);
    }

    // Make sure all of the provided datapoint indexes are in range, now that we know the amount of available datapoints
    for (int i = 0; i < hwd->values_used_per_pixel; i++) {
        if (dp_i[i] > *values_per_pixel) {
            printf("Incorrect datapoint index argument: %d, out of range\n", dp_i[i]);
            exit(EXIT_FAILURE);
        }
    }
}

void read_hardware_config(hw_object *hwd) {
    printf("\n");
    hwd->values_used_per_pixel = hwd->confregs[VALUES_IN_USE_REG_INDEX-1];
    printf("Hardware is configured for %d values in single pixel\n", hwd->values_used_per_pixel);
    hwd->data_element_size_bytes = hwd->confregs[ELEMENT_SIZE_BYTES_REG_INDEX-1]/8;
    printf("Hardware is configured for %d bytes per single pixel value\n", hwd->data_element_size_bytes);
    hwd->num_comparision_registers = hwd->confregs[COMP_REG_COUNT_REG_INDEX-1];
    printf("Hardware is configured to support %d comparision values\n", hwd->num_comparision_registers);
    printf("\n");
}
