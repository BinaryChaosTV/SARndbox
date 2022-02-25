#!/bin/sh
#
#   Script:  Switch-to-snow.sh
#   Script to switch from water or lava to snow
#
cp ~/src/SARndbox-2.8/scripts/SurfaceAddWaterColor-Snow.fs ~/src/SARndbox-2.8/share/SARndbox-2.8/Shaders/SurfaceAddWaterColor.fs
echo 'Switching to snow.'
echo "snow" > ~/src/SARndbox-2.8/scripts/weather_file.tmp
#echo "waterAttenuation 0.12" > SARndbox.fifo
#echo "waterSpeed 0.039" > SARndbox.fifo
