/******************************************************************************
 *
 * Copyright(c) 2007 - 2011 Realtek Corporation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110, USA
 *
 *
 ******************************************************************************/
#define _RTL8812A_MP_C_
#ifdef CONFIG_MP_INCLUDED

//#include <drv_types.h>
#include <rtl8814a_hal.h>

s32 Hal_SetPowerTracking(PADAPTER padapter, u8 enable)
{
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(padapter);
	PDM_ODM_T		pDM_Odm = &(pHalData->odmpriv);


	if (!netif_running(padapter->pnetdev)) {
		RT_TRACE(_module_mp_, _drv_warning_, ("SetPowerTracking! Fail: interface not opened!\n"));
		return _FAIL;
	}

	if (check_fwstate(&padapter->mlmepriv, WIFI_MP_STATE) == _FALSE) {
		RT_TRACE(_module_mp_, _drv_warning_, ("SetPowerTracking! Fail: not in MP mode!\n"));
		return _FAIL;
	}

	if (enable)	
		pDM_Odm->RFCalibrateInfo.TxPowerTrackControl = _TRUE;	
	else
		pDM_Odm->RFCalibrateInfo.TxPowerTrackControl= _FALSE;

	return _SUCCESS;
}


void Hal_GetPowerTracking(PADAPTER padapter, u8 *enable)
{
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(padapter);
	PDM_ODM_T		pDM_Odm = &(pHalData->odmpriv);

	*enable = pDM_Odm->RFCalibrateInfo.TxPowerTrackControl;
}

static void Hal_disable_dm(PADAPTER padapter)
{
	u8 v8;
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(padapter); 
	PDM_ODM_T		pDM_Odm = &(pHalData->odmpriv);


	//3 1. disable firmware dynamic mechanism
	// disable Power Training, Rate Adaptive
	v8 = rtw_read8(padapter, REG_BCN_CTRL);
	v8 &= ~EN_BCN_FUNCTION;
	rtw_write8(padapter, REG_BCN_CTRL, v8);

	//3 2. disable driver dynamic mechanism	
	Switch_DM_Func(padapter, DYNAMIC_FUNC_DISABLE, _FALSE);

	// enable APK, LCK and IQK but disable power tracking
	pDM_Odm->RFCalibrateInfo.TxPowerTrackControl = _FALSE;
	Switch_DM_Func(padapter, ODM_RF_CALIBRATION, _TRUE);
}

/*-----------------------------------------------------------------------------
 * Function:	mpt_SwitchRfSetting
 *
 * Overview:	Change RF Setting when we siwthc channel/rate/BW for MP.
 *
 * Input:       IN	PADAPTER				pAdapter
 *
 * Output:      NONE
 *
 * Return:      NONE
 *
 * Revised History:
 * When			Who		Remark
 * 01/08/2009	MHC		Suggestion from SD3 Willis for 92S series.
 * 01/09/2009	MHC		Add CCK modification for 40MHZ. Suggestion from SD3.
 *
 *---------------------------------------------------------------------------*/
void Hal_mpt_SwitchRfSetting(PADAPTER pAdapter)
{	
	//HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(pAdapter);
	struct mp_priv	*pmp = &pAdapter->mppriv;
	u1Byte				ChannelToSw = pmp->channel;
	ULONG				ulRateIdx = pmp->rateidx;
	ULONG				ulbandwidth = pmp->bandwidth;
	HAL_DATA_TYPE		*pHalData = GET_HAL_DATA(pAdapter);
	#if 0
	// <20120525, Kordan> Dynamic mechanism for APK, asked by Dennis.
		pmp->MptCtx.backup0x52_RF_A = (u1Byte)PHY_QueryRFReg(pAdapter, RF_PATH_A, RF_0x52, 0x000F0);
		pmp->MptCtx.backup0x52_RF_B = (u1Byte)PHY_QueryRFReg(pAdapter, RF_PATH_B, RF_0x52, 0x000F0);
		PHY_SetRFReg(pAdapter, RF_PATH_A, RF_0x52, 0x000F0, 0xD);
		PHY_SetRFReg(pAdapter, RF_PATH_B, RF_0x52, 0x000F0, 0xD);
	#endif
	return ;
}
/*---------------------------hal\rtl8192c\MPT_Phy.c---------------------------*/

/*---------------------------hal\rtl8192c\MPT_HelperFunc.c---------------------------*/
void Hal_MPT_CCKTxPowerAdjust(PADAPTER Adapter, BOOLEAN bInCH14)
{
	u32		TempVal = 0, TempVal2 = 0, TempVal3 = 0;
	u32		CurrCCKSwingVal = 0, CCKSwingIndex = 12;
	u8		i;
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(Adapter);


	// get current cck swing value and check 0xa22 & 0xa23 later to match the table.
	CurrCCKSwingVal = read_bbreg(Adapter, rCCK0_TxFilter1, bMaskHWord);

	if (!bInCH14)
	{
		// Readback the current bb cck swing value and compare with the table to
		// get the current swing index
		for (i = 0; i < CCK_TABLE_SIZE; i++)
		{
			if (((CurrCCKSwingVal&0xff) == (u32)CCKSwingTable_Ch1_Ch13[i][0]) &&
				(((CurrCCKSwingVal&0xff00)>>8) == (u32)CCKSwingTable_Ch1_Ch13[i][1]))
			{
				CCKSwingIndex = i;
//				RT_TRACE(COMP_INIT, DBG_LOUD,("Ch1~13, Current reg0x%x = 0x%lx, CCKSwingIndex=0x%x\n",
//					(rCCK0_TxFilter1+2), CurrCCKSwingVal, CCKSwingIndex));
				break;
			}
		}

		//Write 0xa22 0xa23
		TempVal = CCKSwingTable_Ch1_Ch13[CCKSwingIndex][0] +
				(CCKSwingTable_Ch1_Ch13[CCKSwingIndex][1]<<8) ;


		//Write 0xa24 ~ 0xa27
		TempVal2 = 0;
		TempVal2 = CCKSwingTable_Ch1_Ch13[CCKSwingIndex][2] +
				(CCKSwingTable_Ch1_Ch13[CCKSwingIndex][3]<<8) +
				(CCKSwingTable_Ch1_Ch13[CCKSwingIndex][4]<<16 )+
				(CCKSwingTable_Ch1_Ch13[CCKSwingIndex][5]<<24);

		//Write 0xa28  0xa29
		TempVal3 = 0;
		TempVal3 = CCKSwingTable_Ch1_Ch13[CCKSwingIndex][6] +
				(CCKSwingTable_Ch1_Ch13[CCKSwingIndex][7]<<8) ;
	}
	else
	{
		for (i = 0; i < CCK_TABLE_SIZE; i++)
		{
			if (((CurrCCKSwingVal&0xff) == (u32)CCKSwingTable_Ch14[i][0]) &&
				(((CurrCCKSwingVal&0xff00)>>8) == (u32)CCKSwingTable_Ch14[i][1]))
			{
				CCKSwingIndex = i;
//				RT_TRACE(COMP_INIT, DBG_LOUD,("Ch14, Current reg0x%x = 0x%lx, CCKSwingIndex=0x%x\n",
//					(rCCK0_TxFilter1+2), CurrCCKSwingVal, CCKSwingIndex));
				break;
			}
		}

		//Write 0xa22 0xa23
		TempVal = CCKSwingTable_Ch14[CCKSwingIndex][0] +
				(CCKSwingTable_Ch14[CCKSwingIndex][1]<<8) ;

		//Write 0xa24 ~ 0xa27
		TempVal2 = 0;
		TempVal2 = CCKSwingTable_Ch14[CCKSwingIndex][2] +
				(CCKSwingTable_Ch14[CCKSwingIndex][3]<<8) +
				(CCKSwingTable_Ch14[CCKSwingIndex][4]<<16 )+
				(CCKSwingTable_Ch14[CCKSwingIndex][5]<<24);

		//Write 0xa28  0xa29
		TempVal3 = 0;
		TempVal3 = CCKSwingTable_Ch14[CCKSwingIndex][6] +
				(CCKSwingTable_Ch14[CCKSwingIndex][7]<<8) ;
	}

	write_bbreg(Adapter, rCCK0_TxFilter1, bMaskHWord, TempVal);
	write_bbreg(Adapter, rCCK0_TxFilter2, bMaskDWord, TempVal2);
	write_bbreg(Adapter, rCCK0_DebugPort, bMaskLWord, TempVal3);
}

