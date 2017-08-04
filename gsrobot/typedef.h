#ifndef TYPEDEF_H
#define TYPEDEF_H

#define GSIOT_VERSION	"0.0.3"	// update 201602����ʽ��n.n.n �ַ�
#define GSIOT_DBVER		15060800	// ��ݿ�汾

//#define OS_UBUNTU_FLAG 1
//#define OS_OPENWRT 1
#define OS_ARMLINUX 1

#ifdef OS_UBUNTU_FLAG
#define ROOTDIR  "/home/chen/"
#else
#define ROOTDIR  "/root/"
#endif
	
#define  MAX_PATH 260  
#include <unistd.h>  
#include <fcntl.h>  
#include <stdio.h>  
#include <stdlib.h>  
#include <stdarg.h>  

#include <stdint.h>
#include <list>
#include <map>
#include <set>
#include <vector>
#include "gloox/stanzaextension.h"

#include <string> //20150526
using  std::string;


const std::string c_NullStr = "";
const std::string c_ZeroStr = "0";
const std::string c_DefaultVer = "1.0";

#define XMPP_SERVER_DOMAIN "gsss.cn"
#define XMPP_GSIOTUser_Admin "admin@gsss.cn" // ȫСд
#define XMPP_GSIOTUser_Guest "guest@gsss.cn" // ȫСд
#define XMPP_GSIOTUser_DefaultDomain "@gsss.cn" // �û�����Ĭ��ֵ

#define defRTMFPSession_strjid "RTMFP@local.net/rtmfpthd"	// RTMFP ���ػỰ��
#define defAutoPublish_strjid "AutoPublish@local.net/publishthd"	// ���ַ���/�Զ�����

#define defIotStoreDir_Main	"E:\\iotstore"
#define defIotStoreDir_Data	"data"
#define defIotStoreDir_Pic	"pic"

enum defPicPreSize
{
	defPicPreSize_Unknown = 0,
	defPicPreSize_Full,
	defPicPreSize_Norm,
	defPicPreSize_NS,
	defPicPreSize_S,

	defPicPreSize_MAX,
};

//static char *str_PicPreSize[] = //20160527
static string str_PicPreSize[] =
{
	"nil",
	"full",
	"nm",
	"ns",
	"s",
	""
};


#define XMPP_MESSAGE_PREHEAD	"<SYS>"

const std::string XMLFUNC_media_rtmfp = "media:rtmfp";

const std::string XMLNS_GSIOT = "http://www.gswww.cn/protocol/iot";
const std::string XMLNS_GSIOT_RESULT = "http://www.gswww.cn/protocol/result";
const std::string XMLNS_GSIOT_CONTROL = "http://www.gswww.cn/protocol/device#control";
const std::string XMLNS_GSIOT_DEVICE = "http://www.gswww.cn/protocol/device#info";
const std::string XMLNS_GSIOT_HEARTBEAT = "http://www.gswww.cn/protocol/heartbeat";
const std::string XMLNS_GSIOT_AUTHORITY = "http://www.gswww.cn/protocol/authority";
const std::string XMLNS_GSIOT_AUTHORITY_USER = "http://www.gswww.cn/protocol/authority#user";
const std::string XMLNS_GSIOT_MANAGER = "http://www.gswww.cn/protocol/device#manager";
const std::string XMLNS_GSIOT_EVENT = "http://www.gswww.cn/protocol/event";
const std::string XMLNS_GSIOT_STATE = "http://www.gswww.cn/protocol/device#state";
const std::string XMLNS_GSIOT_TALK = "http://www.gswww.cn/protocol/talk";
const std::string XMLNS_GSIOT_PLAYBACK = "http://www.gswww.cn/protocol/playback";
const std::string XMLNS_GSIOT_RELATION = "http://www.gswww.cn/protocol/relation";
const std::string XMLNS_GSIOT_Preset = "http://www.gswww.cn/protocol/preset";
const std::string XMLNS_GSIOT_VObj = "http://www.gswww.cn/protocol/vobj";
const std::string XMLNS_GSIOT_Trans = "http://www.gswww.cn/protocol/trans";
const std::string XMLNS_GSIOT_Report = "http://www.gswww.cn/protocol/report";
const std::string XMLNS_GSIOT_Change = "http://www.gswww.cn/protocol/change";
const std::string XMLNS_GSIOT_MESSAGE = "http://www.gswww.cn/protocol/message";
const std::string XMLNS_GSIOT_UPDATE = "http://www.gswww.cn/protocol/update";

