# I2C slave for IR decoding

## Introduction

A simple I2C slave running at an attiny85 for decoding IR signals.

The idea came when I wanted to control using IR an arduino and I found out that
IR decoding would take an extremely big amount of time. Thus I came up with
using an attiny85 that would decode IR signals, buffer them, and would be
controlled using I2C. There is also support for sending an interrupt whenever
a new signal is decoded.

## Compilation

There are several options that you can change.

 * PIN\_IR: This is the pin where the signal output from your IR receiver is
   connected.
 * PIN\_INT: This is the pin that will be brought high every time a new signal
   is decoded.
 * INT\_DELAY: The amount of time in msec that the pin will be brought high.
 * BUF\_LEN: The length of the circular buffer than will store received IR
   signals.
 * I2C\_ADDR: The address of the I2C slave.

Most probably you will only need to change the address of the I2C slave, if
there is another I2C slave with the same address.

If you only plan to use a specific type of remote, you can also change the
supported decoders at lib/IRremote/IRremote.h.

## Usage

There are 5 different registers you can read and write to perform different
funtions:

 * 0x01: Reading from this register will return a single byte which is the
   amount of decoded signals in the buffer.
 * 0x02: Reading from this register will return the next signal. The signal
   is encoded as 5 bytes. The first byte is the type and the rest are the value
   sent with the most significant byte first. For type and value, also check
   the IRremote library
 * 0x03: Writing the value 0 to this register will disable sending an interrupt
   using pin PIN\_INT, and any other value will enable it. By default interrupts
   are disabled.
 * 0x04: Whenever an interrupt is sent due to a received signal, a flag is set
   that will block more interrupts from being sent until a signal is read.
   Writing this register will allow you to clear this flag, and start sending
   interrupts again.
 * 0x05: Writing this register will reset the decoder.

## Notes

I have made several tests, and I had no problems using this library. Some
decoding errors were also there when using the IRremote library at an arduino.

Most of my tests showed that the device performs better when using an 16 Mhz
clock. You can enable the internal high frequency PLL clock by setting the
following values for the fuses:

 * lfuse: 0xc1
 * hfuse: 0xdf
 * efuse: 0xfe

For avrdude, you should add the following options:

```-U lfuse:w:0xc1:m -U hfuse:w:0xdf:m -U efuse:w:0xfe:m```

If you use a NEC remote control, the REPEAT code is translated to the previous
IR received.

## License

The code is licensed under the BSD 3-clause "New" or "Revised" License.