void Hal_MPT_CCKTxPowerAdjustbyIndex(PADAPTER pAdapter, BOOLEAN beven)
{
	s32		TempCCk;
	u8		CCK_index, CCK_index_old = 0;
	u8		Action = 0;	//0: no action, 1: even->odd, 2:odd->even
	u8		TimeOut = 100;
	s32		i = 0;
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(pAdapter);
	PMPT_CONTEXT	pMptCtx = &pAdapter->mppriv.MptCtx;
	PDM_ODM_T		pDM_Odm = &(pHalData->odmpriv);

#if 1
	return;
#else
	if (!IS_92C_SERIAL(pHalData->VersionID))
		return;
#if 0
	while(PlatformAtomicExchange(&Adapter->IntrCCKRefCount, TRUE) == TRUE)
	{
		PlatformSleepUs(100);
		TimeOut--;
		if(TimeOut <= 0)
		{
			RTPRINT(FINIT, INIT_TxPower,
			 ("!!!MPT_CCKTxPowerAdjustbyIndex Wait for check CCK gain index too long!!!\n" ));
			break;
		}
	}
#endif
	if (beven && !pMptCtx->bMptIndexEven)	//odd->even
	{
		Action = 2;
		pMptCtx->bMptIndexEven = _TRUE;
	}
	else if (!beven && pMptCtx->bMptIndexEven)	//even->odd
	{
		Action = 1;
		pMptCtx->bMptIndexEven = _FALSE;
	}

	if (Action != 0)
	{
		//Query CCK default setting From 0xa24
		TempCCk = read_bbreg(pAdapter, rCCK0_TxFilter2, bMaskDWord) & bMaskCCK;
		for (i = 0; i < CCK_TABLE_SIZE; i++)
		{
			if (pDM_Odm->RFCalibrateInfo.bCCKinCH14)
			{
				if (_rtw_memcmp((void*)&TempCCk, (void*)&CCKSwingTable_Ch14[i][2], 4) == _TRUE)
				{
					CCK_index_old = (u8) i;
//					RTPRINT(FINIT, INIT_TxPower,("MPT_CCKTxPowerAdjustbyIndex: Initial reg0x%x = 0x%lx, CCK_index=0x%x, ch 14 %d\n",
//						rCCK0_TxFilter2, TempCCk, CCK_index_old, pHalData->bCCKinCH14));
					break;
				}
			}
			else
			{
				if (_rtw_memcmp((void*)&TempCCk, (void*)&CCKSwingTable_Ch1_Ch13[i][2], 4) == _TRUE)
				{
					CCK_index_old = (u8) i;
//					RTPRINT(FINIT, INIT_TxPower,("MPT_CCKTxPowerAdjustbyIndex: Initial reg0x%x = 0x%lx, CCK_index=0x%x, ch14 %d\n",
//						rCCK0_TxFilter2, TempCCk, CCK_index_old, pHalData->bCCKinCH14));
					break;
				}
			}
		}

		if (Action == 1) {
			if (CCK_index_old == 0)
				CCK_index_old = 1;
			CCK_index = CCK_index_old - 1;
		} else {
			CCK_index = CCK_index_old + 1;
		}

		if (CCK_index == CCK_TABLE_SIZE) {
			CCK_index = CCK_TABLE_SIZE -1;
			RT_TRACE(_module_mp_, _drv_info_, ("CCK_index == CCK_TABLE_SIZE\n"));
		}

//		RTPRINT(FINIT, INIT_TxPower,("MPT_CCKTxPowerAdjustbyIndex: new CCK_index=0x%x\n",
//			 CCK_index));

		//Adjust CCK according to gain index
		if (!pDM_Odm->RFCalibrateInfo.bCCKinCH14) {
			rtw_write8(pAdapter, 0xa22, CCKSwingTable_Ch1_Ch13[CCK_index][0]);
			rtw_write8(pAdapter, 0xa23, CCKSwingTable_Ch1_Ch13[CCK_index][1]);
			rtw_write8(pAdapter, 0xa24, CCKSwingTable_Ch1_Ch13[CCK_index][2]);
			rtw_write8(pAdapter, 0xa25, CCKSwingTable_Ch1_Ch13[CCK_index][3]);
			rtw_write8(pAdapter, 0xa26, CCKSwingTable_Ch1_Ch13[CCK_index][4]);
			rtw_write8(pAdapter, 0xa27, CCKSwingTable_Ch1_Ch13[CCK_index][5]);
			rtw_write8(pAdapter, 0xa28, CCKSwingTable_Ch1_Ch13[CCK_index][6]);
			rtw_write8(pAdapter, 0xa29, CCKSwingTable_Ch1_Ch13[CCK_index][7]);
		} else {
			rtw_write8(pAdapter, 0xa22, CCKSwingTable_Ch14[CCK_index][0]);
			rtw_write8(pAdapter, 0xa23, CCKSwingTable_Ch14[CCK_index][1]);
			rtw_write8(pAdapter, 0xa24, CCKSwingTable_Ch14[CCK_index][2]);
			rtw_write8(pAdapter, 0xa25, CCKSwingTable_Ch14[CCK_index][3]);
			rtw_write8(pAdapter, 0xa26, CCKSwingTable_Ch14[CCK_index][4]);
			rtw_write8(pAdapter, 0xa27, CCKSwingTable_Ch14[CCK_index][5]);
			rtw_write8(pAdapter, 0xa28, CCKSwingTable_Ch14[CCK_index][6]);
			rtw_write8(pAdapter, 0xa29, CCKSwingTable_Ch14[CCK_index][7]);
		}
	}
#if 0
	RTPRINT(FINIT, INIT_TxPower,
	("MPT_CCKTxPowerAdjustbyIndex 0xa20=%x\n", PlatformEFIORead4Byte(Adapter, 0xa20)));

	PlatformAtomicExchange(&Adapter->IntrCCKRefCount, FALSE);
#endif
#endif
}
/*---------------------------hal\rtl8192c\MPT_HelperFunc.c---------------------------*/

/*
 * SetChannel
 * Description
 *	Use H2C command to change channel,
 *	not only modify rf register, but also other setting need to be done.
 */
void Hal_SetChannel(PADAPTER pAdapter)
{
#if 0
	struct mp_priv *pmp = &pAdapter->mppriv;

//	SelectChannel(pAdapter, pmp->channel);
	set_channel_bwmode(pAdapter, pmp->channel, pmp->channel_offset, pmp->bandwidth);
#else
	u8 		eRFPath;

	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(pAdapter);
	struct mp_priv	*pmp = &pAdapter->mppriv; 
	PDM_ODM_T		pDM_Odm = &(pHalData->odmpriv);
	
	u8		channel = pmp->channel;
	u8		bandwidth = pmp->bandwidth;
	
	pHalData->bSwChnl = _TRUE;
	pHalData->bSetChnlBW = _TRUE;
	PHY_SetSwChnlBWMode8814(pAdapter, channel, bandwidth, 0, 0);

	if (pHalData->CurrentChannel == 14 && !pDM_Odm->RFCalibrateInfo.bCCKinCH14) {
		pDM_Odm->RFCalibrateInfo.bCCKinCH14 = _TRUE;
		Hal_MPT_CCKTxPowerAdjust(pAdapter, pDM_Odm->RFCalibrateInfo.bCCKinCH14);
	}
	else if (pHalData->CurrentChannel != 14 && pDM_Odm->RFCalibrateInfo.bCCKinCH14) {
		pDM_Odm->RFCalibrateInfo.bCCKinCH14 = _FALSE;
		Hal_MPT_CCKTxPowerAdjust(pAdapter, pDM_Odm->RFCalibrateInfo.bCCKinCH14);
	}

#endif
}

/*
 * Notice
 *	Switch bandwitdth may change center frequency(channel)
 */
void Hal_SetBandwidth(PADAPTER pAdapter)
{
	struct mp_priv *pmp = &pAdapter->mppriv;
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(pAdapter);

	u8		channel = pmp->channel;
	u8		bandwidth = pmp->bandwidth;

	pHalData->bSwChnl = _TRUE;
	pHalData->bSetChnlBW = _TRUE;

	PHY_SetSwChnlBWMode8814(pAdapter, channel, bandwidth, 0, 0 );

}

void Hal_SetCCKTxPower(PADAPTER pAdapter, u8 *TxPower)
{
	u32 tmpval = 0;


	// rf-A cck tx power
	write_bbreg(pAdapter, rTxAGC_A_CCK1_Mcs32, bMaskByte1, TxPower[RF_PATH_A]);
	tmpval = (TxPower[RF_PATH_A]<<16) | (TxPower[RF_PATH_A]<<8) | TxPower[RF_PATH_A];
	write_bbreg(pAdapter, rTxAGC_B_CCK11_A_CCK2_11, bMaskH3Bytes, tmpval);

	// rf-B cck tx power
	write_bbreg(pAdapter, rTxAGC_B_CCK11_A_CCK2_11, bMaskByte0, TxPower[RF_PATH_B]);
	tmpval = (TxPower[RF_PATH_B]<<16) | (TxPower[RF_PATH_B]<<8) | TxPower[RF_PATH_B];
	write_bbreg(pAdapter, rTxAGC_B_CCK1_55_Mcs32, bMaskH3Bytes, tmpval);

	RT_TRACE(_module_mp_, _drv_notice_,
		 ("-SetCCKTxPower: A[0x%02x] B[0x%02x]\n",
		  TxPower[RF_PATH_A], TxPower[RF_PATH_B]));
}

