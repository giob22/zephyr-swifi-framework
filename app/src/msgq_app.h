
#ifndef MSGQ_APP_H
#define MSGQ_APP_H


#include <zephyr/kernel.h>


// tipo di dato che viaggia nella coda: un elemento a dimensione fissa
typedef struct data{
    uint32_t value;
} data_type;

extern struct k_msgq my_msgq;

#endif