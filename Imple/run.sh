# Navigate to the build directory
cd build

# Remove any existing compiled binaries
rm -f ./master ./drone ./input ./server ./watchdog ./world

# Navigate back to the parent directory
cd ..

# Compile all programs
make all

# Navigate to the build directory
cd build

# Run the master program
./master