#define defDeviceTypeTag_media			"media"
#define defDeviceTypeTag_camera			"camera"
#define defDeviceTypeTag_rfdevice		"rfdevice"
#define defDeviceTypeTag_candevice		"candevice"
#define defDeviceTypeTag_rs485device	"rs485device"
#define defDeviceTypeTag_rfremote		"rfremote"


typedef int defLinkID;
#define defLinkID_Local		defLinkID(0)		// 本地主控模块 链路ID
#define defLinkID_Null		defLinkID(-1)		// 无链路
#define defLinkID_All		defLinkID(-9)		// 所有链路

#define defDeviceEnable		1
#define defDeviceDisable	0

#define defAlarmGuardTime_UnAllDay		0 // 全天撤防
#define defAlarmGuardTime_AllDay		1 // 全天布防
#define defAlarmGuardTime_SpecT			2 // 指定时间段

#define macAlarmGuardTime_InvaildAG(flag,begintime,endtime) ( !(flag) || (begintime)==(endtime) ) // 时间段是否无效

#define defAlarmGuard_AGTimeCount		3 // 时间段数量

// 可用状态
enum defUseable
{
	// 小于0 异常
	defUseable_ErrConfig	= -4,	// 异常 - 配置错误
	defUseable_ErrNoData	= -3,	// 异常 - 能连通无数据
	defUseable_ErrNet		= -2,	// 异常 - 连接中断
	defUseable_Err			= -1,	// 异常

	defUseable_Null			= 0,	// 未知状态、初始状态、默认状态

	// 大于0 正常
	defUseable_OK			= 1,	// 正常
};

// 传输模式
enum defTransMod
{
	defTransMod_Transparent		= 0, // 透传
	defTransMod_GSIOT			= 1, // 控制板协议
};

// 连接状态
enum defConnectState
{
	defConnectState_Null		= 0,	// 未知
	defConnectState_Connecting,			// 连接中
	defConnectState_Connected,			// 已连接
	defConnectState_Disconnected,		// 连接中断
};

// 告警状态
enum defAlarmState
{
	// 小于0 告警
	defAlarmState_NormAlarm		= -1,	// 告警

	defAlarmState_UnInit		= 0,	// 未知状态、初始状态、默认状态

	// 大于0 正常
	defAlarmState_OK			= 1,	// 正常
};
#define macAlarmState_IsAlarm(st) (st<0)
#define macAlarmState_IsOK(st) (st>0)

enum defGSReturn
{
	defGSReturn_SuccExecuted	= 2,		// 已执行，没有结果
	defGSReturn_Success			= 1,		// 成功
	defGSReturn_Null			= 0,		// 没有返回，不能确定成功失败的情况，没有结果
	defGSReturn_Err				= -1,		// 错误，一般性错误无明确错误原因时返回此错误码
	defGSReturn_NoExist			= -2,		// 目标不存在而导致失败，例如摄像头不存在
	defGSReturn_IsExist			= -3,		// 目标已存在而导致失败，例如添加用户时用户已存在
	defGSReturn_IsLock			= -4,		// 目标已被锁、正在使用
	defGSReturn_NoAuth			= -5,		// 无权限
	defGSReturn_ResLimit		= -6,		// 资源已满、已达上限
	defGSReturn_FunDisable		= -7,		// 功能屏蔽、功能禁用、功能已关闭等
	defGSReturn_IsSelf			= -8,		// 不能对自身操作
	defGSReturn_ObjDisable		= -9,		// 对象屏蔽、对象禁用等，
	defGSReturn_ConnectObjErr	= -10,		// 连接对象失败，例如连接摄像头失败
	defGSReturn_ConnectSvrErr	= -11,		// 连接服务器失败，例如连接流媒体服务器失败
	defGSReturn_CreateObjErr	= -12,		// 创建对象失败，例如创建录像回放会话
	defGSReturn_NoData			= -13,		// 无数据错误
	defGSReturn_NotFoundContent	= -14,		// 找不到内容，例如找不到符合条件的录像
	defGSReturn_TimeOut			= -15,		// 超时
	defGSReturn_UnSupport		= -16,		// 功能不支持
	defGSReturn_ErrUser			= -17,		// 用户名错误
	defGSReturn_ErrPassword		= -18,		// 密码错误
	defGSReturn_ErrParam		= -19,		// 输入参数错误
	defGSReturn_ErrConfig		= -20,		// 配置信息异常，或配置错误
	defGSReturn_NameEmpty		= -21,		// 名称为空错误
	defGSReturn_ObjEditDisable	= -22,		// 目标对象禁止修改
	defGSReturn_SameName		= -23,		// 重名，名称已存在错误
	defGSReturn_SaveFailed		= -24,		// 存储失败
	defGSReturn_DBNoExist		= -25,		// 目标数据库不存在
	defGSReturn_DBNoRec			= -26,		// 目标数据记录不存在
	defGSReturn_OverTimeRange	= -27,		// 时间超出范围
	defGSReturn_TimeNotYet		= -28,		// 时间未到
};
#define macGSIsReturnNull(ret) (defGSReturn_Null==(ret))
#define macGSSucceeded(ret) (ret>0)
#define macGSFailed(ret) (ret<0)

