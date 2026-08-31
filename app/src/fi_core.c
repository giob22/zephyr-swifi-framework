#include "fi_core.h"
#include "zephyr/random/random.h"
#include <stdint.h>
#include <string.h>
#include <zephyr/kernel.h>

// Filtro sull'iniezione:
// Il metodo di Albinet applica il fault una sola volta e poi disabilità l'iniettore → per ricreare dei fault transitori e non permanenti.
// 
// Senza contatore, il modello di guasto non corrisponde ad un modello transitorio, in cui il fualt avviene una sola volta e si valuta come
// il kernel reagisce.
// 
// Introduco quindi un contatore, che a differenza di un booleano mi permette anche di decidere a quale attivazione dell'iniettore colpire
// quando l'oggetto corrisponde al target. Potrei quindi iniettare il fault alla prima, seconda, ... attivazione; questo permette di considerare diversi
// stati in cui si trova l'oggetto possa trovarsi.
// 

// CONFIG_FI_TRIGGER inietta alla N-esima attivazione sull'oggetto target
// CONFIG_FI_ENABLE abilita l'iniezione del fault
// CONFIG_FI_VALUE valore corrotto applicato al bersaglio

bool fi_core_attiva(unsigned int n_attivazione){
    if( !IS_ENABLED(CONFIG_FI_ENABLE) ){
        return false;
    }

    bool ret = n_attivazione == (unsigned int)CONFIG_FI_TRIGGER;
    #ifdef CONFIG_FI_VERBOSE
    if ( !ret )
        printk("[FI] attivazione #%u: nessuna iniezione\n", n_attivazione);
    #endif
    if ( ret )
        printk("[FI] contesto di esecuzione: %s\n", (k_is_in_isr()) ? "interrupt" : "thread");
    return ret;
}

static uintptr_t leggi(const void *oggetto, const fi_target_t *t){
    uintptr_t v = 0;
    memcpy(&v, (const char *)oggetto + t->offset, t->ampiezza);
    return v;
}

static void scrivi(void *oggetto, const fi_target_t *t, uintptr_t v){
    memcpy((char *)oggetto + t->offset, &v, t->ampiezza);
}

void fi_core_ripristina(void *oggetto, const fi_target_t *t, uintptr_t originale){
    if ( t->ripristino == FI_RIPRISTINA ){
        scrivi(oggetto, t, originale);
    }
}

fi_mode_t fi_core_mode(void){
    return (fi_mode_t)CONFIG_FI_MODE;
}

static uintptr_t fi_core_valore(const fi_target_t *t, uintptr_t originale){

    // (intptr_t) mi permette castare correttamente il valore con il segno 
    // (uintptr_t) poi ottengo il valore come uintptr_t → ottengo il complemento a 2

    switch ( fi_core_mode() ) {
        case FI_MODE_RELATIVO:
            return originale + (uintptr_t)(intptr_t)CONFIG_FI_OFFSET; 
        case FI_MODE_RANDOM:
            return (uintptr_t)sys_rand32_get();
        default:
            return (uintptr_t)CONFIG_FI_VALUE;
    
    }
}

// report pre e post iniezione
// 
static void fi_core_report_pre(const fi_target_t *t, uintptr_t originale, uintptr_t corrotto){
    printk("[FI] inject target=%s mode=%u orig=0x%08lx new=0x%08lx\n", t->nome, (unsigned)fi_core_mode(), (unsigned long)originale, (unsigned long)corrotto);
}
void fi_core_report_post(int ret){
    printk("[FI] done ret=%d\n", ret);
}
// -------------
// 

uintptr_t fi_core_applica(void *oggetto, const fi_target_t *t){
    uintptr_t original = leggi(oggetto, t);

    uintptr_t corrupted = fi_core_valore(t, original);

    scrivi(oggetto, t, corrupted);

    // report prima di ritornare, quindi prima della chiamata alla funzione reale
    // necessario per risolvere il problema con il MODE=2, perché la funzione fi_core_valore non è idempotente
    fi_core_report_pre(t, original, corrupted);

    return original;
}

void* fi_core_base(void *oggetto, void *const *parametri, size_t n, const fi_target_t *t){
    
    if ( t->luogo != FI_PARAMETRO ) return oggetto;
    
    if ( t->indice >= n ){
        printk("[FI] bersaglio %s: nessuna iniezione, parametro %u inesistente\n", t->nome, t->indice);
        return NULL;
    }
    return parametri[t->indice];
}




const fi_target_t *fi_core_bersaglio(const fi_target_t *tabella, size_t n){
    for ( size_t i = 0; i < n; i++ ){
        if ( tabella[i].id == CONFIG_FI_TARGET ){
            if ( tabella[i].ampiezza > sizeof(uintptr_t) || tabella[i].ampiezza == 0 ){
                printk("[FI] bersaglio %d: ampiezza %u non valida\n", CONFIG_FI_TARGET, (unsigned)tabella[i].ampiezza);
                return NULL;
            }
            return &tabella[i];
        }
    }
    printk("[FI] bersaglio %d non disponibile: nessuna iniezione\n", CONFIG_FI_TARGET);
    return NULL;
}

void fi_core_report_riepilogo(const char* nome_primitiva, unsigned int su_bersaglio, unsigned int su_altri){
    printk("[FI] riepilogo su %s: bersaglio=%u altri=%u\n", nome_primitiva, su_bersaglio, su_altri);
}
