/*
 * AVR-Bus-Ninjia.c
 *
 * Created: 2020/7/17 11:18:06
 * Author : A19671
 */ 

#include <avr/io.h>
#include <util/delay.h>

int main(void)
{
    DDRB = 1 << 5;
    
    /* Replace with your application code */
    while (1) 
    {
        PORTB ^= 1 << 5;
        _delay_ms(500);
    }
}

