#!/bin/sh
#
#   Script:  Switch-to-waste.sh
#   Script to switch from snow or lava to toxic waste
#
cd ~/src/SARndbox-2.8/share/SARndbox-2.8/Shaders
echo 'Switching to Toxic Waste'
cp SurfaceAddWaterColor-Waste.fs SurfaceAddWaterColor.fs
cd ~/src/SARndbox-2.8
