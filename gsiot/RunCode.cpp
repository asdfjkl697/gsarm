#include "RunCode.h"
#include "common.h"
#include <cassert>
#include <set>

static char *s_pStrNull="";

#define defRunCode_CheckOK	0x55AA5A5A
#define StrNull s_pStrNull

enum defUseValType
{
	defUseValType_Default = 1,
	defUseValType_Min,
	defUseValType_Max,
};

// ������˵������ value1=defRunCodeVal_Enable ��������
static struRunCode s_RunCode[defCodeIndex_MAX_+1] = 
{
	{ 0, 0, 0, 0, 0, 0,0,0,0, "Unknown", StrNull, StrNull },						// 

	{ 100, 0, 1, 1024, 768, 0,0,0,0, "SYS_IOTWndMod", StrNull, StrNull },			// ������ʾ��ʽ��Ĭ��ȫ��
	{ 101, 1, 2, 0, 0, 0,0,0,0, "SYS_ShowLogWnd", StrNull, StrNull },				// �������Զ���ʾ��־����, val2:SW_SHOWMINIMIZED�ȵ�, val3:�������ʾ
	{ 102, 0, 0, 0, 0, 0,0,0,0, "SYS_ListRunCode", StrNull, StrNull },				// ��ӡ �������б�
	{ 103, 0, 0, 0, 0, 0,0,0,0, "SYS_ShowDebugUI", StrNull, StrNull },				// ��ӡ ��ʾ���Խ���
	{ 104, 0, 0, 0, 0, 0,0,0,0, "SYS_PlayBackSound", "¼��ط�����", "0:�ط�ʱ��������, 1:�ط�ʱ��������" },		// ��ӡ �ط�ʱ�Ƿ��������
	{ 105, 57600, 0, 0, 0, 0,0,0,0, "SYS_COM_baudrate", StrNull, StrNull },			// com baudrate // 115200/57600/19200/9600�ȵ�...
	{ 106, 0, 0, 0, 0, 0,0,0,0, "SYS_Talk", "Զ��ͨ������", "0:����, 1:����" },		// ͨ������
	{ 107, 0, 0, 0, 0, 0,0,0,0, "SYS_GSM", "GSM����֪ͨ����", "0:����, 1:����" },	// GSM����
	{ 108, 64, 0, 0, 0, 0,0,0,0, "SYS_PlayBack_timefix_a", StrNull, StrNull },		// �ط� ��Ƶԭʼʱ������ֵ��64
	{ 109, 9, 0, 0, 0, 0,0,0,0, "SYS_PlayBack_spanfix", StrNull, StrNull },			// �ط� ����ʱ����������Ϊ�����Ѳ���1/5/9�ȵ�...
	{ 110, 1, 0, 0, 0, 0,0,0,0, "SYS_PTZ_TRACE2PTZSel", "���ٹر�ʱ�Զ����ö�λ", StrNull },	// ��λ ���ٹر�ʱתΪ��λ
	{ 111, 1, 0, 0, 0, 0,0,0,0, "SYS_CheckSystem", StrNull, StrNull },				// �Ƿ���ϵͳ״̬����⵱ǰϵͳ�ڴ�
	{ 112, 800, 0, 0, 0, 0,0,0,0, "SYS_CurPsMemOver", StrNull, StrNull },			// ��ǰ����ڴ�ռ�����ޣ���λMB
	{ 113, 1, 0, 0, 0, 0,0,0,0, "SYS_UseUrlDifCnt", StrNull, StrNull },				// urlʹ�ò��촦�� iosδ����ǰ��0��ios���º���1
	{ 114, 0, 0, 0, 0, 0,0,0,0, "SYS_CamReEnc", StrNull, StrNull },					// ����ͷ��Ƶת�빦��֧��
	{ 115, 1, 0, 0, 0, 0,0,0,0, "SYS_CamTalk", StrNull, StrNull },					// ֧�������Խ�����
	{ 116, 1, 0, 0, 0, 0,0,0,0, "SYS_AutotLockForRun", StrNull, StrNull },			// �������Զ���
	{ 117, 1, 0, 0, 0, 0,0,0,0, "SYS_CheckIOTPs", StrNull, StrNull },				// �Ƿ���������״̬����⵱ǰ����ڴ�
	{ 118, 92, 0, 0, 0, 0,0,0,0, "SYS_CurSysMemOver", StrNull, StrNull },			// ��ǰϵͳ�ڴ�ռ�����ޣ��ٷֱ�
	{ 119, 87, 0, 0, 0, 0,0,0,0, "SYS_CurSysMemOverLow", StrNull, StrNull },		// ��ǰϵͳ�ڴ�ռ�ÿ���ʱ�����ޣ��ٷֱ�
	{ 120, 0, 0, 0, 0, 0,0,0,0, "SYS_CamAutoConnect", "��������ӷ�ʽ", "0:ȫ��������������, 1:ȫ�������Զ�����, 2:ȫ�����ǰ�������" },	// ����������Ƿ񱣳��Զ����ӣ�val1: 0���������ã�1���������Զ����ӣ�2�������ǰ�������; val4:�Ƿ���ʾ�����ý���
	{ 121, 1, 0, 0, 0, 0,0,0,0, "SYS_MotionTrack2ParkAction", StrNull, StrNull },	// ģʽ, ���ܸ��������������
	{ 122, 1, 0, 0, 0, 0,0,0,0, "SYS_AlarmGuardTimeW7", StrNull, StrNull },			// ����ʱ��:����, val1:����ʱ���־, 0ȫ����Ч(����)��1ȫ����Ч(����), 2ָ��ʱ���; val2/val3/val4:����ʱ���ֵ��ֵ��ʽ: �Ƿ���Ч��־*100000000+ʱ��ο�ʼʱ��*10000+ʱ��ν���ʱ�䣬ʱ������Ч:08:30-17:00=108301700
	{ 123, 1, 0, 0, 0, 0,0,0,0, "SYS_AutoCheckNetUseable", StrNull, StrNull },		// �Զ��������������Ƿ�ͨ
	{ 124, 9, 0, 0, 0, 0,0,0,0, "SYS_RTMPUrlNumMax", StrNull, StrNull },			// CreateRTMPInstance RTMP url��������
	{ 125, 1, 0, 0, 0, 0,0,0,0, "SYS_AlarmGuardGlobalFlag", StrNull, StrNull },		// ���������ܿ���
	{ 131, 1, 0, 0, 0, 0,0,0,0, "SYS_AlarmGuardTimeW1", StrNull, StrNull },			// ����ʱ��:��һ����ʽͬSYS_AlarmGuardTime����
	{ 132, 1, 0, 0, 0, 0,0,0,0, "SYS_AlarmGuardTimeW2", StrNull, StrNull },			// ����ʱ��:�ܶ�����ʽͬSYS_AlarmGuardTime����
	{ 133, 1, 0, 0, 0, 0,0,0,0, "SYS_AlarmGuardTimeW3", StrNull, StrNull },			// ����ʱ��:�����ʽͬSYS_AlarmGuardTime����
	{ 134, 1, 0, 0, 0, 0,0,0,0, "SYS_AlarmGuardTimeW4", StrNull, StrNull },			// ����ʱ��:���ģ���ʽͬSYS_AlarmGuardTime����
	{ 135, 1, 0, 0, 0, 0,0,0,0, "SYS_AlarmGuardTimeW5", StrNull, StrNull },			// ����ʱ��:���壬��ʽͬSYS_AlarmGuardTime����
	{ 136, 1, 0, 0, 0, 0,0,0,0, "SYS_AlarmGuardTimeW6", StrNull, StrNull },			// ����ʱ��:�����ʽͬSYS_AlarmGuardTime����
	{ 141, 1, 0, 0, 0, 0,0,0,0, "SYS_ShowTaskBtnMod", StrNull, StrNull },			// ��ҳ����������ť��ʾģʽ��0��̬��ʾ��1������ʾ
	{ 142, 1, 0, 0, 0, 0,0,0,0, "SYS_RS485ReadRollMod", StrNull, StrNull },			// RS485�����ȡ����ģʽ ��������
	{ 143, 0, 1000, 0, 0, 0,0,0,0, "SYS_BitRateBaseK", StrNull, StrNull },			// �ض���CAudioCfg::BitRate2Bytes K��׼ֵ��һ��Ϊ 1000 or 1024
	{ 144, 0, 0, 0, 0, 0,0,0,0, "SYS_AutoNotiContAddIOTName", "�Զ���ɵ�֪ͨ���ݴ���������", "0:����, 1:����" },		// �Զ���ɵ�֪ͨ�����Ƿ�������
	{ 145, 1, 0, 0, 0, 0,0,0,0, "SYS_AutoNotiContAddTime", "�Զ���ɵ�֪ͨ���ݴ���ʱ��", "0:����, 1:����" },		// �Զ���ɵ�֪ͨ�����Ƿ����ʱ��
	{ 146, 0, 0, 0, 0, 0,0,0,0, "SYS_PlayBackSetSpeed", StrNull, StrNull },			// �Ƿ� �ط�ʱ������������
	{ 147, 16, 0, 0, 0, 0,0,0,0, "SYS_PlaybackChannelLimit", StrNull, StrNull },	// �Ƿ� �طŻỰ����
	{ 148, 1, 0, 0, 0, 0,0,0,0, "SYS_UIAutoGetAllModSysSet", StrNull, StrNull },	// �򿪽���ʱ�Ƿ��Զ���ȡ����������Ϣ��
	{ 149, 6*3600, 0, 0, 0, 0,0,0,0, "SYS_PlaybackSessionOvertime", StrNull, StrNull },	// �طŻỰ�ʱ�䣬��
	{ 150, 1, 1, 2, 0, 0,0,0,0, "SYS_CheckDevTimeCfg", "�Զ�Уʱ����", StrNull },	// �豸Уʱ���ã�val1: ���ý���; �� val2��val3 ʱ�䷶Χ��ִ�У���ʱ���ⲻִ��
	{ 151, 0, 0, 0, 0, 0,0,0,0, "SYS_CheckDevTimeSave", StrNull, StrNull },			// �豸Уʱ��¼��val1: �ϴ�Уʱ��
	{ 152, 0, 0, 0, 0, 0,0,0,0, "SYS_AutoPublishEnable", StrNull, StrNull },		// ���ַ���/�Զ�������������/����
	
	{ 157, defLinkID_Local, 0, 0, 0, 0,0,0,0, "SYS_GSM_LinkID", "GSMģ��������·", "��·ID" },	// GSMģ��������·

	{ 10100, 1, 0, 0, 0, 0,0,0,0, "PrintRecv_MainPort", StrNull, StrNull },			// ��ӡ �����ֽ�����ϸ��Ϣ ���˿�
	{ 10101, 0, 0, 0, 0, 0,0,0,0, "PrintRecv_RF", StrNull, StrNull },				// ��ӡ �����ֽ�����ϸ��Ϣ ģ�� RXB8_315 & RXB8_433
	{ 10107, 0, 0, 0, 0, 0,0,0,0, "PrintRecv_RS485", StrNull, StrNull },			// ��ӡ �����ֽ�����ϸ��Ϣ RS485ģ��
	{ 10108, 0, 0, 0, 0, 0,0,0,0, "PrintRecv_RF_original", StrNull, StrNull },		// ��ӡ �����ֽ�����ϸ��Ϣ ģ�� RXB8_315_original & RXB8_433_original
	{ 10109, 0, 0, 0, 0, 0,0,0,0, "PrintRecv_IR_code", StrNull, StrNull },			// ��ӡ �����ֽ�����ϸ��Ϣ ģ�� MOD_IR_RX_code
	{ 10110, 0, 0, 0, 0, 0,0,0,0, "PrintRecv_IR_original", StrNull, StrNull },		// ��ӡ �����ֽ�����ϸ��Ϣ ģ�� MOD_IR_RX_original
						 
	{ 10200, 1, 0, 0, 0, 0,0,0,0, "PrintSend_MainPort", StrNull, StrNull },			// ��ӡ �����ֽ�����ϸ��Ϣ ���˿�
	{ 10203, 0, 0, 0, 0, 0,0,0,0, "PrintSend_RF_TX", StrNull, StrNull },			// ��ӡ �����ֽ�����ϸ��Ϣ RS485ģ��
	{ 10207, 0, 0, 0, 0, 0,0,0,0, "PrintSend_RS485", StrNull, StrNull },			// ��ӡ �����ֽ�����ϸ��Ϣ RS485ģ��
	{ 10208, 0, 0, 0, 0, 0,0,0,0, "PrintSend_RF_original", StrNull, StrNull },		// ��ӡ �����ֽ�����ϸ��Ϣ RS485ģ��
						 
	{ 10301, 0, 0, 0, 0, 0,0,0,0, "PrintRecv_val_RF", StrNull, StrNull },			// ��ӡ ���ս������ֵ��Ϣ ģ�� RXB8_315 & RXB8_433
	{ 10307, 1, 0, 0, 0, 0,0,0,0, "PrintRecv_val_RS485", StrNull, StrNull },		// ��ӡ ���ս������ֵ��Ϣ RS485ģ��
	{ 10308, 1, 0, 0, 0, 0,0,0,0, "PrintRecv_val_IR", StrNull, StrNull },			// ��ӡ ���ս������ֵ��Ϣ ģ�� MOD_IR_RX_code & MOD_IR_RX_original

	{ 11001, 1, 300000, 0, 1, 0,0,0,0, "Cam_PrintFrame", StrNull, StrNull },		// ��ӡ Cam ��ӡPush��֡, value2���ʱ��ms, value3:I֡���Ǵ�ӡ, value4:��Ƶ֡��ӡ
	{ 11002, 0, 0, 0, 0, 0,0,0,0, "Cam_PrintFrameSize", StrNull, StrNull },			// ��ӡ Cam ��ӡPush��֡, value2�ֽڴ�С��������޴�ӡ
	{ 11003, 3000, 0, 0, 0, 0,0,0,0, "Cam_PtzAutoMax", StrNull, StrNull },			// Cam �Զ��������ʱ��ms
	{ 11004, 255, 400, 0, 0, 0,0,0,0, "Cam_PtzAutoNorm", StrNull, StrNull },		// Cam �Զ�������ͨģʽʱ��ms��value1����, value2����
	{ 11005, 1200, 1600, 0, 0, 0,0,0,0, "Cam_PtzAutoFast", StrNull, StrNull },		// Cam �Զ����ƿ���ģʽʱ��ms��value1����, value2����
	{ 11006, 700, 0, 0, 0, 0,0,0,0, "Cam_MakeKeyFrameInterval", StrNull, StrNull },	// Cam MakeKeyFrame��С���ms
	{ 11007, 2048*1024, 0, 0, 0, 0,0,0,0, "Cam_VideoBitrateLimit", StrNull, StrNull },	// Cam ��Ƶ�������ֵ�����������ô���

	{ 12001, 1, 300000, 0, 0, 0,0,0,0, "RTMPSend_PrintFrame", StrNull, StrNull },	// ��ӡ RTMPSend ��ӡPush��֡, value2���ʱ��ms, value3:I֡���Ǵ�ӡ
	{ 12002, 220, 0, 0, 0, 0,0,0,0, "RTMPSend_NetSend_WarnTime", StrNull, StrNull },// ��ӡ RTMPSend�������緢�ͺ�ʱ��ʱ��ӡ, value1:��ʱ����ʱ��ms
	{ 12003, 32*1024, 0, 0, 0, 0,0,0,0, "RTMPSend_SetChunkSize", StrNull, StrNull },// RTMPSend.RTMP_SetChunkSize 32*1024, 4096
	{ 12004, 8000, 0, 0, 0, 0,0,0,0, "RTMFP_WaitConnTimeout", StrNull, StrNull },	// RTMFP�ȴ����ӳ�ʱʱ��ms
	{ 12005, 1, 0, 0, 0, 0,0,0,0, "RTMFP_UrlAddStreamID", StrNull, StrNull },		// RTMFP Url��β���StreamID
	{ 12006, 0, 0, 0, 0, 0,0,0,0, "RTMFP_UseSpecStreamID", StrNull, StrNull, "test_sender" },		// RTMFP ʹ��ָ���Ĺ̶�StreamID
	{ 12007, 4, 0, 0, 0, 0,0,0,0, "RTMFP_SessionLimit", StrNull, StrNull },			// RTMFP ��Ե������������ޣ��ﵽ������������ʱ����RTMP����
	{ 12008, 0, 0, 0, 0, 0,0,0,0, "SYS_Enable_RTMFP", StrNull, StrNull },			// RTMFP��������/���ã�value1ʵʱ����, value2¼��ط�
	{ 12009, 1, 0, 0, 0, 0,0,0,0, "RTMFP_DelFromUrlStreamID", StrNull, StrNull },	// �����жϲ�ȥ��RTMFP URL��β��StreamID

	{ 13001, 10000, 0, 0, 0, 0,0,0,0, "xmpp_ConnectInterval", StrNull, StrNull },	// xmpp�������ʱ��
	{ 13002, 1, 0, 0, 0, 0,0,0,0, "RF_Send_CountMin", StrNull, StrNull },			// ����ң�ط��ʹ��� ��Сֵ
	{ 13003, 6, 0, 0, 0, 0,0,0,0, "RF_Send_CountDefault", StrNull, StrNull },		// ����ң�ط��ʹ��� Ĭ��ֵ
	{ 13004, 1000, 0, 0, 0, 0,0,0,0, "RF_Send_FixAddTime", StrNull, StrNull },		// ����ң�ط�����ʱ�ĸ�������ʱ�䣬��λms
	{ 13005, 1, 0, 0, 0, 0,0,0,0, "COM_SendInterval", StrNull, StrNull },			// �Ƿ����÷��ͼ������

	{ 14001, 1, 1024*1, 0, 0, 0,0,0,0, "APlay_ProbeSize", StrNull, StrNull },		// APlayer ProbeSize����value1ʹ�ò���, value2����ֵ, Ӱ����ʱ

	{ 15001, 0, 0, 0, 0, 0,0,0,0, "SYS_CamAlarmDebugLog", StrNull, StrNull },		// �����澯������־��ӡ
	{ 15002, 5*60, 0, 0, 0, 0,0,0,0, "SYS_CamAlarmResumeTime", StrNull, StrNull },	// �����澯�ָ�ʱ�䣬��λ��
	{ 15003, 2*60, 0, 0, 0, 0,0,0,0, "SYS_TriggerAlarmInterval", StrNull, StrNull },// �����������������λ��

	{ 16001, 16, 5, 0, 0, 0,0,0,0, "CommLinkThreadMax", StrNull, StrNull },			// ��·�߳����������value1�����, value2����������ӵ�����
	{ 16002, 20, 0, 0, 0, 0,0,0,0, "CommLinkConnectInterval", StrNull, StrNull },	// ��·�����������λ��
	{ 16003, 30, 0, 0, 0, 0,0,0,0, "CommLinkHb_SendInterval", StrNull, StrNull },	// �����ͼ������λ��
	{ 16004, 150, 0, 0, 0, 0,0,0,0, "CommLinkHb_RecvTimeover", StrNull, StrNull },	// ����ʱ����λ��

	{ 17001, 9, 3, 0, 0, 0,0,0,0, "Dev_RecvFailedCount", StrNull, StrNull },		// �豸����ʧ�ܴ���value1ʧ�ܴ�����ֱֵ����Ϊ�쳣, value2ʧ�ܴ�����ֵ����ʧ��ʱ������n����Ϊ�쳣
	{ 17002, 10, 50, 0, 0, 0,0,0,0, "Dev_RecvFailedTime", StrNull, StrNull },		// ʧ��ʱ������n�룬��λ��

	{ 18001, 0, 0, 0, 0, 0,0,0,0, "SYS_Change_Global", StrNull, StrNull },			// ���ñ仯�����־��value1�仯����, value2�仯ʱ�����Ŀǰֻ���ڴ��д洢��������ݿ�

	{ 19001, 1000, 0, 0, 0, 0,0,0,0, "SYS_VChgRng_Default", StrNull, StrNull },		// �洢�仯���趨ֵ��Ĭ���жϱ仯��1000������
	{ 19002, 2000, 0, 0, 0, 0,0,0,0, "SYS_VChgRng_Temperature", StrNull, StrNull },	// �洢�仯���趨ֵ���¶ȱ仯��1000������ //860
	{ 19003, 4000, 0, 0, 0, 0,0,0,0, "SYS_VChgRng_Humidity", StrNull, StrNull },	// �洢�仯���趨ֵ��ʪ�ȱ仯��1000������ //3000
	{ 19004, 240, 100, 0, 0, 0,0,0,0, "SYS_WinTime_Wind", StrNull, StrNull },		// ���ٴ��ڻ�ʱ�䣬value1����ֵ, value2ʱ���ʱ����Сֵ����λ��

	{ 19005, 20000, 0, 0, 0, 0,0,0,0, "SYS_VChgRng_CO2", StrNull, StrNull },	
	{ 19006, 50, 0, 0, 0, 0,0,0,0, "SYS_VChgRng_HCHO", StrNull, StrNull },
	{ 19007, 10000, 0, 0, 0, 0,0,0,0, "SYS_VChgRng_PM25", StrNull, StrNull },
	
	{ 19101, 0, 0, 0, 0, 0,0,0,0, "SYS_MergeWindLevel", StrNull, StrNull },			// �ϲ����ټ���1����2���ϲ�
	{ 19102, 1, 0, 0, 0, 0,0,0,0, "SYS_DataSamp_DoBatch", StrNull, StrNull },		// DataSample�ɼ�ʱ����������ȡֵ��ʽ�����򣬵���ֵ�ֱ��ȡ
	{ 19103, 5*60, 0, 0, 0, 0,0,0,0, "SYS_DataSamp_LongTimeNoData", StrNull, StrNull },	// DataSample�ɼ�ʱ̫��ʱ��δ��ȡ��ݣ���λ��
	{ 19104, 2*60, 30, 0, 0, 0,0,0,0, "SYS_DataSamp_LowSampTime", StrNull, StrNull },	// DataSample�ɼ�ʱ̫��ʱ��δ��ȡ���������Ƶ�ʲɼ�����λ��
	{ 19105, 2000, 0, 0, 0, 0,0,0,0, "SYS_QueryLstMax", StrNull, StrNull },			// ��ѯ��ݼ�¼���������
	{ 19106, 36, 0, 0, 0, 0,0,0,0, "SYS_DBDataFileMax", StrNull, StrNull },			// ��ݿ�洢�ļ������������=2

	{ 20001, 0, 0, 0, 0, 0,0,0,0, "SYS_ReportDef_spanrate", StrNull, StrNull },		// ��ѯ�������Ĭ��ֵ���仯�ʣ�ǧ��ֵ��1=0.001
	{ 20002, 1, 0, 0, 0, 0,0,0,0, "SYS_ReportDef_ratefortype", StrNull, StrNull },	// ��ѯ�������Ĭ��ֵ���������ͷ�����ݱ仯 0/1
	{ 20003, 0, 0, 0, 0, 0,0,0,0, "SYS_ReportDef_getfortype", StrNull, StrNull },	// ��ѯ�������Ĭ��ֵ����������ת��ֵ���� 0/1
	
	{ 21001, 0, 0, 0, 0, 0,0,0,0, "LED_Config", "LED��ʾ", StrNull },					// LED - �������ã�value1���ñ�־��value2ģʽ0�Զ�
	{ 21002, 30000, 0, 0, 0, 0,0,0,0, "LED_DevParam", StrNull, StrNull, "192.168.1.3" },	// LED - �豸����value1�˿ڣ�str�豸IP
	{ 21003, 30, 10, 0, 0, 0,0,0,0, "LED_ShowInterval", StrNull, StrNull },			// LED - ��ʾ�������; value1����������ˢ��һ��; value2��̼�������ܿ��ڴ�ֵ
	{ 21004, 10*60, 1, 0, 0, 0,0,0,0, "LED_ValueOvertime", StrNull, StrNull, "δ����" },	// LED - ֵ�����쳣������ã�value1�೤ʱ��δˢ��ֵΪ�쳣����; value2�쳣�Ƿ�����ʾ; str�쳣��ʾ����
	{ 21005, 0, 0, 192, 128, 0,0,0,0, "LED_Rect", StrNull, StrNull },				// LED - ��ʾλ��; value1 x; value2 y; value3 ��; value4 ��
	{ 21006, 0, 0, 0, 0, 0,0,0,0, "LED_Title", StrNull, StrNull, "" },				// LED - ��ʾ����; str���⣬��Ϊ����ʾ
	{ 21007, 0, 1, 0, 0, 0,0,0,0, "LED_Format", StrNull, StrNull },					// LED - ��ʾ��ʽ; value1�ı���ʽ��(�μ�SDK); value2����0/1;

	{ 22001, 16, 0, 0, 0, 0,0,0,0, "AC_CtlMAX", StrNull, StrNull },					// �Ž� - �������
	{ 22002, 1, 0, 0, 0, 0,0,0,0, "AC_CheckACMod", StrNull, StrNull },				// �Ž� - ����Ž�ģ��
	
	{ 23001, 800, 0, 0, 0, 0,0,0,0, "PrePic_Size_NM", StrNull, StrNull },			// Ԥ��ͼ - NM
	{ 23002, 400, 0, 0, 0, 0,0,0,0, "PrePic_Size_NS", StrNull, StrNull },			// Ԥ��ͼ - NS
	{ 23003, 128, 0, 0, 0, 0,0,0,0, "PrePic_Size_S", StrNull, StrNull },			// Ԥ��ͼ - S


	{ 80001, 0, 0, 0, 0, 0,0,0,0, "SYS_IOT_NAME", "�豸���", StrNull, "GSIOT" },	// �������豸���

	//--------------------------------------------------
	{ 0, 0, 0, 0, 0, 0,0,0,0, "_TEST__Spec__", StrNull, StrNull },					// ���⹦�ֽܷ���

	{ 90001, 0, 0, 0, 0, 0,0,0,0, "TEST_ModbusMod", StrNull, StrNull },				// ����ģʽ, modbus͸������ģʽ
	{ 90002, 0, 0, 0, 0, 0,0,0,0, "Debug_DisableAuth", StrNull, StrNull },			// ����ģʽ, ����Ȩ��
	{ 90004, 0, 0, 0, 0, 0,0,0,0, "TEST_OpenRFOriginal", StrNull, StrNull },		// һֱ��RFģ���ϱ�ͨ��
	//{ 90005, 0, 0, 0, 0, 0,0,0,0, "TEST_Disable_GSM", StrNull, StrNull },			// �Ѹ���SYS_GSM
	{ 90006, 0, 0, 0, 0, 0,0,0,0, "TEST_TelReplaceSMS", StrNull, StrNull },			// �ò���绰������Ų���
	{ 90007, 0, 0, 0, 0, 0,0,0,0, "TEST_DoEvent_NoInterval", StrNull, StrNull },	// ���ж�ִ�м����ֱ��ִ���¼�����
	{ 90010, 0, 0, 0, 0, 0,0,0,0, "TEST_RTMP_TestMod", StrNull, StrNull },			// ����ģʽ, ����rtmp����ģʽ, value2�̶�ʹ���豸�����ʹ�����������ʵ����
	{ 90011, 0, 0, 0, 0, 0,0,0,0, "TEST_PlayBack_timefix_pnt", StrNull, StrNull },	// ����ģʽ, ��ӡ�ط� ԭʼʱ��������־����־���ܴ�
	{ 90012, 0, 0, 0, 0, 0,0,0,0, "TEST_StreamServer", StrNull, StrNull },			// ����ģʽ, ����ָ����������IP, val1:���ñ�־��val2(ip��1*1000+ip��2), val3(ip��3*1000+ip��4)����192168, 000150
	{ 90013, 0, 0, 0, 0, 0,0,0,0, "TEST_DisableHeartbeatMon", StrNull, StrNull },	// ����ģʽ, �Ƿ����������
	{ 90014, 0, 0, 0, 0, 0,0,0,0, "TEST_DisableAlarmGuard", StrNull, StrNull },		// ����ģʽ, �Ƿ���ò���ʱ�书��
	{ 90015, 0, 0, 0, 0, 0,0,0,0, "TEST_DeCheckFilePlayback", StrNull, StrNull },	// ����ģʽ, �Ƿ���ûط�ǰ����ļ�
	{ 90016, 0, 0, 0, 0, 0,0,0,0, "TEST_UseSpecRTMPUrlList", StrNull, StrNull },	// ����ģʽ, �Ƿ�ʹ��ָ�����λ�õ�url��Ϊ����url, val1:���ñ�־��val2:�ĸ���ŵ���Ϊ����,�±�0��ʼ��
	{ 90017, 0, 0, 0, 0, 0,0,0,0, "Dis_CommLinkTransparent", StrNull, StrNull },	// ����ģʽ, ������·͸��
	{ 90018, 123456, 0, 0, 0, 0,0,0,0, "Dis_ChangeSaveDB", StrNull, StrNull },		// �Ƿ���� ���ñ仯������ݿ�
	{ 90020, 0, 0, 0, 0, 0,0,0,0, "Dis_RunDataProc", StrNull, StrNull },			// ����ģʽ ��ݲɼ������ȵĲ��Թ���
	{ 90021, 0, 0, 0, 0, 0,0,0,0, "TEST_ForceDataSave", StrNull, StrNull },			// ����ģʽ, ���Ǳ������
	{ 90022, 0, 0, 0, 0, 0,0,0,0, "TEST_DlgQueryMain", StrNull, StrNull },			// ����ģʽ, ����ѯѡ��

	{ 90101, 0, 0, 0, 0, 0,0,0,0, "Disable_Recv_RF", StrNull, StrNull },			// ����ģʽ, ���ι��ܣ�(RXB8_315 & RXB8_433)���ڽ��ս���
	{ 90107, 0, 0, 0, 0, 0,0,0,0, "Disable_Recv_RS485", StrNull, StrNull },			// ����ģʽ, ���ι��ܣ�RS485���ڽ��ս���
	{ 90108, 0, 0, 0, 0, 0,0,0,0, "Disable_Recv_RF_original", StrNull, StrNull },	// ����ģʽ, ���ι��ܣ�(RXB8_315_original & RXB8_433_original)���ڽ��ս���
	{ 90110, 0, 0, 0, 0, 0,0,0,0, "Disable_Recv_GSM", StrNull, StrNull },			// ����ģʽ, ���ι��ܣ�Module_GSM���ڽ���
	{ 90111, 0, 0, 0, 0, 0,0,0,0, "Disable_Recv_IR_code", StrNull, StrNull },		// ����ģʽ, ���ι��ܣ�MOD_IR_RX_code���ڽ���
	{ 90112, 0, 0, 0, 0, 0,0,0,0, "Disable_Recv_IR_original", StrNull, StrNull },	// ����ģʽ, ���ι��ܣ�MOD_IR_RX_original���ڽ���

	{ 90203, 0, 0, 0, 0, 0,0,0,0, "Disable_Send_RF_TX", StrNull, StrNull },			// ����ģʽ, ���ι��ܣ�(RXB8_315_TX & RXB8_433_TX)���ڷ���
	{ 90207, 0, 0, 0, 0, 0,0,0,0, "Disable_Send_RS485", StrNull, StrNull },			// ����ģʽ, ���ι��ܣ�RS485���ڷ���
	{ 90208, 0, 0, 0, 0, 0,0,0,0, "Disable_Send_RF_original", StrNull, StrNull },	// ����ģʽ, ���ι��ܣ�(RXB8_315_original & RXB8_433_original)���ڷ���
	{ 90210, 0, 0, 0, 0, 0,0,0,0, "Disable_Send_GSM", StrNull, StrNull },			// ����ģʽ, ���ι��ܣ�Module_GSM���ڷ���

	{ 90901, 0, 0, 0, 0, 0,0,0,0, "TEST_Develop_NewFunc", StrNull, StrNull },		// ��������ģʽ, ���ڿ����е��¹��ܲ����Ƿ�����
	{ 90911, 0, 0, 0, 0, 0,0,0,0, "TEST_Debug_TempCode", StrNull, StrNull },		// ��������ģʽ, ��ʱ���Դ����Ƿ�����

	{ 0, 0, 0, 0, 0, 0,0,0,0, "CODEEND", StrNull, StrNull }							// ��β
};

