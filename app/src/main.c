#include "msgq_app.h"
#include <zephyr/kernel.h>
#include <string.h>
#include "sem_app.h"

#define MAX_MSGS 4
#define CANARY_BYTES 64
#define CANARY_PATTERN 0xAA
#define LIMIT 1
#define INITIAL_COUNT 1


// test per verificare l'utilizzo appropriato di k_magq_put nel contesto di interrupt
#ifdef CONFIG_WL_MSGQ_ISR


struct k_timer my_timer;

void my_expire_fn(struct k_timer *timer);


#endif







// K_MSGQ_DEFINE(my_msgq, sizeof(data_type), 4, 4);


/*
    Definiamo la coda a compile-time:
    my_msgq → nome della coda
    sizeof(data_type) → msg_size, quanti byte per messaggio
    4 → max_msgs, capacità massima della coda
    4 → allineamento del buffer
*/




// cambio di rotta: non utilizzo più la macro per definire la coda di messaggi + buffer perché ho bisogno di accedere al buffer per verificare
// buffer overflow quando CONFIG_ASSERT=n. Essendo che _k_fifo_buf_my_msgq (ring buffer creato dalla macro) è static, non posso accedere da inject.c direttamente
// perché static limita la visibilità all'esterno.
//
// Quindi devo necessariamente seguire il secondo approccio per inizializzare una coda di messaggi, ovvero quello di definire un ring_buffer e di utilizzare
// la funzione k_msgq_init(). Ovvero l'inizializzazione a runtime().
//
// L'idea è di aggiungere valori sentinella subito dopo il buffer per osservare il buffer-overflow

// inserisco il ring buffer della coda di messaggi e la sentinella (il buffer canary) all'interno di una struct in modo che in memoria siano
// disposti in modo contiguo, quindi un overflow su ring_buffer intacca il contenuto del buffer canary.

// CODE DI MESSAGGI




struct k_msgq my_msgq;

#ifdef CONFIG_WL_MSGQ

static struct{
    char ring_buffer[MAX_MSGS * sizeof(data_type)];
    uint8_t canary[CANARY_BYTES];
}area __aligned(__alignof__(data_type));

static void canary_fill(void){
    memset( area.canary, CANARY_PATTERN, CANARY_BYTES );
}

static void msgq_dump(const char *quando){

    // write_off è il dato chiave: deve essere compreso tra 0 e 15.
    // Il ring buffer è 16 byte = MAX_MSGS (4 messaggi) x sizeof(data_type) (4 byte ciascuno).
    // 
    printk("[CODA %s]\tused=%u,\t|\twrite_off=%d\t|\tread_off=%d\n",
        quando,
        my_msgq.used_msgs,
        (int)(my_msgq.write_ptr - my_msgq.buffer_start),
        (int)(my_msgq.read_ptr - my_msgq.buffer_start));
}


static void canary_check(const char *quando){
    unsigned int corrotti = 0;

    int primo = -1, ultimo = -1;

    for ( int i = 0; i < CANARY_BYTES; i++ ){
        if (area.canary[i] != CANARY_PATTERN ){
            corrotti++;
            if ( primo < 0 ) primo = i;
            ultimo = i;
        }
    }

    if ( corrotti == 0 ){
        printk("[CANARY %s] INTATTA: nessuna scrittura oltre il buffer\n", quando);
    }else{
        printk("[CANARY %s] CORROTTA: %u byte su %d, a partire dall'offset %d a %d\n",
                                        quando, corrotti, CANARY_BYTES, primo, ultimo);
    }
}


#endif


// SEMAFORI


#ifdef CONFIG_WL_SEM

K_SEM_DEFINE(my_sem, INITIAL_COUNT, LIMIT); // conteggio 1, limite 1 → mutex

static void sem_dump(const char *quando){

    printk("[SEM %s]\tcount=%u,\t|\tlimit=%u\n",
        quando,
        (unsigned)my_sem.count,
        (unsigned)my_sem.limit
    );

}

