#include "app_main.h"

#define DEFAULT_REPEAT_TIME 60

#define HUMIDITY_TEST       OFF

#define IDLE                0xFF
#define TEMP_IDX            (APP_ENDPOINT1 - 1)
#define HUM_IDX             (APP_ENDPOINT2 - 1)

static uint8_t sw_onoff[ONOFFCFG_AMT] = {IDLE, IDLE};
static uint8_t common_sw_onoff[ONOFFCFG_AMT] = {IDLE, IDLE};
static ev_timer_event_t *timerTempEvt = NULL;
static ev_timer_event_t *timerHumEvt = NULL;
static int16_t temp_save;
static uint16_t hum_save;

#if HUMIDITY_TEST
static uint16_t hum_test = 1800;        // 18%
static uint8_t hum_direction = 1;
#endif

/*********************************************************************
 * @fn      cmdOnOffSend
 *
 * @brief   Send an On/Off/Toggle command to a destination endpoint.
 *
 * @param   ep - source endpoint number
 * @param   dstEpInfo - pointer to destination endpoint information
 * @param   command - the On/Off command to send (off, on, or toggle)
 *
 * @return  status_t - status of the command send operation
 */
static status_t cmdOnOffSend(uint8_t ep, epInfo_t *dstEpInfo, uint8_t command) {

    status_t st = 0xFF;

    /* command 0x00 - off, 0x01 - on, 0x02 - toggle */

    switch(command) {
        case ZCL_CMD_ONOFF_OFF:
            APP_DEBUG(DEBUG_ONOFF_EN, "OnOff command: 'off' in endPoint: %d\r\n", ep);
            st = zcl_onOff_offCmd(ep, dstEpInfo, FALSE);
            break;
        case ZCL_CMD_ONOFF_ON:
            APP_DEBUG(DEBUG_ONOFF_EN, "OnOff command: 'on' in endPoint: %d\r\n", ep);
            st = zcl_onOff_onCmd(ep, dstEpInfo, FALSE);
            break;
        case ZCL_CMD_ONOFF_TOGGLE:
            APP_DEBUG(DEBUG_ONOFF_EN, "OnOff command: 'toggle' in endPoint: %d\r\n", ep);
            st = zcl_onOff_toggleCmd(ep, dstEpInfo, FALSE);
            break;
        default:
            break;
    }

    return st;
}

/*********************************************************************
 * @fn      cmdOnOff
 *
 * @brief   Send On/Off command to all bound groups and binding table entries.
 *
 * @param   ep - source endpoint number
 * @param   command - the On/Off command to send (off, on, or toggle)
 *
 * @return  None
 */
