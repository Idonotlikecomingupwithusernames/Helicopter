#include <cstdlib>
#include <iostream>

#include "mygl/shader.h"
#include "mygl/mesh.h"
#include "mygl/geometry.h"
#include "mygl/camera.h"

/* translation, scale and color for the ground plane */
namespace groundPlane {
    const Vector4D color = {0.9f, 0.9f, 0.9f, 1.0f};
    const Matrix4D scale = Matrix4D::scale(20.0f, 0.0f, 20.0f);
    const Matrix4D trans = Matrix4D::identity();
}

/* translation, scale and color for the scaled cube */
namespace scaledCube {
    const Vector4D color = {0.9f, 0.0f, 0.0f, 1.0f};
    const Matrix4D scale = Matrix4D::scale(5.0f, 2.2f, 1.6f);
    const Matrix4D trans = Matrix4D::translation({0.0f, 4.0f, 0.0f});
}

namespace scaledTail {
    const Vector4D color = {0.9f, 0.0f, 0.0f, 1.0f};
    const Matrix4D scale = Matrix4D::scale(5.0f, 0.6f, 0.6f);
    const Matrix4D trans = Matrix4D::translation({0.0f, 4.0f, 0.0f});
}

/* struct holding all necessary state variables for scene */
struct {
    /* camera */
    Camera camera;
    float zoomSpeedMultiplier;

    /* plane mesh and transformation */
    Mesh planeMesh;
    Matrix4D planeModelMatrix;

    /* cube mesh and transformations */
    Mesh cubeMesh;
    Matrix4D cubeScalingMatrix;
    Matrix4D cubeTranslationMatrix;
    Matrix4D cubeTransformationMatrix;
    float cubeSpinRadPerSecond;

    Mesh tailMesh;
    Matrix4D tailScalingMatrix;
    Matrix4D tailTranslationMatrix;
    Matrix4D tailTransformationMatrix;

    /* shader */
    ShaderProgram shaderColor;
} sScene;

/* struct holding all state variables for input */
struct {
    bool mouseLeftButtonPressed = false;
    Vector2D mousePressStart;
    int buttonMap[12] = {GLFW_KEY_W, GLFW_KEY_S, GLFW_KEY_A, GLFW_KEY_D, GLFW_KEY_Q, GLFW_KEY_E, GLFW_KEY_LEFT_SHIFT, GLFW_KEY_SPACE, GLFW_KEY_I, GLFW_KEY_K, GLFW_KEY_J, GLFW_KEY_L};
    bool buttonPressed[12] = {false, false, false, false, false, false, false, false, false, false, false, false};
    bool buttonReleased[1] = {false};
} sInput;

/* GLFW callback function for keyboard events */
void keyCallback(GLFWwindow *window, int key, int scancode, int action, int mods) {
    /* called on keyboard event */

    /* close window on escape */
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
    }

    /* make screenshot and save in work directory */
    if (key == GLFW_KEY_P && action == GLFW_PRESS) {
        screenshotToPNG("screenshot.png");
    }

    /* input for cube control */
    for(int i = 0; i < 12; i++){
        if(i<8){
            if (key == sInput.buttonMap[i]) {
                sInput.buttonPressed[i] = (action == GLFW_PRESS || action == GLFW_REPEAT);
            }
        }
        else {
            if (key == sInput.buttonMap[i]){
                sInput.buttonPressed[i] = (action == GLFW_PRESS);
            }
        }
    }

    if (key == GLFW_KEY_W) {
        sInput.buttonReleased[0] = action == GLFW_RELEASE;
    }
}

/* GLFW callback function for mouse position events */
void mousePosCallback(GLFWwindow *window, double x, double y) {
    /* called on cursor position change */
    if (sInput.mouseLeftButtonPressed) {
        Vector2D diff = sInput.mousePressStart - Vector2D(x, y);
        cameraUpdateOrbit(sScene.camera, diff, 0.0f);
        sInput.mousePressStart = Vector2D(x, y);
    }
}

