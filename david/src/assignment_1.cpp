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
    bool buttonPressed[12] = {false, false, false, false, false, false, false, false, false, false, false};
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
    if (key == GLFW_KEY_W) {
        sInput.buttonPressed[0] = (action == GLFW_PRESS || action == GLFW_REPEAT);
    }
    if (key == GLFW_KEY_S) {
        sInput.buttonPressed[1] = (action == GLFW_PRESS || action == GLFW_REPEAT);
    }

    if (key == GLFW_KEY_A) {
        sInput.buttonPressed[2] = (action == GLFW_PRESS || action == GLFW_REPEAT);
    }
    if (key == GLFW_KEY_D) {
        sInput.buttonPressed[3] = (action == GLFW_PRESS || action == GLFW_REPEAT);
    }
    if (key == GLFW_KEY_Q) {
        sInput.buttonPressed[4] = (action == GLFW_PRESS || action == GLFW_REPEAT);
    }
    if (key == GLFW_KEY_E) {
        sInput.buttonPressed[5] = (action == GLFW_PRESS || action == GLFW_REPEAT);
    }
    if (key == GLFW_KEY_LEFT_SHIFT) {
        sInput.buttonPressed[6] = (action == GLFW_PRESS || action == GLFW_REPEAT);
    }
    if (key == GLFW_KEY_SPACE) {
        sInput.buttonPressed[7] = (action == GLFW_PRESS || action == GLFW_REPEAT);
    }
    if (key == GLFW_KEY_W) {
        sInput.buttonPressed[8] = (action == GLFW_PRESS);
    }
    if (key == GLFW_KEY_A){
        sInput.buttonPressed[9] = (action == GLFW_PRESS);
    }
    if (key == GLFW_KEY_D){
        sInput.buttonPressed[10] = (action == GLFW_PRESS);
    }
    if (key == GLFW_KEY_S){
        sInput.buttonPressed[11] = (action == GLFW_PRESS);
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
    /* if 'w' or 's' pressed, cube should rotate around x axis */
    int forwardBackward = 0;
    float rotationDirZ = 0;
    if (sInput.buttonPressed[0]) {
        forwardBackward = -1;
      //  rotationDirZ = 1;
    } else if (sInput.buttonPressed[1]) {
        forwardBackward = 1;
    }

    if(sInput.buttonPressed[8]){
        rotationDirZ = 0.01;
    } else if (sInput.buttonPressed[10]){
        rotationDirZ = -0.01;
    }

    float rotationDirX = 0;
    if(sInput.buttonPressed[9]){
        rotationDirX = 0.01;
    } else if (sInput.buttonPressed[11]){
        rotationDirX = -0.01;
    }

/*    if(sInput.buttonPressed[11]){
        rotationDirX = -0.01;
    } else {
        rotationDirX = 0;
    }*/

    /* if 'a' or 'd' pressed, cube should rotate around y axis */
    int leftRight = 0;
    if (sInput.buttonPressed[2]) {
        leftRight = 1;
    } else if (sInput.buttonPressed[3]) {
        leftRight = -1;
    }

    int rotationDirY = 0;
    if (sInput.buttonPressed[4]) {
        rotationDirY = -1;
    } else if (sInput.buttonPressed[5]) {
        rotationDirY = 1;
    }

    int upDown = 0;
    if (sInput.buttonPressed[6]) {
        upDown = -1;
    } else if (sInput.buttonPressed[7]) {
        upDown = 1;
    }

    int i = 0;
    if (sInput.buttonReleased[0]){
        i = 0;
    }

    /* udpate cube transformation matrix to include new rotation if one of the keys was pressed */
    if (rotationDirY != 0) {
        sScene.cubeTransformationMatrix =
                Matrix4D::rotationY(rotationDirY * sScene.cubeSpinRadPerSecond * elapsedTime) *
                //           Matrix4D::rotationX(rotationDirX * sScene.cubeSpinRadPerSecond * elapsedTime) *
                sScene.cubeTransformationMatrix;
    }


  //  if (forwardBackward != 0) {
        Vector3D v = {(float) forwardBackward, 0, 0};
        sScene.cubeTransformationMatrix =
                Matrix4D::translation(v * elapsedTime) *
                Matrix4D::rotationZ(rotationDirZ) *
                sScene.cubeTransformationMatrix;
   // }

   // if (leftRight != 0) {
        v = {0, 0, (float) leftRight};
        sScene.cubeTransformationMatrix =
                Matrix4D::translation(v * elapsedTime) *
                Matrix4D::rotationX(rotationDirX) *
                sScene.cubeTransformationMatrix;
  //  }

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