static void cmdOnOff(uint8_t ep, uint8_t command) {

    status_t st;
    epInfo_t dstEpInfo;
    TL_SETSTRUCTCONTENT(dstEpInfo, 0);
    dstEpInfo.profileId = HA_PROFILE_ID;

    uint16_t groupList[APS_GROUP_TABLE_NUM];
    uint8_t groupCnt = 0;
    aps_group_list_get(&groupCnt, groupList);

    /* command for groups */
    dstEpInfo.dstAddrMode = APS_SHORT_GROUPADDR_NOEP;
    for (uint8_t i = 0; i < groupCnt; i++) {
        aps_group_tbl_ent_t *grEntry = aps_group_search(groupList[i], ep);
        if (grEntry) {
            dstEpInfo.dstAddr.shortAddr = grEntry->group_addr;
            st = cmdOnOffSend(ep, &dstEpInfo, command);
            APP_DEBUG(DEBUG_ONOFF_EN, "OnOff in groups. cmd: %d, src_ep: %d, dst_ep: %d, addr: 0x%04x, status: %d\r\n",
                    (command == 0)?"Off":(command == 1)?"On":"Toggle", ep, grEntry->n_endpoints, grEntry->group_addr, st);
        }
    }

    /* command when binding */
    TL_SETSTRUCTCONTENT(dstEpInfo, 0);
    dstEpInfo.profileId = HA_PROFILE_ID;

    aps_binding_entry_t *bind_tbl = bindTblEntryGet();
    for (uint8_t j = 0; j < APS_BINDING_TABLE_NUM; j++) {
        if (bind_tbl->used && bind_tbl->clusterId == ZCL_CLUSTER_GEN_ON_OFF && bind_tbl->srcEp == ep) {
            dstEpInfo.dstAddrMode = bind_tbl->dstAddrMode;
            if (dstEpInfo.dstAddrMode == APS_SHORT_GROUPADDR_NOEP) {
                dstEpInfo.dstAddr.shortAddr = bind_tbl->groupAddr;
            } else {
                dstEpInfo.dstAddrMode = APS_LONG_DSTADDR_WITHEP;
                dstEpInfo.dstEp = bind_tbl->dstExtAddrInfo.dstEp;
                memcpy(dstEpInfo.dstAddr.extAddr, bind_tbl->dstExtAddrInfo.extAddr, sizeof(extAddr_t));
            }
            st = cmdOnOffSend(ep, &dstEpInfo, command);
            APP_DEBUG(DEBUG_ONOFF_EN, "OnOff for bind. cmd: %s, ep: %d, clId: 0x%04x, addrMode: %d - %s, ",
                    (command == 0)?"Off":(command == 1)?"On":"Toggle",
                     bind_tbl->srcEp, bind_tbl->clusterId, dstEpInfo.dstAddrMode,
                    (dstEpInfo.dstAddrMode == APS_DSTADDR_EP_NOTPRESETNT)?"APS_DSTADDR_EP_NOTPRESETNT":
                    (dstEpInfo.dstAddrMode == APS_SHORT_GROUPADDR_NOEP)?"APS_SHORT_GROUPADDR_NOEP":
                    (dstEpInfo.dstAddrMode == APS_SHORT_DSTADDR_WITHEP)?"APS_SHORT_DSTADDR_WITHEP":"APS_LONG_DSTADDR_WITHEP");
            if (dstEpInfo.dstAddrMode == APS_LONG_DSTADDR_WITHEP) {
                APP_DEBUG(DEBUG_ONOFF_EN, "ieee: 0x%02x%02x%02x%02x%02x%02x%02x%02x, ",
                        bind_tbl->dstExtAddrInfo.extAddr[0], bind_tbl->dstExtAddrInfo.extAddr[1],
                        bind_tbl->dstExtAddrInfo.extAddr[2], bind_tbl->dstExtAddrInfo.extAddr[3],
                        bind_tbl->dstExtAddrInfo.extAddr[4], bind_tbl->dstExtAddrInfo.extAddr[5],
                        bind_tbl->dstExtAddrInfo.extAddr[6], bind_tbl->dstExtAddrInfo.extAddr[7]);
            } else if (dstEpInfo.dstAddrMode == APS_SHORT_GROUPADDR_NOEP) {
                APP_DEBUG(DEBUG_ONOFF_EN, "groupAddr: 0x%04x, ", dstEpInfo.dstAddr.shortAddr);
            } else {
                APP_DEBUG(DEBUG_ONOFF_EN, "shortAddr: 0x%04x, ", dstEpInfo.dstAddr.shortAddr);
            }
            APP_DEBUG(DEBUG_ONOFF_EN, "status: 0x%02x\r\n", st);
        }
        bind_tbl++;
    }
}

/*********************************************************************
 * @fn      temp_cmd_repeatCb
 *
 * @brief   Timer callback for repeating temperature-based On/Off commands.
 *
 * @param   args - pointer containing the endpoint number cast to uint32_t
 *
 * @return  int32_t - 0 to continue timer, -1 to stop timer
 */
