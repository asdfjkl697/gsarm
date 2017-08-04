
# define sources root directory before everything
SRC_ROOT := ../..

# change compiler and linker option before you include rule.mk
#
# link to libscript.a when you need to fetch configuration
# from test_script
#
#CFLAGS := $(CFLAGS) -g
#LDFLAGS := -lscript
LDFLAGS := -lpthread -lstdc++ -lgloox -lsqlite3 -lrt #jyc20170505 modify -lstdcpp -lstdc++ add -lrt

# include rule.mk
include $(SRC_ROOT)/rule+.mk

# define objects collection variable
gsiot_objs = AddressManager.o common.o DeviceAddress.o EditAttrMgr.o GSIOTControl.o \
GSIOTDevice.o GSIOTDeviceInfo.o GSIOTInfo.o XmppGSResult.o SQLiteHelper.o Column.o Database.o Statement.o \
Transaction.o GSIOTConfig.o DBUpdate.o GSIOTUser.o GSIOTUserMgr.o XmppGSAuth.o XmppGSMessage.o \
DataStoreMgr.o RunCode.o GSIOTHeartbeat.o XmppGSState.o XmppGSManager.o DeviceConnection.o \
TriggerControl.o ControlEvent.o IGSMessageHandler.o CANDeviceControl.o DeviceManager.o \
PresetManager.o CommLinkManager.o rs232.o RS485DevControl.o ModbusProc.o RFRemoteControl.o \
RemoteButtonManager.o RemoteButtonClass.o ControlMessage.o GSIOTEvent.o AutoCallEvent.o AutoControlEvent.o \
AutoEventthing.o AutoNoticeEvent.o AutoSendSMSEvent.o XmppRegister.o  SerialDataBuffer.o SerialMessage.o \
HeartbeatMon.o CommLink.o NetBaseClient.o NetMan.o NetModule.o NetTcpClient.o NetUdpClient.o \
RFDeviceControl.o RFDevice.o RFSignalDefine.o XmppGSChange.o XmppGSEvent.o XmppGSReport.o \
XmppGSRelation.o XmppGSUpdate.o XmppGSVObj.o VObjClass.o TimerManager.o HttpRequest.o GSIOTClient.o gsiot.o

# add your target(s) to all
.PHONY: all
all: gsiot

# define you target, the target will be output to dragonboard/output/bin
# directory
gsiot: $(gsiot_objs)
	$(LINK_MSG)
	$(LINKX)

# change 'gsiot_objs' to your objects collection variable
$(gsiot_objs): %.o: %.cpp
	$(COMPILE_MSG)
	$(COMPILEX)