// ͨ��code��codestr��ȡruncode���������
defCodeIndex_ s_GetRunCodeIndex( int code, const char *codestr )
{
	defCodeIndex_ index = defCodeIndex_Unknown;

	for( int i=0; i<(sizeof(s_RunCode)/sizeof(struRunCode)); ++i )
	{
		if( 0 != s_RunCode[i].code )
		{
			if( code == s_RunCode[i].code )
			{
				index = (defCodeIndex_)i;
				break;
			}

			if( codestr && strlen(codestr)>0 )
			{
				if( 0 == strcmp( codestr, s_RunCode[i].codestr ) )
				{
					index = (defCodeIndex_)i;
					break;
				}
			}
		}
	}

	if( defCodeIndex_Unknown == index )
	{
		//LOGMSGEX( defLOGNAME, defLOG_ERROR, "s_GetRunCodeIndex not found codeindex, code={%d,%s}\r\n", code, codestr?codestr:"" );
	}

	if( index<defCodeIndex_Unknown || index>=defCodeIndex_MAX_ )
	{
		//LOGMSGEX( defLOGNAME, defLOG_ERROR, "s_GetRunCodeIndex codeindex error, code={%d,%s}, index=%d!\r\n", code, codestr?codestr:"", index );
		index = defCodeIndex_Unknown;
	}
	else
	{
		// У��code��codestr�Ƿ�һ��
		bool isErrorCode = false;
		if( 0 != code )
		{
			if( code != s_RunCode[index].code )
			{
				isErrorCode = true;
			}
		}

		if( codestr && strlen(codestr)>0 )
		{
			if( 0 != strcmp( codestr, s_RunCode[index].codestr ) )
			{
				isErrorCode = true;
			}
		}

		if( isErrorCode )
		{
			//LOGMSGEX( defLOGNAME, defLOG_ERROR, "s_GetRunCodeIndex code & codestr error, code={%d,%s}, index=%d!\r\n", code, codestr?codestr:"", index );
			index = defCodeIndex_Unknown;
		}
	}

	return index;
}

