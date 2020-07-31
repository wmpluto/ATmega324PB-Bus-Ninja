# ATmega324PB-Bus-Ninja

Port the [tobyjaffey](https://github.com/tobyjaffey)'s [bus-ninja](https://github.com/tobyjaffey/bus-ninja) to ATmega324PB Xplained Pro board. 

## Features

- [x] Hardware I2C (default 100kHz, up to 400kHz)
- [x] I2C address scanner ('s' command from i2c mode)
- [x] SPI (default 500kHz, up to 4mHz@3.3V, 8MHz@5V)
- [ ] PWM
- [ ] GPIO
- [ ] ADC

## Usage

### UART Setting

Default baud rate: 1Mbps@8Mhz SYS_CLK

### SPI

#### Connection

<table>
<tr><th>Function</th><th>ATmega324PB Pin</th></tr>
<tr><td>SPI CLK</td><td>PORTB7</td></tr>
<tr><td>SPI MISO</td><td>PORTB6</td></tr>
<tr><td>SPI MOSI</td><td>PORTB5</td></tr>
<tr><td>SPI CS</td><td>PORTB</td></tr>
<tr><td>LED</td><td>PORTC7</td></tr>
</table>


#### Syntax

    [	Chip select (CS) active (low).
    {	CS active (low), show the SPI read byte after every write.
    ] or }	CS disable (high).
    r	Read one byte by sending dummy byte (0xff). (r:1…255 for bulk reads)
    0b	Write this binary value. Format is 0b00000000 for a byte, but partial bytes are also fine: 0b1001.
    0x	Write this HEX value. Format is 0×01. Partial bytes are fine: 0xA. A-F can be lower-case or capital letters.
    0-255	Write this decimal value. Any number not preceded by 0x or 0b is interpreted as a decimal value.
    
#### Example
    > spi
    > [0x40 0x0A 0x28]
    CS ENABLED
    WRITE: 0x40
    WRITE: 0x0A
    WRITE: 0x28
    CS DISABLED
    > [0x40 r]
    CS ENABLED
    WRITE: 0x40
    READ: 0x00
    CS DISABLED  
    > {0x40 r:2}
    CS ENABLED
    WRITE: 0x40
    READ: 0x00
    READ: 0x01
    READ: 0x02
    CS DISABLED
    
### I2C

#### Connection

<table>
<tr><th>Function</th><th>ATmega324PB Pin</th></tr>
<tr><td>I2C SDA</td><td>PORTE5</td></tr>
<tr><td>I2C SCL</td><td>PORTE6</td></tr>
</table>

#### Syntax

    { or [	Issue I2C start condition.
    ] or }	Issue I2C stop condition.
    r	Read one byte, send ACK. (r:1…255 for bulk reads)
    s   I2C address scanner
    0b	Write this binary value, check ACK. Format is 0b00000000 for a byte, but partial bytes are also fine: 0b1001.
    0x	Write this HEX value, check ACK. Format is 0×01. Partial bytes are fine: 0xA. A-F can be lower-case or capital letters.
    0-255	Write this decimal value, check ACK. Any number not preceded by 0x or 0b is interpreted as a decimal value.
    
#### Example
