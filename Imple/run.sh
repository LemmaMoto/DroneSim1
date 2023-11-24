cd build
rm -f ./master
rm -f ./drone
rm -f ./input
rm -f ./obstacles
rm -f ./server
rm -f ./targets
rm -f ./watchdog
rm -f ./world
cd ..
make all
cd build
./master