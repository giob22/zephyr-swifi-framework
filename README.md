# Framework di software fault injection per Zephyr RTOS

Codice della tesi di laurea magistrale "Progetto e Valutazione di un Framework di Software Fault Injection per Zephyr RTOS", Università degli Studi di Napoli Federico II, Corso di Laurea in Ingegneria Informatica, anno accademico 2025-2026.

Il framework inietta guasti software sui valori che una primitiva del kernel di Zephyr adopera, mediante interposizione a tempo di collegamento ('-Wl,--wrap=z_impl_k_msgq_put', '-Wl,--wrap=z_impl_k_sem_take'). Non richiede modifiche né al sorgente di Zephyr né al carico. Il codice del framework è presente all'interno della directory dell'applicazione e prende il controllo tramite la deviazione delle chiamate fatta a tempo di collegamento.

## Struttura della repository
```
📦swifi-msgq
 ┣ 📂app
 ┃ ┣ 📂src                      il framework e il carico di prova
 ┃ ┃ ┣ 📄fi_core.c              nucleo: decide l'attivazione, legge, corrompe, ripristina, documenta
 ┃ ┃ ┣ 📄fi_core.h              
 ┃ ┃ ┣ 📄fi_msgq.c              modulo d'oggetto per `k_msgq_put`, con la tabella dei descrittori
 ┃ ┃ ┣ 📄fi_sem.c               modulo d'oggetto per `k_sem_take`, con la tabella dei descrittori
 ┃ ┃ ┣ 📄hook.c                 monitor: si aggancia al punto di ingresso del carico (--wrap=main)
 ┃ ┃ ┣ 📄main.c                 carico di prova, parametrizzato da Kconfig
 ┃ ┃ ┣ 📄msgq_app.h
 ┃ ┃ ┣ 📄sem_app.h
 ┃ ┃ ┗ 📄timer_fn.c
 ┃ ┣ 📄CMakeLists.txt           file di costruzione: dove sono presenti le opzioni per il compilatore
 ┃ ┣ 📄Kconfig                  configurazione esterna: bersaglio, attivazione, modo, valore
 ┃ ┣ 📄prj.conf                 valorizzazione delle opzioni
 ┃ ┗ 📄testcase.yaml            la campagna costituita da diversi scenari
 ┣ 📂strumenti                  misura della selettività, non fa parte della build di app/
 ┃ ┣ 📂app_kernel               benchmark di Zephyr usato come carico di terze parti
 ┃ ┃ ┣ 📄CMakeLists.txt         
 ┃ ┃ ┣ 📄prj.conf
 ┃ ┃ ┣ 📄prj_user.conf
 ┃ ┃ ┣ 📄README.txt
 ┃ ┃ ┗ 📄tests.yaml
 ┃ ┣ 📄activation_counter.c     
 ┃ ┗ 📄report_hook.c
 ┣ 📄.gitignore
 ┗ 📄README.md
```

Il nucleo non sa cosa stia corrompendo: riceve dal modulo d'oggetto la tabella dei descrittori da cui poi ricava l'indirizzo e l'ampiezza del valore da corrompere. È totalmente indipendente dal bersaglio scelto, deve solo conoscerne la posizione. Ogni modulo d'oggetto conosce quale funzione intercettare, quale istanza di oggetto del kernel colpire e quali bersagli esistano. Aggiungere una primitiva significa scrivere un nuovo modulo d'oggetto e due righe in `CMakeLists.txt`, `fi_sem.c` è costato 37 linee di codice, mentre `fi_msgq.c` ne costa 105. Il nucleo, invece, non è cambiato.

Le modifiche fatte all'interno del `CMakeLists.txt` sono:

