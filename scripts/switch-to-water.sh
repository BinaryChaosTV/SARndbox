#!/bin/sh
#
#   Script:  Switch-to-water.sh
#   Script to switch from snow or lava to water
#
cp ~/src/SARndbox-2.8/scripts/SurfaceAddWaterColor-Water.fs ~/src/SARndbox-2.8/share/SARndbox-2.8/Shaders/SurfaceAddWaterColor.fs
echo 'Switching to water.'
echo "water" > ~/src/SARndbox-2.8/scripts/weather_file.tmp
#echo "waterAttenuation 0.0078125" > SARndbox.fifo
#echo "waterSpeed 0.85" > SARndbox.fifo
