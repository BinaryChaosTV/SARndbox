#!/bin/bash

# Enter SARndbox directory:
cd ~/src/SARndbox-2.8

# Set weather to Rain
sh ~/src/SARndbox-2.8/scripts/switch-to-water.sh

# Run SARndbox with proper command line arguments:
./bin/SARndbox -uhm -fpv -ncl -rer 15 100 -evr -0.01  -rs 0.25 # -LoadInputGraph ~/src/scripts/SARstandard.inputgraph -cp SARndbox.fifo