void Hal_SetOFDMTxPower(PADAPTER pAdapter, u8 *TxPower)
{
	u32 TxAGC = 0;
	u8 tmpval = 0;
	PMPT_CONTEXT	pMptCtx = &pAdapter->mppriv.MptCtx;
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(pAdapter);


	// HT Tx-rf(A)
	tmpval = TxPower[RF_PATH_A];
	TxAGC = (tmpval<<24) | (tmpval<<16) | (tmpval<<8) | tmpval;

	write_bbreg(pAdapter, rTxAGC_A_Rate18_06, bMaskDWord, TxAGC);
	write_bbreg(pAdapter, rTxAGC_A_Rate54_24, bMaskDWord, TxAGC);
	write_bbreg(pAdapter, rTxAGC_A_Mcs03_Mcs00, bMaskDWord, TxAGC);
	write_bbreg(pAdapter, rTxAGC_A_Mcs07_Mcs04, bMaskDWord, TxAGC);
	write_bbreg(pAdapter, rTxAGC_A_Mcs11_Mcs08, bMaskDWord, TxAGC);
	write_bbreg(pAdapter, rTxAGC_A_Mcs15_Mcs12, bMaskDWord, TxAGC);

	// HT Tx-rf(B)
	tmpval = TxPower[RF_PATH_B];
	TxAGC = (tmpval<<24) | (tmpval<<16) | (tmpval<<8) | tmpval;

	write_bbreg(pAdapter, rTxAGC_B_Rate18_06, bMaskDWord, TxAGC);
	write_bbreg(pAdapter, rTxAGC_B_Rate54_24, bMaskDWord, TxAGC);
	write_bbreg(pAdapter, rTxAGC_B_Mcs03_Mcs00, bMaskDWord, TxAGC);
	write_bbreg(pAdapter, rTxAGC_B_Mcs07_Mcs04, bMaskDWord, TxAGC);
	write_bbreg(pAdapter, rTxAGC_B_Mcs11_Mcs08, bMaskDWord, TxAGC);
	write_bbreg(pAdapter, rTxAGC_B_Mcs15_Mcs12, bMaskDWord, TxAGC);

}

void 
mpt_SetTxPower_8814(
		PADAPTER		pAdapter,
		MPT_TXPWR_DEF	Rate,
		pu1Byte 		pTxPower
	)
{
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(pAdapter);

	u1Byte path = 0 , i = 0, MaxRate = MGN_6M;
	u1Byte StartPath = ODM_RF_PATH_A, EndPath = ODM_RF_PATH_B;
	
	if (IS_HARDWARE_TYPE_8814A(pAdapter))
		EndPath = ODM_RF_PATH_D;

	switch (Rate)
	{
		case MPT_CCK:
		{
			u1Byte rate[] = {MGN_1M, MGN_2M, MGN_5_5M, MGN_11M};

			for (path = StartPath; path <= EndPath; path++)
				for (i = 0; i < sizeof(rate); ++i)
					PHY_SetTxPowerIndex(pAdapter, pTxPower[path], path, rate[i]);	
		}
		break;
		
		case MPT_OFDM:
	{
			u1Byte rate[] = {
				MGN_6M, MGN_9M, MGN_12M, MGN_18M,
				MGN_24M, MGN_36M, MGN_48M, MGN_54M,
				};

			for (path = StartPath; path <= EndPath; path++)
				for (i = 0; i < sizeof(rate); ++i)
					PHY_SetTxPowerIndex(pAdapter, pTxPower[path], path, rate[i]);	
		} break;
		
		case MPT_HT:
		{
			u1Byte rate[] = {
			MGN_MCS0, MGN_MCS1, MGN_MCS2, MGN_MCS3, MGN_MCS4,
			MGN_MCS5, MGN_MCS6, MGN_MCS7, MGN_MCS8, MGN_MCS9,
			MGN_MCS10, MGN_MCS11, MGN_MCS12, MGN_MCS13, MGN_MCS14,
			MGN_MCS15, MGN_MCS16, MGN_MCS17, MGN_MCS18, MGN_MCS19,
			MGN_MCS20, MGN_MCS21, MGN_MCS22, MGN_MCS23, MGN_MCS24,
			MGN_MCS25, MGN_MCS26, MGN_MCS27, MGN_MCS28, MGN_MCS29,
			MGN_MCS30, MGN_MCS31,
			};
			if (pHalData->rf_type == RF_3T3R)
				MaxRate = MGN_MCS23;
			else if (pHalData->rf_type == RF_2T2R)
				MaxRate = MGN_MCS15;
			else
				MaxRate = MGN_MCS7;
			
			for (path = StartPath; path <= EndPath; path++) {
				for (i = 0; i < sizeof(rate); ++i) {
					if (rate[i] > MaxRate)
						break;					
				    PHY_SetTxPowerIndex(pAdapter, pTxPower[path], path, rate[i]);
				}
			}
		} break;
		
		case MPT_VHT:
		{
					u1Byte rate[] = {
					MGN_VHT1SS_MCS0, MGN_VHT1SS_MCS1, MGN_VHT1SS_MCS2, MGN_VHT1SS_MCS3, MGN_VHT1SS_MCS4,
					MGN_VHT1SS_MCS5, MGN_VHT1SS_MCS6, MGN_VHT1SS_MCS7, MGN_VHT1SS_MCS8, MGN_VHT1SS_MCS9,
					MGN_VHT2SS_MCS0, MGN_VHT2SS_MCS1, MGN_VHT2SS_MCS2, MGN_VHT2SS_MCS3, MGN_VHT2SS_MCS4,
					MGN_VHT2SS_MCS5, MGN_VHT2SS_MCS6, MGN_VHT2SS_MCS7, MGN_VHT2SS_MCS8, MGN_VHT2SS_MCS9,
					MGN_VHT3SS_MCS0, MGN_VHT3SS_MCS1, MGN_VHT3SS_MCS2, MGN_VHT3SS_MCS3, MGN_VHT3SS_MCS4,
					MGN_VHT3SS_MCS5, MGN_VHT3SS_MCS6, MGN_VHT3SS_MCS7, MGN_VHT3SS_MCS8, MGN_VHT3SS_MCS9,
					MGN_VHT4SS_MCS0, MGN_VHT4SS_MCS1, MGN_VHT4SS_MCS2, MGN_VHT4SS_MCS3, MGN_VHT4SS_MCS4,
					MGN_VHT4SS_MCS5, MGN_VHT4SS_MCS6, MGN_VHT4SS_MCS7, MGN_VHT4SS_MCS8, MGN_VHT4SS_MCS9,
					};
					
					if (pHalData->rf_type == RF_3T3R)
						MaxRate = MGN_VHT3SS_MCS9;
					else if (pHalData->rf_type == RF_2T2R || pHalData->rf_type == RF_2T4R)
						MaxRate = MGN_VHT2SS_MCS9;
					else
						MaxRate = MGN_VHT1SS_MCS9;
		
					for (path = StartPath; path <= EndPath; path++) {
						for (i = 0; i < sizeof(rate); ++i) {
							if (rate[i] > MaxRate)
								break;	
							PHY_SetTxPowerIndex(pAdapter, pTxPower[path], path, rate[i]);
						}
					}
		} break;
			
		default:
			DBG_871X("<===mpt_SetTxPower_8812: Illegal channel!!\n");
			break;
	}

}

void Hal_SetAntennaPathPower(PADAPTER pAdapter)
{
	HAL_DATA_TYPE *pHalData = GET_HAL_DATA(pAdapter);
	PMPT_CONTEXT		pMptCtx = &(pAdapter->mppriv.MptCtx);
	u8 rf, TxPowerLevel[4];
	u8 StartPath = ODM_RF_PATH_A, EndPath = ODM_RF_PATH_D;
	u32	txagc_table_wd = 0x00801000;
	u8 Path = 0;
	
	pMptCtx->TxPwrLevel[ODM_RF_PATH_A] = pAdapter->mppriv.txpoweridx;
	pMptCtx->TxPwrLevel[ODM_RF_PATH_B] = pAdapter->mppriv.txpoweridx_b;
	pMptCtx->TxPwrLevel[ODM_RF_PATH_C] = pAdapter->mppriv.txpoweridx_c;
	pMptCtx->TxPwrLevel[ODM_RF_PATH_D] = pAdapter->mppriv.txpoweridx_d;
	
	txagc_table_wd |= (Path << 8) | pAdapter->mppriv.rateidx | (pMptCtx->TxPwrLevel[Path] << 24);
	PHY_SetBBReg(pAdapter, 0x1998, bMaskDWord, txagc_table_wd);  /* this is to turn on the table*/
	
	for (Path = StartPath ; Path <= EndPath ; Path++) {
		txagc_table_wd = 0x00801000;
		DBG_871X("Tx Pwr Level[ path %d] = %x,rateidx=%d\n", Path , pMptCtx->TxPwrLevel[Path], pAdapter->mppriv.rateidx);
		txagc_table_wd |= (Path  << 8) | pAdapter->mppriv.rateidx | (pMptCtx->TxPwrLevel[Path] << 24);
		PHY_SetBBReg(pAdapter, 0x1998, bMaskDWord, txagc_table_wd);
		DBG_871X("txagc_table_wd %x\n", txagc_table_wd);
	}

}