static int32_t temp_cmd_repeatCb(void *args) {

//    printf("temp_cmd_repeatCb()\r\n");

    if (!config.repeat_cmd) {
        timerTempEvt = NULL;
        return -1;
    }

    uint8_t ep = (uint8_t)((uint32_t)args);
    uint8_t idx = ep-1;
    bool one_device = onoff_get_one_device();

    zcl_temperatureAttr_t *tempAttrs = zcl_temperatureAttrGet();
    zcl_onOffSwitchCfgAttr_t *onoffCfgAttrs = zcl_onOffSwitchCfgAttrGet();
    onoffCfgAttrs += idx;

    if (sw_onoff[idx]) {
        if (one_device && config.humidity_onoff) cmdOnOff(ep, ZCL_CMD_ONOFF_ON);
        else if (((tempAttrs->value / 10) >= (temp_save / 10) && onoffCfgAttrs->switchActions == ZCL_SWITCH_ACTION_OFF_ON) ||
                ((tempAttrs->value / 10) <= (temp_save / 10) && onoffCfgAttrs->switchActions == ZCL_SWITCH_ACTION_ON_OFF)) {
            cmdOnOff(ep, ZCL_CMD_ONOFF_ON);
        }
    } else {
        if (one_device && config.humidity_onoff) cmdOnOff(ep, ZCL_CMD_ONOFF_OFF);
        else if (((tempAttrs->value / 10) >= (temp_save / 10) && onoffCfgAttrs->switchActions == ZCL_SWITCH_ACTION_ON_OFF) ||
                ((tempAttrs->value / 10) <= (temp_save / 10) && onoffCfgAttrs->switchActions == ZCL_SWITCH_ACTION_OFF_ON)) {
            cmdOnOff(ep, ZCL_CMD_ONOFF_OFF);
        }
    }

    return 0;
}

/*********************************************************************
 * @fn      hum_cmd_repeatCb
 *
 * @brief   Timer callback for repeating humidity-based On/Off commands.
 *
 * @param   args - pointer containing the endpoint number cast to uint32_t
 *
 * @return  int32_t - 0 to continue timer, -1 to stop timer
 */
static int32_t hum_cmd_repeatCb(void *args) {

//    printf("hum_cmd_repeatCb()\r\n");

    if (!config.repeat_cmd) {
        timerHumEvt = NULL;
        return -1;
    }

    uint8_t ep = (uint8_t)((uint32_t)args);
    uint8_t idx = ep-1;
    bool one_device = onoff_get_one_device();

#if HUMIDITY_TEST
    zcl_humidityAttr_t humAttrs_test;
    humAttrs_test.value = hum_test;
    zcl_humidityAttr_t *humAttrs = &humAttrs_test;
#else
    zcl_humidityAttr_t *humAttrs = zcl_humidityAttrGet();
#endif

    zcl_onOffSwitchCfgAttr_t *onoffCfgAttrs = zcl_onOffSwitchCfgAttrGet();
    onoffCfgAttrs += idx;

//    printf("ctrl hum set. hum: %d, hum_save: %d\r\n", humAttrs->value, hum_save);

    if (sw_onoff[idx]) {
        if (one_device && config.temperature_onoff) cmdOnOff(ep, ZCL_CMD_ONOFF_ON);
        else if (((humAttrs->value / 10) >= (hum_save / 10) && onoffCfgAttrs->switchActions == ZCL_SWITCH_ACTION_OFF_ON) ||
                ((humAttrs->value / 10) <= (hum_save / 10) && onoffCfgAttrs->switchActions == ZCL_SWITCH_ACTION_ON_OFF)) {
//            printf("ctrl hum set. hum: %d, hum_save: %d\r\n", humAttrs->value, hum_save);
            cmdOnOff(ep, ZCL_CMD_ONOFF_ON);
        }
    } else {
        if (one_device && config.temperature_onoff) cmdOnOff(ep, ZCL_CMD_ONOFF_OFF);
        else if (((humAttrs->value / 10) >= (hum_save / 10) && onoffCfgAttrs->switchActions == ZCL_SWITCH_ACTION_ON_OFF) ||
                ((humAttrs->value / 10) <= (hum_save / 10) && onoffCfgAttrs->switchActions == ZCL_SWITCH_ACTION_OFF_ON)) {
//            printf("ctrl hum set. hum: %d, hum_save: %d\r\n", humAttrs->value, hum_save);
            cmdOnOff(ep, ZCL_CMD_ONOFF_OFF);
        }
    }

    return 0;
}

/*********************************************************************
 * @fn      proc_temp_onoff
 *
 * @brief   Process temperature-based On/Off control for a given endpoint.
 *
 * @param   ep - endpoint number to process temperature control for
 *
 * @return  None
 */
