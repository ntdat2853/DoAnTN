QT       += core gui network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    doimatkhaudialog.cpp \
    khachhangwindow.cpp \
    khthongtindialog.cpp \
    main.cpp \
    dangnhapwindow.cpp \
    qlgiamudialog.cpp \
    qlthemkhachhangdialog.cpp \
    qlthongtindialog.cpp \
    qlxoakhachhangdialog.cpp \
    qtthemcongtydialog.cpp \
    qtthongtindialog.cpp \
    qtxoacongtydialog.cpp \
    quanlywindow.cpp \
    quantriwindow.cpp

HEADERS += \
    api.h \
    dangnhapwindow.h \
    doimatkhaudialog.h \
    khachhangwindow.h \
    khthongtindialog.h \
    qlgiamudialog.h \
    qlthemkhachhangdialog.h \
    qlthongtindialog.h \
    qlxoakhachhangdialog.h \
    qtthemcongtydialog.h \
    qtthongtindialog.h \
    qtxoacongtydialog.h \
    quanlywindow.h \
    quantriwindow.h

FORMS += \
    dangnhapwindow.ui \
    doimatkhaudialog.ui \
    khachhangwindow.ui \
    khthongtindialog.ui \
    qlgiamudialog.ui \
    qlthemkhachhangdialog.ui \
    qlthongtindialog.ui \
    qlxoakhachhangdialog.ui \
    qtthemcongtydialog.ui \
    qtthongtindialog.ui \
    qtxoacongtydialog.ui \
    quanlywindow.ui \
    quantriwindow.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    Resourch.qrc
