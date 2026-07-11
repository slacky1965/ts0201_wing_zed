/********************************************************************************************************
 * @file    zb_appCb.c
 *
 * @brief   This is the source file for zb_appCb
 *
 * @author  Zigbee Group
 * @date    2021
 *
 * @par     Copyright (c) 2021, Telink Semiconductor (Shanghai) Co., Ltd. ("TELINK")
 *			All rights reserved.
 *
 *          Licensed under the Apache License, Version 2.0 (the "License");
 *          you may not use this file except in compliance with the License.
 *          You may obtain a copy of the License at
 *
 *              http://www.apache.org/licenses/LICENSE-2.0
 *
 *          Unless required by applicable law or agreed to in writing, software
 *          distributed under the License is distributed on an "AS IS" BASIS,
 *          WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *          See the License for the specific language governing permissions and
 *          limitations under the License.
 *
 *******************************************************************************************************/

/**********************************************************************
 * INCLUDES
 */
#include "tl_common.h"
#include "zcl_include.h"
#include "ota.h"

#include "app_main.h"

/**********************************************************************
 * LOCAL CONSTANTS
 */


/**********************************************************************
 * TYPEDEFS
 */


/**********************************************************************
 * LOCAL FUNCTIONS
 */
void zb_bdbInitCb(uint8_t status, uint8_t joinedNetwork);
void zb_bdbCommissioningCb(uint8_t status, void *arg);
void zb_bdbIdentifyCb(uint8_t endpoint, uint16_t srcAddr, uint16_t identifyTime);
void zb_bdbFindBindSuccessCb(findBindDst_t *pDstInfo);


/**********************************************************************
 * LOCAL VARIABLES
 */
bdb_appCb_t g_zbBdbCb =
{
	zb_bdbInitCb,
	zb_bdbCommissioningCb,
	zb_bdbIdentifyCb,
	zb_bdbFindBindSuccessCb
};

#ifdef ZCL_OTA
ota_callBack_t app_otaCb =
{
	app_otaProcessMsgHandler,
};
#endif

ev_timer_event_t *steerTimerEvt = NULL;
ev_timer_event_t *switchRejoinBackoffTimerEvt = NULL;

/**********************************************************************
 * FUNCTIONS
 */
/*********************************************************************
 * @fn      app_bdbNetworkSteerStart
 *
 * @brief   Timer callback to start BDB network steering process
 *
 * @param   arg - pointer to user data (unused)
 *
 * @return  -1 to stop the timer event
 */
int32_t app_bdbNetworkSteerStart(void *arg){
	bdb_networkSteerStart();

	steerTimerEvt = NULL;
	return -1;
}

#if FIND_AND_BIND_SUPPORT
/*********************************************************************
 * @fn      app_bdbFindAndBindStart
 *
 * @brief   Timer callback to start BDB find and bind process
 *
 * @param   arg - pointer to user data (unused)
 *
 * @return  2500 to reschedule timer, -1 to stop
 */
int32_t app_bdbFindAndBindStart(void *arg) {
    APP_DEBUG(DEBUG_BDB_EN, "timer app_bdbFindAndBindStart\r\n");
    _CODE_BDB_ uint8_t st = bdb_findAndBindStart(BDB_COMMISSIONING_ROLE_INITIATOR);
    APP_DEBUG(DEBUG_BDB_EN, "status: 0x%02x\r\n", st);

    if (st != BDB_STATE_IDLE) {
        return 2500;
    }

	g_appCtx.bdbFBTimerEvt = NULL;
	return -1;
}
#endif

/*********************************************************************
 * @fn      app_rejoinBacckoff
 *
 * @brief   Timer callback to perform rejoin with backoff
 *
 * @param   arg - pointer to user data (unused)
 *
 * @return  0 to reschedule timer, -1 to stop
 */
int32_t app_rejoinBacckoff(void *arg){
	if(zb_isDeviceFactoryNew()){
		switchRejoinBackoffTimerEvt = NULL;
		return -1;
	}

	zb_rejoinReq(zb_apsChannelMaskGet(), g_bdbAttrs.scanDuration);
    return 0;
}

/*********************************************************************
 * @fn      zb_bdbInitCb
 *
 * @brief   application callback for bdb initiation
 *
 * @param   status - the status of bdb init BDB_INIT_STATUS_SUCCESS or BDB_INIT_STATUS_FAILURE
 *
 * @param   joinedNetwork  - 1: node is on a network, 0: node isn't on a network
 *
 * @return  None
 */
