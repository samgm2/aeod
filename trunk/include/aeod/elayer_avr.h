/*
 * This is avr version of elayer
 * Please keep thoos _p nad _i as a volatile, else there is 
 * problem with interrupts and compiler.
 * 
 */

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>
#include <avr/wdt.h>
#include <util/twi.h>

#ifndef __ELAYER_H
#define __ELAYER_H									1
	/* None should use this anymore, please make aedin and aeod platform independent. */
#define __ELAYER_AVR								1

#ifndef u8
	typedef unsigned char							u8;
#endif

#ifndef u16
	typedef unsigned short int						u16;
#endif


	/* Theese buffer must be atleast 2 bytes */
#define ELAYER_TXT_BUFF								16
	/* Receive buffer is not so itchy */
#define ELAYER_RECV_BUFF							4


void tiks(u8 i) {
	volatile u8 n;
	for (n = 0;n < i;n ++);
}

	/* miliseconds */
void msleep(u8 i) {
	volatile u8 n;
	for (n = 0;n < i;n ++) {
		tiks(255);
		tiks(255);
		tiks(255);
		tiks(35);
		tiks(255);
		tiks(255);
		tiks(255);
		tiks(35);
	}
}


	/* Show status message */
void elayer_blink(u8 msg) {
	DDRB |= 0x01;
	
	if (msg & 1) PORTB |= 0x01;
	else PORTB &= 255 ^ 0x01;
}

void elayer_error(u8 ecode) {
	__asm("cli");
	
	while(1) {
		msleep(ecode);
		elayer_blink(0);
		msleep(128);
		elayer_blink(1);
	}
}

	/* Not needed */
struct elayer {
	
};


struct elayer_avr_initial {
		/* Receive buffer */
	u8 rec_buff[ELAYER_RECV_BUFF];
		/* Poister which poits current FREE slot */
	volatile u8 rec_p;
		/* Poister which points current getter byte */
	volatile u8 rec_i;
	
	
	
	u8 txt_buff[ELAYER_TXT_BUFF];
		/* Poister which poits current FREE slot */
	volatile u8 txt_p;
		/* Pointer which points current getter */
	volatile u8 txt_i;
} gl_elayer;

#define ELAYER_CTS_DDR								DDRD
#define ELAYER_CTS_PIN								PIND
#define ELAYER_CTS_PORT								PORTD
#define ELAYER_CTS_BIT								0x10
#define ELAYER_CTS									(ELAYER_CTS_PIN & ELAYER_CTS_BIT)

#define ELAYER_RTS_DDR								DDRD
#define ELAYER_RTS_PIN								PIND
#define ELAYER_RTS_PORT								PORTD
#define ELAYER_RTS_BIT								0x08
#define ELAYER_RTS									(ELAYER_RTS_PIN & ELAYER_RTS_BIT)


ISR(USART_UDRE_vect) {
	if (gl_elayer.txt_i != gl_elayer.txt_p) {
		
			/* Nasty way to spin */
		if (ELAYER_CTS) return;
		
		UDR0 = gl_elayer.txt_buff[gl_elayer.txt_p];
		if (gl_elayer.txt_p >= (ELAYER_TXT_BUFF - 1)) gl_elayer.txt_p = 0;
		else gl_elayer.txt_p ++;
	} else {
			/* There is no any stuff to tranfer
			 * - wont wan't this to occur alltime so disable it */
		UCSR0B &= 0x20 ^ 0xFF;
	}
}

	/* Write n bytes */
void elayer_write(struct elayer *ctx,u8 *bytes,u8 len) {
	u8 i;
	for (i = 0;i < len;) {
		if (gl_elayer.txt_i >= (ELAYER_TXT_BUFF - 1)) {
			if (gl_elayer.txt_p != 0) {
				gl_elayer.txt_buff[gl_elayer.txt_i] = bytes[i];
				gl_elayer.txt_i = 0;
				i ++;
			}
		} else {
			if (gl_elayer.txt_p != (gl_elayer.txt_i + 1)) {
				gl_elayer.txt_buff[gl_elayer.txt_i] = bytes[i];
				gl_elayer.txt_i ++;
				i ++;
			}
		}

			/* Set empty int */
		UCSR0B |= 0x20;
	}
}

