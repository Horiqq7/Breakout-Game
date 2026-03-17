# Retro Breakout Arena

A modern **C++** implementation of the classic arcade game *Breakout*, built using an **OpenGL**-based 2D graphics framework. 

This project demonstrates real-time game loop management, rendering pipelines, and applied mathematics for physics and collision detection.

## 🚀 Core Features

* **Advanced Collision Detection**: Implements robust Circle-AABB (Axis-Aligned Bounding Box) collision algorithms for highly accurate ball-to-paddle and ball-to-brick interactions.
* **Procedural Level Generation**: Dynamically generates brick layouts with varying resistance levels (e.g., specific bricks require multiple hits to be destroyed).
* **Power-up System & VFX**: Features collectible power-ups that drop upon breaking specific bricks, altering gameplay dynamics. Includes visual effects for object destruction.
* **Fluid Input Handling**: Smooth, continuous paddle movement controlled via keyboard input, avoiding OS-level key-repeat delays.

## 📁 Technical Stack & Structure

* **Language**: C++
* **Graphics API**: OpenGL
* **Architecture**: 
  * `object2D.cpp/h`: Custom definitions for generating 2D meshes (circles, rectangles).
  * `transform2D.h`: Custom mathematical matrices for 2D transformations (translation, rotation, scaling).
  * `Tema1.cpp/h`: The core game engine containing the loop, state machine, and physics updates.

## 💻 Build and Run

To compile and run the game locally, you need a C++ compiler, **CMake**, and an OpenGL-compatible environment.

```bash
# Example build steps (assuming CMake and dependencies are installed)
mkdir build && cd build
cmake ..
make
# Run the executable generated in the build/bin folder