#define defGSReturnStr_Succeed		"succeed"
#define defGSReturnStr_Fail			"fail"

#define defDoInterval_MAX				60480000// 执行间隔最大值，毫秒
#define defDoInterval_UseSysDefault		0		// 采用系统默认
#define macIsUseSysDefault_DoInterval( DoInterval ) ( DoInterval > defDoInterval_MAX )
#define macUseDoInterval( DoInterval ) ( DoInterval>defDoInterval_MAX ? defDoInterval_MAX:DoInterval )

#define defEventLevel_Default 5

enum defDataFlag_
{
	defDataFlag_Invalid		= 0,	// 无效
	defDataFlag_First		= 1,	// 启动后第一次存储
	defDataFlag_Norm		= 2,	// 普通
	defDataFlag_TimePoint	= 3,	// 时间点存储
	defDataFlag_Changed		= 4,	// 发生变化存储
	defDataFlag_Manual		= 5,	// 手动存储
};

enum defCfgOprt_
{
	defCfgOprt_Unknown = 0,	// 未知
	defCfgOprt_Add,			// 只进行添加，已存在则失败
	defCfgOprt_Modify,		// 只进行修改，不存在则失败
	defCfgOprt_Delete,
	defCfgOprt_AddModify,	// 添加或修改，已存在修改，不存在添加
	defCfgOprt_BatchModify,	// 批量修改
	defCfgOprt_GetSelf,		// 只获取自身信息，比如获取用户信息时
	defCfgOprt_GetSimple,	// 获取简化的信息
};

enum defNotify_
{
	defNotify_Unknown = 0,
	defNotify_Add,
	defNotify_Update,
	defNotify_Delete
};

enum defTalkSendCmd_
{
	defTalkSendCmd_Open,
	defTalkSendCmd_Play,
	defTalkSendCmd_Close,
};

//命令操作
enum defCmd
{
	defCmd_Null		= 0,					// 无
	defCmd_Default	= 1,					// 默认

	defCmd_Set_Spec_Begin_		= 100,		//--
	defCmd_Set_Open				= 101,		// 打开，例：开灯、开门
	defCmd_Set_Close			= 102,		// 关闭，例：关灯、关门
	defCmd_Set_Spec_End_		= 499,		//--

	defCmd_Get_Spec_Begin_		= 500,		//--
	defCmd_Get_ObjState			= 501,		// 获取对象状态，例：获取灯状态、门状态等等
	defCmd_Get_ConnectState		= 502,		// 获取网络状态
	defCmd_Get_Spec_End_		= 899,		//--

	defCmd_Sys_Spec_			= 900,		//--
	defCmd_Sys_Reboot			= 901,		// 重启门禁主机/控制器
};
inline bool g_IsReadOnlyCmd( const defCmd cmd )
{
	return ( cmd>=defCmd_Get_Spec_Begin_ && cmd<=defCmd_Get_Spec_End_ );
}