<!-- embed:file="app/CMakeLists.txt" line="8-10" withLineNumbers="true" new="10" lock="true" -->
[Source: app/CMakeLists.txt](app/CMakeLists.txt#L8-L10)
```
 8: zephyr_link_libraries(-Wl,--wrap=main
 9:                       -Wl,--wrap=z_impl_k_msgq_put
10:                       -Wl,--wrap=z_impl_k_sem_take) // NEW
```
<!-- embed:end -->

## Prerequisiti

L'istallazione di Zephyr, di `west` e delle dipendenze dalla guida ufficiale:

- [Getting Started](https://docs.zephyrproject.org/latest/develop/getting_started/index.html)
- [Zephyr SDK, versione 1.0.1](https://github.com/zephyrproject-rtos/sdk-ng/releases/tag/v1.0.1)
- [west](https://docs.zephyrproject.org/latest/develop/west/index.html)

Per utilizzare la stessa versione del progetto, ovvero lo stesso commit:

```bash
git checkout df1fb529e38f34fa45c1fe1cb6aace42802de3db
```

## Ambiente

|||
|---|---|
|Zephyr|`v4.4.0-8541-gdf1fb529e38f`, commit `df1fb529e38f34fa45c1fe1cb6aace42802de3db`|
|west| 1.5.0|
|Zephyr SDK| 1.0.1 |
|Board|`mps2/an385` (QEMU, ARM Cortex-M3)|



## Build ed esecuzione di una singola configurazione

```bash
west build -p always -b mps2/an385 <percorso>/app
west build -t run
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

Nessuna decisione sull'iniezione è scritta nel codice: nel sorgente compaiono
soltanto le macro, non il valore che assumono.

Tali macro sono definite in un file di intestazione generato dal sistema di configurazione Kconfig durante la costruzione della build

## Faultload

Bersagli e valori corrotti. Gli identificatori sono disgiunti fra una primitiva e l'altra, perché tutti i moduli sono presenti nella stessa immagine.

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

Un bersaglio è un **campo** della struttura dati dell'oggetto del kernel oppure è un **parametro** della chiamata. Nel primo caso il descrittore dichiara lo scostamento dall'indirizzo dell'oggetto, nel secondo caso la posizione nella firma della primitiva.

## Campagna completa

Gli scenari sono definiti in `app/testcase.yaml`, una riga di configurazione per ciascuno: 31 iniezioni e due esecuzioni senza guasto. Si eseguono con twister:

Esempio di scenario:

<!-- embed:file="app/testcase.yaml" line="32-58" withLineNumbers="true" lock="true" -->
[Source: app/testcase.yaml](app/testcase.yaml#L32-L58)
```yaml
32: swifi.msgq.msgsize.badarg4.assert_off:
33:   tags: msgq
34:   extra_configs:
35:     - CONFIG_ASSERT=n
36:     - CONFIG_FI_VALUE=0x40
37:     - CONFIG_FI_ENABLE=y
38:     - CONFIG_FI_TRIGGER=1
39:     - CONFIG_FI_TARGET=1
40:     - CONFIG_FI_VERBOSE=y
41:     - CONFIG_WL_MSGQ=y
42:   harness_config:
43:     type: multi_line
44:     ordered: true
45:     regex:
46:       - '\[FI\] inject target=msg_size'
47:       - 'CORROTTA'  
48:       - '\[RUN\] END' 
49:     record:
50:       regex:
51:         - '\[FI\] riepilogo su k_msgq_put: bersaglio=(?P<su_bersaglio>\d+) altri=(?P<su_altri>\d+)'
52:         - 'CANARY after (?P<op_canary>\w+)#(?P<n_canary>\d+). (?P<canary_status>\w+)'
53:         - 'CANARY after put#(?P<n_put>\d+). CORROTTA: (?P<byte_corrotti>\d+) byte su (?P<byte_totali>\d+)[^0-9]+(?P<primo>\d+) a (?P<ultimo>\d+)'
54:         - 'get#(?P<n_get>\d+): ret=(?P<ret_get>-?\d+)[^=]+=(?P<valore>\d+)'
55:         - '.CODA after (?P<op>\w+)#(?P<n_op>\d+)[^0-9-]+(?P<used>\d+)[^0-9-]+(?P<write_off>-?\d+)[^0-9-]+(?P<read_off>-?\d+)'
56:         - '\[FI\] inject target=(?P<target>\w+) mode=(?P<mode>\d+) orig=(?P<orig>0x[0-9a-fA-F]+) new=(?P<new>0x[0-9a-fA-F]+)'
57:         - '\[FI\] done ret=(?P<ret_op>-?\d+)'
58:         - '\[FI\] contesto [^:]+: (?P<contesto>\w+)'
```
<!-- embed:end -->

Per eseguire tutti gli scenari:

```bash
./scripts/twister -p mps2/an385 -T <percorso>/app -v --inline-logs
```

Un singolo scenario, con il nome della chiave sotto `tests:`:

```bash
./scripts/twister -p mps2/an385 -T <percorso>/app -s <nome-scenario>
```

## Risultati

Nella directory da cui è stato lanciato twister, alla terminazione della campagna viene creata la cartella `twister-out/`:

```
twister-out/twister.json                              esiti e misure di tutti gli scenari
twister-out/<board>/<toolchain>/<app>/<scenario>/
    handler.log                                       output di console della run
    recording.csv                                     misure estratte
    zephyr/.config                                    configurazione risolta
```

Il framework riferisce ciò che riguarda l'iniezione: bersaglio, valore letto, valore scritto, contesto di esecuzione ed esito della chiamata quando questa ritorna. Non osserva il danno che il guasto produce nell'oggetto: quello lo misura la strumentazione del carico, che al framework non appartiene.

## Misura della selettività delle chiamate

L'interposizione al collegamento è globale: devia tutte le chiamate alla primitiva, da qualunque modulo e su qualunque oggetto. Il framework le filtra confrontando l'indirizzo dell'oggetto passato come parametro con quello dell'oggetto scelto, se ne viene adoperato uno.

Per misurare quando il filtro separi, su un carico che usa più oggetti della stessa specie, si è usato un benchmark di terze parti. In particolare `tests/benchmarks/app_kernel`, che fa parte del repository di Zephyr. L'unica modifica fatta è sul `CMakeLists.txt`:

<!-- embed:file="strumenti/app_kernel/CMakeLists.txt" line="14-23" withLineNumbers="true" lock="true" -->
[Source: strumenti/app_kernel/CMakeLists.txt](strumenti/app_kernel/CMakeLists.txt#L14-L23)
```
14: zephyr_link_libraries(
15:        -Wl,--wrap=main
16:        -Wl,--wrap=z_impl_k_mutex_lock
17:        -Wl,--wrap=z_impl_k_sem_take
18:        -Wl,--wrap=z_impl_k_sem_give
19:        -Wl,--wrap=z_impl_k_timer_start
20:        -Wl,--wrap=z_impl_k_sleep
21:        -Wl,--wrap=z_impl_k_msgq_put
22:    )
23: 
```
<!-- embed:end -->

- `activation_counter.c` riproduce fuori dal framework lo stesso confronto
  sull'indirizzo e conta, per ciascuna primitiva, le chiamate sull'oggetto
  scelto e quelle sugli altri oggetti;
- `report_hook.c` intercetta la `main` del benchmark con `--wrap=main` e stampa
  il riepilogo al termine, così che il benchmark resti intatto.

Per lanciare l'esperimento:

```bash
west build -p always -b mps2/an385 <percorso>/strumenti/app_kernel
west build -t run
```