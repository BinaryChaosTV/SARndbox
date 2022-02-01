#!/bin/bash

# Set weather to Rain
sh ~/src/scripts/switch-to-water.sh

# Enter SARndbox directory:
cd ~/src/SARndbox-2.8

# Run SARndbox with proper command line arguments:
./bin/SARndbox -uhm -fpv -rer 15 100 -evr -0.01  -rs 0.25 # -LoadInputGraph ~/src/scripts/SARstandard.inputgraph -cp SARndbox.fifo