enum GSIOTExtensionType
{
	ExtIot = gloox::ExtUser+1,
	ExtIotResult,
	ExtIotControl,
	ExtIotDeviceInfo,
	ExtIotHeartbeat,
	ExtIotAuthority,
	ExtIotAuthority_User,
	ExtIotManager,
	ExtIotEvent,
	ExtIotState,
	ExtIotChange,
	ExtIotTalk,
	ExtIotPlayback,
	ExtIotRelation,
	ExtIotPreset,
	ExtIotVObj,
	ExtIotTrans,
	ExtIotReport,
	ExtIotMessage,
	ExtIotUpdate
};

enum IOTDeviceType
{
    IOT_DEVICE_Unknown = 0,	// 错误的设备
	IOT_DEVICE_Video,		// 视频
	IOT_DEVICE_Audio,		// 声音
	IOT_DEVICE_Switch,		// 开关
	IOT_DEVICE_Temperature,	// 温度
	IOT_DEVICE_Humidity,	// 湿度
	IOT_DEVICE_Trigger,		// 触发器
	IOT_DEVICE_Camera,		// 摄像机
	IOT_DEVICE_Remote,		// 遥控
	IOT_DEVICE_RFDevice,	// 无线设备
	IOT_DEVICE_CANDevice,	// CAN设备
	IOT_DEVICE_Event,		// 驱动事件
	IOT_DEVICE_RS485,		// RS485
	IOT_DEVICE_Wind,		// 风速
	IOT_DEVICE_CO2=15,      //15
	IOT_DEVICE_PM25,		//jyc20170304 add
	IOT_DEVICE_HCHO=19,
	IOT_DEVICE_Custom=100,	// 自定义

	IOTDevice_ExSpec_Begin_		= 500,	//-- 子类型
	IOTDevice_AC_Ctl			= 501,	// 门禁主机/控制器
	IOTDevice_AC_Door			= 502,	// 门禁 - 门对象
	IOTDevice_Combo_Ctl			= 503,	// 组合控制
	IOTDevice_ExSpec_End_		= 900,	//-- 子类型

	IOT_DEVICE_All=999,		// 所有设备

	IOT_Obj_SYS				=7000,	// 对象类型 - 系统本身
	IOT_Obj_User			=7001,	// 对象类型 - 用户
	IOT_Obj_CommLink		=7002,	// 对象类型 - 链路

	IOT_VObj_scene			=8001,	// 虚拟对象 - 场景。可以将多种不同对象关联在一个环境下，构成一个"场景"。
	IOT_VObj_union			=8002,	// 虚拟对象 - 对象集合。可以将多种不同对象集合到一个虚拟对象中，构成一个独立可使用的虚拟对象(虚拟设备)，称为"对象集合"。

	// "场景"和"对象集合"在整体概念定位上有很大区别，"对象集合"是小一点的虚拟设备概念，"场景"是更广的虚拟环境概念，"对象集合"可被包含在"场景"里。
	// * 设计上预留"对象集合"可以包含"对象集合"的能力，但目前暂不实现和使用。

	IOT_Module_Unknown		=90000, // 功能模块定义
	IOT_Module_authority,			// 权限管理
	IOT_Module_record,				// 录像
	IOT_Module_system,				// 系统配置
	IOT_Module_manager,				// 设备管理
	IOT_Module_event,				// 事件配置
	IOT_Module_talk,				// 远程通话
	IOT_Module_reboot,				// 远程重启
	IOT_Module_acctl,				// 门禁控制
	IOT_Module_all			=90999	// 所有功能模块
};

#define defIOTDeviceAddr_DevSelf	0	// 当地址与设备类型和ID一起时，此值表示设备自身
#define defIOTDeviceAddr_AllAddr	-1	// 当地址与设备类型和ID一起时，此值表示设备下所有地址
#define defIOTDeviceAddr_Other		-2	// 当地址与设备类型和ID一起时，此值表示设备相关其它配置

// 一般性返回result协议module定义
enum defNormResultMod_
{
	defNormResultMod_null						= 0,	// 无

	// 命名空间=XMLNS_GSIOT_CONTROL
	defNormResultMod_control_camplay			= 1,	// 视频播放
	defNormResultMod_control_MotionTrack		= 2,	// 打开关闭智能跟踪操作
	defNormResultMod_control_PTZ_ParkAction		= 3,	// 打开关闭守望操作
};

enum defAuthFlag
{
	defAuthFlag_Empty		= 0x00
};

