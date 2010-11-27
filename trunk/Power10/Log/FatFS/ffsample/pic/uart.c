#include <p24FJ64GA002.h>
#include "pic24f.h"
#include "uart.h"

#define BUFFER_SIZE 128
#define BPS 		115200UL


static volatile int TxRun;
static volatile struct {
	int		ri, wi, ct;
	BYTE	buff[BUFFER_SIZE];
} TxFifo, RxFifo;




/* UART Rx interrupt ISR */

void __attribute__((interrupt, auto_psv)) _U1RXInterrupt (void)
{
	BYTE d;
	int i;


	d = (BYTE)U1RXREG;
	_U1RXIF = 0;
	i = RxFifo.ct;
	if (i < BUFFER_SIZE) {
		i++;
		RxFifo.ct = i;
		i = RxFifo.wi;
		RxFifo.buff[i++] = d;
		RxFifo.wi = i % BUFFER_SIZE;
	}
}



/* UART Tx interrupt ISR */

void __attribute__((interrupt, auto_psv)) _U1TXInterrupt (void)
{
	int i;


	_U1TXIF = 0;		/* Clear interrupt flag */

	i = TxFifo.ct;	/* Number of data in the FIFO */
	if (i) {			/* If any data is available, pop a byte and send it. */
		i--;
		TxFifo.ct = i;
		i = TxFifo.ri;
		U1TXREG = TxFifo.buff[i++];
		TxFifo.ri = i % BUFFER_SIZE;
	} else {			/* No data in the FIFO. Stop transmission. */
		TxRun = 0;
	}
}



/* Check number of bytes in the Rx FIFO */

int uart_test (void)
{
	return RxFifo.ct;
}



/* Get a byte from Rx FIFO */

BYTE uart_get (void)
{
	BYTE d;
	int i;


	while (!RxFifo.ct) ;		/* Wait while Rx FIFO empty */

	i = RxFifo.ri;			/* Pop a byte from Rx FIFO */
	d = RxFifo.buff[i++];
	RxFifo.ri = i % BUFFER_SIZE;
	_DI();
	RxFifo.ct--;
	_EI();

	return d;
}



/* Put a byte into Tx FIFO */

void uart_put (BYTE d)
{
	int i;


	while (TxFifo.ct >= BUFFER_SIZE) ;	/* Wait while Tx FIFO full */

	i = TxFifo.wi;		/* Push data into Tx FIFO */
	TxFifo.buff[i++] = d;
	TxFifo.wi = i % BUFFER_SIZE;
	_DI();
	TxFifo.ct++;
	if (!TxRun) {	/* If Tx is not running, generate 1st Tx interrupt */
		TxRun = 1;
		_U1TXIF = 1;
	}
	_EI();
}



/* Initialize UART module */

void uart_init (void)
{
	/* Disable UART1 Tx/Rx interrupts */
	_U1RXIE = 0;
	_U1TXIE = 0;

	/* Initialize UART1 */
	U1BRG = FCY / 16 / BPS - 1;
	_UARTEN = 1;
	_UTXEN = 1;

	/* Clear Tx/Rx FIFOs */
	TxFifo.ri = 0; TxFifo.wi = 0; TxFifo.ct = 0;
	RxFifo.ri = 0; RxFifo.wi = 0; RxFifo.ct = 0;
	TxRun = 0;

	/* Enable UART Tx/Rx interruptrs */
	_U1RXIE = 1;
	_U1TXIE = 1;
}

