@echo off
git submodule update --init --recursive
cd SFML
cmake -G "Visual Studio 14 2015"
cd ..