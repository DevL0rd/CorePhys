rem Use this batch file to build corephys for Visual Studio
rmdir /s /q build
mkdir build
cd build
cmake ..
