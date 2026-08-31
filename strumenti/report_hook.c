/*
 * Aggancio per la stampa del riepilogo di activation_counter.c
 *
 * Il benchmark di terze parti non chiama activation_report(), e non deve farlo:
 * modificarne i sorgenti toglierebbe senso alla misura. Si intercetta quindi la
 * sua main con la stessa interposizione al collegamento usata sulle primitive,
 * cosi' il benchmark resta intatto alla lettera.
 *
 * Per usarlo: aggiungerlo a target_sources e aggiungere al CMakeLists
 *
 *  zephyr_link_libraries(-Wl,--wrap=main)
 */

extern int __real_main(void);

void activation_report(void);

int __wrap_main(void){
    int ret = __real_main();
    activation_report();
    return ret;
}
