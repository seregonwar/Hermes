@echo off
rmdir /s /q build
mkdir build
cd build
cmake ..
cmake --build .
cmake --install . --prefix ../install
cd ..

echo Build completata con successo!
echo I file sono stati installati in install