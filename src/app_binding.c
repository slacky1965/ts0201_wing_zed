#include "app_main.h"

enum {
    BIND_ADDRMODE_GROUP = 1,
    BIND_ADDRMODE_IEEE = 3,
};

static app_bind_tbl_t app_bind_tbl = {
        .one_device = false,
        .idle = true,
        .idx = 0,
        .extAddrCoordinator = {0},
        .addrCrdnrSet = false
};

/*********************************************************************
 * @fn      onoff_cmp_addr
 *
 * @brief   Compare binding table entries to determine if multiple devices share the same address
 *
 * @param   args - unused parameter
 *
 * @return  None
 */
static void onoff_cmp_addr(void *args) {

    aps_binding_entry_t *bind_entry_curr = NULL, *bind_entry_next = NULL;

    app_bind_tbl.one_device = false;

    for (uint8_t i = 0; i < app_bind_tbl.idx; i++) {
        bind_entry_curr = app_bind_tbl.bind_onoff[i].bind_entry;
        for (uint8_t idx = 0; idx < app_bind_tbl.idx; idx++) {
            bind_entry_next = app_bind_tbl.bind_onoff[idx].bind_entry;
            if (bind_entry_curr->dstAddrMode == BIND_ADDRMODE_GROUP) {
                if (bind_entry_next->dstAddrMode == BIND_ADDRMODE_GROUP && i != idx) {
                    if (bind_entry_curr->groupAddr == bind_entry_next->groupAddr) {
                        app_bind_tbl.one_device = true;
                    }
                }
            } else if (bind_entry_curr->dstAddrMode == BIND_ADDRMODE_IEEE) {
                if (!ZB_IEEE_ADDR_CMP(bind_entry_curr->dstExtAddrInfo.extAddr, app_bind_tbl.extAddrCoordinator) && i != idx) {
                    if (ZB_IEEE_ADDR_CMP(bind_entry_curr->dstExtAddrInfo.extAddr, bind_entry_next->dstExtAddrInfo.extAddr)) {
                        if (bind_entry_curr->dstExtAddrInfo.dstEp == bind_entry_next->dstExtAddrInfo.dstEp) {
                            app_bind_tbl.one_device = true;
                        }
                    }
                }
            }
        }
    }

//    printf("onoff_cmp_addr. one device: %d\r\n", app_bind_tbl.one_device);
    app_bind_tbl.idle = false;
}

/*********************************************************************
 * @fn      app_update_bind_tbl
 *
 * @brief   Update the application binding table with on/off cluster entries
 *
 * @param   args - unused parameter
 *
 * @return  None
 */
void app_update_bind_tbl(void *args) {

    aps_binding_entry_t *bind_tbl = bindTblEntryGet();
    app_bind_tbl.idle = true;

    if (!app_bind_tbl.addrCrdnrSet) {
        if (g_appCtx.timerAppBindEvt) {
            TL_ZB_TIMER_CANCEL(&g_appCtx.timerAppBindEvt);
        }
        g_appCtx.timerAppBindEvt = TL_ZB_TIMER_SCHEDULE(app_bindTimerCb, NULL, TIMEOUT_250MS);
        return;
    }

    app_bind_tbl.idx = 0;
    for (uint8_t i = 0; i < APS_BINDING_TABLE_NUM; i++) {
        if (bind_tbl->used) {
            if (bind_tbl->clusterId == ZCL_CLUSTER_GEN_ON_OFF) {
                app_bind_tbl.bind_onoff[app_bind_tbl.idx++].bind_entry = bind_tbl;
            }
        }
        bind_tbl++;
    }

    onoff_cmp_addr(NULL);
}

/*********************************************************************
 * @fn      app_getIeeeCb
 *
 * @brief   Callback for IEEE address response from coordinator
 *
 * @param   args - pointer to ZDO data indication containing the response
 *
 * @return  None
 */
static void app_getIeeeCb(void *args) {
    zdo_zdpDataInd_t *p = (zdo_zdpDataInd_t *)args;
    zdo_ieee_addr_resp_t *rsp = (zdo_ieee_addr_resp_t*)p->zpdu;

    if (rsp->status == ZDO_SUCCESS) {
        ZB_IEEE_ADDR_COPY(app_bind_tbl.extAddrCoordinator, rsp->ieee_addr_remote);
        app_bind_tbl.addrCrdnrSet = true;
    }
}



/*********************************************************************
 * @fn      app_getIeee
 *
 * @brief   Request IEEE address of a device given its network address
 *
 * @param   dstAddr - network address of the target device
 *
 * @return  None
 */
static void app_getIeee(uint16_t dstAddr) {
    zdo_ieee_addr_req_t pReq;
    uint8_t sn = 0;
    pReq.nwk_addr_interest = dstAddr;
    pReq.req_type = ZDO_ADDR_REQ_SINGLE_RESP;
    pReq.start_index = 0;
    zb_zdoIeeeAddrReq(dstAddr, &pReq, &sn, app_getIeeeCb);
}


/*********************************************************************
 * @fn      onoff_get_one_device
 *
 * @brief   Check if multiple on/off bindings target the same device
 *
 * @param   None
 *
 * @return  true if multiple bindings share the same device, false otherwise
 */
bool onoff_get_one_device(void) {
    return app_bind_tbl.one_device;
}

/*********************************************************************
 * @fn      app_idle_bind_tbl
 *
 * @brief   Check if the binding table update is idle
 *
 * @param   None
 *
 * @return  true if idle, false otherwise
 */
bool app_idle_bind_tbl(void) {
    return app_bind_tbl.idle;
}

/*********************************************************************
 * @fn      app_bindTimerCb
 *
 * @brief   Timer callback to handle binding table initialization after network join
 *
 * @param   args - unused parameter
 *
 * @return  0 to keep timer active, -1 to stop timer
 */
int32_t app_bindTimerCb(void *args) {

//    printf("app_bindTimerCb: network: %d\r\n", zb_isDeviceJoinedNwk());
    if (zb_isDeviceJoinedNwk()) {
        if (!app_bind_tbl.addrCrdnrSet) {
            app_getIeee(0x0000);
            return 0;
        }
        TL_SCHEDULE_TASK(app_update_bind_tbl, NULL);
        g_appCtx.timerAppBindEvt = NULL;
        return -1;
    }

    return 0;
}

/*********************************************************************
 * @fn      get_ieee_coordinator
 *
 * @brief   Get the IEEE address of the coordinator
 *
 * @param   None
 *
 * @return  pointer to the coordinator IEEE address
 */
uint8_t *get_ieee_coordinator(void) {
    return app_bind_tbl.extAddrCoordinator;
}

