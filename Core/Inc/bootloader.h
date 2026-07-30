/*
 * bootloader.h
 *
 *  Created on: Jul 28, 2026
 *      Author: matthewschneider
 */

#ifndef INC_BOOTLOADER_H_
#define INC_BOOTLOADER_H_

#include "fatfs.h"

typedef enum {
	CRC_ERR,
	CRC_OK
} CRC_status_t;

CRC_status_t calculateFileCRC();
uint32_t calculateFlashCRC();
void initCRC();
uint32_t calculateBinaryCRC(FIL * Fil);

void bootloader_reflash();

#endif /* INC_BOOTLOADER_H_ */
