#define NOMINMAX
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <sstream>

#include "Game.h"

// Global Game Object (Gia na to vlepoun ta callbacks)
Game SpaceShooter(850, 850);

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_1 && action == GLFW_PRESS) glfwSetWindowShouldClose(window, true);

    if (key >= 0 && key < 1024) {
        if (action == GLFW_PRESS) SpaceShooter.SetKeys(key, true);
        else if (action == GLFW_RELEASE) SpaceShooter.SetKeys(key, false);
    }

    // Events that happen once (Press)
    if (action == GLFW_PRESS) {
        SpaceShooter.ProcessEvents(key);
    }
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

int main() {
    srand(static_cast<unsigned>(time(0)));

    if (!glfwInit()) return -1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(850, 850, "Space Shooter Ultimate", NULL, NULL);
    if (!window) { glfwTerminate(); return -1; }
    glfwMakeContextCurrent(window);

    glfwSetKeyCallback(window, key_callback);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    glewExperimental = true;
    glewInit();

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_DEPTH_TEST);

    // --- GAME INITIALIZATION ---
    SpaceShooter.Init();

    float deltaTime = 0.0f;
    float lastFrame = 0.0f;

    // --- GAME LOOP ---
    while (!glfwWindowShouldClose(window)) {
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // 1. Handle Input
        glfwPollEvents();
        SpaceShooter.ProcessInput(deltaTime);

        // 2. Update Game State
        SpaceShooter.Update(deltaTime);

        // 3. Render
        if (SpaceShooter.State == GAME_OVER) glClearColor(0.3f, 0.0f, 0.0f, 1.0f);
        else if (SpaceShooter.State == GAME_PAUSED) glClearColor(0.1f, 0.1f, 0.2f, 1.0f);
        else glClearColor(0.1f, 0.1f, 0.15f, 1.0f);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        SpaceShooter.Render();

        glfwSwapBuffers(window);

        // Update Title with Score
        std::stringstream ss;
        ss << "Space Shooter | LVL: " << SpaceShooter.Level
            << " | Score: " << SpaceShooter.Score
            << " | High Score: " << SpaceShooter.HighScore;
        glfwSetWindowTitle(window, ss.str().c_str());
    }

    glfwTerminate();
    return 0;
}