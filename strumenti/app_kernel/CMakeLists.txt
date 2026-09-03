# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(app_kernel)

set(SWIFI_STRUMENTI ${CMAKE_CURRENT_SOURCE_DIR}/..)

FILE(GLOB app_sources src/*.c)
target_sources(app PRIVATE ${app_sources}
                           ${SWIFI_STRUMENTI}/activation_counter.c
                           ${SWIFI_STRUMENTI}/report_hook.c)

zephyr_link_libraries(
       -Wl,--wrap=main
       -Wl,--wrap=z_impl_k_mutex_lock
       -Wl,--wrap=z_impl_k_sem_take
       -Wl,--wrap=z_impl_k_sem_give
       -Wl,--wrap=z_impl_k_timer_start
       -Wl,--wrap=z_impl_k_sleep
       -Wl,--wrap=z_impl_k_msgq_put
   )