void Hal_SetTxPower(PADAPTER pAdapter)
	{
	HAL_DATA_TYPE		*pHalData	= GET_HAL_DATA(pAdapter);	
	PMPT_CONTEXT		pMptCtx = &(pAdapter->mppriv.MptCtx);
	u1Byte				path;
	PDM_ODM_T		pDM_Odm = &pHalData->odmpriv;


		DBG_871X("===> MPT_ProSetTxPower: Jaguar\n");
	mpt_SetTxPower_8814(pAdapter, MPT_CCK, pMptCtx->TxPwrLevel);
	mpt_SetTxPower_8814(pAdapter, MPT_OFDM, pMptCtx->TxPwrLevel);
	mpt_SetTxPower_8814(pAdapter, MPT_HT, pMptCtx->TxPwrLevel);
	mpt_SetTxPower_8814(pAdapter, MPT_VHT, pMptCtx->TxPwrLevel);

	ODM_ClearTxPowerTrackingState(pDM_Odm);

}

void Hal_SetTxAGCOffset(PADAPTER pAdapter, u32 ulTxAGCOffset)
{
	u32 TxAGCOffset_B, TxAGCOffset_C, TxAGCOffset_D,tmpAGC;

	TxAGCOffset_B = (ulTxAGCOffset&0x000000ff);
	TxAGCOffset_C = ((ulTxAGCOffset&0x0000ff00)>>8);
	TxAGCOffset_D = ((ulTxAGCOffset&0x00ff0000)>>16);

	tmpAGC = (TxAGCOffset_D<<8 | TxAGCOffset_C<<4 | TxAGCOffset_B);
	write_bbreg(pAdapter, rFPGA0_TxGainStage,
			(bXBTxAGC|bXCTxAGC|bXDTxAGC), tmpAGC);
}

void Hal_SetDataRate(PADAPTER pAdapter)
{
	//Hal_mpt_SwitchRfSetting(pAdapter);
}


VOID
mpt_ToggleIG_8814A(
	IN	PADAPTER	pAdapter 
	)
{
	u1Byte Path = 0;
	u4Byte IGReg = rA_IGI_Jaguar, IGvalue = 0;

	for (Path; Path <= ODM_RF_PATH_D; Path++) {
		switch (Path) {
		case ODM_RF_PATH_B:
			IGReg = rB_IGI_Jaguar;
			break;
		case ODM_RF_PATH_C:
			IGReg = rC_IGI_Jaguar2;
			break;
		case ODM_RF_PATH_D:
			IGReg = rD_IGI_Jaguar2;
			break;
		default:
			IGReg = rA_IGI_Jaguar;
			break;
		}

		IGvalue = PHY_QueryBBReg(pAdapter, IGReg, bMaskByte0);
		PHY_SetBBReg(pAdapter, IGReg, bMaskByte0, IGvalue+2);	   
		PHY_SetBBReg(pAdapter, IGReg, bMaskByte0, IGvalue);
	}
}

