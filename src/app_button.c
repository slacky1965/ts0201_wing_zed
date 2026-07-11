#include "app_main.h"

static ev_timer_event_t *findbindTimerEvt = NULL;

/*********************************************************************
 * @fn      net_steer_start_offCb
 *
 * @brief   Timer callback to stop network steering indication after timeout
 *
 * @param   args - unused callback argument
 *
 * @return  -1 to indicate timer should not repeat
 */
static int32_t net_steer_start_offCb(void *args) {

    g_appCtx.net_steer_start = false;

    light_blink_stop();

    return -1;
}

/*********************************************************************
 * @fn      find_and_bind_clear_flag
 *
 * @brief   Timer callback to clear the find-and-bind flag after timeout
 *
 * @param   args - unused callback argument
 *
 * @return  -1 to indicate timer should not repeat
 */
static int32_t find_and_bind_clear_flag(void *args) {

    g_appCtx.find_bind_flag = false;
    return -1;
}

/*********************************************************************
 * @fn      buttonKeepPressed
 *
 * @brief   Handle long button press (5 seconds) for factory reset
 *
 * @param   btNum - button number (1-based)
 *
 * @return  None
 */
static void buttonKeepPressed(uint8_t btNum) {
    g_appCtx.button[btNum-1].state = APP_FACTORY_NEW_DOING;
    g_appCtx.button[btNum-1].ctn = 0;

    if(btNum == VK_SW1) {
        APP_DEBUG(DEBUG_BUTTON_EN, "The button was keep pressed for 5 seconds\r\n");


        zb_factoryReset();

        g_appCtx.net_steer_start = true;
        TL_ZB_TIMER_SCHEDULE(net_steer_start_offCb, NULL, TIMEOUT_1MIN30SEC);
        light_blink_start(90, 500, 500);

    }
}


/*********************************************************************
 * @fn      buttonSinglePressed
 *
 * @brief   Handle single button press action
 *
 * @param   btNum - button number (1-based)
 *
 * @return  None
 */
static void buttonSinglePressed(uint8_t btNum) {

    switch (btNum) {
        case VK_SW1:
            APP_DEBUG(DEBUG_BUTTON_EN, "Button push 1 time\r\n");
            batteryCb(NULL);
            if(zb_isDeviceJoinedNwk()) {
                app_setPollRate(TIMEOUT_20SEC);
                TL_ZB_TIMER_SCHEDULE(forcedReportCb, NULL, TIMEOUT_100MS);
// for test
//                if (g_appCtx.timerAppBindEvt) {
//                    TL_ZB_TIMER_CANCEL(&g_appCtx.timerAppBindEvt);
//                }
//                g_appCtx.timerAppBindEvt = TL_ZB_TIMER_SCHEDULE(app_bindTimerCb, NULL, TIMEOUT_2SEC);
            } else if (!zb_isDeviceFactoryNew()) {
                zb_rejoinReq(zb_apsChannelMaskGet(), g_bdbAttrs.scanDuration);
            }
            break;
        default:
            break;
    }
}

/*********************************************************************
 * @fn      buttonDoublePressed
 *
 * @brief   Handle double button press - find and bind temperature, humidity, battery with endpoint 1
 *
 * @param   btNum - button number (1-based)
 *
 * @return  None
 */
