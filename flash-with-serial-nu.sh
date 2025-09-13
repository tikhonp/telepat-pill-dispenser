#!/usr/bin/env bash

idf.py erase-flash flash

parttool.py write_partition -n nvs_mfg --input devices-flash-partitions-data/${1}-nvs.bin --ignore-readonly

idf.py monitor
