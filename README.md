# Sonic Adventure 2: Battle Network
*Sonic Adventure 2: Battle Network* is a netplay mod for *Sonic Adventure 2 PC* which is still very much in early development, and *not* a Sonic/Mega Man Battle Network crossover, unfortunately. Originally a stand-alone program, SA2:BN now utilizes the [SA2 Mod Loader](https://github.com/sonicretro/sa2-mod-loader) to check for changed player and input values and synchronizes them with another instance of SA2 over the internet. It also now uses an oh so very slightly modified version of the [SFML](https://github.com/SFML/SFML) networking library (to allow seeking in sf::Packet).

## Usage
Soon. In the meantime, see the [Sonic Retro thread](http://forums.sonicretro.org/index.php?showtopic=31932).

## Compiling
####Requirements

* Visual Studio 2015
* Git installed and in `%PATH%`
* CMake installed and in `%PATH%`

####Build
1. Run `setup.bat`. This will pull required submodules (SA2 Mod Loader) and generate a Visual Studio SFML project.
2. Open `SA2 Battle Network.sln` and build.
