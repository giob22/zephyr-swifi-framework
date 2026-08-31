# SWIFI framework su Zephyr — esperimento

Iniezione di guasti software negli oggetti del kernel Zephyr (code di messaggi
e semafori) mediante interposizione a tempo di collegamento
(`-Wl,--wrap=z_impl_k_msgq_put`, `-Wl,--wrap=z_impl_k_sem_take`, `-Wl,--wrap=main`).

## Ambiente

| | |
|---|---|
| Zephyr | `v4.4.0-8541-gdf1fb529e38f` — commit `df1fb529e38f34fa45c1fe1cb6aace42802de3db` |
| west | 1.5.0 |
| Zephyr SDK | 1.0.1 |
| Board | `mps2/an385` (QEMU, ARM Cortex-M3) |

## Build ed esecuzione di una singola configurazione

```bash
west build -p always -b mps2/an385 <percorso>/app
west build -t run
```

Per cambiare il fault senza modificare i file:

```bash
west build -p always -b mps2/an385 <percorso>/app -- \
    -DCONFIG_FI_ENABLE=y -DCONFIG_FI_TARGET=1 -DCONFIG_FI_VALUE=0x40 \
    -DCONFIG_ASSERT=n
```

| Opzione | Significato |
|---|---|
| `CONFIG_FI_ENABLE` | attiva l'iniezione (`n` = esecuzione di riferimento) |
| `CONFIG_FI_TARGET` | bersaglio dell'iniezione: quale campo corrompere (tabella sotto) |
| `CONFIG_FI_TRIGGER` | attivazione su cui iniettare (il guasto è transitorio: si applica una volta sola) |
| `CONFIG_FI_MODE` | come si calcola il valore corrotto: `0` assoluto, `1` relativo, `2` casuale |
| `CONFIG_FI_VALUE` | valore corrotto, usato con `CONFIG_FI_MODE=0` |
| `CONFIG_FI_OFFSET` | scostamento dal valore originale, usato con `CONFIG_FI_MODE=1` |
| `CONFIG_FI_VERBOSE` | stampa una riga per ogni attivazione non iniettata |
| `CONFIG_WL_MSGQ` / `CONFIG_WL_MSGQ_ISR` / `CONFIG_WL_SEM` | carico da eseguire: code di messaggi (contesto di interruzione o thread) oppure semafori (mutuamente esclusivi) |
| `CONFIG_ASSERT` | asserzioni del kernel |

## Faultload

Bersagli e valori di guasto, nella classificazione di Albinet. Gli identificatori
sono disgiunti fra un oggetto e l'altro, perché tutti i moduli sono presenti nella
stessa immagine.

| `FI_TARGET` | bersaglio | luogo | oggetto | Bad\_Arg 1 | Bad\_Arg 2 | Bad\_Arg 3 | Bad\_Arg 4 |
|---|---|---|---|---|---|---|---|
| 1 | `msg_size` | campo | `k_msgq` | `0x0` | `0x80000000` | `0xFFFFFFFF` | maggiore della dimensione nominale |
| 2 | `data` | parametro | `k_msgq` | `0x0` | casuale | `0xFFFFFFFF` | scostamento dentro l'area del messaggio |
| 3 | `timeout` | parametro | `k_msgq` | `0x0` | `0x80000000` | `0xFFFFFFFF` | attesa finita in contesto di interruzione |
| 4 | `max_msgs` | campo | `k_msgq` | `0x0` | `0x80000000` | `0xFFFFFFFF` | — |
| 5 | `write_ptr` | campo | `k_msgq` | `0x0` | casuale | `0xFFFFFFFF` | scostamento errato dentro il buffer |
| 6 | `buffer_start` | campo | `k_msgq` | `0x0` | casuale | `0xFFFFFFFF` | oltre `buffer_end` |
| 7 | `buffer_end` | campo | `k_msgq` | `0x0` | casuale | `0xFFFFFFFF` | prima di `buffer_start` |
| 9 | `read_ptr` | campo | `k_msgq` | `0x0` | casuale | `0xFFFFFFFF` | area valida ma diversa |
| 11 | `count` | campo | `k_sem` | `0x0` | `0x80000000` | `0xFFFFFFFF` | `+1` |
| 12 | `limit` | campo | `k_sem` | `0x0` | `0x80000000` | `0xFFFFFFFF` | minore di `count` |

Un bersaglio è un **campo** della struttura dati dell'oggetto del kernel oppure
un **parametro** della chiamata. Nel primo caso il descrittore dichiara lo
scostamento dall'indirizzo dell'oggetto, nel secondo la posizione nella firma
della primitiva. La corruzione di un parametro agisce sulla copia locale, quindi
sparisce al ritorno dalla chiamata; quella di un campo resta nell'oggetto se il
descrittore non chiede il ripristino.

I primi tre valori sono assoluti e si impostano con `CONFIG_FI_MODE=0` e
`CONFIG_FI_VALUE`. Il quarto è scelto in funzione del bersaglio: dove conta la
distanza dal valore corretto si esprime **relativamente** al valore corrente, con
`CONFIG_FI_MODE=1` e `CONFIG_FI_OFFSET`, che accetta anche valori negativi;
altrove è assoluto come i primi tre. Il valore casuale si ottiene con
`CONFIG_FI_MODE=2`.

Il bersaglio dev'essere coerente con il carico selezionato: con
`CONFIG_WL_SEM` attivo, un bersaglio delle code non viene mai raggiunto, e
`timeout` è interessante soltanto con `CONFIG_WL_MSGQ_ISR`, dove attendere non è
ammesso.

## Campagna completa

Gli scenari sono definiti in `app/testcase.yaml` ed eseguiti con twister:

```bash
./scripts/twister -p mps2/an385 -T <percorso>/app -v --inline-logs
```

Un singolo scenario — i nomi sono le chiavi sotto `tests:` in `app/testcase.yaml`:

```bash
./scripts/twister -p mps2/an385 -T <percorso>/app \
    -s <nome-scenario>
```


## Risultati

Nella directory da cui è stato lanciato twister:

```
twister-out/twister.json                              esiti e misure di tutti gli scenari
twister-out/<board>/<toolchain>/<app>/<scenario>/
    handler.log                                       output di console della run
    recording.csv                                     misure estratte
    zephyr/.config                                    configurazione risolta
```
