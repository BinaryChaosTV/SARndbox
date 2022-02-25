# weather.sh
#!/bin/bash

# pick up current state
  weather=$(cat ~/src/scripts/weather_file.tmp);
 
# use case statement to set the assignment to the next cycle value
    case $weather in
     "water") sh ~/src/SARndbox-2.8/scripts/switch-to-lava.sh
        ;;
     "lava") sh ~/src/SARndbox-2.8/scripts/switch-to-snow.sh
        ;;
<<<<<<< HEAD
     "snow") sh ~/src/SARndbox-2.8/scripts/switch-to-water.sh
=======
     "snow") sh ~/src/scripts/switch-to-erosion.sh
        ;;
     "erosion") sh ~/src/scripts/switch-to-water.sh
>>>>>>> b6dd75f3afae5e475a9ce0e861446e02fb073bf4
        ;;
    esac
