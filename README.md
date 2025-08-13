Catch Me If You Can 🎮
Catch Me If You Can is a thrilling 60-second survival game built in C++ using OpenGL, GLFW, GLEW, and FreeType. Players use WASD to move, jump, super jump, and dash to evade an AI enemy. Developed for an academic course, this project earned the highest grade (AA) for its technical excellence and innovative design.

🚀 Features

Dynamic Gameplay: Survive 60 seconds by outmaneuvering an AI enemy with smooth movement, jumps, and dashes.
Stunning Graphics: Renders 26 PNG textures (e.g., dashfoto.png, ArenaFloor.png) using OpenGL for immersive visuals.
Portable Executable: Statically linked with FreeType, GLFW, GLEW, libpng, and zlib, requiring no external DLLs or Visual C++ Redistributable.
Custom Icon: Enhanced user experience with a custom .ico embedded in the executable.
Robust Error Handling: Resolved "textures not found" issues by optimizing texture directory setup.


🛠️ Technologies Used



Category
Details



Language
C++


Graphics
OpenGL


Libraries
GLFW, GLEW, FreeType, libpng, zlib


Tools
Visual Studio 2022


Platform
Windows (x64, Release)



📥 Installation

Clone the Repository:git clone https://github.com/Dervis-Oral/CatchMe.git


Navigate to the Directory:cd CatchMe


Run the Game:
Ensure the textures/ folder (containing 26 PNG files) is in the same directory as CatchMe.exe.
Double-click CatchMe.exe or run:CatchMe.exe
Press F to fullscreen as soon as game opens because game designed for fullscreen not windowed.






Note: If you encounter a "textures not found" error, verify that the textures/ folder is in the same directory as CatchMe.exe.


🎮 Controls



Key
Action



WASD
Move the player


Space
Jump


Q
Super Jump (easy mode)


E
Dash (easy mode)


F
Toggle fullscreen


Objective: Survive for 60 seconds by evading the AI enemy!

📂 Repository Structure

main.cpp: Core game logic and OpenGL rendering code.
textures/: Folder containing 26 PNG texture files:
dashfoto.png
superjumpfoto.png
ArenaFloor.png
... (23 more PNG files)


CatchMe.exe: Statically linked executable with a custom icon.
README.md: Project documentation.


🔧 Technical Achievements

Static Compilation: Integrated FreeType, GLFW, GLEW, libpng, and zlib as static libraries, ensuring the executable runs without dependencies.
Texture Management: Implemented robust loading for 26 PNG textures, fixing "textures not found" errors via proper working directory configuration.
Custom Icon: Embedded a custom .ico into CatchMe.exe using resource files for a polished user experience.
Dependency Management: Manually resolved linker errors (e.g., LNK2001) by configuring libpng and zlib.


🏗️ How to Build

Prerequisites:
Install Visual Studio 2022 with the C++ Desktop Development workload.
Install required static libraries using vcpkg:vcpkg install freetype:x64-windows-static glfw3:x64-windows-static glew:x64-windows-static libpng:x64-windows-static zlib:x64-windows-static




Setup:
Copy static libraries (freetype.lib, glfw3.lib, glew32s.lib, libpng16.lib, zlib.lib) to your project’s bin/lib/ folder.
Copy include files to bin/include/.


Build:
Open main.cpp in Visual Studio 2022.
Configure for Release and x64.
Build the project to generate CatchMe.exe.


Run:
Place the textures/ folder in the same directory as CatchMe.exe.
Run CatchMe.exe.




⚠️ Notes

Ensure the textures/ folder is in the same directory as CatchMe.exe to avoid texture loading errors.
The executable is fully static, requiring no external DLLs.
Press F to FullScreen as soon as game starts.




📜 License
This project is for educational purposes and not licensed for commercial use.

📬 Contact

GitHub: Dervis-Oral