// �з�Χ���ƵĽ�������ֵ
static void s_FixRunCodeValue( defCodeIndex_ codeindex, int valueindex, int newval )
{
	if( codeindex<defCodeIndex_Unknown || codeindex>=defCodeIndex_MAX_ )
	{
		return ;
	}

	if( valueindex<defRunCodeValIndex_1 || valueindex>defRunCodeValIndex_4 )
	{
		return ;
	}

	int oldval = 0;
	switch(valueindex)
	{
	case defRunCodeValIndex_1: 
		{
			oldval = s_RunCode[codeindex].value1;
			s_RunCode[codeindex].value1 = newval;
		}

	case defRunCodeValIndex_2: 
		{
			oldval = s_RunCode[codeindex].value2;
			s_RunCode[codeindex].value2 = newval;
		}

	case defRunCodeValIndex_3: 
		{
			oldval = s_RunCode[codeindex].value3;
			s_RunCode[codeindex].value3 = newval;
		}

	case defRunCodeValIndex_4: 
		{
			oldval = s_RunCode[codeindex].value4;
			s_RunCode[codeindex].value4 = newval;
		}
	}

	LOGMSG( ">>>>> s_FixRunCodeValue(%s) oldval=%d, newval=%d\r\n", s_RunCode[codeindex].codestr, oldval, newval );
}

