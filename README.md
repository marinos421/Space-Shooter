# Space Shooter Arcade

A fast–paced 2D space shooter written in C++ with OpenGL.  
The player controls a spaceship, dodges enemy space mines, collects power–ups and tries to reach the highest possible score.  
Every few stages a **boss battle** appears, with unique patterns and difficulties.

---

## 🎮 Gameplay Overview

- Move the ship horizontally using dedicated arcade-style movement keys.
- Shoot enemies, collect power-ups, level up, survive waves and defeat bosses.
- Every **500 score** the level increases.
- Every **5 stages**, a **boss event** appears.
- The difficulty increases dynamically with enemy speed, HP and spawn rate.

---

## ✨ Features

### 🚀 Core Mechanics
- Player movement (left / right) and continuous shooting.
- Multiple enemy types: basic mines, zig–zag mines, splitter mines
- Stage & level system with full difficulty scaling.

### 👾 Boss System (currently 2 bosses)
- **Boss Mine** — large armored mine with multiple phases and minion spawns.
- **Boss Ship** — aggressive enemy ship with zig-zag movement and projectile attacks.
- Reward drops: score boosts and in the future guaranteed power-ups.

### 🔮 Power–Ups
- 🧡 **Extra Life**  
- 🔱 **Triple Shot**  
- 🛡 **Shield**  
- 🎯 **Piercing Shot** (bullets penetrate enemies)

### 🧮 Scoring
- Base points per enemy type.
- Combo & multiplier system that rewards no-hit kill streaks.
- Stage clear and boss kill bonuses.

### 🎨 Visuals & Audio
- Transparent PNG textures for player, enemies, bosses & power-ups.
- Menu screens (main menu, pause, game over).
- Basic SFX for shots & explosions (optional based on build).

---

## 🕹 Controls

| Action            | Key |
|-------------------|-----|
| Move Left         | **L** |
| Move Right        | **J** |
| Fire              | **SPACE** |
| Pause             | **P** |
| Start Game        | **ENTER** |
| Quit Game         | **ESC** |


---

## 📂 Project Structure

- `Main.cpp` – Window creation, OpenGL context, main loop.  
- `Game.h / Game.cpp` – Gameplay logic (player, bullets, score, stages, rendering).  
- `EnemyManager.h / EnemyManager.cpp` – Enemy & boss spawning, AI, minion logic.  
- `PowerUp.h` – Power-up definitions & timers.  
- `Commons.h` – Shared structs, constants & enums.  
- `Basic.vert / Basic.frag` – GLSL shaders for textured quads.  
- `textures/` – Player, mine, splitter, boss, power-up and UI graphics.

---

## 🔧 How to Build

1. Open project in **Visual Studio (Windows)**.  
2. Ensure OpenGL / GLEW / GLFW (or equivalent) are installed and linked.  
3. Build in `Release x64` for best frame-rate.  
4. Run the executable — main menu will display “Press ENTER to start”.

---

## 📌 Future Additions

- Third boss (Mega Ship or Mechanical Eye)
- More advanced bullet patterns (spirals, waves)
- New enemy types (shielded, teleporting, charge mines)
- Shop
- Player Upgrades
- Infinite mode / Hard mode
- Performance settings
- Online or local high-score table
- Dynamic background animations

---

## 🧑‍💻 Credits

- **Code & Design:** Marinos Aristeidou & Team  
- **Graphics:** Custom-made arcade sprites  
- **Tech:** C++ / OpenGL rendering pipeline

---

**Marinos Aristeidou**  
🎓 Computer Engineering Student — University of Ioannina  

📧 Email: [marinosapo54@gmail.com]  
🔗 LinkedIn: [https://www.linkedin.com/in/marinos-aristeidou-786526300/]  
🌐 Portfolio: [github.com/marinos421]

---

Enjoy blasting through the galaxy 🚀🔥
