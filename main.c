#include <msp430.h>

unsigned char byte_tab[];
unsigned int value;

void clock_config(void);
void i2c_init(void);

main()
{
    WDTCTL = WDTPW | WDTHOLD;   // Stop Watchdog Timer

    P1DIR |= BIT0;
    P1OUT &= ~0x01;

    clock_config();
    i2c_init();

    value = 1000;               // 0 - 4095 (12-Bit DAC)
    byte_tab[0] = (value >> 8);        // 8 most significant bits
                                       // 2nd byte (Fast Mode Command - C2 and C1 = 0 + Power Down Select as Normal Mode - PD1 and PD0 = 0 + D11 and D10 = 0 + D9 + D8)

    byte_tab[1] = value & 0x00FF;      // 8 least significant bits
                                       // 3rd byte (D7 + D6 + D5 + D4 + D3 + D2 + D1 + D0)

    while (UCB0STAT & UCBBUSY);

    IE2 |= UCB0TXIE;                // USCI_B0 transmit interrupt enable
    UCB0CTL1 |= UCTR + UCTXSTT;     /* UCTR - Transmit/Receive Select/Flag - Transmitter */
                                    /* UCTXSTT - Transmit START */

    __bis_SR_register(GIE);         // Enter interrupt mode
}

void clock_config(void)
{
    BCSCTL1 = CALBC1_8MHZ;      // 8MHz DCO Frequency (Calibrated DCOCTL and
    DCOCTL  = CALDCO_8MHZ;      // BCSCTL1 register settings)
}

void i2c_init() {
    P1SEL  |= BIT6 | BIT7;      // P1.6 - USCI_B0 I2C mode: SCL I2C clock
    P1SEL2 |= BIT6 | BIT7;      // P1.7 - USCI_B0 I2C mode: SDA I2C data
                                // UCB0SCL - P1SEL.x (1) - P1SEL2.x (1)
                                // UCB0SDA - P1SEL.x (1) - P1SEL2.x (1)

    UCB0CTL1 |= UCSWRST;        /* USCI Software Reset */
                                // Peripheral disable

    UCB0CTL0 |= UCMST + UCMODE_3 + UCSYNC;
                                /* UCSLA10 - 10-bit Slave Address Mode */
                                /* UCMST - Master Select */
                                /* UCMODE_3 - USCI Mode: 3 - I2C */
                                /* UCSYNC - Sync-Mode  0:Asynchronous / 1:Synchronous */

    UCB0CTL1 |= UCSSEL_2 + UCTR + UCSWRST;
                                /* UCSSEL_2 - USCI 0 Clock Source: 2 - SMCLK */
                                /* UCTR - Transmit/Receive Select/Flag - Transmitter */

    UCB0BR0 = 0x50;             /* USCI B0 Baud Rate 0 */

    UCB0BR1 = 0x00;             /* USCI B0 Baud Rate 1 */
                                // fBitClock = fBRCLK/UCBRx 8000000
                                // 100000 = 8000000/UCBRx
                                // UCBRx = 80
                                // UCBRx = UCB0BR0 + UCB0BR1 * 256
                                // 80 = UCB0BR0 + 0
                                // UCBR0 = 80d = 0x50
                                // I2C Standard mode = 100kHz

    UCB0I2CSA = 0x60;           // MCP4725 address (7-bit address only)
                                // R/W bit sent by MSP430 (UCTR)

    UCB0CTL1 &= ~UCSWRST;       // Takes I2C out of SoftWare ReSeT
}

#pragma vector = USCIAB0TX_VECTOR
__interrupt void USCIAB0TX_ISR(void)
{
    static unsigned char Bctr;

    UCB0TXBUF = byte_tab[Bctr++];

    if (Bctr == 0x03) UCB0CTL1 |= UCTXSTP;

    P1OUT |= 0x01;
}
