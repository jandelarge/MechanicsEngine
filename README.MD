# Mechanics Engine

![C++](https://img.shields.io/badge/C++-17-blue.svg?style=flat&logo=c%2B%2B)
![Raylib](https://img.shields.io/badge/Raylib-5.5-red.svg?style=flat)
![CMake](https://img.shields.io/badge/CMake-Build-success.svg?style=flat&logo=cmake)

**An educational physics sandbox for modeling mechanical processes.** Designed to help students understand mechanics by providing an interactive, real-time visualization of objects and forces (gravity, tension, normal reaction, and friction).

The simulator allows you to seamlessly build physical setups using blocks, supports, and ropes, and directly observe how forces interact dynamically.

>

---

## Key Features

* **Interactive Sandbox:** Spawn static supports, dynamic blocks, and ropes on the fly.
* **Direct Parameter Control:** A built-in UI inspector to edit mass, friction coefficient, dimensions, rope length, and stiffness via direct keyboard input.
* **Vector Telemetry:** Real-time visualization of physical forces (mg, N, P, T, F_frict) directly on the objects.
* **Custom Physics Engine:** Built entirely from scratch. No third-party physics libraries (like Box2D) are used.

## Physics Under the Hood

The engine is built on modern computational mechanics methods:
* **Sequential Impulses:** Accurate weight transfer in block stacks and true Coulomb dry friction calculation.
* **OBB (Oriented Bounding Box) + SAT:** Precise collision detection for rotated objects, including lever arm and moment of inertia calculations (blocks can realistically tip over edges).
* **XPBD (Extended Position Based Dynamics):** Elastic and absolutely rigid ropes that obey Hooke's law and accurately account for centripetal acceleration.
* **Elastic Mouse Grab:** Objects are moved via a virtual spring rather than teleportation, preserving the laws of conservation of energy and momentum.

## Controls

The interface features a Liquid Glass style UI dock with vector icons.
* `[Space]` — Start / Pause simulation.
* `[V]` — Toggle force vector display mode (All / On Hover / Hidden).
* `[R]` — Reset the scene.
* `[LMB] on empty space` — Use the selected tool (Cursor, Block, Support, Rope, Delete).
* `[LMB] on object` — Select an object to reveal and edit its properties in the right-side Inspector.

## Build & Run

The project is written in **C++17** using **Raylib** for rendering and **CMake** for the build system. External dependencies are fetched automatically.

### Requirements
* A C++17 compatible compiler (GCC/MinGW, MSVC, Clang).
* CMake 3.15 or higher.

### Build Steps (Terminal / CLion)
1. Clone the repository:
   ```bash
   git clone [https://github.com/YOUR_USERNAME/MechanicsEngine.git](https://github.com/YOUR_USERNAME/MechanicsEngine.git)
   cd MechanicsEngine