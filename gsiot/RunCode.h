#ifndef RunCode_H_
#define RunCode_H_

#include "SQLiteHelper.h"

/*
����������һ���±���:
s_RunCode �� defCodeIndex_ �� s_InitRunCode  ������һ�����ö�����׺���һ�¡�

ʹ��IsRUNCODEEnable()�жϹ����Ƿ���
ʹ��RUNCODE_Get()��ȡ������ֵ
*/

struct struRunCode
{
	const int code;
	const int default_value1;
	const int default_value2;
	const int default_value3;
	const int default_value4;
	int value1;
	int value2;
	int value3;
	int value4;
	const char *codestr;//codestr[64];
	const char *pName;
	const char *pDesc;
	const std::string default_str_val;
	std::string str_val;
	int CheckFlag;
};

// ���������洢λ�������
enum defCodeIndex_
{
	defCodeIndex_Unknown = 0,
	
	// *** һ�㹦�� ***
	defCodeIndex_SYS_IOTWndMod,
	defCodeIndex_SYS_ShowLogWnd,
	defCodeIndex_SYS_ListRunCode,
	defCodeIndex_SYS_ShowDebugUI,
	defCodeIndex_SYS_PlayBackSound,
	defCodeIndex_SYS_COM_baudrate,
	defCodeIndex_SYS_Talk,
	defCodeIndex_SYS_GSM,
	defCodeIndex_SYS_PlayBack_timefix_a,
	defCodeIndex_SYS_PlayBack_spanfix,
	defCodeIndex_SYS_PTZ_TRACE2PTZSel,
	defCodeIndex_SYS_CheckSystem,
	defCodeIndex_SYS_CurPsMemOver,
	defCodeIndex_SYS_UseUrlDifCnt,
	defCodeIndex_SYS_CamReEnc,
	defCodeIndex_SYS_CamTalk,
	defCodeIndex_SYS_AutotLockForRun,
	defCodeIndex_SYS_CheckIOTPs,
	defCodeIndex_SYS_CurSysMemOver,
	defCodeIndex_SYS_CurSysMemOverLow,
	defCodeIndex_SYS_CamAutoConnect,
	defCodeIndex_SYS_MotionTrack2ParkAction,
	defCodeIndex_SYS_AlarmGuardTimeW7,
	defCodeIndex_SYS_AutoCheckNetUseable,
	defCodeIndex_SYS_RTMPUrlNumMax,
	defCodeIndex_SYS_AlarmGuardGlobalFlag,
	defCodeIndex_SYS_AlarmGuardTimeW1,
	defCodeIndex_SYS_AlarmGuardTimeW2,
	defCodeIndex_SYS_AlarmGuardTimeW3,
	defCodeIndex_SYS_AlarmGuardTimeW4,
	defCodeIndex_SYS_AlarmGuardTimeW5,
	defCodeIndex_SYS_AlarmGuardTimeW6,
	defCodeIndex_SYS_ShowTaskBtnMod,
	defCodeIndex_SYS_RS485ReadRollMod,
	defCodeIndex_SYS_BitRateBaseK,
	defCodeIndex_SYS_AutoNotiContAddIOTName,
	defCodeIndex_SYS_AutoNotiContAddTime,
	defCodeIndex_SYS_PlayBackSetSpeed,
	defCodeIndex_SYS_PlaybackChannelLimit,
	defCodeIndex_SYS_UIAutoGetAllModSysSet,
	defCodeIndex_SYS_PlaybackSessionOvertime,
	defCodeIndex_SYS_CheckDevTimeCfg,
	defCodeIndex_SYS_CheckDevTimeSave,
	defCodeIndex_SYS_AutoPublishEnable,
	defCodeIndex_SYS_GSM_LinkID,
	
	defCodeIndex_PrintRecv_MainPort,
	defCodeIndex_PrintRecv_RF,
	defCodeIndex_PrintRecv_RS485,
	defCodeIndex_PrintRecv_RF_original,
	defCodeIndex_PrintRecv_IR_code,
	defCodeIndex_PrintRecv_IR_original,

	defCodeIndex_PrintSend_MainPort,
	defCodeIndex_PrintSend_RF_TX,
	defCodeIndex_PrintSend_RS485,
	defCodeIndex_PrintSend_RF_original,

	defCodeIndex_PrintRecv_val_RF,
	defCodeIndex_PrintRecv_val_RS485,
	defCodeIndex_PrintRecv_val_IR,

