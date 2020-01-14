# hopf
🧣 A graphical program for exploring the Hopf fibration.

<p align="center">
  <img src="https://github.com/mwalczyk/hopf/blob/master/screenshots/fibration_with_ui.png" alt="screenshot" width="400" height="auto"/>
</p>

## Description

## Tested On
- Windows 10
- NVIDIA GeForce GTX 1660 Ti

NOTE: this project will only run on graphics cards that support OpenGL [Direct State Access](https://www.khronos.org/opengl/wiki/Direct_State_Access) (DSA).

## To Build
1. Clone this repo and initialize submodules: 
```shell
git submodule init
git submodule update
```
2. From the root directory, run the following commands:
```shell
mkdir build
cd build
cmake ..
```
3. Open the project file for your IDE of choice (generated above)
4. Build and run the project

## To Use
Notes about using the UI.

## To Do
- [ ] Clean up the `Mesh` class
- [ ] Research ways of generating the topology directly on the GPU (compute shaders?)
- [ ] Figure out path guided extrusion

## Credits
This project was largely inspired by and based on previous work done by [Dr. Niles Johnson](https://nilesjohnson.net/), who teaches at Ohio State University. In particular, I used his parameterization of the fibers, as outlined in his [production](https://nilesjohnson.net/hopf-production.html) notes. His animated videos of the Hopf fibration are what motivated me to delve into this topic.

### License
[Creative Commons Attribution 4.0 International License](https://creativecommons.org/licenses/by/4.0/)