static void proc_temp_onoff(uint8_t ep) {

    uint8_t idx = ep - 1;
    uint32_t seconds = DEFAULT_REPEAT_TIME;
    bool one_device = onoff_get_one_device();

    zcl_temperatureAttr_t *tempAttrs = zcl_temperatureAttrGet();
    zcl_onOffSwitchCfgAttr_t *onoffCfgAttrs = zcl_onOffSwitchCfgAttrGet();
    onoffCfgAttrs += idx;

    if(zb_isDeviceJoinedNwk()) {

        if (!config.temperature_onoff) {
            if(timerTempEvt) {
                TL_ZB_TIMER_CANCEL(&timerTempEvt);
                timerTempEvt = NULL;
            }
            if (!one_device) {
                if (sw_onoff[idx] == ON) {
                    cmdOnOff(ep, ZCL_CMD_ONOFF_OFF);
                }
                if (sw_onoff[idx] != IDLE) {
                    sw_onoff[idx] = IDLE;
                }
            }
        } else {
            if (config.read_sensors_period <= seconds) seconds *= 1000;
            else seconds = config.read_sensors_period * 1000;
            if(!config.repeat_cmd && timerTempEvt) {
                TL_ZB_TIMER_CANCEL(&timerTempEvt);
                timerTempEvt = NULL;
            }
            if (sw_onoff[idx] == IDLE) {
                temp_save = tempAttrs->value;
                if (!one_device) {
                    if(timerTempEvt) {
                        TL_ZB_TIMER_CANCEL(&timerTempEvt);
                        timerTempEvt = NULL;
                    }
                    if (config.repeat_cmd) timerTempEvt = TL_ZB_TIMER_SCHEDULE(temp_cmd_repeatCb, (void *)((uint32_t)ep), seconds);
                }
//                printf("Temp value: %d, low: %d, high: %d\r\n", tempAttrs->value, tempAttrs->temperature_onoff_low, tempAttrs->temperature_onoff_high);
                if (onoffCfgAttrs->switchActions == ZCL_SWITCH_ACTION_ON_OFF) {
                    if (tempAttrs->value < tempAttrs->temperature_onoff_high) {
                        sw_onoff[idx] = ON;
                        if (!one_device) cmdOnOff(ep, ZCL_CMD_ONOFF_ON);
                    } else {
                        sw_onoff[idx] = OFF;
                        if (!one_device) cmdOnOff(ep, ZCL_CMD_ONOFF_OFF);
                    }
                } else {
                    if (tempAttrs->value > tempAttrs->temperature_onoff_low) {
                        sw_onoff[idx] = ON;
                        if (!one_device) cmdOnOff(ep, ZCL_CMD_ONOFF_ON);
                    } else {
                        sw_onoff[idx] = OFF;
                        if (!one_device) cmdOnOff(ep, ZCL_CMD_ONOFF_OFF);
                    }
                }
            } else if (sw_onoff[idx] == OFF) {
                if ((tempAttrs->value >= tempAttrs->temperature_onoff_high &&
                        onoffCfgAttrs->switchActions == ZCL_SWITCH_ACTION_OFF_ON) ||
                        (tempAttrs->value <= tempAttrs->temperature_onoff_low &&
                                onoffCfgAttrs->switchActions == ZCL_SWITCH_ACTION_ON_OFF)) {
                    sw_onoff[idx] = ON;
                    temp_save = tempAttrs->value;
                    if (!one_device) {
                        if(timerTempEvt) {
                            TL_ZB_TIMER_CANCEL(&timerTempEvt);
                            timerTempEvt = NULL;
                        }
                        if (config.repeat_cmd) timerTempEvt = TL_ZB_TIMER_SCHEDULE(temp_cmd_repeatCb, (void *)((uint32_t)ep), seconds);
                        cmdOnOff(ep, ZCL_CMD_ONOFF_ON);
                    }
                }
            } else {
                if ((tempAttrs->value >= tempAttrs->temperature_onoff_high &&
                        onoffCfgAttrs->switchActions == ZCL_SWITCH_ACTION_ON_OFF) ||
                        (tempAttrs->value <= tempAttrs->temperature_onoff_low &&
                                onoffCfgAttrs->switchActions == ZCL_SWITCH_ACTION_OFF_ON)) {
                    sw_onoff[idx] = OFF;
                    temp_save = tempAttrs->value;
                    if (!one_device) {
                        if(timerTempEvt) {
                            TL_ZB_TIMER_CANCEL(&timerTempEvt);
                            timerTempEvt = NULL;
                        }
                        if (config.repeat_cmd) timerTempEvt = TL_ZB_TIMER_SCHEDULE(temp_cmd_repeatCb, (void *)((uint32_t)ep), seconds);
                        cmdOnOff(ep, ZCL_CMD_ONOFF_OFF);
                    }
                }
            }
        }
    }
}

