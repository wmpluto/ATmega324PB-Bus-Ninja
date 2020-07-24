#include "console.h"
#include "ninja.h"
#ifdef CONFIG_HW_UART
#include "hw_uart.h"
#endif
#include "commands.h"
#include <string.h>

#define CMDBUF_SIZ 128
#define HISTORY_SIZ 5

#if CMDBUF_SIZ > 255
#error Commands over 255 bytes not supported, change type of cmdbuf_len
#endif

/**************************************************************/

static putcfunc_t putcf = NULL;

/**************************************************************/

static console_mode_t console_mode;

/**************************************************************/ // FIXME, TODO, union'ise these to save RAM?
// CONSOLE_MODE_LINE
static uint8_t cmdbuf_len; // number of bytes in command buf
static uint8_t cmdbuf[CMDBUF_SIZ];
static BOOL got_line = FALSE;
static BOOL echo = TRUE;
static BOOL silent = FALSE;
static uint8_t historybuf[HISTORY_SIZ][CMDBUF_SIZ];
static uint8_t historypoint;
/**************************************************************/
// CONSOLE_MODE_KEY
static BOOL got_key = FALSE;
static uint8_t last_key = 0x00; // last character received
/**************************************************************/

static uint8_t spcial_key = 0x00;

static void prompt(void)
{
    console_puts_P(PSTR("\r\nbus-ninja> "));
}

void console_init(void)
{
    console_set_mode(CONSOLE_MODE_LINE);

    memset(cmdbuf, 0, sizeof(cmdbuf));
    cmdbuf[0] = 0x20;

    memset(historybuf, 0, sizeof(historybuf));
    historypoint = 0;
}

void console_set_mode(console_mode_t mode)
{
    console_mode = mode;

    switch (mode) {
    case CONSOLE_MODE_LINE:
        cmdbuf_len = 0;
        got_line = FALSE;
        break;
    case CONSOLE_MODE_KEY:
        got_key = FALSE;
        break;
    }
}

BOOL console_key_poll(uint8_t* c)
{
    if (got_key) {
        *c = last_key;
        got_key = FALSE;
        return TRUE;
    }
    return FALSE;
}

BOOL console_rx_ready_callback(void)
{
    switch (console_mode) {
    case CONSOLE_MODE_LINE:
        return !got_line;

    default:
    case CONSOLE_MODE_KEY:
        return TRUE;
    }
}

void console_rx_callback(uint8_t c)
{
    switch (console_mode) {
    case CONSOLE_MODE_KEY:
        got_key = TRUE;
        last_key = c;
        break;

    case CONSOLE_MODE_LINE:
        if (got_line) // throw away chars until the line is handled
            return;

        switch (c) {
        case 0x0D:
            //case '\r':
            got_line = TRUE;
            for (uint8_t ii = HISTORY_SIZ - 1; ii > 0; ii--)
                memcpy(historybuf[ii], historybuf[ii - 1], CMDBUF_SIZ);
            memcpy(historybuf, cmdbuf, CMDBUF_SIZ);
            historypoint = 0;
            console_newline();
            break;
        case '\b': // backspace
        case 0x7F: // del
            if (cmdbuf_len > 0) {
                cmdbuf_len--;
                for (uint8_t ii = cmdbuf_len; ii < CMDBUF_SIZ; ii++)
                    cmdbuf[ii] = cmdbuf[ii + 1];
                console_putc('\b');

                for (uint8_t ii = cmdbuf_len; ii < CMDBUF_SIZ && cmdbuf[ii]; ii++) {
                    console_putc(cmdbuf[ii]);
                }
                console_putc(' ');
                console_putc('\b');
                for (uint8_t ii = cmdbuf_len; ii < CMDBUF_SIZ && cmdbuf[ii]; ii++) {
                    console_puts_P(PSTR("\x1B[D"));
                }
            }
            break;
        case '\x1B':
            spcial_key = 0x01;
            break;
        default:
            if (spcial_key == 0x00) {
                if (c >= 0x20 && c <= 0x7E) {
                    if (cmdbuf_len < sizeof(cmdbuf) - 1) {
                        if (echo)
                            console_putc(c);
                        for (uint8_t ii = CMDBUF_SIZ - 1; ii > cmdbuf_len; ii--)
                            cmdbuf[ii] = cmdbuf[ii - 1];
                        cmdbuf[cmdbuf_len++] = c;

                        for (uint8_t ii = 0; ii < CMDBUF_SIZ; ii++) {
                            if (cmdbuf[ii] == 0) {
                                cmdbuf[ii] = cmdbuf[ii - 1] == 0x20 ? 0 : 0x20;
                                break;
                            }
                        }

                        for (uint8_t ii = cmdbuf_len; ii < CMDBUF_SIZ && cmdbuf[ii]; ii++) {
                            console_putc(cmdbuf[ii]);
                        }

                        for (uint8_t ii = cmdbuf_len; ii < CMDBUF_SIZ && cmdbuf[ii]; ii++) {
                            console_puts_P(PSTR("\x1B[D"));
                        }
                    } else
                        console_putc('\a'); // bell
                }
            } else if (spcial_key == 0x01) {
                if (c == '[') {
                    spcial_key = 0x02;
                } else {
                    spcial_key = 0x00;
                }
            } else if (spcial_key == 0x02) {
                spcial_key = 0x00;
                switch (c) {
                case 'A': // up
                    for (uint8_t ii = 0; ii < cmdbuf_len; ii++)
                        console_puts_P(PSTR("\x1B[D"));
                    console_puts_P(PSTR("\33[0K"));

                    cmdbuf_len = 0;
                    memcpy(cmdbuf, historybuf[historypoint % HISTORY_SIZ], CMDBUF_SIZ);
                    for (uint8_t ii = 0; ii < CMDBUF_SIZ && cmdbuf[ii]; ii++) {
                        console_putc(cmdbuf[ii]);
                        cmdbuf_len++;
                    }
                    historypoint++;
                    break;
                case 'B': // down
                    if (historypoint > 0) {
                        for (uint8_t ii = 0; ii < cmdbuf_len; ii++)
                            console_puts_P(PSTR("\x1B[D"));
                        console_puts_P(PSTR("\33[0K"));

                        cmdbuf_len = 0;
                        memcpy(cmdbuf, historybuf[historypoint % HISTORY_SIZ], CMDBUF_SIZ);
                        for (uint8_t ii = 0; ii < CMDBUF_SIZ && cmdbuf[ii]; ii++) {
                            console_putc(cmdbuf[ii]);
                            cmdbuf_len++;
                        }

                        historypoint--;
                    }
                    break;
                case 'C': // right
                    if (cmdbuf_len < CMDBUF_SIZ - 1 && cmdbuf[cmdbuf_len + 1] != 0) {
                        cmdbuf_len++;
                        console_puts_P(PSTR("\x1B[C"));
                    }
                    break;
                case 'D': // left
                    if (cmdbuf_len > 0) {
                        cmdbuf_len--;
                        console_puts_P(PSTR("\x1B[D"));
                    }
                    break;
                case '3':
                    spcial_key = 0x03;
                    break;
                default:
                    break;
                }
            } else if (spcial_key == 0x03) {
                spcial_key = 0x00;
            }

            break;
        }
        break;
    }
}