void zb_bdbInitCb(uint8_t status, uint8_t joinedNetwork){
//    printf("bdbInitCb: sta = %x, joined = %x\r\n", status, joinedNetwork);

	if(status == BDB_INIT_STATUS_SUCCESS){
		/*
		 * for non-factory-new device:
		 * 		load zcl data from NV, start poll rate, start ota query, bdb_networkSteerStart
		 *
		 * for factory-new device:
		 * 		steer a network
		 *
		 */
		if(joinedNetwork){
            g_appCtx.net_steer_start = false;
            app_setPollRate(TIMEOUT_1MIN);

#ifdef ZCL_OTA
			ota_queryStart(APP_OTA_PERIODIC_QUERY_INTERVAL);
#endif

#ifdef ZCL_POLL_CTRL
			app_zclCheckInStart();
#endif

            if (g_appCtx.timerAppBindEvt) {
                TL_ZB_TIMER_CANCEL(&g_appCtx.timerAppBindEvt);
            }
            g_appCtx.timerAppBindEvt = TL_ZB_TIMER_SCHEDULE(app_bindTimerCb, NULL, TIMEOUT_2SEC);

		} else  if (g_appCtx.net_steer_start) {
			uint16_t jitter = 0;
			do{
				jitter = zb_random() % 0x0fff;
			}while(jitter == 0);

			if(steerTimerEvt){
				TL_ZB_TIMER_CANCEL(&steerTimerEvt);
			}
			steerTimerEvt = TL_ZB_TIMER_SCHEDULE(app_bdbNetworkSteerStart, NULL, jitter);
		}
	}else{
		if(joinedNetwork){
//			zb_rejoinReqWithBackOff(zb_apsChannelMaskGet(), g_bdbAttrs.scanDuration);
			if(!switchRejoinBackoffTimerEvt){
				switchRejoinBackoffTimerEvt = TL_ZB_TIMER_SCHEDULE(app_rejoinBacckoff, NULL, 60 * 1000);
			}
		}
	}
}

/*********************************************************************
 * @fn      zb_bdbCommissioningCb
 *
 * @brief   application callback for bdb commissioning
 *
 * @param   status - the status of bdb commissioning
 *
 * @param   arg
 *
 * @return  None
 */
void zb_bdbCommissioningCb(uint8_t status, void *arg) {
//    printf("zb_bdbCommissioningCb: sta = %x\r\n", status);

	switch(status){
		case BDB_COMMISSION_STA_SUCCESS:
            g_appCtx.net_steer_start = false;

            light_blink_stop();
            light_blink_start(1, 3000, 10);

            app_setPollRate(TIMEOUT_1MIN);

			if(steerTimerEvt){
				TL_ZB_TIMER_CANCEL(&steerTimerEvt);
				steerTimerEvt = NULL;
			}

			if(switchRejoinBackoffTimerEvt){
				TL_ZB_TIMER_CANCEL(&switchRejoinBackoffTimerEvt);
				switchRejoinBackoffTimerEvt = NULL;
			}
			if(!g_zbNwkCtx.joined){
				zb_rejoinReq(zb_apsChannelMaskGet(), g_bdbAttrs.scanDuration);
			}

#ifdef ZCL_POLL_CTRL
			app_zclCheckInStart();
#endif
#ifdef ZCL_OTA
			ota_queryStart(APP_OTA_PERIODIC_QUERY_INTERVAL);
#endif
//#if FIND_AND_BIND_SUPPORT
//			//start Finding & Binding
//			if(!g_appCtx.bdbFBTimerEvt){
//			    g_appCtx.bdbFBTimerEvt = TL_ZB_TIMER_SCHEDULE(app_bdbFindAndBindStart, NULL, 50);
//			}
//#endif

            if (g_appCtx.timerAppBindEvt) {
                TL_ZB_TIMER_CANCEL(&g_appCtx.timerAppBindEvt);
            }
            g_appCtx.timerAppBindEvt = TL_ZB_TIMER_SCHEDULE(app_bindTimerCb, NULL, TIMEOUT_2SEC);

			break;
		case BDB_COMMISSION_STA_IN_PROGRESS:
			break;
		case BDB_COMMISSION_STA_NOT_AA_CAPABLE:
			break;
		case BDB_COMMISSION_STA_NO_NETWORK:
		case BDB_COMMISSION_STA_TCLK_EX_FAILURE:
		case BDB_COMMISSION_STA_TARGET_FAILURE:
		    if (g_appCtx.net_steer_start) {
				uint16_t jitter = 0;
				do{
					jitter = zb_random() % 0x0fff;
				}while(jitter == 0);

				if(steerTimerEvt){
					TL_ZB_TIMER_CANCEL(&steerTimerEvt);
				}
				steerTimerEvt = TL_ZB_TIMER_SCHEDULE(app_bdbNetworkSteerStart, NULL, jitter);
			}
			break;
		case BDB_COMMISSION_STA_FORMATION_FAILURE:
			break;
		case BDB_COMMISSION_STA_NO_IDENTIFY_QUERY_RESPONSE:
			break;
		case BDB_COMMISSION_STA_BINDING_TABLE_FULL:
			break;
		case BDB_COMMISSION_STA_NO_SCAN_RESPONSE:
			break;
		case BDB_COMMISSION_STA_NOT_PERMITTED:
			break;
		case BDB_COMMISSION_STA_PARENT_LOST:
			//zb_rejoinSecModeSet(REJOIN_INSECURITY);
			zb_rejoinReq(zb_apsChannelMaskGet(), g_bdbAttrs.scanDuration);
//			zb_rejoinReqWithBackOff(zb_apsChannelMaskGet(), g_bdbAttrs.scanDuration);
			break;
		case BDB_COMMISSION_STA_REJOIN_FAILURE:
			if(!switchRejoinBackoffTimerEvt){
				switchRejoinBackoffTimerEvt = TL_ZB_TIMER_SCHEDULE(app_rejoinBacckoff, NULL, 60 * 1000);
			}
			break;
		default:
			break;
	}
}

