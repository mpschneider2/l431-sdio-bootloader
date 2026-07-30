/*
 * flash_layout.h
 *
 *  Created on: Jul 28, 2026
 *      Author: matthewschneider
 */

#ifndef INC_FLASH_LAYOUT_H_
#define INC_FLASH_LAYOUT_H_

#define BL_START_ADDR        0x08000000  // 32KB
#define BL_SIZE				 32U << 10 // 32 KB
#define APP_START_ADDR       0x08008000  // a little less than 224KB
#define APP_SIZE		 	 (256U << 10) - BL_SIZE // used to find start of footer
#define APP_FOOTER_SIZE		 64 // 64 bytes


#endif /* INC_FLASH_LAYOUT_H_ */
