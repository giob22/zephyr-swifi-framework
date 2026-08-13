# SWIFI su Zephyr — esperimento

Iniezione di guasti software negli oggetti del kernel Zephyr (code di messaggi
e semafori) mediante interposizione a tempo di collegamento
(`-Wl,--wrap=z_impl_k_msgq_put`, `-Wl,--wrap=z_impl_k_sem_take`).

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
| `CONFIG_WL_MSGQ` / `CONFIG_WL_SEM` | carico da eseguire: code di messaggi oppure semafori (mutuamente esclusivi) |
| `CONFIG_ASSERT` | asserzioni del kernel |

## Faultload

Bersagli e valori di guasto, nella classificazione di Albinet. Gli identificatori
sono disgiunti fra un oggetto e l'altro, perché tutti i moduli sono presenti nella
stessa immagine.

| `FI_TARGET` | campo | oggetto | Bad\_Arg 1 | Bad\_Arg 2 | Bad\_Arg 3 | Bad\_Arg 4 |
|---|---|---|---|---|---|---|
| 1 | `msg_size` | `k_msgq` | `0x0` | `0x80000000` | `0xFFFFFFFF` | maggiore della dimensione nominale |
| 4 | `max_msgs` | `k_msgq` | `0x0` | `0x80000000` | `0xFFFFFFFF` | — |
| 5 | `write_ptr` | `k_msgq` | `0x0` | casuale | `0xFFFFFFFF` | scostamento errato dentro il buffer |
| 6 | `buffer_start` | `k_msgq` | `0x0` | casuale | `0xFFFFFFFF` | oltre `buffer_end` |
| 7 | `buffer_end` | `k_msgq` | `0x0` | casuale | `0xFFFFFFFF` | prima di `buffer_start` |
| 9 | `read_ptr` | `k_msgq` | `0x0` | casuale | `0xFFFFFFFF` | area valida ma diversa |
| 11 | `count` | `k_sem` | `0x0` | `0x80000000` | `0xFFFFFFFF` | `+1` |
| 12 | `limit` | `k_sem` | `0x0` | `0x80000000` | `0xFFFFFFFF` | minore di `count` |

I primi tre valori sono assoluti e si impostano con `CONFIG_FI_MODE=0` e
`CONFIG_FI_VALUE`. Il quarto è **relativo al valore corrente** del campo e si
imposta con `CONFIG_FI_MODE=1` e `CONFIG_FI_OFFSET`, che accetta anche valori
negativi. Il valore casuale si ottiene con `CONFIG_FI_MODE=2`.

Il bersaglio dev'essere coerente con il carico selezionato: con
`CONFIG_WL_SEM` attivo, un bersaglio delle code non viene mai raggiunto.

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

Gli scenari relativi a un solo oggetto del kernel si selezionano con `--tag`. I
tag disponibili sono `msgq` e `sem`:

```bash
./scripts/twister -p mps2/an385 -T <percorso>/app \
    --tag sem
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