#include <avr/io.h>
#include <avr/pgmspace.h>

#include "hw_uart.h"
// #include "usbdev_cdc.h"
#include "console.h"
#include "watchdog.h"

// When doing calculations per page 174 od atmega168 datasheet,
// make sure we use real numbers and round, not truncate
#define UBRR_VAL ((1.0 * F_CPU / (16.0 * BAUD)) + 0.5 - 1)
#define UBRR_DOUBLESPEED_VAL ((1.0 * F_CPU / (8.0 * BAUD)) + 0.5 - 1)

void hw_uart_init(void)
{
#if __AVR_ATmega168__
#if BAUD > 57600
    // Setup lower divider to get higher precision for high baud rate.
    UCSR0A |= _BV(U2X0);
    UBRR0 = UBRR_DOUBLESPEED_VAL;
#else
    UBRR0 = UBRR_VAL;
#endif
    UCSR0B = _BV(TXEN0) | _BV(RXEN0);
#elif __AVR_ATmega328P__
    UBRR0 = (F_CPU / (16UL * BAUD)) - 1;
    UCSR0B = _BV(TXEN0) | _BV(RXEN0);
#elif __AVR_ATmega324PB__
#if BAUD < 57600
    UBRR1 = (F_CPU / (16UL * BAUD)) - 1;
    UCSR1B = _BV(TXEN) | _BV(RXEN); 
#else
    UCSR1A = _BV(U2X);
    UBRR1 = (F_CPU / (8UL * BAUD)) - 1;
    UCSR1B = _BV(TXEN) | _BV(RXEN);    
#endif   
#elif __AVR_AT90USB162__
    UBRR1 = UBRR_VAL;
    UCSR1B = _BV(TXEN1) | _BV(RXEN1);
#else
#error Unsupported device, FIXME
#endif
}

void hw_uart_tick(void)
{
#ifdef CONFIG_HW_UART_CONSOLE
#if __AVR_ATmega168__
    if ((UCSR0A&(1<<RXC0)) != 0)
    {
        uint8_t c = UDR0;
        console_rx_callback(c);
    }
#elif __AVR_ATmega328P__
    if ((UCSR0A&(1<<RXC0)) != 0)
    {
        uint8_t c = UDR0;
        console_rx_callback(c);
    }
#elif __AVR_ATmega324PB__
    if ((UCSR1A&(1<<RXC)) != 0)
    {
        uint8_t c = UDR1;
        console_rx_callback(c);
    }    
#elif __AVR_AT90USB162__
    if ((UCSR1A&(1<<RXC1)) != 0)
    {
        uint8_t c = UDR1;
        console_rx_callback(c);
    }
#else
#error Unsupported device, FIXME
#endif
#endif
}

void hw_uart_putc(char c)
{
#if __AVR_ATmega168__
    while (bit_is_clear(UCSR0A, UDRE0))
        watchdog_reset();
    UDR0 = c;
#elif __AVR_ATmega328P__
    while (bit_is_clear(UCSR0A, UDRE0))
        watchdog_reset();
    UDR0 = c;
#elif __AVR_ATmega324PB__
    while (bit_is_clear(UCSR1A, UDRE))
        watchdog_reset();
    UDR1 = c;    
#elif __AVR_AT90USB162__
    while (bit_is_clear(UCSR1A, UDRE1))
        watchdog_reset();
    UDR1 = c;
#else
#error Unsupported device, FIXME
#endif
}