// �з�Χ���ƵĽ�������ֵ
static bool s_FixRunCodeValue_range( defCodeIndex_ codeindex, int valueindex, bool havemin=false, int minval=-1, bool havemax=false, int maxval=-1, defUseValType UseValType=defUseValType_Default )
{
	if( codeindex<defCodeIndex_Unknown || codeindex>=defCodeIndex_MAX_ )
	{
		return false;
	}

	if( valueindex<defRunCodeValIndex_1 || valueindex>defRunCodeValIndex_4 )
	{
		return false;
	}

	int curval = s_GetRunCodeValue( codeindex, valueindex );
	if( ( havemin && (curval < minval) ) 
		|| ( havemax && (curval > maxval) ) )
	{
		switch(UseValType)
		{
		case defUseValType_Default:
			s_FixRunCodeValue( codeindex, valueindex, s_GetRunCodeValue_Default(codeindex, valueindex) );
			break;

		case defUseValType_Min:
			s_FixRunCodeValue( codeindex, valueindex, minval );
			break;

		case defUseValType_Max:
			s_FixRunCodeValue( codeindex, valueindex, maxval );
			break;
		}

		return true;
	}

	return false;
}

// ͨ��index����runcode��ֵ
bool s_SetRunCodeValue( defCodeIndex_ index, int val1, int val2, int val3, int val4, bool do1, bool do2, bool do3, bool do4 )
{
	if( index<=defCodeIndex_Unknown || index>=defCodeIndex_MAX_ )
	{
		//LOGMSGEX( defLOGNAME, defLOG_ERROR, "s_SetRunCodeValue error! code={%d}, val={%d, %d, %d, %d}\r\n", index, val1, val2, val3, val4 );
		return false;
	}

	if( do1 ) s_RunCode[index].value1 = val1;
	if( do2 ) s_RunCode[index].value2 = val2;
	if( do3 ) s_RunCode[index].value3 = val3;
	if( do4 ) s_RunCode[index].value4 = val4;
	
	bool isWarnSet = false; // ��Ӱ��������л��ƣ���Ҫ���������־
	switch(index)
	{
	case defCodeIndex_TEST_ModbusMod:
		isWarnSet = true;
		break;

	default:

		if( index > defCodeIndex_TEST__Spec__ )
		{
			isWarnSet = true;
		}

		break;
	}

	// ֵ��Χ����
	switch(index)
	{
	case defCodeIndex_xmpp_ConnectInterval:
		{
			isWarnSet |= s_FixRunCodeValue_range( index, defRunCodeValIndex_1, true, 6, false, -1, defUseValType_Default );
		}
		break;

	case defCodeIndex_CommLinkThreadMax:
		{
			isWarnSet |= s_FixRunCodeValue_range( index, defRunCodeValIndex_1, true, 2, false, -1, defUseValType_Default );
			isWarnSet |= s_FixRunCodeValue_range( index, defRunCodeValIndex_2, true, 1, false, -1, defUseValType_Default );
		}
		break;

	default:
		break;
	}

	LOGMSG( "%sSetcode(%d:%s), val={%d, %d, %d, %d}%s\r\n", isWarnSet?">>>>> ":"", s_RunCode[index].code, s_RunCode[index].codestr, val1, val2, val3, val4, isWarnSet?" <<<<<":"" );

	return true;
}

