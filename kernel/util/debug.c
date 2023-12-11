/******************************************************************************/
/* Important Fall 2023 CSCI 402 usage information:                            */
/*                                                                            */
/* This fils is part of CSCI 402 kernel programming assignments at USC.       */
/*         53616c7465645f5fd1e93dbf35cbffa3aef28f8c01d8cf2ffc51ef62b26a       */
/*         f9bda5a68e5ed8c972b17bab0f42e24b19daa7bd408305b1f7bd6c7208c1       */
/*         0e36230e913039b3046dd5fd0ba706a624d33dbaa4d6aab02c82fe09f561       */
/*         01b0fd977b0051f0b0ce0c69f7db857b1b5e007be2db6d42894bf93de848       */
/*         806d9152bd5715e9                                                   */
/* Please understand that you are NOT permitted to distribute or publically   */
/*         display a copy of this file (or ANY PART of it) for any reason.    */
/* If anyone (including your prospective employer) asks you to post the code, */
/*         you must inform them that you do NOT have permissions to do so.    */
/* You are also NOT permitted to remove or alter this comment block.          */
/* If this comment block is removed or altered in a submitted file, 20 points */
/*         will be deducted.                                                  */
/******************************************************************************/

#include "util/debug.h"
#include "util/string.h"
#include "util/printf.h"

#include "main/io.h"
#include "main/interrupt.h"

#include "kernel.h"

/* Below is a truly terrible poll-driven serial driver that we use for debugging
 * purposes - it outputs to COM1, but
 * this can be easily changed. It does not use interrupts, and cannot read input
 * */
/* This port is COM1 */
#define PORT 0x3f8
/* Corresponding interrupt vector */
#define PORT_INTR 0x0d

uint64_t dbg_modes;

typedef struct dbg_mode {
        const char *d_name;
        uint64_t d_mode;
        const char *d_color;
} dbg_mode_t;

void dbg_init()
{
        outb(PORT + 3, 0x80);   /* Enable DLAB (set baud rate divisor) */
        outb(PORT + 0, 0x03);   /* Set divisor to 3 (lo byte) 38400 baud */
        outb(PORT + 1, 0x00);   /*                  (hi byte) */
        outb(PORT + 3, 0x03);   /* 8 bits, no parity, one stop bit */
        outb(PORT + 2, 0xC7);   /* Enable FIFO, clear them, with 14-byte threshold */

        dbg_modes = DBG_DEFAULT;
        dbg_add_modes(QUOTE(__DBG__));
}

static dbg_mode_t dbg_tab[] = {
        DBG_TAB
};

const char *dbg_color(uint64_t d_mode)
{
        dbg_mode_t *mode;
        for (mode = dbg_tab; mode->d_mode != 0UL; mode++) {
                if (mode->d_mode & d_mode)
                        return mode->d_color;
        }
        /* If we get here, something went seriously wrong */
        panic("Unknown debug mode 0x%llx\n", d_mode);
        return NULL;
}

static void dbg_puts(char *c)
{
        while (*c != '\0') {
                /* Wait until the port is free */
                while (!(inb(PORT + 5) & 0x20));
                outb(PORT, *c++);
        }
}

#define BUFFER_SIZE 1024
void dbg_print(char *fmt, ...)
{
        va_list args;
        char buf[BUFFER_SIZE];
        int count;

        va_start(args, fmt);
        count = vsnprintf(buf, BUFFER_SIZE, fmt, args);
        va_end(args);

        if (count >= BUFFER_SIZE) {
                dbg_puts("WARNING: The following message has been "
                         " truncated due to buffer size limitations.\n");
        }
        dbg_puts(buf);
}

void dbg_printinfo(dbg_infofunc_t func, const void *data)
{
        char buf[BUFFER_SIZE];
        func(data, buf, BUFFER_SIZE);
        dbg_puts(buf);
}

/**
 * searches for <code>name</code> in the list of known
 * debugging modes specified above and, if it
 * finds <code>name</code>, adds the corresponding
 * debugging mode to a list
 */
void dbg_add_mode(const char *name)
{
        int cancel;
        dbg_mode_t *mode;

        if (*name == '-') {
                cancel = 1;
                name++;
        } else {
                cancel = 0;
        }

        for (mode = dbg_tab; mode->d_name != NULL; mode++)
                if (strcmp(name, mode->d_name) == 0)
                        break;
        if (mode->d_name == NULL) {
                dbg_print("Warning: Unknown debug option: \"%s\"\n", name);
                return;
        }

        if (cancel) {
                dbg_modes &= ~mode->d_mode;
        } else {
                dbg_modes |= mode->d_mode;
        }

}

/**
 * Cycles through each comma-delimited debugging option and
 * adds it to the debugging modes by calling dbg_add_mode
 */
void dbg_add_modes(const char *modes)
{
        char env[256];
        char *name;

        strncpy(env, modes, sizeof(env));
        /* Maybe it would be good if we did this without strtok, but I'm too lazy */
        for (name = strtok(env, ","); name; name = strtok(NULL, ","))
                dbg_add_mode(name);
}

size_t dbg_modes_info(const void *data, char *buf, size_t size)
{
        KASSERT(NULL == data);
        KASSERT(0 < size);

        size_t osize = size;

        dbg_mode_t *mode;
        for (mode = dbg_tab; mode->d_name != NULL; ++mode) {
                if (dbg_modes &mode->d_mode && mode->d_mode != DBG_ALL) {
                        int len;
                        if ((len = snprintf(buf, size, "%s,", mode->d_name)) >= (int)size) {
                                break;
                        } else {
                                buf += len;
                                size -= len;
                        }
                }
        }

        if (size == osize) {
                buf[0] = '\0';
                return 0;
        } else {
                /* remove trailing comma */
                buf[-1] = '\0';
                return osize - size + 1;
        }
}

/* This is meant as a good point to automatically set a breakpoint which will stop
 * just after a panic has occured and printed its message. */
static void dbg_panic_halt()
{
        __asm__ volatile("cli; hlt");
}

#define PANIC_BUFSIZE 2048
void dbg_panic(const char *file, int line, const char *func, const char *fmt, ...)
{
        char buf[PANIC_BUFSIZE];
        va_list args;
        va_start(args, fmt);

        dbg_print("panic in %s:%u %s(): ", file, line, func);
        vsnprintf(buf, PANIC_BUFSIZE, fmt, args);
        dbg_print("%s", buf);
        dbg_print("\nKernel Halting.\n\n");

        va_end(args);

        dbg_panic_halt();
}
