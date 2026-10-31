SDK_PATH ?= $(shell ls -d $(HOME)/.pico-sdk/sdk/* 2>/dev/null | tail -n 1)
BUILD_DIR := build
TARGET := pico-w-mqtt-template

.PHONY: all build clean flash monitor broker-up broker-down run-backend

all: build

build:
	cmake -B $(BUILD_DIR) -DPICO_SDK_PATH=$(SDK_PATH) -DPICO_BOARD=pico_w
	cmake --build $(BUILD_DIR) -j$$(nproc)

clean:
	rm -rf $(BUILD_DIR)

flash: build
	cp $(BUILD_DIR)/$(TARGET).uf2 /run/media/$(USER)/RPI-RP2/

monitor:
	minicom -b 115200 -o -D /dev/ttyACM0

broker-up:
	cd backend && docker compose up -d

broker-down:
	cd backend && docker compose down

run-backend:
	cd backend && .venv/bin/python3 main.py