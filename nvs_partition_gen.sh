#!/usr/bin/env bash

PART_SIZE=0x3000
GEN=python
NVS_PARTITION="$IDF_PATH/components/nvs_flash/nvs_partition_generator/nvs_partition_gen.py"        

WIFI_SSID="409"
WIFI_PSK="409409409"

# list of device names (or read from CSV/DB)
devices=('2X2000001' '2X2000002' '2X2000003' '2X2000004' '2X2000005' '2X2000006' '2X2000007' '2X2000020' '2X2000021' '2X2000022' '2X2000023' '2X2000024' '2X2000025' '2X2000026' '2X2000027' '2X2000028' '2X2000029' '2X2000030' '2X2000031' '2X2000032' '2X2000033' '2X2000034' '2X2000035' '2X2000036' '2X2000037' '2X2000038' '2X2000039' '2X2000040' '2X2000041' '2X2000042' '2X2000043')

for d in "${devices[@]}"; do
  csv="${d}-${WIFI_SSID}.csv"
  bin="${d}-${WIFI_SSID}-nvs.bin"

  cat > "$csv" <<EOF
key,type,encoding,value
mfg,namespace,,
DEFAULT_SSID,data,string,${WIFI_SSID}
DEFAULT_PSK,data,string,${WIFI_PSK}
SERIAL_NU,data,string,${d}
EOF

  echo "Generating $bin from $csv ..."
  $GEN $NVS_PARTITION generate "$csv" "$bin" $PART_SIZE || { echo "gen failed"; exit 1; }

  echo "Done $d"
done
