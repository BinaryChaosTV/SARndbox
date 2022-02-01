#!/bin/sh
#
#   Script:  Switch-to-Ice.sh
#   Script to switch from snow or lava to Ice
#
cd ~/src/SARndbox-2.8/share/SARndbox-2.8/Shaders
echo 'Switching to Toxic Waste'
cp SurfaceAddWaterColor-Ice.fs SurfaceAddWaterColor.fs
cd ~/src/SARndbox-2.8
