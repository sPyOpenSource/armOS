#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "py/headers/mpconfig.h"
#include "py/headers/stackctrl.h"
#include "py/headers/obj.h"
#include "py/headers/gc.h"
#include "py/headers/nlr.h"
#include "py/headers/compile.h"
#include "py/headers/runtime.h"
#include "py/headers/repl.h"
#include "py/headers/mphal.h"
#include "py/headers/mpstate.h"
#include "lib/utils/pyexec.h"
#include "lib/mp-readline/readline.h"

#include "asf.h"
#include "due_mphal.h"
#include "modpyb.h"
#include "led.h"
#include "modpin.h"
#include "modpinmap.h"
#include "time.h"
#include "modtwi.h"

extern int _end;
static char *stack_top;


void do_str(const char *src, mp_parse_input_kind_t input_kind) {
    mp_lexer_t *lex = mp_lexer_new_from_str_len(MP_QSTR__lt_stdin_gt_, src, strlen(src), 0);
    if (lex == NULL) {
	mp_hal_stdout_tx_str("Error");
        return;
    }

    nlr_buf_t nlr;
    if (nlr_push(&nlr) == 0) {
        qstr source_name = lex->source_name;
        mp_parse_tree_t parse_tree = mp_parse(lex, input_kind);
        mp_obj_t module_fun = mp_compile(&parse_tree, source_name, MP_EMIT_OPT_NONE, true);
        mp_call_function_0(module_fun);
        nlr_pop();
    } else {
        // uncaught exception
        mp_obj_print_exception(&mp_plat_print, (mp_obj_t)nlr.ret_val);
    }
}


void gc_collect(void) {
    // WARNING: This gc_collect implementation doesn't try to get root
    // pointers from CPU registers, and thus may function incorrectly.
    void *dummy;
    gc_collect_start();
    gc_collect_root(&dummy, ((mp_uint_t)stack_top - (mp_uint_t)&dummy) / sizeof(mp_uint_t));
    gc_collect_end();
    // gc_dump_info();
}

mp_lexer_t *mp_lexer_new_from_file(const char *filename) {
    return NULL;
}

mp_import_stat_t mp_import_stat(const char *path) {
    return MP_IMPORT_STAT_NO_EXIST;
}

mp_obj_t mp_builtin_open(size_t n_args, const mp_obj_t *args, mp_map_t *kwargs) {
    return mp_const_none;
}

MP_DEFINE_CONST_FUN_OBJ_KW(mp_builtin_open_obj, 1, mp_builtin_open);

void nlr_jump_fail(void *val) {
	mp_hal_stdout_tx_str("jmp failed");
}

void NORETURN __fatal_error(const char *msg) {
    mp_hal_stdout_tx_str("no rtrn");
    while (1);
}

#ifndef NDEBUG
void MP_WEAK __assert_func(const char *file, int line, const char *func, const char *expr) {
    printf("Assertion '%s' failed, at file %s:%d\n", expr, file, line);
    __fatal_error("Assertion failed");
}
#endif

void mp_keyboard_interrupt(void) {
    MP_STATE_VM(mp_pending_exception) = MP_STATE_PORT(mp_kbd_exception);
}

int main(void)
{
    int stack_dummy;
    stack_top = (char*)&stack_dummy;
    irq_initialize_vectors();
    cpu_irq_enable();
    sleepmgr_init();
    sysclk_init();

    // 1ms systick counter
    init_systick(1000);
    board_init();

    delay_init();
    ADC_init();
    led_init();

    #ifdef ENABLE_TRNG
    init_trng();
    #endif

    #ifdef UART_REPL
    init_uart();
    #endif

    #ifdef USB_REPL
    udc_start();
    #endif

    mp_stack_ctrl_init();
    mp_stack_set_limit(10240);
    #if MICROPY_ENABLE_GC
    gc_init(&_end, &_end + 10240);
    #endif
    mp_init();
    pin_init0();
    MP_STATE_PORT(mp_kbd_exception) = mp_obj_new_exception(&mp_type_KeyboardInterrupt);

    int exit_code = 0;
    for (;;) {
        if (pyexec_mode_kind == PYEXEC_MODE_RAW_REPL) {
            exit_code = pyexec_raw_repl();
        } else {
            exit_code = pyexec_friendly_repl();
        }
        if (exit_code == PYEXEC_FORCED_EXIT) {
            mp_hal_stdout_tx_str("soft reboot\r\n");
            //reset_mp();
        } else if (exit_code != 0) {
            break;
        }
    }
    mp_deinit();
    return 0;
}
