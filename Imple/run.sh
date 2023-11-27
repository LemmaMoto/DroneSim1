cd build
rm -f ./master ./drone ./input ./server ./watchdog ./world
cd ..
make all
cd build
./master
