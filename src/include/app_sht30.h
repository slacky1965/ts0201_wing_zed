#ifndef SRC_INCLUDE_APP_SHT30_H_
#define SRC_INCLUDE_APP_SHT30_H_

#include "app_cfg.h"

uint8_t sht30_read(int16_t *temp_zcl, uint16_t *hum_zcl);

#endif

