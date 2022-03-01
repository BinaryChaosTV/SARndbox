#!/bin/bash

# pick up current state
  weather=$(cat ~/src/SARndbox-2.8/scripts/weather_file.tmp);
 
# use case statement to set the assignment to the next cycle value
    case $weather in
     "water") sh ~/src/SARndbox-2.8/scripts/switch-to-lava.sh
        ;;
     "lava") sh ~/src/SARndbox-2.8/scripts/switch-to-snow.sh
        ;;
     "snow") sh ~/src/SARndbox-2.8/scripts/switch-to-erosion.sh
        ;;
     "erosion") sh ~/src/SARndbox-2.8/scripts/switch-to-water.sh
        ;;
    esac