// 用户权限
//0 无权限,1只读,2写，3读写
enum defUserAuth
{
	defUserAuth_Null = 0,
	defUserAuth_RO = 1,
	defUserAuth_WO,
	defUserAuth_RW
};

// 所有权限的判断值
#define defAllAuth_DeviceType		IOT_DEVICE_All
#define defAllAuth_DeviceID			0
#define defAllAuth_DeviceAuth		defUserAuth_RW
#define defAllAuth_DeviceAuth_Null	defUserAuth_Null
#define defAllAuth_DeviceShowName	"所有设备"

// 所有权限的判断值
#define defAllAuth_ModuleType		IOT_Module_all
#define defAllAuth_ModuleID			0
#define defAllAuth_ModuleAuth		defUserAuth_RW
#define defAllAuth_ModuleAuth_Null	defUserAuth_Null

// 功能模块的ID默认值
#define defAuth_ModuleDefaultID		0

enum IOTDeviceReadType
{
	IOT_Unknow,
	IOT_READ,
	IOT_WRITE,
	IOT_READWRITE
};

enum SerialPortModule
{
	MOD_NULL			= 0x00,

	RXB8_315			= 0x01,
	RXB8_433			= 0x02,
	RXB8_315_TX			= 0x03,
	RXB8_433_TX			= 0x04,
	CC1101_433			= 0x05,
	CAN_Chip			= 0x06,
	Module_RS485		= 0x07,
	RXB8_315_original	= 0x08,
	RXB8_433_original	= 0x09,
	Module_GSM			= 0x0A,
	Read_IOT_Ver		= 0x0B,

	// V2
	MOD_IR_RX_original	= 0x30,	// 红外接收	只读
	MOD_IR_RX_code		= 0x31,	// 红外接收编码	只读
	MOD_IR_TX_original	= 0x3A,	// 红外发射	只写
	MOD_IR_TX_code		= 0x3B,	// 红外发射编码	只写
	MOD_SYS_set			= 0xE0, // 系统设置
	MOD_SYS_get			= 0xEA, // 系统设置返回
};

enum DataType
{
	IOT_DataUnknow  = 0x00,
	IOT_Integer = 0x01,
    IOT_String	= 0x02,
    IOT_Boolean = 0x03,
	IOT_Byte	= 0x04,
	IOT_Int16	= 0x05,
	IOT_Long	= 0x06,
	IOT_Double	= 0x07,
	IOT_Float	= 0x08,
	IOT_Object  = 0x09
};

enum EventType
{
	Unknown_Event	= 0,
	SMS_Event		= 1,
	EMAIL_Event,
	NOTICE_Event,
	CONTROL_Event,
	Eventthing_Event,
	CALL_Event
};

// 品牌厂家
enum defFactory
{
	defFactory_Unknown = 0,	// 无
	CameraType_Unkown = defFactory_Unknown,

	SSD1935			= 1,
	TI368			= 2,
	CameraType_hik	= 3,		// 海康
	CameraType_dh	= 4,		// 大华

	defFactory_ZK	= 5,			// 中控（门禁）

	defFactory_HIK	=CameraType_hik,	// 按厂家取类型名 海康
	defFactory_DH	=CameraType_dh,		// 按厂家取类型名 大华
};
typedef defFactory IPCameraType;


enum defRecMod
{
	defRecMod_NoRec = 0,	// 无录像
	defRecMod_OnCamera,		// 在摄像机设备上
	defRecMod_OnReordSvr	// 在录像服务器上
};

// 云台功能标志
enum GSPtzFlag
{
	GSPtzFlag_Null = 0,					// 无
	GSPtzFlag_forward_direction = 1,	// 正常方向
	GSPtzFlag_negative_direction		// 方向倒置
};

// 调焦功能标志
enum GSFocalFlag
{
	GSFocalFlag_Null = 0,				// 无
	GSFocalFlag_Enable = 1				// 启用
};

// 预置点索引 33以上、100出头这些有些特殊意义的值。
// 而1-8本身无特殊意义，但在具体使用时在设定上作为特殊用途，比如用于定时、守望。
#define defGSPresetIndex_Min 201		// 预置点索引最小值
#define defGSPresetIndex_Max 232		// 预置点索引最大值