/*********************************************************************
 * @fn      proc_hum_onoff
 *
 * @brief   Process humidity-based On/Off control for a given endpoint.
 *
 * @param   ep - endpoint number to process humidity control for
 *
 * @return  None
 */
static void proc_hum_onoff(uint8_t ep) {

    uint8_t idx = ep - 1;
    uint32_t seconds = DEFAULT_REPEAT_TIME;
    bool one_device = onoff_get_one_device();

#if HUMIDITY_TEST
    zcl_humidityAttr_t *orig_humAttrs = zcl_humidityAttrGet();
    zcl_humidityAttr_t humAttrs_test;
    memcpy(&humAttrs_test, orig_humAttrs, sizeof(zcl_humidityAttr_t));
    humAttrs_test.value = hum_test;
    zcl_humidityAttr_t *humAttrs = &humAttrs_test;
    printf("humidity: %d%%\r\n", humAttrs->value / 100);
    if (hum_direction) {
        if (hum_test >= 2200) {
            hum_direction = 0;
            hum_test -= 100;
        } else {
            hum_test += 100;
        }
    } else {
        if (hum_test <= 1800) {
            hum_direction = 1;
            hum_test += 100;
        } else {
            hum_test -= 100;
        }
    }
#else
    zcl_humidityAttr_t *humAttrs = zcl_humidityAttrGet();
#endif

    zcl_onOffSwitchCfgAttr_t *onoffCfgAttrs = zcl_onOffSwitchCfgAttrGet();
    onoffCfgAttrs += idx;

    if(zb_isDeviceJoinedNwk()) {


        if (!config.humidity_onoff) {
            if(timerHumEvt) {
                TL_ZB_TIMER_CANCEL(&timerHumEvt);
                timerHumEvt = NULL;
            }
            if (!one_device) {
                if (sw_onoff[idx] == ON) {
                    cmdOnOff(ep, ZCL_CMD_ONOFF_OFF);
                }
                if (sw_onoff[idx] != IDLE) {
                    sw_onoff[idx] = IDLE;
                }
            }
        } else {
            if (config.read_sensors_period <= seconds) seconds *= 1000;
            else seconds = config.read_sensors_period * 1000;
            if(!config.repeat_cmd && timerHumEvt) {
                TL_ZB_TIMER_CANCEL(&timerHumEvt);
                timerHumEvt = NULL;
            }
            if (sw_onoff[idx] == IDLE) {
                hum_save = humAttrs->value;
                if (!one_device) {
                    if(timerHumEvt) {
                        TL_ZB_TIMER_CANCEL(&timerHumEvt);
                        timerHumEvt = NULL;
                    }
                    if (config.repeat_cmd) timerHumEvt = TL_ZB_TIMER_SCHEDULE(hum_cmd_repeatCb, (void *)((uint32_t)ep), seconds);
                }
//                printf("Hum value: %d, low: %d, high: %d\r\n", humAttrs->value, humAttrs->humidity_onoff_low, humAttrs->humidity_onoff_high);
                if (onoffCfgAttrs->switchActions == ZCL_SWITCH_ACTION_ON_OFF) {
                    if (humAttrs->value < humAttrs->humidity_onoff_high) {
                        sw_onoff[idx] = ON;
                        if (!one_device) cmdOnOff(ep, ZCL_CMD_ONOFF_ON);
                    } else {
                        sw_onoff[idx] = OFF;
                        if (!one_device) cmdOnOff(ep, ZCL_CMD_ONOFF_OFF);
                    }
                } else {
                    if (humAttrs->value > humAttrs->humidity_onoff_low) {
                        sw_onoff[idx] = ON;
                        if (!one_device) cmdOnOff(ep, ZCL_CMD_ONOFF_ON);
                    } else {
                        sw_onoff[idx] = OFF;
                        if (!one_device) cmdOnOff(ep, ZCL_CMD_ONOFF_OFF);
                    }
                }
            } else if (sw_onoff[idx] == OFF) {
                if ((humAttrs->value >= humAttrs->humidity_onoff_high &&
                        onoffCfgAttrs->switchActions == ZCL_SWITCH_ACTION_OFF_ON) ||
                        (humAttrs->value <= humAttrs->humidity_onoff_low &&
                                onoffCfgAttrs->switchActions == ZCL_SWITCH_ACTION_ON_OFF)) {
                    sw_onoff[idx] = ON;
                    hum_save = humAttrs->value;
                    if (!one_device) {
                        if(timerHumEvt) {
                            TL_ZB_TIMER_CANCEL(&timerHumEvt);
                            timerHumEvt = NULL;
                        }
                        if (config.repeat_cmd) timerHumEvt = TL_ZB_TIMER_SCHEDULE(hum_cmd_repeatCb, (void *)((uint32_t)ep), seconds);
                        cmdOnOff(ep, ZCL_CMD_ONOFF_ON);
                    }
                    APP_DEBUG(HUMIDITY_TEST, "ON. hum: %d%%\r\n", humAttrs->value / 100);
                }
            } else {
                if ((humAttrs->value >= humAttrs->humidity_onoff_high &&
                        onoffCfgAttrs->switchActions == ZCL_SWITCH_ACTION_ON_OFF) ||
                        (humAttrs->value <= humAttrs->humidity_onoff_low &&
                                onoffCfgAttrs->switchActions == ZCL_SWITCH_ACTION_OFF_ON)) {
                    sw_onoff[idx] = OFF;
                    hum_save = humAttrs->value;
                    if (!one_device) {
                        if(timerHumEvt) {
                            TL_ZB_TIMER_CANCEL(&timerHumEvt);
                            timerHumEvt = NULL;
                        }
                        if (config.repeat_cmd) timerHumEvt = TL_ZB_TIMER_SCHEDULE(hum_cmd_repeatCb, (void *)((uint32_t)ep), seconds);
                        cmdOnOff(ep, ZCL_CMD_ONOFF_OFF);
                    }
                    APP_DEBUG(HUMIDITY_TEST, "OFF. hum: %d%%\r\n", humAttrs->value / 100);
                }
            }
        }
    }
}