#define RF_PATH_AB	22
void Hal_SetAntenna(PADAPTER pAdapter)
{
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(pAdapter);
	PMPT_CONTEXT	pMptCtx = &pAdapter->mppriv.MptCtx;
	R_ANTENNA_SELECT_OFDM *p_ofdm_tx;	/* OFDM Tx register */
	R_ANTENNA_SELECT_CCK *p_cck_txrx;

	u8 ForcedDataRate = HwRateToMRate(pAdapter->mppriv.rateidx);

	u8 HtStbcCap = pAdapter->registrypriv.stbc_cap;
	//PRT_HIGH_THROUGHPUT		pHTInfo = GET_HT_INFO(pMgntInfo);
	//PRT_VERY_HIGH_THROUGHPUT	pVHTInfo = GET_VHT_INFO(pMgntInfo);

	u32				ulAntennaTx = pHalData->AntennaTxPath;
	u32				ulAntennaRx = pHalData->AntennaRxPath;
	u8				NssforRate = MgntQuery_NssTxRate (ForcedDataRate);
	
	if (NssforRate == RF_2TX) {	
		DBG_871X("===> SetAntenna 2T ForcedDataRate %d NssforRate %d AntennaTx %d\n", ForcedDataRate, NssforRate, ulAntennaTx);
		switch(ulAntennaTx){
			case ANTENNA_BC:
				pMptCtx->MptRfPath = ODM_RF_PATH_BC;
				PHY_SetBBReg(pAdapter, rTxAnt_23Nsts_Jaguar2, 0x0000fff0, 0x106);	// 0x940[15:4]=12'b0000_0100_0011
				break;

			case ANTENNA_CD:
				pMptCtx->MptRfPath = ODM_RF_PATH_CD;
				PHY_SetBBReg(pAdapter, rTxAnt_23Nsts_Jaguar2, 0x0000fff0, 0x40c);	// 0x940[15:4]=12'b0000_0100_0011
				break;

			case ANTENNA_AB: default:
				pMptCtx->MptRfPath = ODM_RF_PATH_AB;
				PHY_SetBBReg(pAdapter, rTxAnt_23Nsts_Jaguar2, 0x0000fff0, 0x043);	// 0x940[15:4]=12'b0000_0100_0011
				break;
		}
	}
	else if(NssforRate == RF_3TX)
	{
			DBG_871X("===> SetAntenna 3T ForcedDataRate %d NssforRate %d AntennaTx %d\n", ForcedDataRate, NssforRate, ulAntennaTx);
		switch(ulAntennaTx){
			case ANTENNA_BCD:
				pMptCtx->MptRfPath = ODM_RF_PATH_BCD;
				PHY_SetBBReg(pAdapter, rTxAnt_23Nsts_Jaguar2, 0x0fff0000, 0x90e);	// 0x940[27:16]=12'b0010_0100_0111
				break;
				
			case ANTENNA_ABC: default:
				pMptCtx->MptRfPath = ODM_RF_PATH_ABC;
				PHY_SetBBReg(pAdapter, rTxAnt_23Nsts_Jaguar2, 0x0fff0000, 0x247);	// 0x940[27:16]=12'b0010_0100_0111
				break;
		}
	}
	else //if(NssforRate == RF_1TX)
	{
		DBG_871X("===> SetAntenna 1T ForcedDataRate %d NssforRate %d AntennaTx %d\n", ForcedDataRate, NssforRate, ulAntennaTx);
		switch(ulAntennaTx){
			case ANTENNA_B:
				pMptCtx->MptRfPath = ODM_RF_PATH_B;
				PHY_SetBBReg(pAdapter, rRxPath_Jaguar2, 0xf0000000, 0x4);			// 0xa07[7:4] = 4'b0100
				PHY_SetBBReg(pAdapter, rTxAnt_1Nsts_Jaguar2, 0xfff00000, 0x002);	// 0x93C[31:20]=12'b0000_0000_0010
				PHY_SetBBReg(pAdapter, rTxPath_Jaguar, 0xf0, 0x2);					// 0x80C[7:4] = 4'b0010
			    	break;
					
			case ANTENNA_C:
				pMptCtx->MptRfPath = ODM_RF_PATH_C;
				PHY_SetBBReg(pAdapter, rRxPath_Jaguar2, 0xf0000000, 0x2);			// 0xa07[7:4] = 4'b0010
				PHY_SetBBReg(pAdapter, rTxAnt_1Nsts_Jaguar2, 0xfff00000, 0x004);	// 0x93C[31:20]=12'b0000_0000_0100
				PHY_SetBBReg(pAdapter, rTxPath_Jaguar, 0xf0, 0x4);					// 0x80C[7:4] = 4'b0100
			    	break;
					
			case ANTENNA_D:
				pMptCtx->MptRfPath = ODM_RF_PATH_D;
				PHY_SetBBReg(pAdapter, rRxPath_Jaguar2, 0xf0000000, 0x1);			// 0xa07[7:4] = 4'b0001
				PHY_SetBBReg(pAdapter, rTxAnt_1Nsts_Jaguar2, 0xfff00000, 0x008);	// 0x93C[31:20]=12'b0000_0000_1000
				PHY_SetBBReg(pAdapter, rTxPath_Jaguar, 0xf0, 0x8);					// 0x80C[7:4] = 4'b1000
			    	break;

			case ANTENNA_A: default:
				pMptCtx->MptRfPath = ODM_RF_PATH_A;
				PHY_SetBBReg(pAdapter, rRxPath_Jaguar2, 0xf0000000, 0x8);			// 0xa07[7:4] = 4'b1000
				PHY_SetBBReg(pAdapter, rTxAnt_1Nsts_Jaguar2, 0xfff00000, 0x001);	// 0x93C[31:20]=12'b0000_0000_0001
				PHY_SetBBReg(pAdapter, rTxPath_Jaguar, 0xf0, 0x1);					// 0x80C[7:4] = 4'b0001
				break;
			}
	}
	
	switch(ulAntennaRx){
		case ANTENNA_A:
			PHY_SetBBReg(pAdapter, 0x1000, bMaskByte2, 0x2);
			PHY_SetBBReg(pAdapter, rRxPath_Jaguar, bMaskByte0, 0x11);
			PHY_SetBBReg(pAdapter, 0x1000, bMaskByte2, 0x3);
			PHY_SetBBReg(pAdapter, rRxPath_Jaguar2, 0x0C000000, 0x0);
			PHY_SetBBReg(pAdapter, rAGC_table_Jaguar, 0x0F000000, 0x6);
			PHY_SetBBReg(pAdapter, rPwed_TH_Jaguar, 0x0000000F, 0xB);
			PHY_SetRFReg(pAdapter, ODM_RF_PATH_A, RF_AC_Jaguar, 0xF0000, 0x3); // RF_A_0x0[19:16] = 3, RX mode
			PHY_SetRFReg(pAdapter, ODM_RF_PATH_B, RF_AC_Jaguar, 0xF0000, 0x1); // RF_B_0x0[19:16] = 1, Standby mode
			PHY_SetRFReg(pAdapter, ODM_RF_PATH_C, RF_AC_Jaguar, 0xF0000, 0x1); // RF_C_0x0[19:16] = 1, Standby mode
			PHY_SetRFReg(pAdapter, ODM_RF_PATH_D, RF_AC_Jaguar, 0xF0000, 0x1); // RF_D_0x0[19:16] = 1, Standby mode
			break;
				
		case ANTENNA_B:
			PHY_SetBBReg(pAdapter, 0x1000, bMaskByte2, 0x2);
			PHY_SetBBReg(pAdapter, rRxPath_Jaguar, bMaskByte0, 0x22);	
			PHY_SetBBReg(pAdapter, 0x1000, bMaskByte2, 0x3);
			PHY_SetBBReg(pAdapter, rRxPath_Jaguar2, 0x0C000000, 0x1);	
			PHY_SetBBReg(pAdapter, rAGC_table_Jaguar, 0x0F000000, 0x6);
			PHY_SetBBReg(pAdapter, rPwed_TH_Jaguar, 0x0000000F, 0xB);
			PHY_SetRFReg(pAdapter, ODM_RF_PATH_A, RF_AC_Jaguar, 0xF0000, 0x1); // RF_A_0x0[19:16] = 1, Standby mode
			PHY_SetRFReg(pAdapter, ODM_RF_PATH_B, RF_AC_Jaguar, 0xF0000, 0x3); // RF_B_0x0[19:16] = 3, RX mode
			PHY_SetRFReg(pAdapter, ODM_RF_PATH_C, RF_AC_Jaguar, 0xF0000, 0x1); // RF_C_0x0[19:16] = 1, Standby mode
			PHY_SetRFReg(pAdapter, ODM_RF_PATH_D, RF_AC_Jaguar, 0xF0000, 0x1); // RF_D_0x0[19:16] = 1, Standby mode
			break;
	
		case ANTENNA_C:
			PHY_SetBBReg(pAdapter, 0x1000, bMaskByte2, 0x2);
			PHY_SetBBReg(pAdapter, rRxPath_Jaguar, bMaskByte0, 0x44);	
			PHY_SetBBReg(pAdapter, 0x1000, bMaskByte2, 0x3);
			PHY_SetBBReg(pAdapter, rRxPath_Jaguar2, 0x0C000000, 0x2);
			PHY_SetBBReg(pAdapter, rAGC_table_Jaguar, 0x0F000000, 0x6);
			PHY_SetBBReg(pAdapter, rPwed_TH_Jaguar, 0x0000000F, 0xB);
			PHY_SetRFReg(pAdapter, ODM_RF_PATH_A, RF_AC_Jaguar, 0xF0000, 0x1); // RF_A_0x0[19:16] = 1, Standby mode
			PHY_SetRFReg(pAdapter, ODM_RF_PATH_B, RF_AC_Jaguar, 0xF0000, 0x1); // RF_B_0x0[19:16] = 1, Standby mode
			PHY_SetRFReg(pAdapter, ODM_RF_PATH_C, RF_AC_Jaguar, 0xF0000, 0x3); // RF_C_0x0[19:16] = 3, RX mode
			PHY_SetRFReg(pAdapter, ODM_RF_PATH_D, RF_AC_Jaguar, 0xF0000, 0x1); // RF_D_0x0[19:16] = 1, Standby mode
			break;
	
		case ANTENNA_D:
			PHY_SetBBReg(pAdapter, 0x1000, bMaskByte2, 0x2);
			PHY_SetBBReg(pAdapter, rRxPath_Jaguar, bMaskByte0, 0x88);	
			PHY_SetBBReg(pAdapter, 0x1000, bMaskByte2, 0x3);
			PHY_SetBBReg(pAdapter, rRxPath_Jaguar2, 0x0C000000, 0x3);
			PHY_SetBBReg(pAdapter, rAGC_table_Jaguar, 0x0F000000, 0x6);
			PHY_SetBBReg(pAdapter, rPwed_TH_Jaguar, 0x0000000F, 0xB);
			PHY_SetRFReg(pAdapter, ODM_RF_PATH_A, RF_AC_Jaguar, 0xF0000, 0x1); // RF_A_0x0[19:16] = 1, Standby mode
			PHY_SetRFReg(pAdapter, ODM_RF_PATH_B, RF_AC_Jaguar, 0xF0000, 0x1); // RF_B_0x0[19:16] = 1, Standby mode
			PHY_SetRFReg(pAdapter, ODM_RF_PATH_C, RF_AC_Jaguar, 0xF0000, 0x1); // RF_C_0x0[19:16] = 1, Standby mode
			PHY_SetRFReg(pAdapter, ODM_RF_PATH_D, RF_AC_Jaguar, 0xF0000, 0x3); // RF_D_0x0[19:16] = 3, RX mode
			break;
	
		case ANTENNA_BC: 
			PHY_SetBBReg(pAdapter, rAGC_table_Jaguar, 0x0F000000, 0x5);
			PHY_SetBBReg(pAdapter, rPwed_TH_Jaguar, 0x0000000F, 0xA);
			PHY_SetBBReg(pAdapter, 0x1000, bMaskByte2, 0x2);
			PHY_SetBBReg(pAdapter, rRxPath_Jaguar, bMaskByte0, 0x66);
			PHY_SetBBReg(pAdapter, 0x1000, bMaskByte2, 0x3);
			PHY_SetBBReg(pAdapter, rRxPath_Jaguar2, 0x0C000000, 0x1);
			PHY_SetRFReg(pAdapter, ODM_RF_PATH_A, RF_AC_Jaguar, 0xF0000, 0x1); // RF_A_0x0[19:16] = 1, Standby mode
			PHY_SetRFReg(pAdapter, ODM_RF_PATH_B, RF_AC_Jaguar, 0xF0000, 0x3); // RF_B_0x0[19:16] = 3, RX mode
			PHY_SetRFReg(pAdapter, ODM_RF_PATH_C, RF_AC_Jaguar, 0xF0000, 0x3); // RF_C_0x0[19:16] = 3, Rx mode
			PHY_SetRFReg(pAdapter, ODM_RF_PATH_D, RF_AC_Jaguar, 0xF0000, 0x1); // RF_D_0x0[19:16] = 1, Standby mode
			break;
	
		case ANTENNA_CD: 
			PHY_SetBBReg(pAdapter, rAGC_table_Jaguar, 0x0F000000, 0x5);
			PHY_SetBBReg(pAdapter, rPwed_TH_Jaguar, 0x0000000F, 0xA);
			PHY_SetBBReg(pAdapter, 0x1000, bMaskByte2, 0x2);
			PHY_SetBBReg(pAdapter, rRxPath_Jaguar, bMaskByte0, 0xcc);
			PHY_SetBBReg(pAdapter, 0x1000, bMaskByte2, 0x3);
			PHY_SetBBReg(pAdapter, rRxPath_Jaguar2, 0x0C000000, 0x10);
			PHY_SetRFReg(pAdapter, ODM_RF_PATH_A, RF_AC_Jaguar, 0xF0000, 0x1); // RF_A_0x0[19:16] = 1, Standby mode
			PHY_SetRFReg(pAdapter, ODM_RF_PATH_B, RF_AC_Jaguar, 0xF0000, 0x1); // RF_B_0x0[19:16] = 1, Standby mode
			PHY_SetRFReg(pAdapter, ODM_RF_PATH_C, RF_AC_Jaguar, 0xF0000, 0x3); // RF_C_0x0[19:16] = 3, Rx mode
			PHY_SetRFReg(pAdapter, ODM_RF_PATH_D, RF_AC_Jaguar, 0xF0000, 0x3); // RF_D_0x0[19:16] = 3, RX mode
			break;
	
		case ANTENNA_BCD: 
			PHY_SetBBReg(pAdapter, rAGC_table_Jaguar, 0x0F000000, 0x3);
			PHY_SetBBReg(pAdapter, rPwed_TH_Jaguar, 0x0000000F, 0x8);
			PHY_SetBBReg(pAdapter, 0x1000, bMaskByte2, 0x2);
			PHY_SetBBReg(pAdapter, rRxPath_Jaguar, bMaskByte0, 0xee);
			PHY_SetBBReg(pAdapter, 0x1000, bMaskByte2, 0x3);
			PHY_SetBBReg(pAdapter, rRxPath_Jaguar2, 0x0C000000, 0x1);
			PHY_SetRFReg(pAdapter, ODM_RF_PATH_A, RF_AC_Jaguar, 0xF0000, 0x1); // RF_A_0x0[19:16] = 1, Standby mode
			PHY_SetRFReg(pAdapter, ODM_RF_PATH_B, RF_AC_Jaguar, 0xF0000, 0x3); // RF_B_0x0[19:16] = 3, RX mode
			PHY_SetRFReg(pAdapter, ODM_RF_PATH_C, RF_AC_Jaguar, 0xF0000, 0x3); // RF_C_0x0[19:16] = 3, RX mode
			PHY_SetRFReg(pAdapter, ODM_RF_PATH_D, RF_AC_Jaguar, 0xF0000, 0x3); // RF_D_0x0[19:16] = 3, Rx mode
			break;
	
		case ANTENNA_ABCD: 
			PHY_SetBBReg(pAdapter, rAGC_table_Jaguar, 0x0F000000, 0x3);
			PHY_SetBBReg(pAdapter, rPwed_TH_Jaguar, 0x0000000F, 0x8);
			PHY_SetBBReg(pAdapter, 0x1000, bMaskByte2, 0x2);
			PHY_SetBBReg(pAdapter, rRxPath_Jaguar, bMaskByte0, 0xff);
			PHY_SetBBReg(pAdapter, 0x1000, bMaskByte2, 0x3);
			PHY_SetBBReg(pAdapter, rRxPath_Jaguar2, 0x0C000000, 0x0);
			PHY_SetRFReg(pAdapter, ODM_RF_PATH_A, RF_AC_Jaguar, 0xF0000, 0x3); // RF_A_0x0[19:16] = 3, RX mode
			PHY_SetRFReg(pAdapter, ODM_RF_PATH_B, RF_AC_Jaguar, 0xF0000, 0x3); // RF_B_0x0[19:16] = 3, RX mode
			PHY_SetRFReg(pAdapter, ODM_RF_PATH_C, RF_AC_Jaguar, 0xF0000, 0x3); // RF_C_0x0[19:16] = 3, RX mode
			PHY_SetRFReg(pAdapter, ODM_RF_PATH_D, RF_AC_Jaguar, 0xF0000, 0x3); // RF_D_0x0[19:16] = 3, RX mode
			break;
			
		default:
			RT_TRACE(_module_mp_, _drv_warning_, ("Unknown Rx antenna.\n"));
			break;
	}
	mpt_ToggleIG_8814A(pAdapter);
	RT_TRACE(_module_mp_, _drv_notice_, ("-SwitchAntenna: finished\n"));
}