#define GSPTZ_STOP_OFFSET		100
#define GSPTZ_FUNCTION_OFFSET	200
#define GSPTZ_SYSCMD_OFFSET		900
enum GSPTZ_CtrlCmd
{
	GSPTZ_Null	  = -1,		//
	GSPTZ_STOPAll = 0,		// 云台停止，通用停止
	GSPTZ_UP,				// 云台向上
	GSPTZ_DOWN,				// 云台向下
	GSPTZ_RIGHT,			// 云台向右
	GSPTZ_RIGHT_UP,			// 云台右上
	GSPTZ_RIGHT_DOWN,		// 云台右下
	GSPTZ_LEFT,				// 云台向左
	GSPTZ_LEFT_UP,			// 云台左上
	GSPTZ_LEFT_DOWN,		// 云台左下
	GSPTZ_AUTO,				// 云台自动

	GSPTZ_ZOOM_IN =11,		// 焦距变大(倍率变大)
	GSPTZ_ZOOM_OUT,			// 焦距变小(倍率变小)
	GSPTZ_FOCUS_NEAR,		// 焦点前调
	GSPTZ_FOCUS_FAR,		// 焦点后调
	GSPTZ_IRIS_OPEN,		// 光圈扩大
	GSPTZ_IRIS_CLOSE,		// 光圈缩小

	//----------------------------------------
	// 命令对应停止
	GSPTZ_STOP_UP = GSPTZ_STOP_OFFSET+1,		// 云台向上
	GSPTZ_STOP_DOWN,			// 云台向下
	GSPTZ_STOP_RIGHT,			// 云台向右
	GSPTZ_STOP_RIGHT_UP,		// 云台右上
	GSPTZ_STOP_RIGHT_DOWN,		// 云台右下
	GSPTZ_STOP_LEFT,			// 云台向左
	GSPTZ_STOP_LEFT_UP,			// 云台左上
	GSPTZ_STOP_LEFT_DOWN,		// 云台左下
	GSPTZ_STOP_AUTO,			// 云台自动

	GSPTZ_STOP_ZOOM_IN = GSPTZ_STOP_OFFSET+11,	// 焦距变大(倍率变大)
	GSPTZ_STOP_ZOOM_OUT,		// 焦距变小(倍率变小)
	GSPTZ_STOP_FOCUS_NEAR,		// 焦点前调
	GSPTZ_STOP_FOCUS_FAR,		// 焦点后调
	GSPTZ_STOP_IRIS_OPEN,		// 光圈扩大
	GSPTZ_STOP_IRIS_CLOSE,		// 光圈缩小

	//----------------------------------------
	// 预置点
	GSPTZ_Add_Preset = GSPTZ_STOP_OFFSET+21,		// 添加预置点
	GSPTZ_Del_Preset,								// 删除预置点
	GSPTZ_Goto_Preset,								// 转到预置点
	GSPTZ_Rename_Preset,							// 修改预置点名称
	GSPTZ_SetNew_Preset,							// 更新预置点，设置新的位置到一个已存在的预置点

	//----------------------------------------
	// 更多功能命令
	GSPTZ_MOTION_TRACK_Enable = GSPTZ_FUNCTION_OFFSET+1,	// 智能跟踪功能 打开
	GSPTZ_MOTION_TRACK_Disable,								// 智能跟踪功能 关闭
	GSPTZ_MANUALTRACE,										// 手动跟踪控制
	GSPTZ_MANUALPTZSel,										// 手动定位
	GSPTZ_MANUALZoomRng,									// 手动区域选择放大缩小

	GSPTZ_PTZ_ParkAction_Enable,							// 守望功能 打开
	GSPTZ_PTZ_ParkAction_Disable,							// 守望功能 关闭

	GSPTZ_DoPrePic,									// 生成预览图

	//----------------------------------------
	// 系统级命令
	GSPTZ_CameraReboot = GSPTZ_SYSCMD_OFFSET+1,		// 重启摄像机
};

enum RFCommand
{
	Heartbeat_Request       = 0x0101,
	Heartbeat_Response      = 0x0102,
	Device_Error            = 0x0103,
	Device_Broadcast        = 0x0202,
	Device_Info_Request     = 0x0301,
	Device_Info_Response    = 0x0302,
	Device_Address_Request  = 0x0401,
	Device_Address_Response = 0x0402,
	Device_Write_Request    = 0x0501,
	Device_Write_Response   = 0x0502,
	Device_Setting_Request  = 0x0601,
	Device_Setting_Response = 0x0602
};

