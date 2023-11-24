cd build
rm -f ./master ./drone ./input ./obstacles ./server ./targets ./watchdog ./world
cd ..
make all
cd build
./master
clear