s32 Hal_SetThermalMeter(PADAPTER pAdapter, u8 target_ther)
{
	HAL_DATA_TYPE *pHalData = GET_HAL_DATA(pAdapter);


	if (!netif_running(pAdapter->pnetdev)) {
		RT_TRACE(_module_mp_, _drv_warning_, ("SetThermalMeter! Fail: interface not opened!\n"));
		return _FAIL;
	}

	if (check_fwstate(&pAdapter->mlmepriv, WIFI_MP_STATE) == _FALSE) {
		RT_TRACE(_module_mp_, _drv_warning_, ("SetThermalMeter: Fail! not in MP mode!\n"));
		return _FAIL;
	}

	target_ther &= 0xff;
	if (target_ther < 0x07)
		target_ther = 0x07;
	else if (target_ther > 0x1d)
		target_ther = 0x1d;

	pHalData->EEPROMThermalMeter = target_ther;

	return _SUCCESS;
}

void Hal_TriggerRFThermalMeter(PADAPTER pAdapter)
{
	PHY_SetRFReg(pAdapter, ODM_RF_PATH_A, RF_T_METER_8814A, BIT17 | BIT16, 0x03);

//	RT_TRACE(_module_mp_,_drv_alert_, ("TriggerRFThermalMeter() finished.\n" ));
}

u8 Hal_ReadRFThermalMeter(PADAPTER pAdapter)
{
	u32 ThermalValue = 0;

	ThermalValue = (u1Byte)PHY_QueryRFReg(pAdapter, ODM_RF_PATH_A, RF_T_METER_8814A, 0xfc00);	// 0x42: RF Reg[15:10]	
	
//	RT_TRACE(_module_mp_, _drv_alert_, ("ThermalValue = 0x%x\n", ThermalValue));
	return (u8)ThermalValue;
}

void Hal_GetThermalMeter(PADAPTER pAdapter, u8 *value)
{
#if 0
	fw_cmd(pAdapter, IOCMD_GET_THERMAL_METER);
	rtw_msleep_os(1000);
	fw_cmd_data(pAdapter, value, 1);
	*value &= 0xFF;
#else

	Hal_TriggerRFThermalMeter(pAdapter);
	rtw_msleep_os(1000);
	*value = Hal_ReadRFThermalMeter(pAdapter);
#endif
}

void Hal_SetSingleCarrierTx(PADAPTER pAdapter, u8 bStart)
{
    HAL_DATA_TYPE *pHalData = GET_HAL_DATA(pAdapter);
	pAdapter->mppriv.MptCtx.bSingleCarrier = bStart;
	if (bStart)// Start Single Carrier.
	{
		RT_TRACE(_module_mp_,_drv_alert_, ("SetSingleCarrierTx: test start\n"));
		// 1. if OFDM block on?
		if(!read_bbreg(pAdapter, rFPGA0_RFMOD, bOFDMEn))
			write_bbreg(pAdapter, rFPGA0_RFMOD, bOFDMEn, bEnable);//set OFDM block on

		// 2. set CCK test mode off, set to CCK normal mode
		write_bbreg(pAdapter, rCCK0_System, bCCKBBMode, bDisable);
		// 3. turn on scramble setting
		write_bbreg(pAdapter, rCCK0_System, bCCKScramble, bEnable);

		// 4. Turn On Continue Tx and turn off the other test modes.
		if (IS_HARDWARE_TYPE_JAGUAR(pAdapter) || IS_HARDWARE_TYPE_8814A(pAdapter)|| IS_HARDWARE_TYPE_8822B(pAdapter))
	
			PHY_SetBBReg(pAdapter, rSingleTone_ContTx_Jaguar, BIT18|BIT17|BIT16, OFDM_SingleCarrier);
		else
			PHY_SetBBReg(pAdapter, rOFDM1_LSTF, BIT30|BIT29|BIT28, OFDM_SingleCarrier);
			
		//for dynamic set Power index.
		write_bbreg(pAdapter, rFPGA0_XA_HSSIParameter1, bMaskDWord, 0x01000500);
		write_bbreg(pAdapter, rFPGA0_XB_HSSIParameter1, bMaskDWord, 0x01000500);
		
	}
	else// Stop Single Carrier.
	{
		RT_TRACE(_module_mp_,_drv_alert_, ("SetSingleCarrierTx: test stop\n"));
		 //Turn off all test modes.
		 if (IS_HARDWARE_TYPE_JAGUAR(pAdapter) || IS_HARDWARE_TYPE_8814A(pAdapter)|| IS_HARDWARE_TYPE_8822B(pAdapter))
			PHY_SetBBReg(pAdapter, rSingleTone_ContTx_Jaguar, BIT18|BIT17|BIT16, OFDM_ALL_OFF);
		else
			PHY_SetBBReg(pAdapter, rOFDM1_LSTF, BIT30|BIT29|BIT28, OFDM_ALL_OFF);

		//Delay 10 ms //delay_ms(10);
		rtw_msleep_os(10);

		//BB Reset
		write_bbreg(pAdapter, rPMAC_Reset, bBBResetB, 0x0);
		write_bbreg(pAdapter, rPMAC_Reset, bBBResetB, 0x1);

		//Stop for dynamic set Power index.
		write_bbreg(pAdapter, rFPGA0_XA_HSSIParameter1, bMaskDWord, 0x01000100);
		write_bbreg(pAdapter, rFPGA0_XB_HSSIParameter1, bMaskDWord, 0x01000100);
		
	}
}



