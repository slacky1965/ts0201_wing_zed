#include "tl_common.h"
#include "zcl_include.h"

#include "app_main.h"

uint32_t mcuBootAddrGet(void);

/*********************************************************************
 * @fn      start_message
 *
 * @brief   Print startup message with OTA mode status and firmware version
 *
 * @param   None
 *
 * @return  None
 */
void start_message(void) {
#ifdef ZCL_OTA
        APP_DEBUG(UART_PRINTF_MODE, "OTA mode enabled. MCU boot from address: 0x%x\r\n", mcuBootAddrGet());
#else
        APP_DEBUG(UART_PRINTF_MODE, "OTA mode desabled. MCU boot from address: 0x%x\r\n", mcuBootAddrGet());
#endif

#if UART_PRINTF_MODE
    const uint8_t version[] = ZCL_BASIC_SW_BUILD_ID;
    APP_DEBUG(UART_PRINTF_MODE, "Firmware version: %s\r\n", version+1);
#endif
}

/*********************************************************************
 * @fn      delayedMcuResetCb
 *
 * @brief   Timer callback to perform a delayed MCU reset
 *
 * @param   arg - pointer to callback argument (unused)
 *
 * @return  -1 to indicate timer should not be rescheduled
 */
int32_t delayedMcuResetCb(void *arg) {

    //printf("mcu reset\r\n");
    zb_resetDevice();
    return -1;
}

/*********************************************************************
 * @fn      delayedFactoryResetCb
 *
 * @brief   Timer callback to perform a delayed factory reset
 *
 * @param   arg - pointer to callback argument (unused)
 *
 * @return  -1 to indicate timer should not be rescheduled
 */
int32_t delayedFactoryResetCb(void *arg) {

    zb_resetDevice2FN();
    zb_deviceFactoryNewSet(true);

    g_appCtx.timerFactoryReset = NULL;

//    printf("Cb Factory new: %s\r\n", zb_isDeviceFactoryNew()?"yes":"no");


    return -1;
}

/*********************************************************************
 * @fn      delayedFullResetCb
 *
 * @brief   Timer callback to perform a delayed full reset
 *
 * @param   arg - pointer to callback argument (unused)
 *
 * @return  -1 to indicate timer should not be rescheduled
 */
int32_t delayedFullResetCb(void *arg) {

    //printf("full reset\r\n");
    return -1;
}

/*********************************************************************
 * @fn      set_pollRateCb
 *
 * @brief   Timer callback to restore the default poll rate after a temporary change
 *
 * @param   args - pointer to callback argument (unused)
 *
 * @return  -1 to indicate timer should not be rescheduled
 */
int32_t set_pollRateCb(void *args) {

    zb_setPollRate(POLL_RATE * g_appCtx.read_sensor_period);

//    printf("set_pollRateCb: %d\r\n", g_appCtx.read_sensor_period);

    g_appCtx.timerSetPollRateEvt = NULL;
    return -1;
}

/*********************************************************************
 * @fn      app_setPollRate
 *
 * @brief   Temporarily change the poll rate and schedule a callback to restore it
 *
 * @param   sec - duration in seconds before the poll rate is restored
 *
 * @return  None
 */
void app_setPollRate(uint32_t sec) {

//    printf("app_setPollRate. sec: %d\r\n", sec);
    zb_setPollRate(POLL_RATE * 3);
    if (g_appCtx.timerSetPollRateEvt) {
        TL_ZB_TIMER_CANCEL(&g_appCtx.timerSetPollRateEvt);
    }
    g_appCtx.timerSetPollRateEvt = TL_ZB_TIMER_SCHEDULE(set_pollRateCb, NULL, sec /*TIMEOUT_20SEC*/);

}


//int32_t no_joinedCb(void *arg) {
//
//    if (!zb_isDeviceJoinedNwk()) {
//
//        if (tl_stackBusy() || !zb_isTaskDone()) {
//
////            printf("tl_stackBusy: %s,  zb_isTaskDone: %d\r\n", tl_stackBusy()?"true":"false", zb_isTaskDone());
//            return TIMEOUT_1MIN;
//        }
//
//#if UART_PRINTF_MODE && DEBUG_PM
//        printf("Without network more then 30 minutes! Deep sleep ...\r\n");
//#endif
//
//        drv_pm_wakeupPinLevelChange(pin_PmCfg, sizeof(pin_PmCfg)/sizeof(drv_pm_pinCfg_t));
//
//        apsCleanToStopSecondClock();
//
//        drv_disable_irq();
//        rf_paShutDown();
//        drv_pm_deepSleep_frameCnt_set(ss_outgoingFrameCntGet());
//
//        drv_pm_longSleep(PM_SLEEP_MODE_DEEPSLEEP, PM_WAKEUP_SRC_PAD, 1);
//    }
//
//    g_appCtx.timerNoJoinedEvt = NULL;
//    return -1;
//}
