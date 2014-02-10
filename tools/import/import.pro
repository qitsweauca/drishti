TEMPLATE = app

DRISHTI_DEFINES = IMPORT NETCDF
include(../../drishti.pri )

RESOURCES = import.qrc

DEPENDPATH += .

QT += xml

CONFIG += release

TARGET = drishtiimport

DESTDIR = ../../bin

win32 {
LIBS += netcdf.lib
}

unix {
!macx {

INCLUDEPATH += 16bit

LIBS += -lnetcdf_c++ \
        -lnetcdf \
}
}


macx {

LIBS += \
	-lnetcdf \
	-lnetcdf_c++
}

FORMS += remapwidget.ui \
	 savepvldialog.ui \
	 drishtiimport.ui \
	 fileslistdialog.ui

# Input
HEADERS += global.h \
	   common.h \
	   staticfunctions.h \
	   fileslistdialog.h \
	   remapwidget.h \
           remaphistogramline.h \
           remaphistogramwidget.h \
	   remapimage.h \
	   gradienteditor.h \
	   gradienteditorwidget.h \
	   dcolordialog.h \
	   dcolorwheel.h \
	   drishtiimport.h \
	   myslider.h \
	   raw2pvl.h \
	   savepvldialog.h \
	   volumefilemanager.h \
	   volumedata.h \
	   volinterface.h \
	   lookuptable.h marchingcubes.h ply.h

SOURCES += global.cpp \
	   staticfunctions.cpp \
	   fileslistdialog.cpp \
	   main.cpp \
           remapwidget.cpp \
           remaphistogramline.cpp \
           remaphistogramwidget.cpp \
	   remapimage.cpp \
	   gradienteditor.cpp \
	   gradienteditorwidget.cpp \
	   dcolordialog.cpp \
	   dcolorwheel.cpp \
	   drishtiimport.cpp \
	   myslider.cpp \
	   raw2pvl.cpp \
	   savepvldialog.cpp \
	   volumedata.cpp \
	   volumefilemanager.cpp \
	   marchingcubes.cpp ply.c