/* find&bind temperature, humidity, battery with endpoint 1 */
static void buttonDoublePressed(uint8_t btNum) {
    APP_DEBUG(DEBUG_BUTTON_EN, "Button push 2 times\r\n");
    aps_binding_entry_t *bind_tbl = aps_bindingTblEntryGet();
    for (uint8_t i = 0; i < APS_BINDING_TABLE_NUM; i++) {
        if (bind_tbl->used && bind_tbl->srcEp == APP_ENDPOINT1 &&
                (bind_tbl->clusterId == ZCL_CLUSTER_MS_TEMPERATURE_MEASUREMENT ||
                bind_tbl->clusterId == ZCL_CLUSTER_MS_RELATIVE_HUMIDITY ||
                bind_tbl->clusterId == ZCL_CLUSTER_GEN_POWER_CFG)) {
            if (memcmp(bind_tbl->dstExtAddrInfo.extAddr, get_ieee_coordinator(), 8)) {
                APP_DEBUG(DEBUG_ONOFF_EN, "Deleted src_ep: %d, dst_ep: %d, ieee: 0x%02x%02x%02x%02x%02x%02x%02x%02x\r\n",
                        bind_tbl->srcEp, bind_tbl->dstExtAddrInfo.dstEp,
                        bind_tbl->dstExtAddrInfo.extAddr[0], bind_tbl->dstExtAddrInfo.extAddr[1],
                        bind_tbl->dstExtAddrInfo.extAddr[2], bind_tbl->dstExtAddrInfo.extAddr[3],
                        bind_tbl->dstExtAddrInfo.extAddr[4], bind_tbl->dstExtAddrInfo.extAddr[5],
                        bind_tbl->dstExtAddrInfo.extAddr[6], bind_tbl->dstExtAddrInfo.extAddr[7]);
                bind_tbl->used = false;
            }
        }
        bind_tbl++;
    }
    if (!g_appCtx.bdbFBTimerEvt) {
        g_appCtx.find_bind_src_ep = APP_ENDPOINT1;
        g_appCtx.find_bind_flag = true;
        g_appCtx.bdbFBTimerEvt = TL_ZB_TIMER_SCHEDULE(app_bdbFindAndBindStart, NULL, TIMEOUT_50MS);
        if (findbindTimerEvt) {
            TL_ZB_TIMER_CANCEL(&findbindTimerEvt);
        }
        findbindTimerEvt = TL_ZB_TIMER_SCHEDULE(find_and_bind_clear_flag, NULL, TIMEOUT_3MIN);
    }
}

/*********************************************************************
 * @fn      buttonTriplePressed
 *
 * @brief   Handle triple button press - find and bind on/off with endpoint 1
 *
 * @param   btNum - button number (1-based)
 *
 * @return  None
 */
/* find&bind onoff with endpoint 1 */
static void buttonTriplePressed(uint8_t btNum) {
    APP_DEBUG(DEBUG_BUTTON_EN, "Button push 3 times\r\n");
    aps_binding_entry_t *bind_tbl = aps_bindingTblEntryGet();
    for (uint8_t i = 0; i < APS_BINDING_TABLE_NUM; i++) {
        if (bind_tbl->used && bind_tbl->srcEp == APP_ENDPOINT1 && bind_tbl->clusterId == ZCL_CLUSTER_GEN_ON_OFF) {
            if (memcmp(bind_tbl->dstExtAddrInfo.extAddr, get_ieee_coordinator(), 8)) {
                APP_DEBUG(DEBUG_ONOFF_EN, "Deleted src_ep: %d, dst_ep: %d, ieee: 0x%02x%02x%02x%02x%02x%02x%02x%02x\r\n",
                        bind_tbl->srcEp, bind_tbl->dstExtAddrInfo.dstEp,
                        bind_tbl->dstExtAddrInfo.extAddr[0], bind_tbl->dstExtAddrInfo.extAddr[1],
                        bind_tbl->dstExtAddrInfo.extAddr[2], bind_tbl->dstExtAddrInfo.extAddr[3],
                        bind_tbl->dstExtAddrInfo.extAddr[4], bind_tbl->dstExtAddrInfo.extAddr[5],
                        bind_tbl->dstExtAddrInfo.extAddr[6], bind_tbl->dstExtAddrInfo.extAddr[7]);
                bind_tbl->used = false;
            }
        }
        bind_tbl++;
    }
    if (!g_appCtx.bdbFBTimerEvt) {
        g_appCtx.find_bind_src_ep = APP_ENDPOINT1;
        g_appCtx.find_bind_flag = true;
        g_appCtx.bdbFBTimerEvt = TL_ZB_TIMER_SCHEDULE(app_bdbFindAndBindStart, NULL, TIMEOUT_50MS);
        if (findbindTimerEvt) {
            TL_ZB_TIMER_CANCEL(&findbindTimerEvt);
        }
        findbindTimerEvt = TL_ZB_TIMER_SCHEDULE(find_and_bind_clear_flag, NULL, TIMEOUT_3MIN);
    }
}

/*********************************************************************
 * @fn      buttonQuadruplePressed
 *
 * @brief   Handle quadruple button press - find and bind on/off with endpoint 2
 *
 * @param   btNum - button number (1-based)
 *
 * @return  None
 */
