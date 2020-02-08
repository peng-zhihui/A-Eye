/* Copyright 2018 Canaan Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdlib.h>
#include "atomic.h"
#include "clint.h"
#include "dmac.h"
#include "entry.h"
#include "fpioa.h"
#include "platform.h"
#include "plic.h"
#include "syscalls.h"
#include "sysctl.h"
#include "syslog.h"
#include "uart.h"

extern volatile uint64_t g_wake_up[2];

core_instance_t core1_instance;

volatile char *const ram = (volatile char *)RAM_BASE_ADDR;

extern char _heap_start[];
extern char _heap_end[];

void __attribute__((weak)) initialize_kendryte_ide_hook(void) {}

void thread_entry(int core_id)
{
    while(!atomic_read(&g_wake_up[core_id]))
        ;
}

void core_enable(int core_id)
{
    clint_ipi_send(core_id);
    atomic_set(&g_wake_up[core_id], 1);
}

int register_core1(core_function func, void *ctx)
{
    if(func == NULL)
        return -1;
    core1_instance.callback = func;
    core1_instance.ctx = ctx;
    core_enable(1);
    return 0;
}

int __attribute__((weak)) os_entry(int core_id, int number_of_cores, int (*user_main)(int, char **))
{
    /* Call main if there is no OS */
    return user_main(0, 0);
}

void _init_bsp(int core_id, int number_of_cores)
{
    extern int main(int argc, char *argv[]);
    extern void __libc_init_array(void);
    extern void __libc_fini_array(void);

    if(core_id == 0)
    {
        /* Initialize bss data to 0 */
        init_bss();
        /* Init UART */
        fpioa_set_function(4, FUNC_UART3_RX);
        fpioa_set_function(5, FUNC_UART3_TX);
        uart_debug_init(UART_DEVICE_3);
        /* Init FPIOA */
        fpioa_init();
        /* Register finalization function */
        atexit(__libc_fini_array);
        /* Init libc array for C++ */
        __libc_init_array();
        /* Get reset status */
        sysctl_get_reset_status();
        /* Init plic */
        plic_init();
        /* Enable global interrupt */
        sysctl_enable_irq();
        /* Hook entry for kendryte IDE */
        initialize_kendryte_ide_hook();
    }

    int ret = 0;
    if(core_id == 0)
    {
        core1_instance.callback = NULL;
        core1_instance.ctx = NULL;
        ret = os_entry(core_id, number_of_cores, main);
    } else
    {
        plic_init();
        sysctl_enable_irq();
        thread_entry(core_id);
        if(core1_instance.callback == NULL)
            asm volatile("wfi");
        else
            ret = core1_instance.callback(core1_instance.ctx);
    }
    exit(ret);
}

int pthread_setcancelstate(int __state, int *__oldstate)
{
    return 0;
}