/* GLFW callback function for mouse button events */
void mouseButtonCallback(GLFWwindow *window, int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        sInput.mouseLeftButtonPressed = (action == GLFW_PRESS || action == GLFW_REPEAT);

        double x, y;
        glfwGetCursorPos(window, &x, &y);
        sInput.mousePressStart = Vector2D(x, y);
    }
}

/* GLFW callback function for mouse scroll events */
void mouseScrollCallback(GLFWwindow *window, double xoffset, double yoffset) {
    cameraUpdateOrbit(sScene.camera, {0, 0}, sScene.zoomSpeedMultiplier * yoffset);
}

/* GLFW callback function for window resize event */
void windowResizeCallback(GLFWwindow *window, int width, int height) {
    glViewport(0, 0, width, height);
    sScene.camera.width = width;
    sScene.camera.height = height;
}

/* function to setup and initialize the whole scene */
void sceneInit(float width, float height) {
    /* initialize camera */
    sScene.camera = cameraCreate(width, height, to_radians(45.0f), 0.01f, 500.0f, {10.0f, 14.0f, 10.0f},
                                 {0.0f, 4.0f, 0.0f});
    sScene.zoomSpeedMultiplier = 0.05f;

    /* create opengl buffers for mesh */
    sScene.planeMesh = meshCreate(quad::vertexPos, quad::indices, groundPlane::color);
    sScene.cubeMesh = meshCreate(cube::vertexPos, cube::indices, scaledCube::color);
    sScene.tailMesh = meshCreate(tail::vertexPos, tail::indices, scaledCube::color);

    /* setup transformation matrices for objects */
    sScene.planeModelMatrix = groundPlane::trans * groundPlane::scale;

    sScene.cubeScalingMatrix = scaledCube::scale;
    sScene.cubeTranslationMatrix = scaledCube::trans;
    sScene.cubeTransformationMatrix = Matrix4D::identity();

    sScene.tailScalingMatrix = scaledTail::scale;
    sScene.tailTranslationMatrix = scaledTail::trans;
    sScene.tailTransformationMatrix = Matrix4D::identity();


    sScene.cubeSpinRadPerSecond = M_PI / 2.0f;

    /* load shader from file */
    sScene.shaderColor = shaderLoad("shader/default.vert", "shader/default.frag");
}

/* function to move and update objects in scene (e.g., rotate cube according to user input) */
void sceneUpdate(float elapsedTime) {
    int forwardBackward = sInput.buttonPressed[0] - sInput.buttonPressed[1];

    int upDown = (sInput.buttonPressed[6] - sInput.buttonPressed[7]);

    int leftRight = sInput.buttonPressed[2] - sInput.buttonPressed[3];

    /* if 'w' or 's' pressed, cube should rotate around x axis */
    float rotationDirX = 0.01 * (sInput.buttonPressed[8] - sInput.buttonPressed[9]);

    /* if 'a' or 'd' pressed, cube should rotate around y axis */
    int rotationDirY = (sInput.buttonPressed[4] - sInput.buttonPressed[5]);

    float rotationDirZ = 0.01 * (sInput.buttonPressed[10] - sInput.buttonPressed[11]);

    /* udpate cube transformation matrix to include new rotation if one of the keys was pressed */
    if (rotationDirY != 0) {
        sScene.cubeTransformationMatrix =
                Matrix4D::rotationY(rotationDirY * sScene.cubeSpinRadPerSecond * elapsedTime) *
                //           Matrix4D::rotationX(rotationDirX * sScene.cubeSpinRadPerSecond * elapsedTime) *
                sScene.cubeTransformationMatrix;
    }


    if (forwardBackward != 0) {
        Vector3D v = {(float) forwardBackward, 0, 0};
        if(rotationDirZ<0){
            printf("Negative \n");
        }
        /*if(rotationDirZ>0){
            printf("Positive \n");
        }*/
        sScene.cubeTransformationMatrix =
                Matrix4D::translation(v * elapsedTime) *
                Matrix4D::rotationZ(rotationDirZ) *
                sScene.cubeTransformationMatrix;
    }

    if (leftRight != 0) {
        Vector3D v = {0, 0, (float) leftRight};
        sScene.cubeTransformationMatrix =
                Matrix4D::translation(v * elapsedTime) *
                Matrix4D::rotationX(rotationDirX) *
                sScene.cubeTransformationMatrix;
    }

    if (upDown != 0){
        Vector3D v = { 0, (float) upDown, 0};
        sScene.cubeTransformationMatrix =
                Matrix4D::translation(v * elapsedTime) *
                sScene.cubeTransformationMatrix;
    }

    if (sInput.buttonReleased[0]){
        sScene.cubeTransformationMatrix = Matrix4D::identity() * sScene.cubeTransformationMatrix;
               // Matrix4D::rotationZ(0) * sScene.cubeTransformationMatrix;
    }

    /*   if(leftRight != 0){
           sScene.cubeTransformationMatrix =
                   Matrix4D::
       }*/
}