void console_tick(void)
{
    switch (console_mode) {
    case CONSOLE_MODE_LINE:
        if (got_line) {
            uint8_t ii = 0;
            for (ii = 0; ii < CMDBUF_SIZ; ii++)
                if (cmdbuf[ii] == 0)
                    break;
            if (ii > 0)
                execute_command_line(cmdbuf, ii);
            memset(cmdbuf, 0, sizeof(cmdbuf));
            cmdbuf_len = 0;
            prompt();
            got_line = FALSE;
        }
        break;
    default:
        break;
    }
}

void console_set_putc(putcfunc_t f)
{
    putcf = f;
}

void console_putc(uint8_t c)
{
    if (!silent) {
#ifdef CONFIG_HW_UART_CONSOLE
        hw_uart_putc(c);
#endif
        if (NULL != putcf)
            (*putcf)(c);
    }
}

void console_set_echo(BOOL newecho)
{
    echo = newecho;
}

void console_set_silent(BOOL newsilent)
{
    silent = newsilent;
}

void console_newline(void)
{
    console_putc('\r');
    console_putc('\n');
}

void console_puts(uint8_t* str)
{
    while (*str != 0)
        console_putc(*str++);
}

void console_puts_P(PGM_P str)
{
    while (pgm_read_byte(str) != 0)
        console_putc(pgm_read_byte(str++));
}

static char nibble_to_char(uint8_t nibble)
{
    if (nibble < 0xA)
        return nibble + '0';
    return nibble - 0xA + 'A';
}

void console_puthex8(uint8_t h)
{
    console_putc(nibble_to_char((h & 0xF0) >> 4));
    console_putc(nibble_to_char(h & 0x0F));
}

void console_puthex16(uint16_t h)
{
    console_puthex8((h & 0xFF00) >> 8);
    console_puthex8(h & 0xFF);
}

void console_put0x8(uint8_t h)
{
    console_putc('0');
    console_putc('x');
    console_puthex8(h);
}

void console_putsmem(const uint8_t* a, const uint8_t* b)
{
    while (a != b) {
        console_putc(*a);
        a++;
    }
}

void console_putdec(uint32_t n)
{
    uint32_t m;
    BOOL in_leading_zeroes = TRUE;

    for (m = 1000000000; m != 1; m /= 10) {
        if ((n / m) != 0)
            in_leading_zeroes = FALSE;
        if (!in_leading_zeroes)
            console_putc(nibble_to_char(n / m));
        n = n % m;
    }
    console_putc(nibble_to_char(n));
}

void console_putbin(uint8_t b)
{
    console_putc('b');
    (b & 0x80) ? console_putc('1') : console_putc('0');
    (b & 0x40) ? console_putc('1') : console_putc('0');
    (b & 0x20) ? console_putc('1') : console_putc('0');
    (b & 0x10) ? console_putc('1') : console_putc('0');
    (b & 0x08) ? console_putc('1') : console_putc('0');
    (b & 0x04) ? console_putc('1') : console_putc('0');
    (b & 0x02) ? console_putc('1') : console_putc('0');
    (b & 0x01) ? console_putc('1') : console_putc('0');
}
