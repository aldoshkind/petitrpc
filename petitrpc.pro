TEMPLATE = app
CONFIG += console c++17
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += \
        main.cpp

INCLUDEPATH += modules/json/include/

HEADERS += \
	command.h \
	command_executor.h \
	func_caller.h \
	func_caller_base.h \
	func_caller_serializer.h \
	interface_client.h \
	interface_server.h \
	json_interface_wrapper.h \
	json_serialization.h \
	rpc_exception.h

LIBS += -pthread
