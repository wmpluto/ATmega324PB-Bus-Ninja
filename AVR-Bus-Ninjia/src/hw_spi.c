#include "ninja.h"
#include "hw_spi.h"

void hw_spi_init(void)
{
    uint8_t tmp;

    // SCK and MOSI as output, SS as output - we're master
    SPI_DDR |= _BV(SPI_SCLK) | _BV(SPI_MOSI) | _BV(SPI_SS);
    hw_spi_cs_deassert();

    // Enable MISO pull-up
    SPI_PORT |= _BV(SPI_MISO);

    // F_CPU    // div 128 @ 8Mhz = 62KHz
    SPCR0 = _BV(SPE) | _BV(MSTR) | _BV(SPR0);

    // Clear status flags
    tmp = SPSR0;
    tmp = SPDR0;
}

void hw_spi_shutdown(void)
{
    // disable SPI
    SPCR0 &= ~ _BV(SPE);
}

// clock a byte in and out
uint8_t hw_spi_write8(uint8_t data)
{
    SPDR0 = data;
    while (!(SPSR0 & (1<<SPIF)));
    return SPDR0;
}


