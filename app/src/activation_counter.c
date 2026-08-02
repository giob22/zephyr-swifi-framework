#include <zephyr/kernel.h>


static unsigned int calls_lock;
static unsigned int calls_take;
static unsigned int calls_give;
static unsigned int calls_start;
static unsigned int calls_sleep;


extern int __real_z_impl_k_mutex_lock(struct k_mutex * mutex, k_timeout_t timeout);

extern int __real_z_impl_k_sem_take(struct k_sem * sem, k_timeout_t timeout);

extern void __real_z_impl_k_sem_give(struct k_sem * sem);

extern void __real_z_impl_k_timer_start(struct k_timer * timer, k_timeout_t duration, k_timeout_t period);

extern int32_t __real_z_impl_k_sleep(k_timeout_t timeout);


void fi_report(void){
    printk("[INJECT] riepilogo: activations\nlock:%u\ntake:%u\ngive:%u\nstart:%u\nsleep:%u\n",
        calls_lock, calls_take, calls_give, calls_start, calls_sleep);   
}

int __wrap_z_impl_k_mutex_lock(struct k_mutex * mutex, k_timeout_t timeout){
    calls_lock++;
    return __real_z_impl_k_mutex_lock(mutex, timeout);
}

int __wrap_z_impl_k_sem_take(struct k_sem * sem, k_timeout_t timeout){
    calls_take++;
    return __real_z_impl_k_sem_take(sem, timeout);
}

void __wrap_z_impl_k_sem_give(struct k_sem * sem){
    calls_give++;
    __real_z_impl_k_sem_give(sem);
}

void __wrap_z_impl_k_timer_start(struct k_timer * timer, k_timeout_t duration, k_timeout_t period){
    calls_start++;
    __real_z_impl_k_timer_start(timer, duration, period);
}

int32_t __wrap_z_impl_k_sleep(k_timeout_t timeout){
    calls_sleep++;
    return __real_z_impl_k_sleep(timeout);
}