#endif


int main(void){
    #ifdef CONFIG_WL_MSGQ
    // inizializzazione della coda a run-time
    k_msgq_init(&my_msgq, area.ring_buffer, sizeof(data_type), MAX_MSGS);

    // riempiamo la sentinella con un valore riconoscibile come 0xAA
    canary_fill();


    data_type rx = { 0 };
    
    int ret;
    
    printk("\n\n--- SWIFI k_msgq: avvio ---\n");
    
    msgq_dump("iniziale");
    
    #ifndef CONFIG_WL_MSGQ_ISR 
    data_type tx = { .value = 42 };

    printk("\nPRODUTTORE\n");

    
    // PRODUTTORE: copia (memcpy) msg_size byte da &tx dentro la coda
    ret = k_msgq_put(&my_msgq, &tx, K_NO_WAIT);
    canary_check("after put#1");
    msgq_dump("after put#1");
    printk("put#1: ret=%d (inviato value=%u)\n", ret, tx.value);

    // ----------------------------------------------
    // SECONDA put dopo che lo stato della coda è corrotto. Abbiamo scritto al di fuori del ring_buffer

    data_type tx2 = { .value = 99 };
    ret = k_msgq_put(&my_msgq, &tx2, K_NO_WAIT);
    canary_check("after put#2");
    msgq_dump("after put#2");
    printk("put#2: ret=%d (inviato value=%u)\n", ret, tx2.value);

    // ----------------------------------------------


    printk("\nCONSUMATORE\n");
    
    // CONSUMATORE: copia (memcpy) msg_size byte dalla coda my_msgq in &rx

    ret = k_msgq_get(&my_msgq, &rx, K_NO_WAIT);

    printk("get#1: ret=%d (ricevuto value=%u)\n", ret, rx.value);

    canary_check("after get#1");
    msgq_dump("after get#1");



    // ----------------------------------------------
    // SECONDA get

    ret = k_msgq_get(&my_msgq, &rx, K_NO_WAIT);

    printk("get#2: ret=%d (ricevuto value=%u)\n", ret, rx.value);

    canary_check("after get#2");
    msgq_dump("after get#2");


    // ----------------------------------------------

    #endif

    #ifdef CONFIG_WL_MSGQ_ISR


    // check se sono in contesto interrupted

    printk("[MAIN] k_is_in_isr()=%d\n", k_is_in_isr());


    k_timer_init(&my_timer, my_expire_fn, NULL);


    k_timer_start(&my_timer, K_MSEC(5), K_NO_WAIT);

    k_timer_status_sync(&my_timer);

    // check della sentinel memory
    canary_check("after put#1");
    msgq_dump("after put#1");


    ret = k_msgq_get(&my_msgq, &rx, K_NO_WAIT);

    printk("get#1: ret=%d (ricevuto value=%u)\n", ret, rx.value);
    msgq_dump("after get#1");
    

    
    
    #endif



    // marcatore di fine run, aggiunto per discriminare i diversi failure mode
    // presente: WC
    // assente con panic: WA
    // assente senza panic: KH
    // senza twister non riesce a discriminare un workload completato da uno morto a metà.



    #endif

    #ifdef CONFIG_WL_SEM
    
    int r1 = k_sem_take(&my_sem, K_NO_WAIT);
    printk("sem_take#1: ret=%d\n", r1);
    sem_dump("after take#1");

    int r2 = k_sem_take(&my_sem, K_NO_WAIT);
    printk("sem_take#2: ret=%d\n", r2);
    sem_dump("after take#2");

    int r3 = k_sem_take(&my_sem, K_NO_WAIT);
    printk("sem_take#3: ret=%d\n", r3);

    sem_dump("after take#3");

    
    k_sem_give(&my_sem);
    sem_dump("after give#1");

    #endif

    
    
    printk("[RUN] END\n");
    return 0;
}
