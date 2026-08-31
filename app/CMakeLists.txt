cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(swifi_framework)


target_sources(app PRIVATE src/main.c src/fi_msgq.c src/fi_core.c src/timer_fn.c src/fi_sem.c src/hook.c)
target_compile_options(app PRIVATE -Wmissing-field-initializers -Werror)
zephyr_link_libraries(-Wl,--wrap=main
                      -Wl,--wrap=z_impl_k_msgq_put
                      -Wl,--wrap=z_impl_k_sem_take)


                      


