#ifndef FI_CORE
#define FI_CORE

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

// per discriminare come trattare il valore del bersaglio: su uno scalare uno scostamento è un numero, su un puntatore è aritmetica dei puntatori
typedef enum {FI_SCALARE, FI_PUNTATORE} fi_classe_t;

// per capire se necessario ripristinare il campo dopo la chiamata alla funzione reale.
// La regola da seguire: si ripristina SOLO se la funzione reale non modifica il campo di suo. Il ripristino rende transitorio il fault;
// su un campo che la funzione aggiorna cancellerebbe anche l'errore
typedef enum {FI_RIPRISTINA, FI_NON_RIPRISTINARE} fi_ripristino_t;

// specifica dove si trova il bersaglio: un campo di una struct dell'oggetto, oppure un parametro della chiamata
typedef enum {FI_CAMPO = 0, FI_PARAMETRO = 1} fi_luogo_t;

// specifica come calcolare il valore corrotto a partire dall'originale
typedef enum {
    FI_MODE_ASSOLUTO = 0,   // FI_VALUE
    FI_MODE_RELATIVO = 1,   // originale + FI_OFFSET → utilizzato per i puntatori
    FI_MODE_RANDOM = 2,     // valore estratto
} fi_mode_t;


// descrittore di un bersaglio, ovvero un CAMPO di una struct del kernel.
// specifica: dove si trova, quanto è grande e come si tratta. 
typedef struct {
    unsigned int id;
    const char *nome;
    size_t offset;
    size_t ampiezza;
    // La classe è presente ma non ancora letta da nessuna funzione: con la rappresentazione a uintptr_t
    // l'aritmetica su scalari e su puntatori coincide, quindi fi_core_valore non ha bisogno di distinguerli. 
    // Resta come documentazione del bersaglio
    fi_classe_t classe;
    fi_luogo_t luogo;
    unsigned int indice; // utile solo se luogo == FI_PARAMETRO, indica la posizione nella firma
    fi_ripristino_t ripristino;
} fi_target_t;


// necessaria per eseguire l'iniezione in una sola attivazione
bool fi_core_attiva(unsigned int n_attivazione);

// sostituisce il valore con quello corrotto
uintptr_t fi_core_applica(void *oggetto, const fi_target_t *t);
// ripristina il valore seguendo la regola spiegata sopra ↑

// permette di scegliere l'indirizzo base dell'iniezione: ha come risultato l'indirizzo del parametro, oppure l'indirizzo 
// della struct dell'oggetto
// ritorna NULL nel caso in cui è FI_PARAMETRO, ma l'indice non corrisponde a nessun parametro reale.
void* fi_core_base(void *oggetto, void *const *parametri, size_t n, const fi_target_t *t);

void fi_core_ripristina(void *oggetto, const fi_target_t *t, uintptr_t originale);


// report pre e post iniezione
void fi_core_report_post(int ret);
// --------

// cerca il bersaglio per id. NULL se non c'e'.
const fi_target_t *fi_core_bersaglio(const fi_target_t *tabella, size_t n);

void fi_core_report_riepilogo(const char* nome_primitiva, unsigned int su_bersaglio, unsigned int su_altri);



#endif