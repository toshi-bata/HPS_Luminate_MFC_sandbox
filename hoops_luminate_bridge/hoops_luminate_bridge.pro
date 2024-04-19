# ================================================================
# Run `qmake` to generate makefile for a release build
# Run `qmake CONFIG+=debug` to generate makefile for a debug build
# ================================================================

# Get Hoops Luminate Path
hoops_luminate_path = $$(HLUMINATE_INSTALL_DIR)

# If using debug VIZ libs set this to 1
USE_DEBUG_VIZ_LIBS = 0

win32-msvc* {
    equals(QT_ARCH, x86_64) {
        LIBDIR = win64_v140
    } else {
        LIBDIR = win32_v140
    }
    vc_toolset = $$(VCToolsVersion)
    greaterThan(vc_toolset, 14.2)|equals(vc_toolset, 14.2){
        equals(QT_ARCH, x86_64) {
            LIBDIR = win64_v142
        } else {
            LIBDIR = win32_v142
        }
    }
    else {
        greaterThan(vc_toolset, 14.1)|equals(vc_toolset, 14.1){
	    equals(QT_ARCH, x86_64) {
                LIBDIR = win64_v141
	    } else {
                LIBDIR = win32_v141
            }
        }
    }
}

linux*: {
    equals(QT_ARCH, x86_64) {
        LIBDIR = linux_x86_64
    } else {
        LIBDIR = linux
    }
}

macx: {
    LIBDIR = macos
}

BASE_LIBDIR = $${LIBDIR}
CONFIG(debug, debug|release) {
    equals(USE_DEBUG_VIZ_LIBS, 1) {
        EXT = d
    }
    LIBDIR = $${LIBDIR}d
}

TEMPLATE = lib

CONFIG += staticlib

TARGET = hoops_luminate_bridge

SOURCES += \
    src/AxisTriad.cpp \
    src/ConversionTools.cpp \
    src/HoopsHPSLuminateBridge.cpp \
    src/HoopsLuminateBridge.cpp \
    src/LightingEnvironment.cpp

HEADERS  += \
    include/hoops_luminate_bridge/AxisTriad.h \
    include/hoops_luminate_bridge/ConversionTools.h \
    include/hoops_luminate_bridge/HoopsHPSLuminateBridge.h \
    include/hoops_luminate_bridge/HoopsLuminateBridge.h \
    include/hoops_luminate_bridge/LightingEnvironment.h \
    include/hoops_luminate_bridge/LuminateRCTest.h

HOOPS_DIR = $${PWD}/../..
DESTDIR = $${HOOPS_DIR}/lib/$${BASE_LIBDIR}$${EXT}

INCLUDEPATH += ./include
INCLUDEPATH += $${HOOPS_DIR}/include
INCLUDEPATH += $${hoops_luminate_path}/Redsdk.m/Public


linux*: {
    DOLLAR=$
    QMAKE_CXXFLAGS += -std=c++11
    QMAKE_LFLAGS += -Wl,-rpath,\'$${DOLLAR}$${DOLLAR}ORIGIN\'

    DEFINES += IS_X11 \
        _LIN32 \
        USE_GLX_VISUAL
    LIBS += -L$${DESTDIR}
    LIBS += -lfreetype \
        -lhoops_mvo_mgk \
        -lhoops_hardcopy \
        -lhoops_stream \
        -lbase_stream \
        -lhoops \
        -lhoops_utils \
        -lXmu \
        -lXext \
        -lX11 \
        -lm \
        -ldl \
        -lpthread \
        -lGL \
        -lGLU
}

win32-msvc* {
    DEFINES += IS_WIN \
        IS_QT \
        WINDOWS_SYSTEM

    LIBS += -L$${HOOPS_DIR}/lib/$${LIBDIR} \
            -L$${hoops_luminate_path}/Redsdk.m/Lib/Win64

    LIBS += -lhoops \
            -lhoops_mvo_mgk \
            -lhoops_utilsstat_md \
            -lREDCore
}
