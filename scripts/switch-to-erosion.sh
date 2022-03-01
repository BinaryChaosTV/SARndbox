#!/bin/sh
#
#   Script:  Switch-to-erosion.sh
#   Script to switch from snow, lava to water to erosion
#
cp ~/src/SARndbox-2.8/scripts/SurfaceAddWaterColor-Erosion.fs ~/src/SARndbox-2.8/share/SARndbox-2.8/Shaders/SurfaceAddWaterColor.fs
echo 'Switching to Erosion.'
echo "erosion" > ~/src/SARndbox-2.8/scripts/weather_file.tmp
#echo "waterAttenuation 0.0078125" > SARndbox.fifo
#echo "waterSpeed 0.85" > SARndbox.fifo