ISR(USART_RX_vect) {
		/* You can put here you own RTS handler */
	
	
		/* RTS UP */
	ELAYER_RTS_PORT |= ELAYER_RTS_BIT;

	gl_elayer.rec_buff[gl_elayer.rec_p] = UDR0;
	if (gl_elayer.rec_p >= (ELAYER_RECV_BUFF - 1)) {
		gl_elayer.rec_p = 0;
	}
	else {
		gl_elayer.rec_p ++;
	}
	
	
}

	/* Return one byte, this must be non blocking */
u8 elayer_read(struct elayer *ctx,u8 *byte) {
		/* This must not happen */
	if (UCSR0A & 0x10) elayer_error(8);			/* Frame error */
	if (UCSR0A & 0x08) elayer_error(64);		/* Data overrun */
	
	if (gl_elayer.rec_i != gl_elayer.rec_p) {
		*byte = gl_elayer.rec_buff[gl_elayer.rec_i];
		if (gl_elayer.rec_i >= (ELAYER_RECV_BUFF - 1)) {
			gl_elayer.rec_i = 0;
		} else {
			gl_elayer.rec_i ++;
		}
		
			/* RTS down */
		ELAYER_RTS_PORT &= 255 ^ ELAYER_RTS_BIT;
		return 1;
	}
		/* RTS down */
	ELAYER_RTS_PORT &= 255 ^ ELAYER_RTS_BIT;
	return 0;
}

	/* Reset elayer tranfer buffers */
void elayer_reset(struct elayer *ctx) {
	gl_elayer.txt_p = 0;
	gl_elayer.txt_i = 0;
	gl_elayer.rec_p = 0;
	gl_elayer.rec_i = 0;
	UCSR0A &= 255 ^ 0x18;
}


	/* Make hardware reset */
void elayer_hwreset(struct elayer *ctx) {
	__asm("cli");

	
		/* RTS down */
	ELAYER_RTS_PORT &= 255 ^ ELAYER_RTS_BIT;
		/* BT-chip is in reset state */
	PORTD |= 0x04;

	msleep(200);

		/* Up and wait for hunk data */
	PORTD &= 255 ^ 0x04;

	msleep(200);
	msleep(200);
	msleep(200);
	elayer_reset(ctx);
	
	__asm("sei");	
}

	/* Return 1 on success */
char elayer_init_port(struct elayer *ctx) {
	__asm("cli");
		/* Power stuff register */
	PRR = 0x00;
	
		/* Reset, leave it active in boot */
	PORTD |= 0x04;
	DDRD |= 0x04;
	
		/* RXD */
	PORTD &= 255 ^ 0x01;
	DDRD &= 255 ^ 0x01;
	
		/* TXD */
	PORTD |= 0x02;
	DDRD |= 0x02;
	
		/* RTS */
	ELAYER_RTS_DDR |= ELAYER_RTS_BIT;
	ELAYER_RTS_PORT |= ELAYER_RTS_BIT;
	
		/* CTS */
	ELAYER_CTS_DDR &= ELAYER_CTS_BIT ^ 255;
	ELAYER_CTS_PORT &= ELAYER_CTS_BIT ^ 255;
	
	
	UCSR0A = 0x02;		/* Double speed  */
	UCSR0B = 0xB8;		/* 8 B Enable,rx_isr,empty_isr,rx,tx */
	UCSR0C = 0x06;		/* 8-bit */
	
		/* This is for internal oscillator ~8Mhz -> 38400 */
	UBRR0 = 25;
		/* This is for internal oscillator ~16Mhz -> 38400 */
	UBRR0 = 51;
	elayer_hwreset(ctx);
	
	__asm("sei");
	return 0;
}


void elayer_init(struct elayer *ctx) {
	elayer_init_port(ctx);
}


void elayer_free(struct elayer *ctx) {}


#endif /* __ELAYER_H */
