#include <glad/glad.h>
#include "world.h"
#include <GLFW/glfw3.h>

void OpenWindow();
void PrepareOpenGL();

GLFWwindow* window;
vec2 windowSize;

int main() {
    // Initialize GLFW
    if (!glfwInit()) {
        cout << "Could not initialize GLFW" << endl;
        return 0;
    }

    // ?
    const double TARGET_FRAME = 0.016667;                   // 1/60??60?
    const double FRAME_ALPHA = 0.25;                        // ?
    double currentFrame;
    double deltaTime;
    double lastFrame;
    double frameTime = 0.0;
    double renderAccum = 0.0;
    double smoothFrameTime = TARGET_FRAME;

    srand(time(0));

    GLuint gameModel = 1;
    cout << "Welcome to the Game!\n";
        
    OpenWindow();
    PrepareOpenGL();

    World world(window, windowSize);

    currentFrame = glfwGetTime();
    lastFrame = currentFrame;

    float gameTime = 0;

    while (!glfwWindowShouldClose(window) && !glfwGetKey(window, GLFW_KEY_ESCAPE)) {
        // ?
        currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;
        renderAccum += deltaTime;

        if (renderAccum >= TARGET_FRAME) {
            
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            renderAccum -= TARGET_FRAME;

            world.Update(deltaTime);
            if (world.IsOver())
                break;
            world.Render();
        }
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    glfwTerminate();
    cout << "----------------------------Your Score: " << world.GetScore() << " ----------------------------" << endl;
    cout << "----------------------------Remaining Health: " << world.GetPlayerHealth() << "/" << world.GetMaxPlayerHealth() << " ----------------------------" << endl;
    cout << "----------------------------Health Packs on Field: " << world.GetActiveHealthPackCount() << " ----------------------------" << endl;
    return 0;
}

void OpenWindow() {
    const char* TITLE = "Shoot Game";
    int WIDTH = 1960;
    int HEIGHT = 1080;

    // ³õÊ¼»¯GLFW
    if (!glfwInit()) {
        cout << "Could not initialize GLFW" << endl;
        return;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_REFRESH_RATE, 60);

    window = glfwCreateWindow(WIDTH, HEIGHT, TITLE, NULL, NULL);
    if (window == NULL) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return;
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return;
    }

    glGetError();

    windowSize = vec2(WIDTH, HEIGHT);
}

void PrepareOpenGL() {
    // Enable depth testing and blending
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Set background color to dark
    glClearColor(0.f, 0.f, 0.1f, 0.0f);
}