/*********************************************************************
 * @fn      zb_bdbIdentifyCb
 *
 * @brief   Application callback for BDB identify event
 *
 * @param   endpoint    - endpoint number
 * @param   srcAddr     - source address of the identify device
 * @param   identifyTime - identify time remaining
 *
 * @return  None
 */
void zb_bdbIdentifyCb(uint8_t endpoint, uint16_t srcAddr, uint16_t identifyTime){
#if FIND_AND_BIND_SUPPORT
	extern void app_zclIdentifyCmdHandler(uint8_t endpoint, uint16_t srcAddr, uint16_t identifyTime);

	app_zclIdentifyCmdHandler(endpoint, srcAddr, identifyTime);
#endif
}

/*********************************************************************
 * @fn      zb_bdbFindBindSuccessCb
 *
 * @brief   application callback for finding & binding
 *
 * @param   pDstInfo
 *
 * @return  None
 */
void zb_bdbFindBindSuccessCb(findBindDst_t *pDstInfo){
//    printf("zb_bdbFindBindSuccessCb\r\n");
#if FIND_AND_BIND_SUPPORT
	epInfo_t dstEpInfo;
	TL_SETSTRUCTCONTENT(dstEpInfo, 0);

	dstEpInfo.dstAddrMode = APS_SHORT_DSTADDR_WITHEP;
	dstEpInfo.dstAddr.shortAddr = pDstInfo->addr;
	dstEpInfo.dstEp = pDstInfo->endpoint;
	dstEpInfo.profileId = HA_PROFILE_ID;

	zcl_identify_identifyCmd(APP_ENDPOINT1, &dstEpInfo, FALSE, 0, 0);
#endif
}



#ifdef ZCL_OTA
/*********************************************************************
 * @fn      app_otaProcessMsgHandler
 *
 * @brief   Handler for OTA process message events
 *
 * @param   evt    - OTA event type (start, complete, image done)
 * @param   status - status of the OTA event
 *
 * @return  None
 */
void app_otaProcessMsgHandler(uint8_t evt, uint8_t status)
{
	//printf("app_otaProcessMsgHandler: status = %x\n", status);
	if(evt == OTA_EVT_START){
		if(status == ZCL_STA_SUCCESS){
			zb_setPollRate(QUEUE_POLL_RATE);
		}else{

		}
	}else if(evt == OTA_EVT_COMPLETE){
		zb_setPollRate(POLL_RATE * 3);

		if(status == ZCL_STA_SUCCESS){
			ota_mcuReboot();
		}else{
			ota_queryStart(APP_OTA_PERIODIC_QUERY_INTERVAL);
		}
	}else if(evt == OTA_EVT_IMAGE_DONE){
		zb_setPollRate(POLL_RATE * 3);
	}
}
#endif

/*********************************************************************
 * @fn      app_leaveCnfHandler
 *
 * @brief   Handler for ZDO Leave Confirm message.
 *
 * @param   pRsp - parameter of leave confirm
 *
 * @return  None
 */
void app_leaveCnfHandler(nlme_leave_cnf_t *pLeaveCnf)
{
//    printf("app_leaveCnfHandler\r\n");

    if(pLeaveCnf->status == SUCCESS){
    	//SYSTEM_RESET();

		if(switchRejoinBackoffTimerEvt){
			TL_ZB_TIMER_CANCEL(&switchRejoinBackoffTimerEvt);
			switchRejoinBackoffTimerEvt = NULL;
		}

        zb_deviceFactoryNewSet(true);

        uint16_t jitter = 0;
        do {
            jitter = zb_random() % 0x0fff;
        } while (jitter == 0);

        if (steerTimerEvt) {
            TL_ZB_TIMER_CANCEL(&steerTimerEvt);
        }
        steerTimerEvt = TL_ZB_TIMER_SCHEDULE(app_bdbNetworkSteerStart, NULL, jitter);

//        if (!g_appCtx.net_steer_start) light_blink_start(90, 100, 1000);
    }
}

/*********************************************************************
 * @fn      app_leaveIndHandler
 *
 * @brief   Handler for ZDO leave indication message.
 *
 * @param   pInd - parameter of leave indication
 *
 * @return  None
 */
void app_leaveIndHandler(nlme_leave_ind_t *pLeaveInd)
{
    //printf("app_leaveIndHandler, rejoin = %d\n", pLeaveInd->rejoin);
    //printfArray(pLeaveInd->device_address, 8);
}

