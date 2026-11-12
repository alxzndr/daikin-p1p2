# Host build of the ESP decoder for offline replay - test/host/Makefile
#
# Part of the P1P2MQTT fork, CC BY-NC-ND 4.0 with the exceptions in LICENSE.md.
#
# This builds the *firmware* sources (unmodified) with the host compiler against the
# stubs in stubs/. It is not a firmware build: nothing here runs on the ESP and
# nothing here changes the PlatformIO build.
#
#   make                      build ../..../examples/P1P2MQTT-bridge with -DE_SERIES
#   make SERIES=F_SERIES      build the same sources for another brand
#   make FIRMWARE_DIR=/tmp/rc23/examples/P1P2MQTT-bridge   build another checkout
#   make clean
#
# FIRMWARE_DIR is what lets task 03 replay one capture through rc23 and through main.

# The firmware casts the address of the ESP linker symbol _EEPROM_start to uint32_t.
# Apple clang treats that as a hard error on a 64-bit host; GCC makes it a warning
# with -fpermissive. So: build with GCC when it is available, and document that a
# clang host build needs the same trick or a 32-bit host.
GCC_CANDIDATES := g++-16 g++-15 g++-14 g++-13 g++-12 g++-11 g++ c++
CXX_DETECT := $(shell for c in $(GCC_CANDIDATES); do \
    if command -v $$c >/dev/null 2>&1 && $$c --version 2>/dev/null | grep -q 'Free Software Foundation'; then echo $$c; break; fi; done)
# make predefines CXX=g++ (which is clang on macOS), so only override the default
CXX := $(if $(CXX_DETECT),$(CXX_DETECT),c++)
IS_GCC   := $(shell $(CXX) --version 2>/dev/null | grep -qi 'Free Software Foundation' && echo 1)
SERIES   ?= E_SERIES
REPO_ROOT:= $(abspath ../..)
FIRMWARE_DIR ?= $(REPO_ROOT)/examples/P1P2MQTT-bridge
SKETCH   := $(FIRMWARE_DIR)/P1P2MQTT-bridge.ino
BUILD_DIR?= build/$(SERIES)
BIN      := $(BUILD_DIR)/replay

# The firmware is upstream code with its own warning story; the harness wants a
# quiet, reproducible build. -Wall stays on for our own files (checked separately).
CXXFLAGS ?= -std=gnu++20 -O1 -g3 -fno-omit-frame-pointer
CXXFLAGS += -D$(SERIES)
CXXFLAGS += -DSERIES_NAME='"$(SERIES)"'
CXXFLAGS += -DHOST_REPLAY_BUILD=1
CXXFLAGS += -DFIRMWARE_SKETCH='"$(SKETCH)"'
CXXFLAGS += -include $(CURDIR)/stubs/host_env.h
CXXFLAGS += -I$(CURDIR)/stubs -I$(BUILD_DIR) -I$(FIRMWARE_DIR)
CXXFLAGS += -Wno-write-strings -Wno-format -Wno-format-security -Wno-unused-variable \
            -Wno-unused-but-set-variable -Wno-unused-function -Wno-sign-compare \
            -Wno-reorder -Wno-unknown-pragmas -Wno-attributes -Wno-c++11-narrowing
ifeq ($(IS_GCC),1)
CXXFLAGS += -fpermissive -Wno-pointer-to-int-cast -Wno-narrowing
# Homebrew GCC on macOS does not know where the SDK is; without this it cannot even
# find stdint.h. On Linux these variables come out empty and nothing is added.
MACOS_SDK  := $(shell xcrun --show-sdk-path 2>/dev/null)
ifneq ($(MACOS_SDK),)
CXXFLAGS += -isysroot $(MACOS_SDK) -idirafter $(MACOS_SDK)/usr/include
LDFLAGS  += -isysroot $(MACOS_SDK)
endif
endif
LDFLAGS  ?=

OBJS := $(BUILD_DIR)/replay.o
PROTOCOL_GEN := $(BUILD_DIR)/prototypes.h

.PHONY: all clean dirs check-firmware
all: $(BIN)

check-firmware:
	@test -f "$(SKETCH)" || { echo "error: sketch not found: $(SKETCH)"; echo "       pass FIRMWARE_DIR=<path>/examples/P1P2MQTT-bridge"; exit 1; }

$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)

# The Arduino IDE / PlatformIO insert prototypes for sketch functions; a plain host
# compiler does not, so the sketch would not compile in its own call order. This
# generates the same declarations from the sketch under test (build artifact only).
$(PROTOCOL_GEN): gen_prototypes.py $(SKETCH) | $(BUILD_DIR)
	python3 gen_prototypes.py "$(SKETCH)" > $@.tmp && mv $@.tmp $@

$(BUILD_DIR)/replay.o: replay.cpp stubs/host_env.h $(PROTOCOL_GEN) check-firmware | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c replay.cpp -o $@

$(BIN): $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) -o $@ $(LDFLAGS)
	@echo "built $@ from $(FIRMWARE_DIR)"

clean:
	rm -rf build
