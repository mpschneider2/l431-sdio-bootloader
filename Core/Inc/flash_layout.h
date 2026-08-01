/*
 * flash_layout.h
 *
 *  Created on: Jul 28, 2026
 *      Author: matthewschneider
 */

#ifndef INC_FLASH_LAYOUT_H_
#define INC_FLASH_LAYOUT_H_

#include "main.h"

#define BL_START_ADDR        FLASH_BASE  // 32KB // 0x08000000
#define BL_SIZE				 (32U << 10) // 32 KB // 0x00008000
#define APP_START_ADDR       (BL_START_ADDR + BL_SIZE)//0x08008000  // a little less than 224KB
#define APP_SIZE		 	 (FLASH_END - APP_START_ADDR + 1U)//((256U << 10) - BL_SIZE) // used to find start of footer
#define APP_FOOTER_SIZE		 64 // 64 bytes


#endif /* INC_FLASH_LAYOUT_H_ */
