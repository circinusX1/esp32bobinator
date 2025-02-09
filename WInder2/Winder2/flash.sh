#!/bin/bash
pkill screen

make upload

screen /dev/ttyUSB0  115200

pkill screen
