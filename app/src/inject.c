#include <zephyr/kernel.h>
#include "msgq_app.h"
#include "zephyr/sys/util_macro.h"

// Filtro sulla coda:
// --wrap è globale: il linker in questo momento devia tutte le chiamate dei moduli che dichiarano z_impl_k_msgq_put senza implementarla
// verso __wrap_z_impl_k_msgq_put (il wrapper). Significa che se un'altro modulo fa uso di una coda, corromperei anche quella.
// 
// Per localizzare il fault e renderlo controllabile allora dobbiamo far in modo che questo venga iniettato unicamente sull'utilizzo di una 
// coda di messaggi in specifico.
// 
// Dobbiamo realizzare un filtro che valuti l'identità di una coda, ovvero il suo indirizzo in memoria.
// 
// Essendo che my_msgq è definita in main.c, ovvero in un'altra unità di compilazione, la dichiaro extern in modo da fare una promessa 
// a compile-time che verrà soddisfatta dal linker successivamente.


// Filtro sull'iniezione:
// Il metodo di Albinet applica il fault una sola volta e poi disabilità l'iniettore → per ricreare dei fault transitori e non permanenti.
// 
// Senza contatore, il modello di guasto non corrisponde ad un modello transitorio, in cui il fualt avviene una sola volta e si valuta come
// il kernel reagisce.
// 
// Introduco quindi un contatore, che a differenza di un booleano mi permette anche di decidere a quale attivazione dell'iniettore colpire
// quando la coda corrisponde al target. Potrei quindi iniettare il fault alla prima, seconda, ... put; questo permette di considerare diversi
// stati in cui si trova la coda (piena, vuota, piena a metà).
// 

// CONFIG_FI_TRIGGER inietta alla N-esima attivazione sulla coda target
// CONFIG_FI_ENABLE abilita l'iniezione del fault
// CONFIG_FI_VALUE valore corrotto scritto in msg_size

static unsigned int fi_calls; // conta quante volte la coda bersaglio è stata utilizzata
// static a livello di file non modifica il ciclo di vita della variabile (rimane permanente, comportamento che otterei anche senza static)
// l'ho utilizzata solo per limitare la visibilità a livello di file; serve per non esportare il simbolo.


// SECONDO CONTATORE PER LE ATTIVAZIONI GLOBALI
// Serve a capire quali altri moduli oltre al mio utilizzano questa k_msgq_put.
static unsigned int fi_calls_other; // chiamate su altre code






// con --wrap=z_impl_k_msgq_put il linker ci fornisce __real_z_impl_k_msgq_put come simbolo per la funzione originale.
extern int __real_z_impl_k_msgq_put(struct k_msgq *msgq, const void *data, k_timeout_t timeout);


void fi_report(void){
    printk("[INJECT] riepilogo: coda bersaglio=%u, altre code=%u\n",
                                                fi_calls, fi_calls_other);   
}


// ogni chiamata a k_msgq_put finisce qui.
int __wrap_z_impl_k_msgq_put(struct k_msgq *msgq, const void *data, k_timeout_t timeout){

    //printk("[INJECT] intercettata k_msgq_put (msg_size=%u), max_msgs=%d\n", (unsigned)msgq->msg_size, msgq->max_msgs);

    // FILTRO SULL'IDENTITÀ DELLA CODA 
    if ( msgq != &my_msgq){
        // chiamo direttamente l'implementazione reale senza iniettare il fault
        fi_calls_other++;
        return __real_z_impl_k_msgq_put(msgq, data, timeout);
    }

    // incremento del contatore delle attivazione

    fi_calls++;

    if ( !IS_ENABLED(CONFIG_FI_ENABLE) ){
        return __real_z_impl_k_msgq_put(msgq, data, timeout);
    }

    
    // se non è l'attivazione scelta, vado direttamente a chiamare l'implementazione reale
    if ( fi_calls != (unsigned int)CONFIG_FI_TRIGGER){
        printk("[INJECT] attivazione #%u: nessuna iniezione\n", fi_calls);
        return __real_z_impl_k_msgq_put(msgq, data, timeout);
    }

    
    size_t original_size = msgq->msg_size;
    size_t corrupted_size = (size_t)CONFIG_FI_VALUE;
    // reale =4; con 64 (default) la memcpy copia 64 byte → overflow


    printk("[INJECT] msg_size=%u -> %u (corrupted)\n", original_size, corrupted_size);

    
    msgq->msg_size = corrupted_size; // FAULT
    int ret = __real_z_impl_k_msgq_put(msgq, data, timeout); // atteso un overflow
    msgq->msg_size = original_size; // ripristino dopo il fault altrimenti in modo che la get legga sempre 4byte

    printk("[INJECT] put ret=%d (dopo la corruzione)\n", ret);
    return ret;
}
