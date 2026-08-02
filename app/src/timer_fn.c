#include <string.h>
#include <zephyr/kernel.h>
#include "msgq_app.h"



void my_expire_fn(struct k_timer *timer){
    printk("[TIMER] timer expired, status=%u\n", k_timer_status_get(timer));


    
    
    printk("[TIMER] k_is_in_isr()=%d\n", k_is_in_isr());
    

    // creo il messaggio data_type con calore riconoscibile
    data_type tx = { .value=100 };

    int ret = k_msgq_put(&my_msgq, &tx, K_NO_WAIT);
    printk("put#1: ret=%d (inviato value=%u)\n", ret, tx.value);

    printk("[TIMER] expired_fn terminated\n");
}