enum CANCommand
{
	CAN_Heartbeat_Request = 0x10,
	CAN_Heartbeat_Response = 0x11,
	CAN_Info_Request = 0x20,
	CAN_Info_Response = 0x21,
	CAN_Address_Request = 0x30,
	CAN_Address_Response= 0x31,
	CAN_Address_Read_Request = 0x40,
	CAN_Address_Read_Response = 0x41,
	CAN_Address_Write_Request = 0x50,
	CAN_Address_Write_Response = 0x51,
	CAN_Address_Setting_Request = 0x60,
	CAN_Address_Setting_Response = 0x61,
	CAN_Address_Packet_Ext = 0x70,
	CAN_Address_Error = 0x80
};

enum GSIOTObjType
{
	GSIOTObjType_Unknown = 0,
	GSIOTObjType_GSIOTDevice = 1,
	GSIOTObjType_DeviceAddress,
	GSIOTObjType_RemoteButton,
	GSIOTObjType_PresetObj,
};

// 回放控制码
enum GSPlayBackCode_
{
	GSPlayBackCode_Stop			= -1,
	GSPlayBackCode_NULL			= 0,
	GSPlayBackCode_GetState		= 1,

	GSPlayBackCode_PLAYPAUSE	= 11,	// 暂停播放
	GSPlayBackCode_PLAYRESTART,			// 恢复播放，只能从暂停时的状态恢复继续播放，当前状态不是暂停时操作无效
	GSPlayBackCode_PLAYNORMAL,			// 正常速度，任何状态恢复到正常播放模式
	GSPlayBackCode_PLAYFAST,			// 快放，播放速度升级1级，速度级别：慢2-慢1-正常速-快1-快2，不会超过下限和上限
	GSPlayBackCode_PLAYSLOW,			// 慢放，播放速度降级1级

	GSPlayBackCode_PLAYGETPOS	= 21,	// 播放进度百分比
	GSPlayBackCode_PLAYSETPOS,			// 跳转到播放进度百分比位置继续播放
	GSPlayBackCode_SETSPEED,			// 设置码流速度，速度单位：kbps，最小为256kbps
	GSPlayBackCode_PlaySetTime,			// 播放时间定位
	GSPlayBackCode_SkipTime,			// 向前快跳n秒，快跳n秒后恢复成正常速度播放
};

enum GSDevViewMod_
{
	GSDevViewMod_Default = 0,
	GSDevViewMod_Normal = 1,
	GSDevViewMod_Debug = 123456
};


// 系统整体综合所有布防相关设置条件，目前是否处于布防生效状态
// AlarmGuardCurState
enum GSAGCurState_
{
	GSAGCurState_UnArmed		= 0,  // 撤防
	GSAGCurState_AllArmed		= 1,  // 全部布防并生效；
	GSAGCurState_PartOfArmed	= 2,  // 部分触发器布防并生效；
	GSAGCurState_WaitTimeArmed	= 3,  // 暂时未生效等待条件达到后自动生效。
};

struct struGSTime
{
    unsigned short Year;
    unsigned char Month;
    unsigned char Day;
    unsigned char Hour;
    unsigned char Minute;
    unsigned char Second;
    unsigned short Milliseconds;

	struGSTime(
		const unsigned short in_Year=0, const unsigned char in_Month=0, const unsigned char in_Day=0,
		const unsigned char in_Hour=0, const unsigned char in_Minute=0, const unsigned char in_Second=0, const unsigned short in_Milliseconds=0 )
		: Year(in_Year), Month(in_Month), Day(in_Day), Hour(in_Hour), Minute(in_Minute), Second(in_Second), Milliseconds(in_Milliseconds)
	{
	}

	void AddSelfMonth()
	{
		Month++;
		if( Month > 12 )
		{
			Month = 1;
			Year++;
		}
	}
	
	void SubtractSelfMonth()
	{
		Month--;
		if( Month < 1 )
		{
			Month = 12;
			Year--;
		}
	}
	bool IsValidYearMonthOnly() const
	{
		return (
			Year>0 && Year<2999
			&& Month>0 && Month<13
			);
	}