// ͨ��codeindex��ȡruncode��ֵ
int s_GetRunCodeValue( defCodeIndex_ codeindex, int valueindex )
{
	if( codeindex<defCodeIndex_Unknown || codeindex>=defCodeIndex_MAX_ )
	{
		return defRunCodeVal_Disable;
	}

	switch(valueindex)
	{
	case defRunCodeValIndex_1: 
		return s_RunCode[codeindex].value1;

	case defRunCodeValIndex_2: 
		return s_RunCode[codeindex].value2;

	case defRunCodeValIndex_3: 
		return s_RunCode[codeindex].value3;

	case defRunCodeValIndex_4: 
		return s_RunCode[codeindex].value4;
	}

	return defRunCodeVal_Disable;
}

// ͨ��codeindex��ȡruncode��ֵ
const std::string& s_GetRunCodeStrVal( defCodeIndex_ codeindex )
{
	if( codeindex<defCodeIndex_Unknown || codeindex>=defCodeIndex_MAX_ )
	{
		return c_NullStr;
	}

	return s_RunCode[codeindex].str_val;
}

// ͨ��codeindex��ȡĬ��ֵ
int s_GetRunCodeValue_Default( defCodeIndex_ codeindex, int valueindex )
{
	if( codeindex<defCodeIndex_Unknown || codeindex>=defCodeIndex_MAX_ )
	{
		return defRunCodeVal_Disable;
	}

	switch(valueindex)
	{
	case defRunCodeValIndex_1: 
		return s_RunCode[codeindex].default_value1;

	case defRunCodeValIndex_2: 
		return s_RunCode[codeindex].default_value2;

	case defRunCodeValIndex_3: 
		return s_RunCode[codeindex].default_value3;

	case defRunCodeValIndex_4: 
		return s_RunCode[codeindex].default_value4;
	}

	return defRunCodeVal_Disable;
}