/*********************************************************************
 * @fn      procTempHumOnOff
 *
 * @brief   Process combined temperature and humidity On/Off control for dual endpoints.
 *
 * @param   t_ep - temperature endpoint number
 * @param   h_ep - humidity endpoint number
 *
 * @return  None
 */
static void procTempHumOnOff(uint8_t t_ep, uint8_t h_ep) {

    uint32_t seconds = DEFAULT_REPEAT_TIME;
    uint8_t t_idx = t_ep - 1;
    uint8_t h_idx = h_ep - 1;

    if(zb_isDeviceJoinedNwk()) {
        proc_temp_onoff(t_ep);
        proc_hum_onoff(h_ep);

        if (config.read_sensors_period <= seconds) seconds *= 1000;
        else seconds = config.read_sensors_period * 1000;

//        printf("temp: %d, hum: %d\r\n", sw_onoff[t_idx], sw_onoff[h_idx]);

        if (!config.temperature_onoff && !config.humidity_onoff) {
            if(timerTempEvt) {
                TL_ZB_TIMER_CANCEL(&timerTempEvt);
                timerTempEvt = NULL;
            }
            if(timerHumEvt) {
                TL_ZB_TIMER_CANCEL(&timerHumEvt);
                timerHumEvt = NULL;
            }

            if (sw_onoff[t_idx] == ON || sw_onoff[h_idx] == ON) {
                cmdOnOff(t_ep, ZCL_CMD_ONOFF_OFF);
            }
            if (sw_onoff[t_idx] != IDLE) sw_onoff[t_idx] = IDLE;
            if (sw_onoff[h_idx] != IDLE) sw_onoff[h_idx] = IDLE;
            if (common_sw_onoff[t_idx] != IDLE) common_sw_onoff[t_idx] = IDLE;
            if (common_sw_onoff[h_idx] != IDLE) common_sw_onoff[h_idx] = IDLE;
        }

        if (!config.temperature_onoff && config.humidity_onoff) {
            if(timerTempEvt) {
                TL_ZB_TIMER_CANCEL(&timerTempEvt);
                timerTempEvt = NULL;
            }
            if (sw_onoff[t_idx] == ON && sw_onoff[h_idx] == OFF) {
                cmdOnOff(t_ep, ZCL_CMD_ONOFF_OFF);
            }
            if (sw_onoff[t_idx] != IDLE) sw_onoff[t_idx] = IDLE;
            if (common_sw_onoff[t_idx] != IDLE) common_sw_onoff[t_idx] = IDLE;

            if (sw_onoff[h_idx] == ON && common_sw_onoff[h_idx] != ON) {
                common_sw_onoff[h_idx] = ON;
                cmdOnOff(h_ep, ZCL_CMD_ONOFF_ON);
                if(timerHumEvt) {
                    TL_ZB_TIMER_CANCEL(&timerHumEvt);
                    timerHumEvt = NULL;
                }
            }

            if (sw_onoff[h_idx] == OFF && common_sw_onoff[h_idx] != OFF) {
                common_sw_onoff[h_idx] = OFF;
                cmdOnOff(h_ep, ZCL_CMD_ONOFF_OFF);
                if(timerHumEvt) {
                    TL_ZB_TIMER_CANCEL(&timerHumEvt);
                    timerHumEvt = NULL;
                }
            }

            if (!timerHumEvt && config.repeat_cmd) timerHumEvt = TL_ZB_TIMER_SCHEDULE(hum_cmd_repeatCb, (void *)((uint32_t)h_ep), seconds);

        }

        if (config.temperature_onoff && !config.humidity_onoff) {
            if(timerHumEvt) {
                TL_ZB_TIMER_CANCEL(&timerHumEvt);
                timerHumEvt = NULL;
            }
            if (sw_onoff[h_idx] == ON && sw_onoff[t_idx] == OFF) {
                cmdOnOff(h_ep, ZCL_CMD_ONOFF_OFF);
            }
            if (sw_onoff[h_idx] != IDLE) sw_onoff[h_idx] = IDLE;
            if (common_sw_onoff[h_idx] != IDLE) common_sw_onoff[h_idx] = IDLE;

            if (sw_onoff[t_idx] == ON && common_sw_onoff[t_idx] != ON) {
                common_sw_onoff[t_idx] = ON;
                cmdOnOff(t_ep, ZCL_CMD_ONOFF_ON);
                if(timerTempEvt) {
                    TL_ZB_TIMER_CANCEL(&timerTempEvt);
                    timerTempEvt = NULL;
                }
            }

            if (sw_onoff[t_idx] == OFF && common_sw_onoff[t_idx] != OFF) {
                common_sw_onoff[t_idx] = OFF;
                cmdOnOff(t_ep, ZCL_CMD_ONOFF_OFF);
                if(timerTempEvt) {
                    TL_ZB_TIMER_CANCEL(&timerTempEvt);
                    timerTempEvt = NULL;
                }
            }

            if (!timerTempEvt && config.repeat_cmd) timerTempEvt = TL_ZB_TIMER_SCHEDULE(temp_cmd_repeatCb, (void *)((uint32_t)t_ep), seconds);
        }

        if (config.temperature_onoff && config.humidity_onoff) {
            if (sw_onoff[t_idx] == ON && common_sw_onoff[t_idx] != ON) {
                common_sw_onoff[t_idx] = ON;
                cmdOnOff(t_ep, ZCL_CMD_ONOFF_ON);
                if(timerTempEvt) {
                    TL_ZB_TIMER_CANCEL(&timerTempEvt);
                    timerTempEvt = NULL;
                }
                if(timerHumEvt) {
                    TL_ZB_TIMER_CANCEL(&timerHumEvt);
                    timerHumEvt = NULL;
                }
                if (config.repeat_cmd) timerTempEvt = TL_ZB_TIMER_SCHEDULE(temp_cmd_repeatCb, (void *)((uint32_t)t_ep), seconds);
            }

            if (sw_onoff[h_idx] == ON && common_sw_onoff[h_idx] != ON) {
                common_sw_onoff[h_idx] = ON;
                cmdOnOff(h_ep, ZCL_CMD_ONOFF_ON);
                if(timerTempEvt) {
                    TL_ZB_TIMER_CANCEL(&timerTempEvt);
                    timerTempEvt = NULL;
                }
                if(timerHumEvt) {
                    TL_ZB_TIMER_CANCEL(&timerHumEvt);
                    timerHumEvt = NULL;
                }
                if (config.repeat_cmd) timerHumEvt = TL_ZB_TIMER_SCHEDULE(hum_cmd_repeatCb, (void *)((uint32_t)h_ep), seconds);
            }

            if (sw_onoff[t_idx] == OFF && common_sw_onoff[t_idx] != OFF) {
                common_sw_onoff[t_idx] = OFF;
                if (common_sw_onoff[h_idx] == OFF) {
                    cmdOnOff(t_ep, ZCL_CMD_ONOFF_OFF);
                    if(timerTempEvt) {
                        TL_ZB_TIMER_CANCEL(&timerTempEvt);
                        timerTempEvt = NULL;
                    }
                    if(timerHumEvt) {
                        TL_ZB_TIMER_CANCEL(&timerHumEvt);
                        timerHumEvt = NULL;
                    }
                    if (config.repeat_cmd) timerTempEvt = TL_ZB_TIMER_SCHEDULE(temp_cmd_repeatCb, (void *)((uint32_t)t_ep), seconds);
                }
            }

            if (sw_onoff[h_idx] == OFF && common_sw_onoff[h_idx] != OFF) {
                common_sw_onoff[h_idx] = OFF;
                if (common_sw_onoff[t_idx] == OFF) {
                    cmdOnOff(h_ep, ZCL_CMD_ONOFF_OFF);
                    if(timerTempEvt) {
                        TL_ZB_TIMER_CANCEL(&timerTempEvt);
                        timerTempEvt = NULL;
                    }
                    if(timerHumEvt) {
                        TL_ZB_TIMER_CANCEL(&timerHumEvt);
                        timerHumEvt = NULL;
                    }
                    if (config.repeat_cmd) timerHumEvt = TL_ZB_TIMER_SCHEDULE(hum_cmd_repeatCb, (void *)((uint32_t)h_ep), seconds);
                }
            }
        }

//        printf("t_onoff: %d, h_onoff: %d\r\n", common_sw_onoff[t_idx], common_sw_onoff[h_idx]);
    }

}

