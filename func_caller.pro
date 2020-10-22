TEMPLATE = app
CONFIG += console c++17
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += \
        main.cpp

INCLUDEPATH += modules/json/include/

HEADERS += \
	func_caller.h \
	func_caller_base.h \
	func_caller_serializer.h \
	interface.h \
	json_interface_wrapper.h \
	json_serialization.h
