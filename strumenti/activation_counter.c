#include <zephyr/kernel.h>


/*
 * Strumento di misura, NON parte della build.
 * Conta le attivazioni degli oggetti del kernel usati dal workload: serve a stabilire quali oggetti
 * un carico esercita davvero (mi permette di valutare la rappresentatività)
 * 
 * Per usarlo: aggiungerlo a target_source e aggiungere al CMakeLists
 * 
 *  zephyr_link_libraries(
 *      -Wl,--wrap=z_impl_k_mutex_lock
 *      -Wl,--wrap=z_impl_k_sem_take
 *      -Wl,--wrap=z_impl_k_sem_give
 *      -Wl,--wrap=z_impl_k_timer_start
 *      -Wl,--wrap=z_impl_k_sleep 
 *  )
 * 
 */


static unsigned int calls_lock;
static unsigned int calls_lock_others;

static unsigned int calls_take;
static unsigned int calls_take_others;

static unsigned int calls_give;
static unsigned int calls_give_others;

static unsigned int calls_start;
static unsigned int calls_start_others;

static unsigned int calls_sleep;
static unsigned int calls_sleep_others;

static unsigned int calls_msgq_put;
static unsigned int calls_msgq_put_others;


extern int __real_z_impl_k_mutex_lock(struct k_mutex * mutex, k_timeout_t timeout);

extern int __real_z_impl_k_sem_take(struct k_sem * sem, k_timeout_t timeout);

extern void __real_z_impl_k_sem_give(struct k_sem * sem);

extern void __real_z_impl_k_timer_start(struct k_timer * timer, k_timeout_t duration, k_timeout_t period);

extern int32_t __real_z_impl_k_sleep(k_timeout_t timeout);

extern int __real_z_impl_k_msgq_put(struct k_msgq *msgq, const void *data, k_timeout_t timeout);


extern struct k_msgq DEMOQX192;
extern struct k_sem SEM0;
extern struct k_mutex DEMO_MUTEX;


void activation_report(void){
    printk("[INJECT] riepilogo: activations\nput:%u|%u\nlock:%u|%u\ntake:%u|%u\ngive:%u|%u\nstart:%u|%u\nsleep:%u|%u\n",
        calls_msgq_put, calls_msgq_put_others ,calls_lock,calls_lock_others, calls_take,calls_take_others, calls_give,calls_give_others, calls_start,calls_start_others, calls_sleep,calls_sleep_others);   
}

int __wrap_z_impl_k_msgq_put(struct k_msgq *msgq, const void *data, k_timeout_t timeout){
    if ( msgq == &DEMOQX192){
        calls_msgq_put++;
        
    }else{
        calls_msgq_put_others++;
    }
    return __real_z_impl_k_msgq_put(msgq, data, timeout);
}

int __wrap_z_impl_k_mutex_lock(struct k_mutex * mutex, k_timeout_t timeout){
    if (mutex == &DEMO_MUTEX){
        calls_lock++;
        
    }else{
        calls_lock_others++;
    }
    return __real_z_impl_k_mutex_lock(mutex, timeout);
}

int __wrap_z_impl_k_sem_take(struct k_sem * sem, k_timeout_t timeout){
    if (sem == &SEM0){
        calls_take++;
        
    }else{
        calls_take_others++;
    }
    return __real_z_impl_k_sem_take(sem, timeout);
}

void __wrap_z_impl_k_sem_give(struct k_sem * sem){
    __real_z_impl_k_sem_give(sem);
}

void __wrap_z_impl_k_timer_start(struct k_timer * timer, k_timeout_t duration, k_timeout_t period){
    __real_z_impl_k_timer_start(timer, duration, period);
}

int32_t __wrap_z_impl_k_sleep(k_timeout_t timeout){
    return __real_z_impl_k_sleep(timeout);
}
