#!/bin/sh
#
#   Script:  Switch-to-lava.sh
#   Script to switch from water or snow to lava
#
cp ~/src/scripts/SurfaceAddWaterColor-Lava.fs ~/src/SARndbox-2.8/share/SARndbox-2.8/Shaders/SurfaceAddWaterColor.fs
echo "Switching to lava."
echo "lava" > ~/src/scripts/weather_file.tmp
#echo "waterAttenuation 0.99" > SARndbox.fifo
#echo "waterSpeed 0.40" > SARndbox.fifo
