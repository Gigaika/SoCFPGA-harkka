#ifndef CLASSIFIER_INTERFACE_H
#define CLASSIFIER_INTERFACE_H

#include "classifier.h"

void print_usage(void);
void parse_args(int argc, char *argv[], hw_object *hwd, int *values_per_pixel, int *endianness, int *dp_i, int *cp_v, char **fp);
void read_hardware_config(hw_object *hwd);

#endif //CLASSIFIER_INTERFACE_H
