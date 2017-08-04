
# define sources root directory before everything
SRC_ROOT := ../..

# change compiler and linker option before you include rule.mk
#
# link to libscript.a when you need to fetch configuration
# from test_script
#
#CFLAGS := $(CFLAGS) -g
#LDFLAGS := -lopencv_highgui -lopencv_imgcodecs -lopencv_imgproc -lopencv_core -lopencv_videoio -lx264 -lfaad -lfaac -lasound -luuid -lpthread -lstdc++ -lgloox -lsqlite3 -lrt 
LDFLAGS := -lhighgui -lx264 -lfaad -lfaac -lasound -luuid -lpthread -lstdc++ -lgloox -lsqlite3 -lrt 
# include rule.mk
include $(SRC_ROOT)/rule+.mk

# define objects collection variable
gsrtmp_objs = librtmp/hashswf.o librtmp/parseurl.o librtmp/log.o librtmp/amf.o librtmp/rtmp.o \
h264/h264_xu_ctrls.o h264/v4l2uvc.o h264/nalu.o h264/cap_desc_parser.o h264/cap_desc.o h264/videoh264.o 
gsrobot_objs = xmpp/XmppGSResult.o xmpp/XmppGSAuth.o xmpp/XmppGSMessage.o xmpp/XmppGSState.o xmpp/XmppGSManager.o xmpp/XmppGSRelation.o \
xmpp/XmppGSUpdate.o xmpp/XmppGSVObj.o xmpp/XmppGSChange.o xmpp/XmppGSEvent.o xmpp/XmppGSReport.o xmpp/XmppRegister.o \
AddressManager.o common.o DeviceAddress.o EditAttrMgr.o \
SQLiteHelper.o Column.o Database.o Statement.o DataStoreMgr.o RunCode.o Transaction.o DBUpdate.o DeviceConnection.o \
TriggerControl.o ControlEvent.o IGSMessageHandler.o CANDeviceControl.o DeviceManager.o \
PresetManager.o CommLinkManager.o rs232.o RS485DevControl.o ModbusProc.o RFRemoteControl.o \
RemoteButtonManager.o RemoteButtonClass.o ControlMessage.o  AutoCallEvent.o AutoControlEvent.o \
AutoEventthing.o AutoNoticeEvent.o AutoSendSMSEvent.o   SerialDataBuffer.o SerialMessage.o \
HeartbeatMon.o CommLink.o NetBaseClient.o NetMan.o NetModule.o NetTcpClient.o NetUdpClient.o \
RFDeviceControl.o RFDevice.o RFSignalDefine.o VObjClass.o TimerManager.o HttpRequest.o \
audio/AudioCap_AAC.o audio/AudioCapBase.o audio/AudioCap_File.o audio/AudioCap_G711.o \
audio/AudioCfg.o audio/Audio_WAV2AAC.o audio/AudioCap.o \
IPCamConnection.o IPCameraBase.o IPCameraManager.o CameraControl.o CameraFocal.o CameraPTZ.o \
RTMPSend.o RTMPSession.o RTSPConnection.o RTSPManager.o MediaControl.o MediaStream.o H264MediaFile.o H264NalReader.o \
GSRemoteTalkMgr.o xmpp/XmppGSTalk.o xmpp/XmppGSPreset.o xmpp/XmppGSPlayback.o \
Video.o recorder.o player.o Aacdecoder.o GsCamera.o \
gsiot/GSIOTControl.o gsiot/GSIOTDevice.o gsiot/GSIOTDeviceInfo.o gsiot/GSIOTInfo.o gsiot/GSIOTHeartbeat.o\
gsiot/GSIOTConfig.o gsiot/GSIOTUser.o gsiot/GSIOTUserMgr.o gsiot/GSIOTEvent.o gsiot/GSIOTClient.o \
gsarm.o

# add your target(s) to all
.PHONY: all
all: gsrobot

# define you target, the target will be output to dragonboard/output/bin
# directory
gsrobot: $(gsrtmp_objs) $(gsrobot_objs)
	$(LINK_MSG)
	$(LINKX)
	
# change 'gsrobot_objs' to your objects collection variable
$(gsrtmp_objs): %.o: %.c
	$(COMPILEC_MSG)
	$(COMPILEC)

$(gsrobot_objs): %.o: %.cpp
	$(COMPILEX_MSG)
	$(COMPILEX)