/* find&bind onoff with endpoint 2 */
static void buttonQuadruplePressed(uint8_t btNum) {
    APP_DEBUG(DEBUG_BUTTON_EN, "Button push 4 times\r\n");
    aps_binding_entry_t *bind_tbl = aps_bindingTblEntryGet();
    for (uint8_t i = 0; i < APS_BINDING_TABLE_NUM; i++) {
        if (bind_tbl->used && bind_tbl->srcEp == APP_ENDPOINT2 && bind_tbl->clusterId == ZCL_CLUSTER_GEN_ON_OFF) {
            if (memcmp(bind_tbl->dstExtAddrInfo.extAddr, get_ieee_coordinator(), 8)) {
                APP_DEBUG(DEBUG_ONOFF_EN, "Deleted src_ep: %d, dst_ep: %d, ieee: 0x%02x%02x%02x%02x%02x%02x%02x%02x\r\n",
                        bind_tbl->srcEp, bind_tbl->dstExtAddrInfo.dstEp,
                        bind_tbl->dstExtAddrInfo.extAddr[0], bind_tbl->dstExtAddrInfo.extAddr[1],
                        bind_tbl->dstExtAddrInfo.extAddr[2], bind_tbl->dstExtAddrInfo.extAddr[3],
                        bind_tbl->dstExtAddrInfo.extAddr[4], bind_tbl->dstExtAddrInfo.extAddr[5],
                        bind_tbl->dstExtAddrInfo.extAddr[6], bind_tbl->dstExtAddrInfo.extAddr[7]);
                bind_tbl->used = false;
            }
        }
        bind_tbl++;
    }
    if (!g_appCtx.bdbFBTimerEvt) {
        g_appCtx.find_bind_src_ep = APP_ENDPOINT2;
        g_appCtx.find_bind_flag = true;
        g_appCtx.bdbFBTimerEvt = TL_ZB_TIMER_SCHEDULE(app_bdbFindAndBindStart, NULL, TIMEOUT_50MS);
        if (findbindTimerEvt) {
            TL_ZB_TIMER_CANCEL(&findbindTimerEvt);
        }
        findbindTimerEvt = TL_ZB_TIMER_SCHEDULE(find_and_bind_clear_flag, NULL, TIMEOUT_3MIN);
    }
}

/*********************************************************************
 * @fn      buttonQuintuplePressed
 *
 * @brief   Handle quintuple button press - clear all on/off bindings
 *
 * @param   btNum - button number (1-based)
 *
 * @return  None
 */
static void buttonQuintuplePressed(uint8_t btNum) {
//    APP_DEBUG(DEBUG_BUTTON_EN, "Button push 5 times\r\n");
//    aps_binding_entry_t *bind_tbl = bindTblEntryGet();
//    for (uint8_t i = 0; i < APS_BINDING_TABLE_NUM; i++) {
//        if (bind_tbl->used && bind_tbl->clusterId == ZCL_CLUSTER_GEN_ON_OFF) {
//            APP_DEBUG(DEBUG_ONOFF_EN, "src_ep: %d, dst_ep: %d, ieee: 0x%02x%02x%02x%02x%02x%02x%02x%02x\r\n",
//                    bind_tbl->srcEp, bind_tbl->dstExtAddrInfo.dstEp,
//                    bind_tbl->dstExtAddrInfo.extAddr[0], bind_tbl->dstExtAddrInfo.extAddr[1],
//                    bind_tbl->dstExtAddrInfo.extAddr[2], bind_tbl->dstExtAddrInfo.extAddr[3],
//                    bind_tbl->dstExtAddrInfo.extAddr[4], bind_tbl->dstExtAddrInfo.extAddr[5],
//                    bind_tbl->dstExtAddrInfo.extAddr[6], bind_tbl->dstExtAddrInfo.extAddr[7]);
//        }
//        bind_tbl++;
//    }
    APP_DEBUG(DEBUG_BUTTON_EN, "Button push 5 times\r\n");
    aps_binding_entry_t *bind_tbl = aps_bindingTblEntryGet();
    for (uint8_t i = 0; i < APS_BINDING_TABLE_NUM; i++) {
        if (bind_tbl->used && bind_tbl->clusterId == ZCL_CLUSTER_GEN_ON_OFF) {
            if (memcmp(bind_tbl->dstExtAddrInfo.extAddr, get_ieee_coordinator(), 8)) {
                APP_DEBUG(DEBUG_ONOFF_EN, "Deleted src_ep: %d, dst_ep: %d, ieee: 0x%02x%02x%02x%02x%02x%02x%02x%02x\r\n",
                        bind_tbl->srcEp, bind_tbl->dstExtAddrInfo.dstEp,
                        bind_tbl->dstExtAddrInfo.extAddr[0], bind_tbl->dstExtAddrInfo.extAddr[1],
                        bind_tbl->dstExtAddrInfo.extAddr[2], bind_tbl->dstExtAddrInfo.extAddr[3],
                        bind_tbl->dstExtAddrInfo.extAddr[4], bind_tbl->dstExtAddrInfo.extAddr[5],
                        bind_tbl->dstExtAddrInfo.extAddr[6], bind_tbl->dstExtAddrInfo.extAddr[7]);
                bind_tbl->used = false;
            }
        }
        bind_tbl++;
    }
}


