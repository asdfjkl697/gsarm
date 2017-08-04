#-------------------------------------------------
#
# Project created by QtCreator 2013-06-20T08:42:56
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = recordAudio
TEMPLATE = app


SOURCES += main.cpp\
    recorder.cpp \
    player.cpp

HEADERS  += \
    recorder.h \
    player.h

FORMS    +=


INCLUDEPATH += -I ./faac/include \
                ./alsa/include

LIBS  += -L./alsa/lib -lasound

LIBS +=  -L./faac/lib -lfaac



