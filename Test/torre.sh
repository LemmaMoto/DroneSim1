#!/bin/bash

gcc drone.c -o drone -lmenu -lncurses

gcc telecomando.c -o telecomando -lmenu -lncurses


konsole  -e ./telecomando &
sleep 2
konsole  -e ./drone &
