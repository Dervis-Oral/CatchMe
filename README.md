Catch Me If You Can
A 60-second survival game developed in C++ using OpenGL, GLFW, GLEW, and FreeType. Players navigate with WASD, jump (Space), super jump (Q, easy mode), and dash (E, easy mode) to evade an AI enemy. The project was completed for an academic course, earning the highest grade (AA).
Features

Gameplay: Survive 60 seconds by avoiding an AI enemy using movement, jumps, and dashes.
Graphics: Renders 26 PNG textures (e.g., dashfoto.png, ArenaFloor.png) via OpenGL.
Static Linking: Uses FreeType, GLFW, GLEW, libpng, and zlib statically, requiring no external DLLs or Visual C++ Redistributable.
Custom Icon: Includes a custom .ico for the executable.
Portability: Resolved "textures not found" issues by ensuring proper directory setup.

Technologies

Language: C++
Graphics: OpenGL
Libraries: GLFW, GLEW, FreeType, libpng, zlib
Tools: Visual Studio 2022
Platform: Windows (x64, Release)

Installation

Clone the repository:git clone https://github.com/Dervis-Oral/CatchMe.git


Navigate to the directory:cd CatchMe


Run the game:
Ensure the textures/ folder (with 26 PNG files) is in the same directory as CatchMe.exe.
Double-click CatchMe.exe or run:CatchMe.exe
Press F to full screen as soon as game opens because game designed for full screen not windowed.





Controls

WASD: Move
Space: Jump
Q: Super Jump (easy mode)
E: Dash (easy mode)
F: Toggle fullscreen
Objective: Survive 60 seconds against an AI enemy

Repository Structure
CatchMe/
├── main.cpp
├── textures/
│   ├── dashfoto.png
│   ├── superjumpfoto.png
│   ├── ArenaFloor.png
│   └── ... (26 PNG files)
├── CatchMe.exe
├── README.md

Notes

The textures/ folder must be in the same directory as CatchMe.exe to avoid "textures not found" errors.
The executable is fully static, requiring no external dependencies.

License
This project is for educational purposes and not licensed for commercial use.