	defCodeIndex_Cam_PrintFrame,
	defCodeIndex_Cam_PrintFrameSize,
	defCodeIndex_Cam_PtzAutoMax,
	defCodeIndex_Cam_PtzAutoNorm,
	defCodeIndex_Cam_PtzAutoFast,
	defCodeIndex_Cam_MakeKeyFrameInterval,
	defCodeIndex_Cam_VideoBitrateLimit,

	defCodeIndex_RTMPSend_PrintFrame,
	defCodeIndex_RTMPSend_NetSend_WarnTime,
	defCodeIndex_RTMPSend_SetChunkSize,
	defCodeIndex_RTMFP_WaitConnTimeout,
	defCodeIndex_RTMFP_UrlAddStreamID,
	defCodeIndex_RTMFP_UseSpecStreamID,
	defCodeIndex_RTMFP_SessionLimit,
	defCodeIndex_SYS_Enable_RTMFP,
	defCodeIndex_RTMFP_DelFromUrlStreamID,

	defCodeIndex_xmpp_ConnectInterval,
	defCodeIndex_RF_Send_CountMin,
	defCodeIndex_RF_Send_CountDefault,
	defCodeIndex_RF_Send_FixAddTime,
	defCodeIndex_COM_SendInterval,

	defCodeIndex_APlay_ProbeSize,

	defCodeIndex_SYS_CamAlarmDebugLog,
	defCodeIndex_SYS_CamAlarmResumeTime,
	defCodeIndex_SYS_TriggerAlarmInterval,

	defCodeIndex_CommLinkThreadMax,
	defCodeIndex_CommLinkConnectInterval,
	defCodeIndex_CommLinkHb_SendInterval,
	defCodeIndex_CommLinkHb_RecvTimeover,

	defCodeIndex_Dev_RecvFailedCount,
	defCodeIndex_Dev_RecvFailedTime,

	defCodeIndex_SYS_Change_Global,

	defCodeIndex_SYS_VChgRng_Default,
	defCodeIndex_SYS_VChgRng_Temperature,
	defCodeIndex_SYS_VChgRng_Humidity,
	defCodeIndex_SYS_WinTime_Wind,
	defCodeIndex_SYS_VChgRng_CO2,  //jyc201703034 add notice
	defCodeIndex_SYS_VChgRng_HCHO,
	defCodeIndex_SYS_VChgRng_PM25,  //jyc20170424 add for debug 1min cap and save
	defCodeIndex_SYS_MergeWindLevel,
	defCodeIndex_SYS_DataSamp_DoBatch,
	defCodeIndex_SYS_DataSamp_LongTimeNoData,
	defCodeIndex_SYS_DataSamp_LowSampTime,
	defCodeIndex_SYS_QueryLstMax,
	defCodeIndex_SYS_DBDataFileMax,

	defCodeIndex_SYS_ReportDef_spanrate,
	defCodeIndex_SYS_ReportDef_ratefortype,
	defCodeIndex_SYS_ReportDef_getfortype,

	defCodeIndex_LED_Config,
	defCodeIndex_LED_DevParam,
	defCodeIndex_LED_ShowInterval,
	defCodeIndex_LED_ValueOvertime,
	defCodeIndex_LED_Rect,
	defCodeIndex_LED_Title,
	defCodeIndex_LED_Format,

	defCodeIndex_AC_CtlMAX,
	defCodeIndex_AC_CheckACMod,

	defCodeIndex_PrePic_Size_NM,
	defCodeIndex_PrePic_Size_NS,
	defCodeIndex_PrePic_Size_S,


	// *** �ַ���� ***
	defCodeIndex_SYS_IOT_NAME,


	// *** ���⹦�� ***
	defCodeIndex_TEST__Spec__, // ���⹦�ֽܷ���

