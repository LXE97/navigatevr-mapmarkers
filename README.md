## SKSE plugin source for Skyrim mod "Map Markers for NavigateVR"

### User requirements:
* SKSE
* Address Library VR


## Build instructions:
1. install [vcpkg](https://github.com/microsoft/vcpkg)
2. add vcpkg path to environment variables as VCPKG_ROOT and run /vcpkg/bootstrap-vcpkg.bat
3. install Visual Studio 2022 with "C++ Desktop Development" module
4. install VSCode
5. open this repository's root folder in VSCode and it will automatically configure itself

This project is built with commonlibsse-ng-vr, but it uses PlayerCharacter.h from [CommonLibVR](https://github.com/alandtse/CommonLibVR) because I'm in the process of migrating