VOID
mpt_SetSingleTone_8814A(
	IN	PADAPTER		pAdapter,
	IN	BOOLEAN 		bSingleTone, 
	IN	BOOLEAN 		bEnPMacTx
	)
{

	PMPT_CONTEXT	pMptCtx = &(pAdapter->mppriv.MptCtx);
	u1Byte StartPath = ODM_RF_PATH_A,  EndPath = ODM_RF_PATH_A;
	static u4Byte		regIG0 = 0, regIG1 = 0, regIG2 = 0, regIG3 = 0;

	if(bSingleTone)
	{		
		regIG0= PHY_QueryBBReg(pAdapter, rA_TxScale_Jaguar, bMaskDWord);		// 0xC1C[31:21]
		regIG1 = PHY_QueryBBReg(pAdapter, rB_TxScale_Jaguar, bMaskDWord);		// 0xE1C[31:21]
		regIG2 = PHY_QueryBBReg(pAdapter, rC_TxScale_Jaguar2, bMaskDWord);	// 0x181C[31:21]
		regIG3 = PHY_QueryBBReg(pAdapter, rD_TxScale_Jaguar2, bMaskDWord);	// 0x1A1C[31:21]

		switch(pMptCtx->MptRfPath){
		case ODM_RF_PATH_A: case ODM_RF_PATH_B:
		case ODM_RF_PATH_C: case ODM_RF_PATH_D:
			StartPath = pMptCtx->MptRfPath;
			EndPath = pMptCtx->MptRfPath;
			break;
		case ODM_RF_PATH_AB:
			EndPath = ODM_RF_PATH_B;
			break;
		case ODM_RF_PATH_BC:
			StartPath = ODM_RF_PATH_B;
			EndPath = ODM_RF_PATH_C;
			break;
		case ODM_RF_PATH_ABC:
			EndPath = ODM_RF_PATH_C;
			break;
		case ODM_RF_PATH_BCD:
			StartPath = ODM_RF_PATH_B;
			EndPath = ODM_RF_PATH_D;
			break;
		case ODM_RF_PATH_ABCD:
			EndPath = ODM_RF_PATH_D;
			break;
		}

		if(bEnPMacTx == FALSE)
		{
			//mpt_StartOfdmContTx(pAdapter);
			Hal_SetOFDMContinuousTx(pAdapter,_TRUE);
			issue_nulldata(pAdapter, NULL, 1, 3, 500);
			//SendPSPoll(pAdapter);
		}

		PHY_SetBBReg(pAdapter, rCCAonSec_Jaguar, BIT1, 0x1); // Disable CCA

		for(StartPath; StartPath <= EndPath; StartPath++)
		{
			PHY_SetRFReg(pAdapter, StartPath, RF_AC_Jaguar, 0xF0000, 0x2); // Tx mode: RF0x00[19:16]=4'b0010 
			PHY_SetRFReg(pAdapter, StartPath, RF_AC_Jaguar, 0x1F, 0x0); // Lowest RF gain index: RF_0x0[4:0] = 0

			PHY_SetRFReg(pAdapter, StartPath, LNA_Low_Gain_3, BIT1, 0x1); // RF LO enabled
		}

		PHY_SetBBReg(pAdapter, rA_TxScale_Jaguar, 0xFFE00000, 0); // 0xC1C[31:21]
		PHY_SetBBReg(pAdapter, rB_TxScale_Jaguar, 0xFFE00000, 0); // 0xE1C[31:21]
		PHY_SetBBReg(pAdapter, rC_TxScale_Jaguar2, 0xFFE00000, 0); // 0x181C[31:21]
		PHY_SetBBReg(pAdapter, rD_TxScale_Jaguar2, 0xFFE00000, 0); // 0x1A1C[31:21]
	}
	else
	{
		switch(pMptCtx->MptRfPath){
			case ODM_RF_PATH_A: case ODM_RF_PATH_B:
			case ODM_RF_PATH_C: case ODM_RF_PATH_D:
				StartPath = pMptCtx->MptRfPath;
				EndPath = pMptCtx->MptRfPath;
				break;
			case ODM_RF_PATH_AB:
				EndPath = ODM_RF_PATH_B;
				break;
			case ODM_RF_PATH_BC:
				StartPath = ODM_RF_PATH_B;
				EndPath = ODM_RF_PATH_C;
				break;
			case ODM_RF_PATH_ABC:
				EndPath = ODM_RF_PATH_C;
				break;
			case ODM_RF_PATH_BCD:
				StartPath = ODM_RF_PATH_B;
				EndPath = ODM_RF_PATH_D;
				break;
			case ODM_RF_PATH_ABCD:
				EndPath = ODM_RF_PATH_D;
				break;
		}
		for(StartPath; StartPath <= EndPath; StartPath++)
		{
			PHY_SetRFReg(pAdapter, StartPath, LNA_Low_Gain_3, BIT1, 0x0); // RF LO disabled
		}	
		PHY_SetBBReg(pAdapter, rCCAonSec_Jaguar, BIT1, 0x0); // Enable CCA

		if(bEnPMacTx == FALSE)
		{
			//mpt_StopOfdmContTx(pAdapter);
			Hal_SetOFDMContinuousTx(pAdapter,_FALSE);
		}

		PHY_SetBBReg(pAdapter, rA_TxScale_Jaguar, bMaskDWord, regIG0); // 0xC1C[31:21]
		PHY_SetBBReg(pAdapter, rB_TxScale_Jaguar, bMaskDWord, regIG1); // 0xE1C[31:21]
		PHY_SetBBReg(pAdapter, rC_TxScale_Jaguar2, bMaskDWord, regIG2); // 0x181C[31:21]
		PHY_SetBBReg(pAdapter, rD_TxScale_Jaguar2, bMaskDWord, regIG3); // 0x1A1C[31:21]
	}
}


void Hal_SetSingleToneTx(PADAPTER pAdapter, u8 bStart)
{
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(pAdapter);
	PMPT_CONTEXT	pMptCtx = &pAdapter->mppriv.MptCtx;
	

	u8 rfPath;
	u32              reg58 = 0x0;
	static u4Byte       regRF0x0 = 0x0;
    static u4Byte       reg0xCB0 = 0x0;
    static u4Byte       reg0xEB0 = 0x0;
    static u4Byte       reg0xCB4 = 0x0;
    static u4Byte       reg0xEB4 = 0x0;
	switch (pAdapter->mppriv.antenna_tx)
	{
		case ANTENNA_A:
		default:
			pMptCtx->MptRfPath = rfPath = RF_PATH_A;
			break;
		case ANTENNA_B:
			pMptCtx->MptRfPath = rfPath = RF_PATH_B;
			break;
		case ANTENNA_C:
			pMptCtx->MptRfPath = rfPath = RF_PATH_C;
			break;
	}

	pAdapter->mppriv.MptCtx.bSingleTone = bStart;
	if (bStart)// Start Single Tone.
	{
		if(IS_HARDWARE_TYPE_8814A(pAdapter))
			mpt_SetSingleTone_8814A(pAdapter, TRUE, FALSE);
		else
		{
			// Turn On SingleTone and turn off the other test modes.
			PHY_SetBBReg(pAdapter, rOFDM1_LSTF, BIT30|BIT29|BIT28, OFDM_SingleTone);			
		}

		//for dynamic set Power index.
		write_bbreg(pAdapter, rFPGA0_XA_HSSIParameter1, bMaskDWord, 0x01000500);
		write_bbreg(pAdapter, rFPGA0_XB_HSSIParameter1, bMaskDWord, 0x01000500);
		
	}
	else// Stop Single Tone.
	{
		if(IS_HARDWARE_TYPE_8814A(pAdapter))
			mpt_SetSingleTone_8814A(pAdapter, FALSE, FALSE);

		//Stop for dynamic set Power index.
		write_bbreg(pAdapter, rFPGA0_XA_HSSIParameter1, bMaskDWord, 0x01000100);
		write_bbreg(pAdapter, rFPGA0_XB_HSSIParameter1, bMaskDWord, 0x01000100);
		
	}
	
}



void Hal_SetCarrierSuppressionTx(PADAPTER pAdapter, u8 bStart)
{
	pAdapter->mppriv.MptCtx.bCarrierSuppression = bStart;
	if (bStart) // Start Carrier Suppression.
	{
		RT_TRACE(_module_mp_,_drv_alert_, ("SetCarrierSuppressionTx: test start\n"));
		//if(pMgntInfo->dot11CurrentWirelessMode == WIRELESS_MODE_B)
		if (pAdapter->mppriv.rateidx <= MPT_RATE_11M)
		  {
			// 1. if CCK block on?
			if(!read_bbreg(pAdapter, rFPGA0_RFMOD, bCCKEn))
				write_bbreg(pAdapter, rFPGA0_RFMOD, bCCKEn, bEnable);//set CCK block on

			//Turn Off All Test Mode
			
			if (IS_HARDWARE_TYPE_JAGUAR(pAdapter) || IS_HARDWARE_TYPE_8814A(pAdapter)|| IS_HARDWARE_TYPE_8822B(pAdapter))
				PHY_SetBBReg(pAdapter, rSingleTone_ContTx_Jaguar, BIT18|BIT17|BIT16, OFDM_ALL_OFF);
			else
				PHY_SetBBReg(pAdapter, rOFDM1_LSTF, BIT30|BIT29|BIT28, OFDM_ALL_OFF);

			PHY_SetBBReg(pAdapter, rCCK0_System, bCCKBBMode, 0x2);    //transmit mode
			PHY_SetBBReg(pAdapter, rCCK0_System, bCCKScramble, 0x0);  //turn off scramble setting
       		//Set CCK Tx Test Rate
			//PHY_SetBBReg(pAdapter, rCCK0_System, bCCKTxRate, pMgntInfo->ForcedDataRate);
			PHY_SetBBReg(pAdapter, rCCK0_System, bCCKTxRate, 0x0);    //Set FTxRate to 1Mbps
		}

		//for dynamic set Power index.
		write_bbreg(pAdapter, rFPGA0_XA_HSSIParameter1, bMaskDWord, 0x01000500);
		write_bbreg(pAdapter, rFPGA0_XB_HSSIParameter1, bMaskDWord, 0x01000500);
		
	}
	else// Stop Carrier Suppression.
	{
		RT_TRACE(_module_mp_,_drv_alert_, ("SetCarrierSuppressionTx: test stop\n"));
		//if(pMgntInfo->dot11CurrentWirelessMode == WIRELESS_MODE_B)
		if (pAdapter->mppriv.rateidx <= MPT_RATE_11M ) {
			PHY_SetBBReg(pAdapter, rCCK0_System, bCCKBBMode, 0x0);    //normal mode
			PHY_SetBBReg(pAdapter, rCCK0_System, bCCKScramble, 0x1);  //turn on scramble setting

			//BB Reset
			PHY_SetBBReg(pAdapter, rPMAC_Reset, bBBResetB, 0x0);
			PHY_SetBBReg(pAdapter, rPMAC_Reset, bBBResetB, 0x1);
		}

		//Stop for dynamic set Power index.
		write_bbreg(pAdapter, rFPGA0_XA_HSSIParameter1, bMaskDWord, 0x01000100);
		write_bbreg(pAdapter, rFPGA0_XB_HSSIParameter1, bMaskDWord, 0x01000100);
		
	}
	//DbgPrint("\n MPT_ProSetCarrierSupp() is finished. \n");
}