// ͨ��codeindex��ȡĬ��ֵ
const std::string& s_GetRunCodeStrVal_Default( defCodeIndex_ codeindex )
{
	if( codeindex<defCodeIndex_Unknown || codeindex>=defCodeIndex_MAX_ )
	{
		return c_NullStr;
	}

	return s_RunCode[codeindex].default_str_val;
}

// ͨ��codeindex��ȡruncode��ֵ��ֵ��Чʱ����Ĭ��ֵ
int s_GetRunCodeValue_UseDefWhenNul( defCodeIndex_ codeindex, int valueindex, int nulval )
{
	if( codeindex<defCodeIndex_Unknown || codeindex>=defCodeIndex_MAX_ )
	{
		return defRunCodeVal_Disable;
	}

	switch(valueindex)
	{
	case defRunCodeValIndex_1: 
		return ( (nulval==s_RunCode[codeindex].value1) ? s_RunCode[codeindex].default_value1 : s_RunCode[codeindex].value1 );

	case defRunCodeValIndex_2: 
		return ( (nulval==s_RunCode[codeindex].value2) ? s_RunCode[codeindex].default_value2 : s_RunCode[codeindex].value2 );

	case defRunCodeValIndex_3: 
		return ( (nulval==s_RunCode[codeindex].value3) ? s_RunCode[codeindex].default_value3 : s_RunCode[codeindex].value3 );

	case defRunCodeValIndex_4: 
		return ( (nulval==s_RunCode[codeindex].value4) ? s_RunCode[codeindex].default_value4 : s_RunCode[codeindex].value4 );
	}

	return defRunCodeVal_Disable;
}

// �ж��������Ӧ�����Ƿ���
bool s_IsRunCodeEnable( defCodeIndex_ codeindex )
{
	int val1 = s_GetRunCodeValue( codeindex, defRunCodeValIndex_1 );

	if( codeindex < defCodeIndex_TEST__Spec__ )
	{
		return ( defRunCodeVal_Disable != val1 );
	}
	else
	{
		return ( defRunCodeVal_Spec_Enable == val1 );
	}

	return false;
}

