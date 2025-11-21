# 🚀 Space Shooter Ultimate [IN PROGRESS]

A high-performance, arcade-style vertical shooter built from scratch using **C++** and **OpenGL**.
Featuring infinite progression, dynamic difficulty scaling, and intense boss battles.

![Screenshot](https://via.placeholder.com/800x400?text=Gameplay+Screenshot+Placeholder)
*(Add your gameplay screenshot here)*

## ✨ Features

*   **Custom Game Engine:** Built with a modular architecture (Managers, Clean State Management).
*   **Dynamic Difficulty:** Enemies get faster, tougher, and smarter as you level up.
*   **Boss Battles:**
    *   **Level 5 - The Crusher:** A massive mine-layer boss with dive attacks and minion spawning.
    *   **Level 10 - The Dreadnought:** A heavy battleship with targeted lasers and bullet-hell fan attacks.
*   **Power-Up System:**
    *   ❤️ **Extra Life:** Stay in the fight longer.
    *   ⚡ **Triple Shot:**Spread your fire to control the crowd.
    *   🛡️ **Shield:** Temporary invulnerability.
*   **Visual FX:** Particle system for explosions, hit-flash shaders, and smooth parallax scrolling.
*   **Audio:** Complete sound effects and looping background music management.

## 🎮 Controls

| Key | Action |
| :--- | :--- |
| **J / L** | Move Spaceship |
| **SPACE** | Shoot |
| **P** | Pause Game |
| **R** | Restart (on Game Over) |
| **ESC** | Exit |

## 🛠️ Tech Stack

*   **Language:** C++
*   **Graphics:** OpenGL 3.3 (Core Profile)
*   **Libraries:**
    *   `GLFW` (Windowing & Input)
    *   `GLEW` (Extension Loading)
    *   `GLM` (Mathematics)
    *   `stb_image` (Texture Loading)
    *   `miniaudio` (Audio Engine)

## 🏗️ Architecture

The project follows a clean, object-oriented design pattern:
*   **Game Loop:** Decoupled Update and Render cycles.
*   **EnemyManager:** Handles spawning logic, pooling, and unique AI behaviors for different enemy types.
*   **Single Source of Truth:** Centralized `GameData` struct prevents state desynchronization.

## 👾 Installation

1.  Clone the repository:
    ```bash
    git clone https://github.com/YOUR_USERNAME/Space-Shooter.git
    ```
2.  Open the `.sln` file in **Visual Studio 2022**.
3.  Ensure solution platform is set to **x64**.
4.  Build and Run!

---
*Created by [Marinos Aristeidou]*