void Hal_SetCCKContinuousTx(PADAPTER pAdapter, u8 bStart)
{
	u32 cckrate;

	if (bStart)
	{
		RT_TRACE(_module_mp_, _drv_alert_,
			 ("SetCCKContinuousTx: test start\n"));

		// 1. if CCK block on?
		if(!read_bbreg(pAdapter, rFPGA0_RFMOD, bCCKEn))
			write_bbreg(pAdapter, rFPGA0_RFMOD, bCCKEn, bEnable);//set CCK block on

		//Turn Off All Test Mode
		if (IS_HARDWARE_TYPE_JAGUAR_AND_JAGUAR2(pAdapter))
			PHY_SetBBReg(pAdapter, rSingleTone_ContTx_Jaguar, BIT18|BIT17|BIT16, OFDM_ALL_OFF);
		else
			PHY_SetBBReg(pAdapter, rOFDM1_LSTF, BIT30|BIT29|BIT28, OFDM_ALL_OFF);
		//Set CCK Tx Test Rate
		#if 0
		switch(pAdapter->mppriv.rateidx)
		{
			case 2:
				cckrate = 0;
				break;
			case 4:
				cckrate = 1;
				break;
			case 11:
				cckrate = 2;
				break;
			case 22:
				cckrate = 3;
				break;
			default:
				cckrate = 0;
				break;
		}
		#else
		cckrate  = pAdapter->mppriv.rateidx;
		#endif
		PHY_SetBBReg(pAdapter, rCCK0_System, bCCKTxRate, cckrate);

		PHY_SetBBReg(pAdapter, rCCK0_System, bCCKBBMode, 0x2);    //transmit mode
		PHY_SetBBReg(pAdapter, rCCK0_System, bCCKScramble, 0x1);  //turn on scramble setting

		PHY_SetBBReg(pAdapter, rFPGA0_XA_HSSIParameter1, bMaskDWord, 0x01000500);
		PHY_SetBBReg(pAdapter, rFPGA0_XB_HSSIParameter1, bMaskDWord, 0x01000500);

		pAdapter->mppriv.MptCtx.bCckContTx = TRUE;
		pAdapter->mppriv.MptCtx.bOfdmContTx = FALSE;

	}
	else {
		RT_TRACE(_module_mp_, _drv_info_,
			 ("SetCCKContinuousTx: test stop\n"));

	pAdapter->mppriv.MptCtx.bCckContTx = FALSE;
	pAdapter->mppriv.MptCtx.bOfdmContTx = FALSE;

	PHY_SetBBReg(pAdapter, rCCK0_System, bCCKBBMode, 0x0);    //normal mode
	PHY_SetBBReg(pAdapter, rCCK0_System, bCCKScramble, 0x1);  //turn on scramble setting

		//BB Reset
	PHY_SetBBReg(pAdapter, rPMAC_Reset, bBBResetB, 0x0);
	PHY_SetBBReg(pAdapter, rPMAC_Reset, bBBResetB, 0x1);

	PHY_SetBBReg(pAdapter, rFPGA0_XA_HSSIParameter1, bMaskDWord, 0x01000100);
	PHY_SetBBReg(pAdapter, rFPGA0_XB_HSSIParameter1, bMaskDWord, 0x01000100);
	}
}/* mpt_StartCckContTx */

void Hal_SetOFDMContinuousTx(PADAPTER pAdapter, u8 bStart)
{
    HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(pAdapter);

	if (bStart) {
		RT_TRACE(_module_mp_, _drv_info_, ("SetOFDMContinuousTx: test start\n"));
		// 1. if OFDM block on?
		if(!read_bbreg(pAdapter, rFPGA0_RFMOD, bOFDMEn))
			write_bbreg(pAdapter, rFPGA0_RFMOD, bOFDMEn, bEnable);//set OFDM block on
        

		// 2. set CCK test mode off, set to CCK normal mode
		write_bbreg(pAdapter, rCCK0_System, bCCKBBMode, bDisable);

		// 3. turn on scramble setting
		write_bbreg(pAdapter, rCCK0_System, bCCKScramble, bEnable);
        
		// 4. Turn On Continue Tx and turn off the other test modes.
		if (IS_HARDWARE_TYPE_JAGUAR(pAdapter) || IS_HARDWARE_TYPE_8814A(pAdapter)|| IS_HARDWARE_TYPE_8822B(pAdapter))
			PHY_SetBBReg(pAdapter, rSingleTone_ContTx_Jaguar, BIT18|BIT17|BIT16, OFDM_ContinuousTx);
		else
			PHY_SetBBReg(pAdapter, rOFDM1_LSTF, BIT30|BIT29|BIT28, OFDM_ContinuousTx);
		//for dynamic set Power index.
		write_bbreg(pAdapter, rFPGA0_XA_HSSIParameter1, bMaskDWord, 0x01000500);
		write_bbreg(pAdapter, rFPGA0_XB_HSSIParameter1, bMaskDWord, 0x01000500);
		
	} else {
		RT_TRACE(_module_mp_,_drv_info_, ("SetOFDMContinuousTx: test stop\n"));
		
		if (IS_HARDWARE_TYPE_JAGUAR(pAdapter) || IS_HARDWARE_TYPE_8814A(pAdapter)|| IS_HARDWARE_TYPE_8822B(pAdapter))
				PHY_SetBBReg(pAdapter, rSingleTone_ContTx_Jaguar, BIT18|BIT17|BIT16, OFDM_ALL_OFF);
			else
				PHY_SetBBReg(pAdapter, rOFDM1_LSTF, BIT30|BIT29|BIT28, OFDM_ALL_OFF);
		
		rtw_msleep_os(10);
		//BB Reset
		write_bbreg(pAdapter, rPMAC_Reset, bBBResetB, 0x0);
		write_bbreg(pAdapter, rPMAC_Reset, bBBResetB, 0x1);

		//Stop for dynamic set Power index.
		write_bbreg(pAdapter, rFPGA0_XA_HSSIParameter1, bMaskDWord, 0x01000100);
		write_bbreg(pAdapter, rFPGA0_XB_HSSIParameter1, bMaskDWord, 0x01000100);
	}

	pAdapter->mppriv.MptCtx.bCckContTx = _FALSE;
	pAdapter->mppriv.MptCtx.bOfdmContTx = bStart;
}/* mpt_StartOfdmContTx */

void Hal_SetContinuousTx(PADAPTER pAdapter, u8 bStart)
{
#if 0
	// ADC turn off [bit24-21] adc port0 ~ port1
	if (bStart) {
		write_bbreg(pAdapter, rRx_Wait_CCCA, read_bbreg(pAdapter, rRx_Wait_CCCA) & 0xFE1FFFFF);
		rtw_usleep_os(100);
	}
#endif
	RT_TRACE(_module_mp_, _drv_info_,
		 ("SetContinuousTx: rate:%d\n", pAdapter->mppriv.rateidx));

	pAdapter->mppriv.MptCtx.bStartContTx = bStart;

	if (pAdapter->mppriv.rateidx <= MPT_RATE_11M) 
	{  //CCK
		Hal_SetCCKContinuousTx(pAdapter, bStart);
	}
	else if ((pAdapter->mppriv.rateidx >= MPT_RATE_6M) &&
			(pAdapter->mppriv.rateidx <= MPT_RATE_MCS15)) 
	{ //OFDM
		Hal_SetOFDMContinuousTx(pAdapter, bStart);
	} else if ((pAdapter->mppriv.rateidx >= MPT_RATE_VHT1SS_MCS0) &&
			(pAdapter->mppriv.rateidx <= MPT_RATE_VHT2SS_MCS9)) 
	{ //OFDM
		Hal_SetOFDMContinuousTx(pAdapter, bStart);
	} else {
		RT_TRACE(_module_mp_,_drv_err_,("\nERROR: incorrect rateidx=%d\n",pAdapter->mppriv.rateidx));
	}

#if 0
	// ADC turn on [bit24-21] adc port0 ~ port1
	if (!bStart) {
		write_bbreg(pAdapter, rRx_Wait_CCCA, read_bbreg(pAdapter, rRx_Wait_CCCA) | 0x01E00000);
	}
#endif
}

#endif // CONFIG_MP_INCLUDE