/*********************************************************************
 * @fn      proc_temp_hum_onoff
 *
 * @brief   Main entry point for processing temperature and humidity On/Off control.
 *
 * @param   None
 *
 * @return  None
 */
void proc_temp_hum_onoff(void) {

#if UART_PRINTF_MODE && DEBUG_ONOFF
    printf("Start OnOff control\r\n");
#endif /* UART_PRINTF_MODE */

    if (app_idle_bind_tbl()) return;

    if (onoff_get_one_device()) {
//        printf("one device\r\n");
        procTempHumOnOff(APP_ENDPOINT1, APP_ENDPOINT2);
    } else {
//        printf("different devices\r\n");
        proc_temp_onoff(APP_ENDPOINT1);
        proc_hum_onoff(APP_ENDPOINT2);
    }
}

/*********************************************************************
 * @fn      reset_control_temp
 *
 * @brief   Reset the temperature On/Off control state to idle.
 *
 * @param   None
 *
 * @return  None
 */
void reset_control_temp(void) {
    sw_onoff[TEMP_IDX] = IDLE;
    common_sw_onoff[TEMP_IDX] = IDLE;
}

/*********************************************************************
 * @fn      reset_control_hum
 *
 * @brief   Reset the humidity On/Off control state to idle.
 *
 * @param   None
 *
 * @return  None
 */
void reset_control_hum(void) {
    sw_onoff[HUM_IDX] = IDLE;
    common_sw_onoff[HUM_IDX] = IDLE;
}

/*********************************************************************
 * @fn      repeat_timer_stop
 *
 * @brief   Stop all repeat command timers and reset control states.
 *
 * @param   None
 *
 * @return  None
 */
void repeat_timer_stop(void) {
    if(timerTempEvt) {
        TL_ZB_TIMER_CANCEL(&timerTempEvt);
        timerTempEvt = NULL;
    }
    if(timerHumEvt) {
        TL_ZB_TIMER_CANCEL(&timerHumEvt);
        timerHumEvt = NULL;
    }
    reset_control_temp();
    reset_control_hum();
}
