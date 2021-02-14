#
# Simple Arduino Makefile
#
# Author: Nick Gammon
# Edited by: Viktor Tomkovič
# Date: 18th March 2015
# Edited: 5th February 2021
#
# Example of usage: VER=10 make rebuild

ifndef VER
VER=00
endif

# Input paths
REPO_DIR=$${HOME}/srcveci/workspace-micro/
SKETCH_DIR=IR-USBPlusGate/
MAIN_SKETCH=IR-USBPlusGate.ino
SKETCH_PATH="$(REPO_DIR)$(SKETCH_DIR)$(MAIN_SKETCH)"

# Output path
OUTPUT_PATH="$(REPO_DIR)build/$(VER)/"

# Arduino paths
ARDUINO_DIR=$${HOME}/srcveci/arduino-1.8.13/
ARDUINO_LOCAL_DIR=$${HOME}/.arduino15/
ARDUINO_LIBRARIES=$${HOME}/Arduino/libraries

# Various parameters
F_CPU=16500000
MCU=attiny85
ARDUINOIDEVER=10813


# Various programs
CC="$(ARDUINO_DIR)hardware/tools/avr/bin/avr-gcc"
CPP="$(ARDUINO_DIR)hardware/tools/avr/bin/avr-g++"
AR="$(ARDUINO_DIR)hardware/tools/avr/bin/avr-ar"
OBJ_COPY="$(ARDUINO_DIR)hardware/tools/avr/bin/avr-objcopy"
OBJ_DUMP="$(ARDUINO_DIR)hardware/tools/avr/bin/avr-objdump"
AB="$(ARDUINO_DIR)arduino-builder"
CTAGS="$(ARDUINO_DIR)tools-builder/ctags/5.8-arduino11/ctags"

# Compile flags
AB_FLAGS=-logger=machine -hardware $(ARDUINO_DIR)hardware -hardware $(ARDUINO_LOCAL_DIR)packages -tools $(ARDUINO_DIR)tools-builder -tools $(ARDUINO_DIR)hardware/tools/avr -tools $(ARDUINO_LOCAL_DIR)packages -built-in-libraries $(ARDUINO_DIR)libraries -libraries $(ARDUINO_LIBRARIES) -fqbn=digistump:avr:digispark-tiny -ide-version=10813 -build-path $(OUTPUT_PATH) -warnings=default -build-cache $(OUTPUT_PATH)cache/ -prefs=build.warn_data_percentage=75 -prefs=runtime.tools.avr-gcc.path=$(ARDUINO_LOCAL_DIR)packages/arduino/tools/avr-gcc/4.8.1-arduino5 -prefs=runtime.tools.avr-gcc-4.8.1-arduino5.path=$(ARDUINO_LOCAL_DIR)arduino/tools/avr-gcc/4.8.1-arduino5 -prefs=runtime.tools.micronucleus.path=$(ARDUINO_LOCAL_DIR)packages/digistump/tools/micronucleus/2.0a4 -prefs=runtime.tools.micronucleus-2.0a4.path=$(ARDUINO_LOCAL_DIR)packages/digistump/tools/micronucleus/2.0a4

GENERAL_FLAGS=-c -g -Os -Wall -ffunction-sections -fdata-sections -mmcu=$(MCU) -DF_CPU=$(F_CPU)L -MMD -DUSB_VID=null -DUSB_PID=null -DARDUINO=$(ARDUINOIDEVER) -DARDUINO_AVR_DIGISPARK -DARDUINO_ARCH_AVR
CPP_FLAGS=$(GENERAL_FLAGS) -fno-exceptions -w -x c++ -CC
CC_FLAGS=$(GENERAL_FLAGS)
A_FLAGS=$(GENERAL_FLAGS) -x assembler-with-cpp

# Location of include files
INCLUDE_FILES="-I$(ARDUINO_LOCAL_DIR)packages/digistump/hardware/avr/1.6.7/cores/tiny" "-I$(ARDUINO_LOCAL_DIR)packages/digistump/hardware/avr/1.6.7/variants/digispark"

# Library sources
LIBRARY_DIR="$(ARDUINO_LOCAL_DIR)packages/digistump/hardware/avr/1.6.7/libraries/"
DS_KEYBOARD="$(LIBRARY_DIR)DigisparkKeyboard/"

	#rm -rf $(OUTPUT_PATH)
	#$(AB) -dump-prefs $(AB_FLAGS) -verbose $(SKETCH_DIR)$(MAIN_SKETCH)
	#$(CTAGS) -u --language-force=c++ -f - --c++-kinds=svpf --fields=KSTtzns --line-directives $(OUTPUT_PATH)preproc/ctags_target_for_gcc_minus_e.cpp

build:
	mkdir -p $(OUTPUT_PATH)
	$(AB) -compile $(AB_FLAGS) -verbose $(SKETCH_PATH)
	$(CPP) $(CPP_FLAGS) $(INCLUDE_FILES) -I$(DS_KEYBOARD) $(OUTPUT_PATH)sketch/$(MAIN_SKETCH).cpp -o $(OUTPUT_PATH)$(MAIN_SKETCH).cpp.o
	$(CC) $(A_FLAGS) $(DS_KEYBOARD)usbdrvasm.S -o $(OUTPUT_PATH)libraries/DigisparkKeyboard/usbdrvasm.S.o
	$(CC) -Os -Wl,--gc-sections -mmcu=attiny85 -o $(OUTPUT_PATH)$(MAIN_SKETCH).elf $(OUTPUT_PATH)$(MAIN_SKETCH).cpp.o $(OUTPUT_PATH)libraries/DigisparkKeyboard/usbdrvasm.S.o $(OUTPUT_PATH)libraries/DigisparkKeyboard/oddebug.c.o $(OUTPUT_PATH)libraries/DigisparkKeyboard/osccal.c.o $(OUTPUT_PATH)libraries/DigisparkKeyboard/usbdrv.c.o $(OUTPUT_PATH)core/pins_arduino.c.o $(OUTPUT_PATH)cache/core/core_digistump_avr_digispark-tiny_0b3a87ccc0b70a8dec8f52b062d1baa7.a -L$(OUTPUT_PATH) -lm
	$(OBJ_DUMP) -S $(OUTPUT_PATH)$(MAIN_SKETCH).elf > $(OUTPUT_PATH)$(MAIN_SKETCH).S

rmdir:
	rm -rf $(OUTPUT_PATH)

rebuild: rmdir build
