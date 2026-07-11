# Add inputs and outputs from these tool invocations to the build variables
OUT_DIR += \
/$(SRC_PATH) \
/$(SRC_PATH)/zigbee/zcl \
/$(SRC_PATH)/zigbee/zdo \
/$(SRC_PATH)/zigbee/bdb
 
OBJS += \
$(OUT_PATH)/$(SRC_PATH)/zigbee/zcl/zcl_relative_humidity_measurement.o \
$(OUT_PATH)/$(SRC_PATH)/zigbee/zcl/zcl_onoffSwitchCfg.o \
$(OUT_PATH)/$(SRC_PATH)/zigbee/zcl/zcl_reporting.o \
$(OUT_PATH)/$(SRC_PATH)/zigbee/zdo/zdp.o \
$(OUT_PATH)/$(SRC_PATH)/zigbee/bdb/bdb.o \
$(OUT_PATH)/$(SRC_PATH)/main.o \
$(OUT_PATH)/$(SRC_PATH)/flash.o \
$(OUT_PATH)/$(SRC_PATH)/app_i2c.o \
$(OUT_PATH)/$(SRC_PATH)/app_sht30.o \
$(OUT_PATH)/$(SRC_PATH)/app_lcd.o \
$(OUT_PATH)/$(SRC_PATH)/app_sensor.o \
$(OUT_PATH)/$(SRC_PATH)/app_timer.o \
$(OUT_PATH)/$(SRC_PATH)/app_config.o \
$(OUT_PATH)/$(SRC_PATH)/app_binding.o \
$(OUT_PATH)/$(SRC_PATH)/app_button.o \
$(OUT_PATH)/$(SRC_PATH)/app_pm.o \
$(OUT_PATH)/$(SRC_PATH)/app_on_off.o \
$(OUT_PATH)/$(SRC_PATH)/app_battery.o \
$(OUT_PATH)/$(SRC_PATH)/app_reporting.o \
$(OUT_PATH)/$(SRC_PATH)/app_endpoint_cfg.o \
$(OUT_PATH)/$(SRC_PATH)/app_utility.o \
$(OUT_PATH)/$(SRC_PATH)/app_main.o \
$(OUT_PATH)/$(SRC_PATH)/zb_appCb.o \
$(OUT_PATH)/$(SRC_PATH)/zcl_appCb.o

# Each subdirectory must supply rules for building sources it contributes
$(OUT_PATH)/$(SRC_PATH)/%.o: $(SRC_PATH)/%.c 
	@echo 'Building file: $<'
	@$(CC) $(GCC_FLAGS) $(INCLUDE_PATHS) -c -o"$@" "$<"