/* function to draw all objects in the scene */
void sceneDraw() {
    /* clear framebuffer color */
    glClearColor(135.0 / 255, 206.0 / 255, 235.0 / 255, 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    /*------------ render scene -------------*/
    /* use shader and set the uniforms (names match the ones in the shader) */
    {
        glUseProgram(sScene.shaderColor.id);
        shaderUniform(sScene.shaderColor, "uProj", cameraProjection(sScene.camera));
        shaderUniform(sScene.shaderColor, "uView", cameraView(sScene.camera));

        /* draw ground plane */
        shaderUniform(sScene.shaderColor, "uModel", sScene.planeModelMatrix);
        shaderUniform(sScene.shaderColor, "checkerboard", true);
        glBindVertexArray(sScene.planeMesh.vao);
        glDrawElements(GL_TRIANGLES, sScene.planeMesh.size_ibo, GL_UNSIGNED_INT, nullptr);

        /* draw cube, requires to calculate the final model matrix from all transformations */
        shaderUniform(sScene.shaderColor, "uModel",
                      sScene.cubeTranslationMatrix * sScene.cubeTransformationMatrix * sScene.cubeScalingMatrix);
   //     shaderUniform(sScene.shaderColor, "uModel",
   //                   sScene.tailTranslationMatrix * sScene.tailTransformationMatrix * sScene.tailScalingMatrix);
        shaderUniform(sScene.shaderColor, "checkerboard", false);
        glBindVertexArray(sScene.cubeMesh.vao);
        glDrawElements(GL_TRIANGLES, sScene.cubeMesh.size_ibo, GL_UNSIGNED_INT, nullptr);
    }

    /* cleanup opengl state */
    glBindVertexArray(0);
    glUseProgram(0);
}

int main(int argc, char **argv) {
    /* create window/context */
    int width = 1280;
    int height = 720;
    GLFWwindow *window = windowCreate("Assignment 1 - Transformations, User Input and Camera", width, height);
    if (!window) { return EXIT_FAILURE; }

    /* set window callbacks */
    glfwSetKeyCallback(window, keyCallback);
    glfwSetCursorPosCallback(window, mousePosCallback);
    glfwSetMouseButtonCallback(window, mouseButtonCallback);
    glfwSetScrollCallback(window, mouseScrollCallback);
    glfwSetFramebufferSizeCallback(window, windowResizeCallback);


    /*---------- init opengl stuff ------------*/
    glEnable(GL_DEPTH_TEST);


    /* setup scene */
    sceneInit(width, height);

    /*-------------- main loop ----------------*/
    double timeStamp = glfwGetTime();
    double timeStampNew = 0.0;

    /* loop until user closes window */
    while (!glfwWindowShouldClose(window)) {
        /* poll and process input and window events */
        glfwPollEvents();

        /* update model matrix of cube */
        timeStampNew = glfwGetTime();
        sceneUpdate(timeStampNew - timeStamp);
        timeStamp = timeStampNew;

        /* draw all objects in the scene */
        sceneDraw();

        /* swap front and back buffer */
        glfwSwapBuffers(window);
    }


    /*-------- cleanup --------*/
    /* delete opengl shader and buffers */
    shaderDelete(sScene.shaderColor);
    meshDelete(sScene.planeMesh);
    meshDelete(sScene.cubeMesh);

    /* cleanup glfw/glcontext */
    windowDelete(window);

    return EXIT_SUCCESS;
}
