/*
 * bootloader.c
 *
 *  Created on: Jul 28, 2026
 *      Author: matthewschneider
 */

#include "bootloader.h"
#include "main.h"
#include "fatfs.h"
#include "flash_layout.h"
#include <stdio.h>
#include <string.h>

extern SD_HandleTypeDef hsd1;
static FATFS FatFs;
FRESULT FR_Status;

void initCRC() {
	RCC->AHB1ENR |= RCC_AHB1ENR_CRCEN;
}

CRC_status_t calculateFileCRC(uint32_t * crc_out) {

  if (HAL_SD_Init(&hsd1) != HAL_OK) {
	  printf("Issue initializing SDIO.\r\n");
	  return CRC_ERR;
  } else {
	  MX_FATFS_Init();
	  FR_Status = f_mount(&FatFs, SDPath, 1);

	   		if (FR_Status != FR_OK) {
	   			printf("Unable to Mount Card. Status: %d\r\n", FR_Status);
	   			return CRC_ERR;
	   		} else {
	   			FIL Fil;

	   			FR_Status = f_open(&Fil, "app.bin", FA_READ);
	   			if (FR_Status != FR_OK) {
	   				//can't find firmware file. boot normally
	   				printf("Unable to find firmware. Boot normally. Status: %d\r\n", FR_Status);
	   				return CRC_ERR;
	   			}

	   			//calculate CRC
	   			uint32_t file_crc = calculateBinaryCRC(&Fil);

	   			f_close(&Fil);

	   			*crc_out = file_crc;
	   			return CRC_OK;

	   		}
  }
}

uint32_t calculateBinaryCRC(FIL * Fil) {

	CRC->CR |= CRC_CR_RESET;

	static uint32_t crc_buffer[512/4] = {0}; // same as sector size on uSD card (512 bytes). Sized in bytes. CRC takes bytes.
	UINT bytesRead = 0;
	uint32_t bytesWritten = 0;

	while (f_read(Fil, crc_buffer, sizeof(crc_buffer), &bytesRead) == FR_OK && bytesRead > 0) { // fine because of short circuit
		int i = 0;
		for (i = 0; i < bytesRead/4; i++) { // stream in word length
			CRC->DR = crc_buffer[i]; //takes uint32_t (word)
			bytesWritten += 4;
		}

		uint8_t remaining_bytes = (bytesRead % 4);
		for (int j = 0; j < remaining_bytes; j++) {
			*(volatile uint8_t *)(&(CRC->DR)) = *(((uint8_t *)&crc_buffer[i])+j); //takes uint8_t (byte)
			bytesWritten += 1;
		}
	}

	while (bytesWritten < APP_SIZE && bytesWritten % 4 != 0) { // pad and get to word boundary
		*(volatile uint8_t *)(&(CRC->DR)) = (uint8_t) 0xFF;
		bytesWritten += 1;
	}

	while (bytesWritten < APP_SIZE) { // APP_SIZE refers to flash size, which must be multiple of 2kb because it must align with pages, therefore must be page-aligned
		CRC->DR = 0xFFFFFFFF; //Fill with same equivalent as empty flash
		bytesWritten += 4;
	}

	return CRC->DR;
}

uint32_t calculateFlashCRC() {
	CRC->CR |= CRC_CR_RESET;

	for (uint32_t * addr = (uint32_t *) APP_START_ADDR; addr < (uint32_t *) FLASH_END; addr++) { //advance word by word, flash should be word-aligned
		CRC->DR = *addr;
	}

	return CRC->DR;
}


void bootloader_reflash() {
	if (FR_Status != FR_OK) {
		printf("Unable to Mount Card. Status: %d\r\n", FR_Status);
		return; // do nothing
	} else {
		FIL Fil;

		FR_Status = f_open(&Fil, "app.bin", FA_READ);
		if (FR_Status != FR_OK) {
			//can't find firmware file. boot normally
			printf("Unable to find firmware. Boot normally. Status: %d\r\n", FR_Status);
			return; // do nothing
		}

		//REFLASH STUFF!

		HAL_FLASH_Unlock();

		FLASH_EraseInitTypeDef erase_config = {
			TYPEERASE_PAGEERASE,
			FLASH_BANK_1,
			(uint32_t) BL_SIZE/(2<<10), //calculate pages from bytes
			(uint32_t) APP_SIZE/(2<<10)
		};

		uint32_t page_error;

		if (HAL_FLASHEx_Erase(&erase_config, &page_error) != HAL_OK) {
			printf("Error erasing. Booting as normal.\r\n");
			f_close(&Fil);
			HAL_FLASH_Lock();
			return;
		}

		static uint64_t data_buffer[512/8] = {0xFF}; // same as sector size on uSD card (512 bytes). Sized for double word (required by program)
		UINT bytesRead = 0;
		uint32_t current_addr = APP_START_ADDR;

		while (f_read(&Fil, data_buffer, sizeof(data_buffer), &bytesRead) == FR_OK && bytesRead > 0) { // fine because of short circuit
			int i = 0;
			for (i = 0; i < (bytesRead+7)/8; i++) { // stream in word length; add 7 so any remainder also gets flushed
				HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, current_addr, data_buffer[i]); //takes uint32_t (word)
				current_addr += 8;
			}

			memset(data_buffer, 0xFF, sizeof(data_buffer));

			// want to assume binary will always land on page size (2kb) and therefore also on double word size
			// but apparently binary file does not equal size of flash.
		}

		HAL_FLASH_Lock();
		f_close(&Fil);

	}
}

void bootloader_jump_to_app() {

	HAL_SD_DeInit(&hsd1);

	__disable_irq();

	SysTick->CTRL = 0;
	SysTick->LOAD = 0;
	SysTick->VAL = 0;

	__set_MSP(*((volatile uint32_t *) APP_START_ADDR));

	uint32_t reset_handler_addr = *(volatile uint32_t * )(APP_START_ADDR + 4);
	void (*appEntry)(void) = (void (*)(void))reset_handler_addr;

	appEntry();

//	SCB->VTOR = APP_START_ADDR; //NOPE. Do in main app
}
