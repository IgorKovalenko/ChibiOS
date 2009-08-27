/*
    ChibiOS/RT - Copyright (C) 2006-2007 Giovanni Di Sirio.

    This file is part of ChibiOS/RT.

    ChibiOS/RT is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    ChibiOS/RT is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/**
 * @file templates/serial_lld.c
 * @brief Serial Driver subsystem low level driver source template
 * @addtogroup SERIAL_LLD
 * @{
 */

#include <ch.h>
#include <serial.h>

#if USE_AVR_USART0 || defined(__DOXYGEN__)
/**
 * @brief USART0 serial driver identifier.
 * @note The name does not follow the convention used in the other ports
 *       (COMn) because a name conflict with the AVR headers.
 */
SerialDriver SD1;
#endif
#if USE_AVR_USART1 || defined(__DOXYGEN__)
/**
 * @brief USART1 serial driver identifier.
 * @note The name does not follow the convention used in the other ports
 *       (COMn) because a name conflict with the AVR headers.
 */
SerialDriver SD2;
#endif

/** @brief Driver default configuration.*/
static const SerialDriverConfig default_config = {
  UBRR(DEFAULT_USART_BITRATE),
  (1 << UCSZ1) | (1 << UCSZ0)
};

/*===========================================================================*/
/* Low Level Driver local functions.                                         */
/*===========================================================================*/

static void set_error(uint8_t sra, SerialDriver *sdp) {
  sdflags_t sts = 0;

  if (sra & (1 << DOR))
    sts |= SD_OVERRUN_ERROR;
  if (sra & (1 << UPE))
    sts |= SD_PARITY_ERROR;
  if (sra & (1 << FE))
    sts |= SD_FRAMING_ERROR;
  chSysLockFromIsr();
  sdAddFlagsI(sdp, sts);
  chSysUnlockFromIsr();
}

#if USE_AVR_USART0 || defined(__DOXYGEN__)
static void notify1(void) {

  UCSR0B |= (1 << UDRIE);
}

/**
 * @brief USART0 initialization.
 * @param[in] config the architecture-dependent serial driver configuration
 */
void usart0_init(const SerialDriverConfig *config) {

  UBRR0L = config->brr;
  UBRR0H = config->brr >> 8;
  UCSR0A = 0;
  UCSR0B = (1 << RXEN) | (1 << TXEN) | (1 << RXCIE);
  UCSR0C = config->csrc;
}

/**
 * @brief USART0 de-initialization.
 */
void usart0_deinit(void) {

  UCSR0A = 0;
  UCSR0B = 0;
  UCSR0C = 0;
}
#endif

#if USE_AVR_USART1 || defined(__DOXYGEN__)
static void notify2(void) {

  UCSR1B |= (1 << UDRIE);
}

/**
 * @brief USART1 initialization.
 * @param[in] config the architecture-dependent serial driver configuration
 */
void usart1_init(const SerialDriverConfig *config) {

  UBRR1L = config->brr;
  UBRR1H = config->brr >> 8;
  UCSR1A = 0;
  UCSR1B = (1 << RXEN) | (1 << TXEN) | (1 << RXCIE);
  UCSR1C = config->csrc;
}

/**
 * @brief USART1 de-initialization.
 */
void usart1_deinit(void) {

  UCSR1A = 0;
  UCSR1B = 0;
  UCSR1C = 0;
}
#endif

/*===========================================================================*/
/* Low Level Driver interrupt handlers.                                      */
/*===========================================================================*/

#if USE_AVR_USART0 || defined(__DOXYGEN__)
CH_IRQ_HANDLER(USART0_RX_vect) {
  uint8_t sra;

  CH_IRQ_PROLOGUE();

  sra = UCSR0A;
  if (sra & ((1 << DOR) | (1 << UPE) | (1 << FE)))
    set_error(sra, &SD1);
  chSysLockFromIsr();
  sdIncomingDataI(&SD1, UDR0);
  chSysUnlockFromIsr();

  CH_IRQ_EPILOGUE();
}

CH_IRQ_HANDLER(USART0_UDRE_vect) {
  msg_t b;

  CH_IRQ_PROLOGUE();

  chSysLockFromIsr();
  b = sdRequestDataI(&SER1);
  chSysUnlockFromIsr();
  if (b < Q_OK)
    UCSR0B &= ~(1 << UDRIE);
  else
    UDR0 = b;

  CH_IRQ_EPILOGUE();
}
#endif /* USE_AVR_USART0 */

#if USE_AVR_USART1 || defined(__DOXYGEN__)
CH_IRQ_HANDLER(USART1_RX_vect) {
  uint8_t sra;

  CH_IRQ_PROLOGUE();

  sra = UCSR1A;
  if (sra & ((1 << DOR) | (1 << UPE) | (1 << FE)))
    set_error(sra, &SD2);
  chSysLockFromIsr();
  sdIncomingDataI(&SD2, UDR1);
  chSysUnlockFromIsr();

  CH_IRQ_EPILOGUE();
}

CH_IRQ_HANDLER(USART1_UDRE_vect) {
  msg_t b;

  CH_IRQ_PROLOGUE();

  chSysLockFromIsr();
  b = sdRequestDataI(&SD2);
  chSysUnlockFromIsr();
  if (b < Q_OK)
    UCSR1B &= ~(1 << UDRIE);
  else
    UDR1 = b;

  CH_IRQ_EPILOGUE();
}
#endif /* USE_AVR_USART1 */

/*===========================================================================*/
/* Low Level Driver exported functions.                                      */
/*===========================================================================*/

/**
 * Low level serial driver initialization.
 */
void sd_lld_init(void) {

#if USE_AVR_USART0
  sdObjectInit(&SD1, NULL, notify1);
#endif
#if USE_AVR_USART1
  sdObjectInit(&SD2, NULL, notify2);
#endif
}

/**
 * @brief Low level serial driver configuration and (re)start.
 *
 * @param[in] sdp pointer to a @p SerialDriver object
 * @param[in] config the architecture-dependent serial driver configuration.
 *                   If this parameter is set to @p NULL then a default
 *                   configuration is used.
 */
void sd_lld_start(SerialDriver *sdp, const SerialDriverConfig *config) {

  if (config == NULL)
    config = &default_config;

#if USE_AVR_USART0
  usart0_init(config);
#endif
#if USE_AVR_USART1
  usart1_init(config);
#endif
}

/**
 * @brief Low level serial driver stop.
 * @details De-initializes the USART, stops the associated clock, resets the
 *          interrupt vector.
 *
 * @param[in] sdp pointer to a @p SerialDriver object
 */
void sd_lld_stop(SerialDriver *sdp) {

#if USE_AVR_USART0
  usart0_deinit();
#endif
#if USE_AVR_USART1
  usart1_deinit();
#endif
}

/** @} */