	bool IsValidDateOnly() const
	{
		return (
			IsValidYearMonthOnly()
			&& Day>0 && Day<32
			);
	}

	bool IsValid() const
	{
		return (
			IsValidDateOnly()
			&& Hour>=0 && Hour<25
			&& Minute>=0 && Minute<61
			&& Second>=0 && Second<61
			);
	}
};

struct GSIOTAddrObjKey
{
	IOTDeviceType dev_type;
	int dev_id;
	IOTDeviceType address_type;
	int address_id;

	GSIOTAddrObjKey( const IOTDeviceType in_dev_type, const int in_dev_id, const IOTDeviceType in_address_type, const int in_address_id )
		: dev_type(in_dev_type), dev_id(in_dev_id), address_type(in_address_type), address_id(in_address_id)
	{
	}

	GSIOTAddrObjKey()
		: dev_type(IOT_DEVICE_Unknown), dev_id(0), address_type(IOT_DEVICE_Unknown), address_id(0)
	{
	}

	std::string get_str( const bool simple=false, const std::string &strpre=c_NullStr ) const
	{
		char buf[32] = {0};

		if( simple )
			snprintf( buf, sizeof(buf), "%s%d.%d.%d.%d", strpre.c_str(), dev_type, dev_id, address_type, address_id );
		else
			snprintf( buf, sizeof(buf), "%sdev(%d,%d) addr(%d,%d)", strpre.c_str(), dev_type, dev_id, address_type, address_id );

		return std::string(buf);
	}
};

typedef struct _IotSwitch
{
	int state_default;
	int state;
}IotSwitch;

typedef struct _IotTemperature
{
	float fahrenheit;
	float celsius;
}IotTemperature;

typedef struct _IotMedia
{
	std::string address;
	std::string protocol;
	std::string state;
}IotVideo,IotAudio;

typedef struct _IotConfig
{
	std::string serialnumber;
	std::string jid;
	std::string password;
}IotConfig;

typedef struct _DeviceCamera
{
	int id;
	int type;
	std::string url;	
	int isStop;
}DeviceCamera;

typedef struct _rtsignal
{
	uint32_t one_low_time;
	uint32_t one_high_time;
	uint32_t zero_low_time;
	uint32_t zero_high_time;
	uint32_t pulse_split_time;
	uint32_t code;
	uint8_t pulse_bit_size;
	uint32_t trigger_count;
}REMOTESignal;

typedef struct _ipcambroadcast
{
	std::string deviceID;
	std::string ipAddr;
	std::string macAddr;
	std::string mask;
	std::string gateway;
	std::string dns;
	std::string dns2;
	uint16_t tcpPort;
	uint16_t webPort;
	bool enableDHCP;
	std::string name;
}IPCAMERABROADCAST;

struct x264_nal_s
{
    int i_ref_idc;  /* nal_priority_e */
    int i_type;     /* nal_unit_type_e */
    int b_long_startcode;
    int i_first_mb; /* If this NAL is a slice, the index of the first MB in the slice. */
    int i_last_mb;  /* If this NAL is a slice, the index of the last MB in the slice. */

    /* Size of payload in bytes. */
    int     i_payload;
    /* If param->b_annexb is set, Annex-B bytestream with startcode.
     * Otherwise, startcode is replaced with a 4-byte size.
     * This size is the size used in mp4/similar muxing; it is equal to i_payload-4 */
    unsigned char *p_payload;

	x264_nal_s()
	{
		i_ref_idc = 0;
		i_type = 0;
		b_long_startcode = 0;
		i_first_mb = 0;
		i_last_mb = 0;

		i_payload = 0;
		p_payload = NULL;
	}
};

// tagʱ�Ĳ�����Ϣ
struct struTagParam 
{
	bool isValid; // ������Чʱ����Ĭ��tag��������Чʱ��������Ϣtag
	bool isResult;
	int fmt;
	defUserAuth Auth;

	struTagParam( bool in_isValid=false, bool in_isResult=false, int in_fmt=0, defUserAuth in_Auth=defUserAuth_RW )
		:isValid(in_isValid), isResult(in_isResult), fmt(in_fmt), Auth(in_Auth)
	{
	}
};

#endif