/*********************************************************************
 * @fn      buttonCheckCommand
 *
 * @brief   Dispatch button action based on press count
 *
 * @param   btNum - button number (1-based)
 *
 * @return  None
 */
static void buttonCheckCommand(uint8_t btNum) {
    g_appCtx.button[btNum-1].state = APP_STATE_NORMAL;

    if (g_appCtx.button[btNum-1].ctn == 1) {
        buttonSinglePressed(btNum);
    } else if (g_appCtx.button[btNum-1].ctn == 2) {
        buttonDoublePressed(btNum);
    } else if (g_appCtx.button[btNum-1].ctn == 3) {
        buttonTriplePressed(btNum);
    } else if (g_appCtx.button[btNum-1].ctn == 4) {
        buttonQuadruplePressed(btNum);
    } else if (g_appCtx.button[btNum-1].ctn == 5) {
        buttonQuintuplePressed(btNum);
    }

    g_appCtx.button[btNum-1].ctn = 0;
}


/*********************************************************************
 * @fn      keyScan_keyPressedCB
 *
 * @brief   Callback for key press event from keyboard scan
 *
 * @param   kbEvt - pointer to keyboard event data
 *
 * @return  None
 */
void keyScan_keyPressedCB(kb_data_t *kbEvt) {

    uint8_t keyCode = kbEvt->keycode[0];

    if(keyCode != 0xff) {
        g_appCtx.button[keyCode-1].pressed_time = clock_time();
        g_appCtx.button[keyCode-1].state = APP_FACTORY_NEW_SET_CHECK;
        g_appCtx.button[keyCode-1].ctn++;
        lcd_link_icon_set(ON);
//        light_blink_start(1, 30, 1);
        if (zb_isDeviceJoinedNwk()) {
        }
    }
}


/*********************************************************************
 * @fn      keyScan_keyReleasedCB
 *
 * @brief   Callback for key release event from keyboard scan
 *
 * @param   keyCode - released key code
 *
 * @return  None
 */
void keyScan_keyReleasedCB(uint8_t keyCode){
    if (keyCode != 0xff) {
        g_appCtx.button[keyCode-1].released_time = clock_time();
        g_appCtx.button[keyCode-1].state = APP_STATE_RELEASE;

        lcd_link_icon_set(OFF);

//        g_appCtx.button[keyCode-1].state = APP_STATE_NORMAL;
    }
}

/*********************************************************************
 * @fn      button_handler
 *
 * @brief   Main button handler - processes press/release events and dispatches actions
 *
 * @param   None
 *
 * @return  None
 */
void button_handler(void) {
    static uint8_t valid_keyCode = 0xff;

    for (uint8_t i = 0; i < MAX_BUTTON_NUM; i++) {
        if (g_appCtx.button[i].state == APP_FACTORY_NEW_SET_CHECK) {
            if(clock_time_exceed(g_appCtx.button[i].pressed_time, TIMEOUT_TICK_5SEC)) {
                buttonKeepPressed(i+1);
            }
        }

        if (g_appCtx.button[i].state == APP_STATE_RELEASE) {
            if(clock_time_exceed(g_appCtx.button[i].released_time, TIMEOUT_TICK_250MS)) {
                buttonCheckCommand(i+1);
            }

        }
    }

    if(kb_scan_key(0, 1)){
        if(kb_event.cnt){
            g_appCtx.keyPressed = 1;
            keyScan_keyPressedCB(&kb_event);
            if(kb_event.cnt == 1){
                valid_keyCode = kb_event.keycode[0];
            }
        }else{
            keyScan_keyReleasedCB(valid_keyCode);
            valid_keyCode = 0xff;
            g_appCtx.keyPressed = 0;
        }
    }
}

/*********************************************************************
 * @fn      button_idle
 *
 * @brief   Check if any button activity is in progress
 *
 * @param   None
 *
 * @return  true if a button is pressed or has pending actions, false otherwise
 */
uint8_t button_idle(void) {

    if (g_appCtx.keyPressed) {
        return true;
    }

    for (uint8_t i = 0; i < MAX_BUTTON_NUM; i++) {
        if (g_appCtx.button[i].ctn) return true;
    }

    return false;
}

