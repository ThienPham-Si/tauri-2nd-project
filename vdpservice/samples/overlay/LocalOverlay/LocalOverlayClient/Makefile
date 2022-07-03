# ################################################################################# #
# Copyright (C) 2018-2021 VMware, Inc.  All rights reserved. -- VMware Confidential #
# ################################################################################# #

PWD  := $(shell pwd)
PWD1 := $(shell dirname -z $(PWD))
PWD2 := $(shell dirname -z $(PWD1))
PWD3 := $(shell dirname -z $(PWD2))
SAMPLES_DIR := $(PWD3)

SRCS = LocalOverlayClient.cpp
SRCS += $(SAMPLES_DIR)/common/helpers.cpp
SRCS += $(SAMPLES_DIR)/common/LogUtils.cpp
SRCS += $(SAMPLES_DIR)/common/RPCManager.cpp
SRCS += $(SAMPLES_DIR)/common/RPCManagerPosix.cpp

INC = stdafx.h
INC += $(SAMPLES_DIR)/common/wintypes.h
INC += $(SAMPLES_DIR)/common/helpers.h
INC += $(SAMPLES_DIR)/common/LogUtils.h
INC += $(SAMPLES_DIR)/common/RPCManager.h

OBJS = $(SRCS:.cpp=.o)
LIB = libLocalOverlay.so

INCLUDE = -I$(PWD) -I$(SAMPLES_DIR)/common -I$(SAMPLES_DIR)/../include
LIBS = -lstdc++ -lm -ldl -lrt

CC = g++
CFLAGS = -c $(INCLUDE) $(LIBS)  -fPIC  -fvisibility=hidden

DST_PATH = /usr/lib/vmware/view/vdpService
DST_EXISTS := $(shell test -d $(DST_PATH) ; echo $$? )

.PHONY: all clean
all: $(LIB)

$(LIB): $(OBJS) $(INC)
	$(CC) -shared -rdynamic -o $@ $(OBJS)
ifeq ($(DST_EXISTS), 0)
	sudo cp $@ $(DST_PATH)/$(@F)
endif

%.o: %.cpp $(INC)
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -f *.o *~ $(OBJS) $(LIB)
