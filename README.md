# SWIFI su `k_msgq`

Iniezione di guasti software sulle code di messaggi del kernel Zephyr, mediante
interposizione a tempo di link (`-Wl,--wrap=z_impl_k_msgq_put`).

## Ambiente

| | |
|---|---|
| Zephyr | `v4.4.0-8541-gdf1fb529e38f` |
| west | 1.5.0 |
| Zephyr SDK | 1.0.1 |
| Board | `mps2/an385` (QEMU, ARM Cortex-M3) |

## Build ed esecuzione di una singola configurazione

```bash
west build -p always -b mps2/an385 <percorso>/esperimento-swifi-msgq/app
west build -t run
```

| Opzione | Significato |
|---|---|
| `CONFIG_FI_ENABLE` | attiva l'iniezione (`n` = esecuzione di riferimento) |
| `CONFIG_FI_VALUE` | valore corrotto scritto in `msg_size` |
| `CONFIG_FI_TRIGGER` | attivazione su cui iniettare |
| `CONFIG_ASSERT` | asserzioni del kernel |

## Campagna

Gli scenari sono definiti in `app/testcase.yaml`. Per eseguirli:

```bash
./scripts/twister -p mps2/an385 -T esperimento-swifi-msgq/app -v --inline-logs
```
