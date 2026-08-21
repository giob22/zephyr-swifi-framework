#include "sem_app.h"
#include "fi_core.h"
#include <zephyr/kernel.h>

/*
 * Tale file contiene: la funzione da intercettare, l'oggetto da colpire e quali campi esistono
 * Non contiene log, logica di iniezione, il calcolo del valore corrotto e la conoscenza delle macro definite a tempo di compilazione
 * tutte queste proprietà fanno parte della parte comune del framework contenute in fi_core.c
 */


/*
 * __real_... Non esiste in nessun sorgente di Zephyr, lo crea il linker quando passiamo come flag -Wl,--wrap=z_impl... . Tale flag rinomina l'implementazione originale
 * in __real_... e devia tutte le chiamate al simbolo originale z_impl_... verso __wrap_z... .
 * 
 * Anche in questo caso non avremmo potuto wrappare k_sem_take perché nel sorgente è static inline → non genera un simbolo, a tempo di link questo simbolo non 
 * esiste perché a tempo di compilazione viene sostituita direttamente l'implementazione reale.
 * 
 */

extern int __real_z_impl_k_sem_take(struct k_sem *sem, k_timeout_t timeout);

static const fi_target_t sem_targets[] = {
    { .id=11, .nome="count", .offset=offsetof(struct k_sem, count), .ampiezza=sizeof(unsigned int), .classe=FI_SCALARE, .ripristino=FI_NON_RIPRISTINARE }, // take decrementa count → regola applicata
    { .id=12, .nome="limit", .offset=offsetof(struct k_sem, limit), .ampiezza=sizeof(unsigned int), .classe=FI_SCALARE, .ripristino=FI_RIPRISTINA }
};
/*
 * .id → il numero del bersaglio nella campagna
 * .nome → compare nel log
 * .offset → distanza in byte dall'inizio della struct per uno specifico campo
 * .ampiezza → quanti byte leggere e scrivere
 * .classe → presente ma non utilizzata, era necessaria a capire se stessimo trattando uno scalare (int) o un puntatore, quindi cambiava l'aritmetica da considerare
 * .ripristino → necessario per capire se un campo debba esser ripristinato a seguito dell'iniezione, la regola che ho imposto è: si ripristina solo se la funzione reale non modifica il campo di suo
 * 
 */

static unsigned int fi_calls;
static unsigned int fi_calls_other;

void fi_sem_report(void){
    fi_core_report_riepilogo(fi_calls, fi_calls_other);
}

int __wrap_z_impl_k_sem_take(struct k_sem *sem, k_timeout_t timeout){
    // primo filtro sulla struttura dati

    /*
     * Questo primo filtro serve per garantire che l'iniezione avviene in una particolare attivazione del semaforo bersaglio.
     * Essendo --wrap globale: intercetta ogni chiamata a qual simbolo in tutta l'immagine, da qualunque modulo
     * 
     * Per garantire controllabilità è necessario che il fault sia localizzato su un solo oggetto
     * 
     * fi_calls_other mi permette di capire quanti altri moduli utilizzino questa primitiva su semafori che non sono il bersaglio
     * Su questo workload sintetico non esistono altri moduli che sfruttano semafori per sincronizzarsi, ma su uno reale non è detto che non esistino
     */
    
    if ( &my_sem != sem){
        fi_calls_other++;
        return __real_z_impl_k_sem_take(sem, timeout);
    }

    fi_calls++;

    if ( !fi_core_attiva(fi_calls) ){
        return __real_z_impl_k_sem_take(sem, timeout); 
    }

    const fi_target_t *t = fi_core_bersaglio(sem_targets, ARRAY_SIZE(sem_targets), CONFIG_FI_TARGET);

    if ( t == NULL ){
        return __real_z_impl_k_sem_take(sem, timeout);
    }

    uintptr_t original = fi_core_applica(sem, t);

    int ret = __real_z_impl_k_sem_take(sem, timeout);

    fi_core_ripristina(sem, t, original);

    fi_core_report_post(ret);

    return ret;
}

/*
 * 
 * questo file è ciò che serve per estendere il framework a un nuovo oggetto del kernel: un wrapper e una tabella. Non contiene logica di iniezione, non stampa nulla
 * e non ha richiesto di modificare una riga della parte comune. 
 */