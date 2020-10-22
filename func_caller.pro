TEMPLATE = app
CONFIG += console c++17
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += \
        main.cpp

INCLUDEPATH += /home/dmitry/downloads/vcs/cereal/include/

HEADERS += \
	func_caller.h \
	func_caller_base.h \
	func_caller_serializer.h \
	interface.h \
	json_serialization.h