#define macCheckRunCode(in_codestr)  \
if(  defCodeIndex_##in_codestr == s_GetRunCodeIndex( 0, ""#in_codestr"" ) ) \
{ \
	/*LOGMSG( "s_InitRunCode: "#codestr" code ok.\r\n" );*/ \
	struRunCode &rc = s_RunCode[defCodeIndex_##in_codestr]; \
	if( code_set.find(rc.code) == code_set.end() && codestr_set.find(rc.codestr) == codestr_set.end() ) \
	{ \
		rc.CheckFlag = defRunCode_CheckOK; \
		code_set.insert( rc.code ); \
		codestr_set.insert( rc.codestr ); \
	} \
	else \
	{ \
	\
	} \
} \
else \
{ \
\
}

void s_InitRunCode(void)
{
	//LOGMSGEX( defLOGNAME, defLOG_SYS, "s_InitRunCode and Check..." );

	for( int i=0; i<(sizeof(s_RunCode)/sizeof(struRunCode)); ++i )
	{
		//s_RunCode[i].codestr[64-1] = 0;
		s_RunCode[i].CheckFlag = 0;

		s_RunCode[i].value1 = s_RunCode[i].default_value1;
		s_RunCode[i].value2 = s_RunCode[i].default_value2;
		s_RunCode[i].value3 = s_RunCode[i].default_value3;
		s_RunCode[i].value4 = s_RunCode[i].default_value4;
		s_RunCode[i].str_val = s_RunCode[i].default_str_val;
	}

	std::set<int> code_set;
	std::set<std::string> codestr_set;
	//--------------------------------------------------
	macCheckRunCode(SYS_IOTWndMod);
	macCheckRunCode(SYS_ShowLogWnd);
	macCheckRunCode(SYS_ListRunCode);
	macCheckRunCode(SYS_ShowDebugUI);
	macCheckRunCode(SYS_PlayBackSound);
	macCheckRunCode(SYS_COM_baudrate);
	macCheckRunCode(SYS_Talk);
	macCheckRunCode(SYS_GSM);
	macCheckRunCode(SYS_PlayBack_timefix_a);
	macCheckRunCode(SYS_PlayBack_spanfix);
	macCheckRunCode(SYS_PTZ_TRACE2PTZSel);
	macCheckRunCode(SYS_CheckSystem);
	macCheckRunCode(SYS_CurPsMemOver);
	macCheckRunCode(SYS_UseUrlDifCnt);
	macCheckRunCode(SYS_CamReEnc);
	macCheckRunCode(SYS_CamTalk);
	macCheckRunCode(SYS_AutotLockForRun);
	macCheckRunCode(SYS_CheckIOTPs);
	macCheckRunCode(SYS_CurSysMemOver);
	macCheckRunCode(SYS_CurSysMemOverLow);
	macCheckRunCode(SYS_CamAutoConnect);
	macCheckRunCode(SYS_MotionTrack2ParkAction);
	macCheckRunCode(SYS_AlarmGuardTimeW7);
	macCheckRunCode(SYS_AutoCheckNetUseable);
	macCheckRunCode(SYS_RTMPUrlNumMax);
	macCheckRunCode(SYS_AlarmGuardGlobalFlag);
	macCheckRunCode(SYS_AlarmGuardTimeW1);
	macCheckRunCode(SYS_AlarmGuardTimeW2);
	macCheckRunCode(SYS_AlarmGuardTimeW3);
	macCheckRunCode(SYS_AlarmGuardTimeW4);
	macCheckRunCode(SYS_AlarmGuardTimeW5);
	macCheckRunCode(SYS_AlarmGuardTimeW6);
	macCheckRunCode(SYS_ShowTaskBtnMod);
	macCheckRunCode(SYS_RS485ReadRollMod);
	macCheckRunCode(SYS_BitRateBaseK);
	macCheckRunCode(SYS_AutoNotiContAddIOTName);
	macCheckRunCode(SYS_AutoNotiContAddTime);
	macCheckRunCode(SYS_PlayBackSetSpeed);
	macCheckRunCode(SYS_PlaybackChannelLimit);
	macCheckRunCode(SYS_UIAutoGetAllModSysSet);
	macCheckRunCode(SYS_PlaybackSessionOvertime);
	macCheckRunCode(SYS_CheckDevTimeCfg);
	macCheckRunCode(SYS_CheckDevTimeSave);
	macCheckRunCode(SYS_AutoPublishEnable);
	macCheckRunCode(SYS_GSM_LinkID);

	macCheckRunCode(PrintRecv_MainPort);
	macCheckRunCode(PrintRecv_RF);
	macCheckRunCode(PrintRecv_RS485);
	macCheckRunCode( PrintRecv_RF_original );
	macCheckRunCode( PrintRecv_IR_code );
	macCheckRunCode( PrintRecv_IR_original );

	macCheckRunCode(PrintSend_MainPort);
	macCheckRunCode(PrintSend_RF_TX);
	macCheckRunCode(PrintSend_RS485);
	macCheckRunCode(PrintSend_RF_original);

	macCheckRunCode(PrintRecv_val_RF);
	macCheckRunCode(PrintRecv_val_RS485);
	macCheckRunCode(PrintRecv_val_IR);

	macCheckRunCode(Cam_PrintFrame);
	macCheckRunCode(Cam_PrintFrameSize);
	macCheckRunCode(Cam_PtzAutoMax);
	macCheckRunCode(Cam_PtzAutoNorm);
	macCheckRunCode(Cam_PtzAutoFast);
	macCheckRunCode(Cam_MakeKeyFrameInterval);
	macCheckRunCode(Cam_VideoBitrateLimit);

	macCheckRunCode(RTMPSend_PrintFrame);
	macCheckRunCode(RTMPSend_NetSend_WarnTime);
	macCheckRunCode(RTMPSend_SetChunkSize);
	macCheckRunCode(RTMFP_WaitConnTimeout);
	macCheckRunCode(RTMFP_UrlAddStreamID);
	macCheckRunCode(RTMFP_UseSpecStreamID);
	macCheckRunCode(RTMFP_SessionLimit);
	macCheckRunCode(SYS_Enable_RTMFP);
	macCheckRunCode(RTMFP_DelFromUrlStreamID);

	macCheckRunCode(xmpp_ConnectInterval);
	macCheckRunCode(RF_Send_CountMin);
	macCheckRunCode(RF_Send_CountDefault);
	macCheckRunCode(RF_Send_FixAddTime);
	macCheckRunCode(COM_SendInterval);

	macCheckRunCode(APlay_ProbeSize);

	macCheckRunCode(SYS_CamAlarmDebugLog);
	macCheckRunCode(SYS_CamAlarmResumeTime);
	macCheckRunCode(SYS_TriggerAlarmInterval);

	macCheckRunCode(CommLinkThreadMax);
	macCheckRunCode(CommLinkConnectInterval);
	macCheckRunCode(CommLinkHb_SendInterval);
	macCheckRunCode(CommLinkHb_RecvTimeover);

	macCheckRunCode(Dev_RecvFailedCount);
	macCheckRunCode(Dev_RecvFailedTime);

	macCheckRunCode(SYS_Change_Global);

	macCheckRunCode(SYS_VChgRng_Default);
	macCheckRunCode(SYS_VChgRng_Temperature);
	macCheckRunCode(SYS_VChgRng_Humidity);
	macCheckRunCode(SYS_WinTime_Wind);
	macCheckRunCode(SYS_VChgRng_CO2);
	macCheckRunCode(SYS_VChgRng_HCHO);
	macCheckRunCode(SYS_VChgRng_PM25); //jyc20170424 add for PM2.5
	macCheckRunCode(SYS_MergeWindLevel);
	macCheckRunCode(SYS_DataSamp_DoBatch);
	macCheckRunCode(SYS_DataSamp_LongTimeNoData);
	macCheckRunCode(SYS_DataSamp_LowSampTime);
	macCheckRunCode(SYS_QueryLstMax);
	macCheckRunCode(SYS_DBDataFileMax);

	macCheckRunCode(SYS_ReportDef_spanrate);
	macCheckRunCode(SYS_ReportDef_ratefortype);
	macCheckRunCode(SYS_ReportDef_getfortype);

	macCheckRunCode(LED_Config);
	macCheckRunCode(LED_DevParam);
	macCheckRunCode(LED_ShowInterval);
	macCheckRunCode(LED_ValueOvertime);
	macCheckRunCode(LED_Rect);
	macCheckRunCode(LED_Title);
	macCheckRunCode(LED_Format);

	macCheckRunCode(AC_CtlMAX);
	macCheckRunCode(AC_CheckACMod);
	
	macCheckRunCode(PrePic_Size_NM);
	macCheckRunCode(PrePic_Size_NS);
	macCheckRunCode(PrePic_Size_S);


	//--------------------------------------------------
	macCheckRunCode(SYS_IOT_NAME);


	//--------------------------------------------------
	macCheckRunCode(TEST_ModbusMod);
	macCheckRunCode(Debug_DisableAuth);
	macCheckRunCode(TEST_OpenRFOriginal);
	//macCheckRunCode(TEST_Disable_GSM);
	macCheckRunCode(TEST_TelReplaceSMS);
	macCheckRunCode(TEST_DoEvent_NoInterval);
	macCheckRunCode(TEST_RTMP_TestMod);
	macCheckRunCode(TEST_PlayBack_timefix_pnt);
	macCheckRunCode(TEST_StreamServer);
	macCheckRunCode(TEST_DisableHeartbeatMon);
	macCheckRunCode(TEST_DisableAlarmGuard);
	macCheckRunCode(TEST_DeCheckFilePlayback);
	macCheckRunCode(TEST_UseSpecRTMPUrlList);
	macCheckRunCode(Dis_CommLinkTransparent);
	macCheckRunCode(Dis_ChangeSaveDB);
	macCheckRunCode(Dis_RunDataProc);
	macCheckRunCode(TEST_ForceDataSave);
	macCheckRunCode(TEST_DlgQueryMain);

	macCheckRunCode(Disable_Recv_RF);
	macCheckRunCode(Disable_Recv_RS485);
	macCheckRunCode(Disable_Recv_RF_original);
	macCheckRunCode(Disable_Recv_GSM);
	macCheckRunCode(Disable_Recv_IR_code);
	macCheckRunCode(Disable_Recv_IR_original);

	macCheckRunCode(Disable_Send_RF_TX);
	macCheckRunCode(Disable_Send_RS485);
	macCheckRunCode(Disable_Send_RF_original);
	macCheckRunCode(Disable_Send_GSM);

	macCheckRunCode(TEST_Develop_NewFunc);
	macCheckRunCode(TEST_Debug_TempCode);


	for( int i=0; i<(sizeof(s_RunCode)/sizeof(struRunCode)); ++i )
	{
		if( 0 != s_RunCode[i].code )
		{
			if( defRunCode_CheckOK != s_RunCode[i].CheckFlag )
			{
				// ÿ��code����������macCheckRunCode����У��
				//LOGMSGEX( defLOGNAME, defLOG_ERROR, "s_InitRunCode: code=%d, codestr=%s check error!!!\r\n\r\n", s_RunCode[i].code, s_RunCode[i].codestr );
				assert(false);
			}
		}
	}

	code_set.clear();
	codestr_set.clear();
	//LOGMSGEX( defLOGNAME, defLOG_SYS, "s_InitRunCode and Check Finish." );
}

void s_ListRunCode( int nfmt )
{
	LOGMSG( "s_ListRunCode Begin...(fmt=1/2)" );
	
	std::string strfmt;

	switch( nfmt )
	{
	case 2:
		LOGMSG( "id;enable;code;codestr;value1;value2;value3;value4;" );
		strfmt = "%d;%d;%d;%s;%d;%d;%d;%d;";
		break;

	case 1:
	default:
		LOGMSG( "id;enable; code,%-32s,% 8s,% 8s,% 8s,% 8s,", "codestr", "value1", "value2", "value3", "value4" );
		strfmt = "% 3d,% 2d,% 8d,%-32s,% 8d,% 8d,% 8d,% 8d,";
		break;
	}

	int printid = 1;
	for( int i=0; i<(sizeof(s_RunCode)/sizeof(struRunCode)); ++i )
	{
		if( 0 != s_RunCode[i].code )
		{
			LOGMSG( strfmt.c_str(), 
				printid++, 1, s_RunCode[i].code, s_RunCode[i].codestr, 
				s_RunCode[i].value1, s_RunCode[i].value2, s_RunCode[i].value3, s_RunCode[i].value4 );
		}
	}

	LOGMSG( "s_ListRunCode End." );
}

const struRunCode& s_RefRunCode( defCodeIndex_ codeindex )
{
	if( codeindex<defCodeIndex_Unknown || codeindex>=defCodeIndex_MAX_ )
	{
		return s_RunCode[defCodeIndex_Unknown];
	}

	return s_RunCode[codeindex];
}

bool s_IsDebugMod()
{
	for( int i=defCodeIndex_TEST__Spec__; i<defCodeIndex_MAX_; ++i )
	{
		if( s_RunCode[i].default_value1 != s_RunCode[i].value1 )
		{
			if( defRunCodeVal_Spec_Enable == s_RunCode[i].value1 )
			{
				return true;
			}
		}
	}

	return false;
}

//////////////////////////////////////////////////////////////////////////

CRunCodeMgr::CRunCodeMgr(void)
	:SQLiteHelper()
{
}

CRunCodeMgr::~CRunCodeMgr(void)
{
}

void CRunCodeMgr::Init()
{
	static bool s_isInitRunCode = false;

	if( s_isInitRunCode )
		return ;

	RUNCODE_Init();
	LoadDB_code();

	if( IsRUNCODEEnable(defCodeIndex_SYS_ListRunCode) )
	{
		s_ListRunCode( RUNCODE_Get(defCodeIndex_SYS_ListRunCode,defRunCodeValIndex_2) );
	}

	s_isInitRunCode = true;
}

void CRunCodeMgr::LoadDB_code()
{
	LOGMSG( "LoadDB_code and SetRunCode...\r\n" );

	SQLite::Statement query(*this->db,"select * from code where enable=1 order by code");
	while(query.executeStep()){
		int col = 0;
		int id = query.getColumn(col++).getInt();
		int enable = query.getColumn(col++).getInt();
		int code = query.getColumn(col++).getInt();

		SQLite::Column colobj = query.getColumn(col++);
		std::string codestr = colobj.isNull()?"":colobj;

		int val1 = query.getColumn(col++).getInt();
		int val2 = query.getColumn(col++).getInt();
		int val3 = query.getColumn(col++).getInt();
		int val4 = query.getColumn(col++).getInt();

		//jyc20160825 can not run have trouble
		std::string str_val = query.getColumn(col++);
		//std::string str_val = "jyc20160825";
		
		s_RunCode[RUNCODE_Index(code, codestr.c_str())].str_val = str_val;
		RUNCODE_Set( RUNCODE_Index(code, codestr.c_str()), val1, val2, val3, val4, true, true, true, true );
	}
}

bool CRunCodeMgr::SaveDB_code( defCodeIndex_ index, bool save_str_val )
{
	if( index<=defCodeIndex_Unknown || index>=defCodeIndex_MAX_ )
	{
		//LOGMSGEX( defLOGNAME, defLOG_ERROR, "SaveDB_code error! code={%d}\r\n", index );
		return false;
	}

	char sqlbuf[1024] = {0};
	snprintf( sqlbuf, sizeof(sqlbuf), "SELECT id FROM [code] WHERE code=%d OR codestr='%s' LIMIT 1", s_RunCode[index].code, s_RunCode[index].codestr );

	SQLite::Statement queryid( *this->db, sqlbuf );

	if(queryid.executeStep())
	{
		int id = queryid.getColumn(0);

		std::string sql = "UPDATE [code] SET enable=?,code=?,codestr=?,value1=?,value2=?,value3=?,value4=? WHERE id=?";
		if( save_str_val )
		{
			sql = "UPDATE [code] SET enable=?,code=?,codestr=?,str_val=? WHERE id=?";
		}

		SQLite::Statement query( *this->db, sql.c_str() );

		int col = 1;

		query.bind(col++, int(1));
		query.bind(col++, s_RunCode[index].code);
		query.bind(col++, s_RunCode[index].codestr);

		if( save_str_val )
		{
			query.bind(col++, s_RunCode[index].str_val);
		}
		else
		{
			query.bind(col++, s_RunCode[index].value1);
			query.bind(col++, s_RunCode[index].value2);
			query.bind(col++, s_RunCode[index].value3);
			query.bind(col++, s_RunCode[index].value4);
		}

		query.bind(col++, id);
		query.exec();
	}
	else
	{
	    SQLite::Statement query( *this->db, "INSERT INTO [code] VALUES(NULL,:enable,:code,:codestr,:value1,:value2,:value3,:value4,:str_val)" );

		int col = 1;

		query.bind(col++, int(1));
		query.bind(col++, s_RunCode[index].code);
		query.bind(col++, s_RunCode[index].codestr);
		query.bind(col++, s_RunCode[index].value1);
		query.bind(col++, s_RunCode[index].value2);
		query.bind(col++, s_RunCode[index].value3);
		query.bind(col++, s_RunCode[index].value4);
		query.bind(col++, s_RunCode[index].str_val);

		query.exec();
	}

	if( save_str_val )
	{

	}
	else
	{

	}

	return true;
}

bool CRunCodeMgr::SetCodeAndSaveDb( defCodeIndex_ index, int val1, int val2, int val3, int val4, bool do1, bool do2, bool do3, bool do4 )
{
	if( !( RUNCODE_Set( index, val1, val2, val3, val4, do1, do2, do3, do4 ) ) )
		return false;

	return this->SaveDB_code( index );
}

bool CRunCodeMgr::SetCodeStrValAndSaveDb( defCodeIndex_ index, const std::string &str_val )
{
	s_RunCode[index].str_val = str_val;
	return this->SaveDB_code( index, true );	
}