	defCodeIndex_TEST_ModbusMod,
	defCodeIndex_Debug_DisableAuth,
	defCodeIndex_TEST_OpenRFOriginal,
	//defCodeIndex_TEST_Disable_GSM,
	defCodeIndex_TEST_TelReplaceSMS,
	defCodeIndex_TEST_DoEvent_NoInterval,
	defCodeIndex_TEST_RTMP_TestMod,
	defCodeIndex_TEST_PlayBack_timefix_pnt,
	defCodeIndex_TEST_StreamServer,
	defCodeIndex_TEST_DisableHeartbeatMon,
	defCodeIndex_TEST_DisableAlarmGuard,
	defCodeIndex_TEST_DeCheckFilePlayback,
	defCodeIndex_TEST_UseSpecRTMPUrlList,
	defCodeIndex_Dis_CommLinkTransparent,
	defCodeIndex_Dis_ChangeSaveDB,
	defCodeIndex_Dis_RunDataProc,
	defCodeIndex_TEST_ForceDataSave,
	defCodeIndex_TEST_DlgQueryMain,

	defCodeIndex_Disable_Recv_RF,
	defCodeIndex_Disable_Recv_RS485,
	defCodeIndex_Disable_Recv_RF_original,
	defCodeIndex_Disable_Recv_GSM,
	defCodeIndex_Disable_Recv_IR_code,
	defCodeIndex_Disable_Recv_IR_original,

	defCodeIndex_Disable_Send_RF_TX,
	defCodeIndex_Disable_Send_RS485,
	defCodeIndex_Disable_Send_RF_original,
	defCodeIndex_Disable_Send_GSM,

	defCodeIndex_TEST_Develop_NewFunc,
	defCodeIndex_TEST_Debug_TempCode,

	defCodeIndex_MAX_
};

#define defRunCodeVal_Disable		0		// һ�㹦�ܿ�����valueֵ
#define defRunCodeVal_Enable		1		// һ�㹦�ܿ�����valueֵ
#define defRunCodeVal_Spec_Enable	123456	// ���⹦�ܿ�����valueֵ

// ������ֵ�����
#define defRunCodeValIndex_1	1
#define defRunCodeValIndex_2	2
#define defRunCodeValIndex_3	3
#define defRunCodeValIndex_4	4

// ��ʼ��������
#define RUNCODE_Init s_InitRunCode

// �ж��������Ӧ�����Ƿ���
#define IsRUNCODEEnable s_IsRunCodeEnable

#define RUNCODE_Index s_GetRunCodeIndex

// ��ȡ������ֵ
#define RUNCODE_Get s_GetRunCodeValue
#define RUNCODE_GetStr s_GetRunCodeStrVal
#define RUNCODE_GetDefault s_GetRunCodeValue_Default
#define RUNCODE_GetStrDefault s_GetRunCodeStrVal_Default
#define RUNCODE_GetUseDefWhenNul s_GetRunCodeValue_UseDefWhenNul

// �޸�������ֵ
#define RUNCODE_Set s_SetRunCodeValue

#define RUNCODE_List s_ListRunCode

#define RUNCODE_Ref s_RefRunCode

#define RUNCODE_IsDebugMod s_IsDebugMod

void s_InitRunCode(void);
defCodeIndex_ s_GetRunCodeIndex( int code, const char *codestr );
bool s_SetRunCodeValue( defCodeIndex_ index, int val1, int val2=0, int val3=0, int val4=0, bool do1=true, bool do2=false, bool do3=false, bool do4=false );
bool s_IsRunCodeEnable( defCodeIndex_ codeindex );
int s_GetRunCodeValue( defCodeIndex_ codeindex, int valueindex=defRunCodeValIndex_1 );
const std::string& s_GetRunCodeStrVal( defCodeIndex_ codeindex );
int s_GetRunCodeValue_Default( defCodeIndex_ codeindex, int valueindex=defRunCodeValIndex_1 );
const std::string& s_GetRunCodeStrVal_Default( defCodeIndex_ codeindex );
int s_GetRunCodeValue_UseDefWhenNul( defCodeIndex_ codeindex, int valueindex, int nulval );
void s_ListRunCode( int nfmt=0 );
const struRunCode& s_RefRunCode( defCodeIndex_ codeindex );
bool s_IsDebugMod();

class CRunCodeMgr: public SQLiteHelper
{
public:
	CRunCodeMgr(void);
	~CRunCodeMgr(void);
	
	void Init();
	bool SetCodeAndSaveDb( defCodeIndex_ index, int val1, int val2=0, int val3=0, int val4=0, bool do1=true, bool do2=false, bool do3=false, bool do4=false );
	bool SetCodeStrValAndSaveDb( defCodeIndex_ index, const std::string &str_val );

private:
	void LoadDB_code();
	bool SaveDB_code( defCodeIndex_ index, bool save_str_val=false );
};

#endif // RunCode_H_
