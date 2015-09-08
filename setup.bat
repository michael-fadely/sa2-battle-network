@echo off
rem git submodule update --init --recursive
cd SFML
cmake -DBUILD_SHARED_LIBS:BOOL="0" -G "Visual Studio 14 2015"
cd ..