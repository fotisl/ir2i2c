/*
 * IR 2 I2C
 * Since IR decoding can sometimes take too much time, I decided to implement
 * a decoder on an ATtiny85. It acts as an I2C slave.
 */
#include <Arduino.h>
/*
 * We need to remove some all send functions, and some protocols to save space.
 */
#include <IRremote.h>
#include <TinyWireS.h>
#include <avr/wdt.h>

/* Pin where the IR receiver is connected */
#define PIN_IR          1
/* Pin where the interrupt will be sent */
#define PIN_INT         3
/* How long will the interrupt high last (shouldn't be longer than watchdog) */
#define INT_DELAY       5
/* Length of the buffer (how many codes can be saved before overflowing) */
#define BUF_LEN         10

/*
 * Address of the slave, choose something that doesn't conflict with the rest
 * of your hardware
 */
#define I2C_ADDR        0x10
/* You should probably leave the following as-is */
#define I2C_REG_BUFLEN  0x01
#define I2C_REG_READ    0x02
#define I2C_REG_SETINT  0x03
#define I2C_REG_INTFLG  0x04
#define I2C_REG_RESET   0x05

struct result {
    uint8_t type;
    uint32_t value;
};

IRrecv irrecv(PIN_IR);
decode_results prev, res;
volatile uint8_t reg, opt;
volatile bool intenabled, intsent;
struct result buf[BUF_LEN];
uint8_t start, end;

inline uint8_t getbuflen()
{
    if(start <= end)
        return (end - start);
    else
        return (BUF_LEN + end - start);
}

inline void pushbuf(struct result r)
{
    buf[end] = r;
    end = (end + 1) % BUF_LEN;
}

inline struct result popbuf()
{
    struct result r = buf[start];
    start = (start + 1) % BUF_LEN;
    return r;
}

void receiveEvent(uint8_t howMany)
{
    if(howMany < 1)
        return;

    /* First byte is the register */
    reg = TinyWireS.receive();

    /* If there is an option, receive it */
    if(TinyWireS.available())
        opt = TinyWireS.receive();
    else
        opt = 0;

    /* And we ignore the rest */
    while(TinyWireS.available())
        (void) TinyWireS.receive();

    if(reg == I2C_REG_SETINT) {
        if(opt == 0)
            intenabled = false;
        else
            intenabled = true;
        return;
    }

    if(reg == I2C_REG_INTFLG) {
        intsent = false;
    }

    if(reg == I2C_REG_RESET) {
        /* Wait for watchdog to reset us */
        wdt_enable(WDTO_15MS);
        while(1) ;
    }
}

void requestEvent()
{
    struct result res;

    if(reg == I2C_REG_BUFLEN) {
        TinyWireS.send(getbuflen());
        return;
    }

    if(reg == I2C_REG_READ) {
        if(getbuflen() == 0) {
            res.type = 0;
            res.value = 0;
        } else {
            res = popbuf();
        }
        TinyWireS.send(res.type);
        TinyWireS.send(res.value >> 24);
        TinyWireS.send((res.value >> 16) & 0xff);
        TinyWireS.send((res.value >> 8) & 0xff);
        TinyWireS.send(res.value & 0xff);
        intsent = false;
    }
}

void setup()
{
    wdt_enable(WDTO_2S);

    pinMode(PIN_IR, INPUT);
    pinMode(PIN_INT, OUTPUT);
    digitalWrite(PIN_INT, LOW);

    wdt_reset();

    TinyWireS.begin(I2C_ADDR);
    TinyWireS.onReceive(receiveEvent);
    TinyWireS.onRequest(requestEvent);

    wdt_reset();

    intenabled = intsent = false;
    irrecv.enableIRIn();
}

void loop()
{
    struct result newinput;

    TinyWireS_stop_check();
    wdt_reset();

    if(irrecv.decode(&res)) {
        if((res.decode_type == NEC) && (res.value == REPEAT))
            res = prev;
        prev = res;
        newinput.type = res.decode_type;
        newinput.value = res.value;
        pushbuf(newinput);
        if((intenabled == true) && (intsent == false)) {
            wdt_reset();
            digitalWrite(PIN_INT, HIGH);
            delay(INT_DELAY);
            digitalWrite(PIN_INT, LOW);
            intsent = true;
        }
        irrecv.resume();
    }
}
