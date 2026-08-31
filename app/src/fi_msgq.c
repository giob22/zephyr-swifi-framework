#include <stdint.h>
#include <zephyr/kernel.h>
#include "msgq_app.h"
#include "fi_core.h"
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


static unsigned int fi_calls; // conta quante volte la coda bersaglio è stata utilizzata
// static a livello di file non modifica il ciclo di vita della variabile (rimane permanente, comportamento che otterei anche senza static)
// l'ho utilizzata solo per limitare la visibilità a livello di file; serve per non esportare il simbolo.


// SECONDO CONTATORE PER LE ATTIVAZIONI GLOBALI
// Serve a capire quali altri moduli oltre al mio utilizzano questa k_msgq_put.
static unsigned int fi_calls_other; // chiamate su altre code



// TABELLA dei bersagli
static const fi_target_t msgq_targets[] = {
    { .id = 1,
      .nome = "msg_size",
      .offset = offsetof(struct k_msgq, msg_size),
      .ampiezza = sizeof(size_t),
      .classe = FI_SCALARE,
      .ripristino = FI_RIPRISTINA,
	  .luogo = FI_CAMPO
	},

    { .id = 4,
      .nome = "max_msgs",
      .offset = offsetof(struct k_msgq, max_msgs),
      .ampiezza = sizeof(uint32_t),
      .classe = FI_SCALARE,
      .ripristino = FI_RIPRISTINA,
	  .luogo = FI_CAMPO
	},

    { .id = 5,
      .nome = "write_ptr",
      .offset = offsetof(struct k_msgq, write_ptr),
      .ampiezza = sizeof(char *),
      .classe = FI_PUNTATORE,
      .ripristino = FI_NON_RIPRISTINARE,
	  .luogo = FI_CAMPO 
	},   // <- la put lo avanza di suo

    { .id = 6,
      .nome = "buffer_start",
      .offset = offsetof(struct k_msgq, buffer_start),
      .ampiezza = sizeof(char *),
      .classe = FI_PUNTATORE,
      .ripristino = FI_RIPRISTINA,
	  .luogo = FI_CAMPO
	},

    { .id = 7,
      .nome = "buffer_end",
      .offset = offsetof(struct k_msgq, buffer_end),
      .ampiezza = sizeof(char *),
      .classe = FI_PUNTATORE,
      .ripristino = FI_RIPRISTINA,
	  .luogo = FI_CAMPO
	},

    { .id = 9,
      .nome = "read_ptr",
      .offset = offsetof(struct k_msgq, read_ptr),
      .ampiezza = sizeof(char *),
      .classe = FI_PUNTATORE,
      .ripristino = FI_RIPRISTINA,
	  .luogo = FI_CAMPO
	},

	{
		.id = 2,
		.nome = "data",
		.luogo = FI_PARAMETRO,
		.indice = 1,
		.offset = 0,
		.ampiezza = sizeof(const void *),
		.classe = FI_PUNTATORE,
		.ripristino = FI_NON_RIPRISTINARE
	},

	{
	    .id = 3,
		.nome = "timeout",
		.luogo = FI_PARAMETRO,
		.indice = 2,
		.offset = 0,
		.ampiezza = sizeof(uintptr_t),
		.classe = FI_SCALARE,
		.ripristino = FI_NON_RIPRISTINARE
	},
	
};

// con --wrap=z_impl_k_msgq_put il linker ci fornisce __real_z_impl_k_msgq_put come simbolo per la funzione originale.
extern int __real_z_impl_k_msgq_put(struct k_msgq *msgq, const void *data, k_timeout_t timeout);


void fi_msgq_report(void){
    fi_core_report_riepilogo("k_msgq_put", fi_calls, fi_calls_other);  
}


// ogni chiamata a k_msgq_put finisce qui.
int __wrap_z_impl_k_msgq_put(struct k_msgq *msgq, const void *data, k_timeout_t timeout){


    // FILTRO SULL'IDENTITÀ DELLA CODA 
    if ( msgq != &my_msgq ){
        // chiamo direttamente l'implementazione reale senza iniettare il fault
        fi_calls_other++;
        return __real_z_impl_k_msgq_put(msgq, data, timeout);
    }

    // incremento del contatore delle attivazione

    fi_calls++;

    if ( !fi_core_attiva(fi_calls) ){
        return __real_z_impl_k_msgq_put(msgq, data, timeout);
    }
    

    const fi_target_t *t = fi_core_bersaglio(msgq_targets, ARRAY_SIZE(msgq_targets));

    if ( t == NULL ){
        return __real_z_impl_k_msgq_put(msgq, data, timeout);
    }

	// array contenente gli argomenti nell'ordine della lista
	// necessario per identificare la base di un bersaglio di tipo parametro
	void *param_addr[] = { (void*)&msgq, (void*)&data, (void*)&timeout };

	void* base = fi_core_base(msgq, param_addr, ARRAY_SIZE(param_addr), t);

	if ( base == NULL ){
		return __real_z_impl_k_msgq_put(msgq, data, timeout);
	}

    
    uintptr_t original = fi_core_applica(base, t);
    
    int ret = __real_z_impl_k_msgq_put(msgq, data, timeout);

    // report post

    fi_core_ripristina(base, t, original);
    fi_core_report_post(ret);
    
    return ret;
}
