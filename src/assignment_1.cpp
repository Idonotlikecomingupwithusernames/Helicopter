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
namespace scaledBody {
    const Vector4D color = {0.9f, 0.0f, 0.0f, 1.0f};
    const Matrix4D scale = Matrix4D::scale(2.5f, 1.1f, 0.8f);
    const Matrix4D trans = Matrix4D::translation({0.0f, 4.0f, 0.0f});
}

namespace scaledTail {
    const Vector4D color = {0.9f, 0.0f, 0.0f, 1.0f};
    const Matrix4D scale = Matrix4D::scale(2.5f, 0.3f, 0.3f);
    const Matrix4D trans = Matrix4D::translation({-5.0f, 4.8f, 0.0f});
}

namespace scaledPontoonLeft {
    const Vector4D color = {0.0f, 0.0f, 0.0f, 1.0f};
    const Matrix4D scale = Matrix4D::scale(2.8f, 0.15f, 0.2f);
    const Matrix4D trans = Matrix4D::translation({-0.1f, 2.75f, -0.9f});
}

namespace scaledPontoonRight {
    const Vector4D color = {0.0f, 0.0f, 0.0f, 1.0f};
    const Matrix4D scale = Matrix4D::scale(2.8f, 0.15f, 0.2f);
    const Matrix4D trans = Matrix4D::translation({-0.1f, 2.75f, 0.9f});
}

namespace scaledRotor {
    const Vector4D color = {0.0f, 0.0f, 0.0f, 1.0f};
    const Matrix4D scale = Matrix4D::scale(5.0f, 0.05f, 0.1f);
    const Matrix4D trans = Matrix4D::translation({0.0f, 5.15f, 0.0f});
}

namespace scaledTailRotor {
    const Vector4D color = {0.0f, 0.0f, 0.0f, 1.0f};
    const Matrix4D scale = Matrix4D::scale(0.1f, 0.8f, 0.05f);
    const Matrix4D trans = Matrix4D::translation({-7.0f, 4.8f, 0.35f});
}

typedef struct {
        Mesh mesh;
        Matrix4D scalingMatrix;
        Matrix4D translationMatrix;
        Matrix4D transformationMatrix;
    } component;

/* struct holding all necessary state variables for scene */
struct {
    /* cameras */
    Camera camera;
    Camera cameraOrigin;
    Camera cameraFollow;
    float zoomSpeedMultiplier;

    /* this is a bandaid-fix but we're tired */
    int whichCamera;

    /* plane mesh and transformation */
    Mesh planeMesh;
    Matrix4D planeModelMatrix;

    /* components are defined in struct component */
    component body;
    component tail;
    component pontoonL;
    component pontoonR;
    component rotor;
    component tailRotor;

    /* This was an attempt to streamline component processing */
    /*component heliParts[6];*/

    float helicopterSpinRadPerSecond;

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

    /* input for helicopter control */
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

    /*if (key == GLFW_KEY_W) {
        sInput.buttonReleased[0] = action == GLFW_RELEASE;
    }*/

    /* define active camera control */
    if (key == GLFW_KEY_1 && action == GLFW_PRESS) {
        sScene.camera = sScene.cameraOrigin;
        sScene.whichCamera = 0;
    }

    if (key == GLFW_KEY_2 && action == GLFW_PRESS) {
        sScene.camera = sScene.cameraFollow;
        sScene.whichCamera = 1;
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
    /* initialize cameras */
    sScene.cameraOrigin = cameraCreate(width, height, to_radians(45.0f), 0.01f, 500.0f, {10.0f, 14.0f, 10.0f},
                                 {0.0f, 0.0f, 0.0f});
    sScene.cameraFollow = cameraCreate(width, height, to_radians(45.0f), 0.01f, 500.0f, {10.0f, 14.0f, 10.0f},
                                 {0.0f, 4.0f, 0.0f});
    sScene.zoomSpeedMultiplier = 0.05f;

    sScene.camera = sScene.cameraOrigin;
    sScene.whichCamera = 0;

    /* create opengl buffers for meshes */
    sScene.planeMesh = meshCreate(quad::vertexPos, quad::indices, groundPlane::color);
    sScene.body.mesh = meshCreate(cube::vertexPos, cube::indices, scaledBody::color);
    sScene.tail.mesh = meshCreate(cube::vertexPos, cube::indices, scaledTail::color);
    sScene.pontoonL.mesh = meshCreate(cube::vertexPos, cube::indices, scaledPontoonLeft::color);
    sScene.pontoonR.mesh = meshCreate(cube::vertexPos, cube::indices, scaledPontoonRight::color);
    sScene.rotor.mesh = meshCreate(cube::vertexPos, cube::indices, scaledRotor::color);
    sScene.tailRotor.mesh = meshCreate(cube::vertexPos, cube::indices, scaledTailRotor::color);

    /* This was an attempt to streamline component processing */
    /*sScene.heliParts[0] = sScene.body;
    sScene.heliParts[1] = sScene.tail;
    sScene.heliParts[2] = sScene.pontoonL;
    sScene.heliParts[3] = sScene.pontoonR;
    sScene.heliParts[4] = sScene.rotor;
    sScene.heliParts[5] = sScene.tailRotor;*/


    /* setup transformation matrices for objects */
    sScene.planeModelMatrix = groundPlane::trans * groundPlane::scale;

    sScene.body.scalingMatrix = scaledBody::scale;
    sScene.body.translationMatrix = scaledBody::trans;
    sScene.body.transformationMatrix = Matrix4D::identity();

    sScene.tail.scalingMatrix = scaledTail::scale;
    sScene.tail.translationMatrix = scaledTail::trans;
    sScene.tail.transformationMatrix = Matrix4D::identity();

    sScene.pontoonL.scalingMatrix = scaledPontoonLeft::scale;
    sScene.pontoonL.translationMatrix = scaledPontoonLeft::trans;
    sScene.pontoonL.transformationMatrix = Matrix4D::identity();

    sScene.pontoonR.scalingMatrix = scaledPontoonRight::scale;
    sScene.pontoonR.translationMatrix = scaledPontoonRight::trans;
    sScene.pontoonR.transformationMatrix = Matrix4D::identity();

    sScene.rotor.scalingMatrix = scaledRotor::scale;
    sScene.rotor.translationMatrix = scaledRotor::trans;
    sScene.rotor.transformationMatrix = Matrix4D::identity();

    sScene.tailRotor.scalingMatrix = scaledTailRotor::scale;
    sScene.tailRotor.translationMatrix = scaledTailRotor::trans;
    sScene.tailRotor.transformationMatrix = Matrix4D::identity();

    sScene.helicopterSpinRadPerSecond = M_PI / 2.0f;

    /* load shader from file */
    sScene.shaderColor = shaderLoad("shader/default.vert", "shader/default.frag");
}

/* function to move and update objects in scene (e.g., rotate helicopter according to user input) */
void sceneUpdate(float elapsedTime) {
    
    /* if 'w' or 's' is pressed, helicopter should move along x axis */
    int forwardBackward = sInput.buttonPressed[0] - sInput.buttonPressed[1];

    /* if 'space' or 'left_shift' is pressed, helicopter should move along y axis */
    int upDown = (sInput.buttonPressed[7] - sInput.buttonPressed[6] );

    /* if 'a' or 'd' is pressed, helicopter should move along z axis */
    int leftRight = sInput.buttonPressed[3] - sInput.buttonPressed[2];

    /* if 'i' or 'k' is pressed, helicopter should rotate around x axis */
    float rotationDirX = 0.01 * (sInput.buttonPressed[8] - sInput.buttonPressed[9]);

    /* if 'q' or 'e' is pressed, helicopter should rotate around y axis */
    float rotationDirY = 0.03 * (sInput.buttonPressed[4] - sInput.buttonPressed[5]);

    /* if 'j' or 'l' is pressed, helicopter should rotate around z axis */
    float rotationDirZ = 0.01 * (sInput.buttonPressed[10] - sInput.buttonPressed[11]);

    /* update translation matrices with new positions (looping over components did not work) */
    Vector3D v = { (float) forwardBackward, (float) upDown, (float) leftRight};

    sScene.body.translationMatrix = Matrix4D::translation(v * elapsedTime) * sScene.body.translationMatrix;
    sScene.tail.translationMatrix = Matrix4D::translation(v * elapsedTime) * sScene.tail.translationMatrix;
    sScene.pontoonL.translationMatrix = Matrix4D::translation(v * elapsedTime) * sScene.pontoonL.translationMatrix;
    sScene.pontoonR.translationMatrix = Matrix4D::translation(v * elapsedTime) * sScene.pontoonR.translationMatrix;
    sScene.rotor.translationMatrix = Matrix4D::translation(v * elapsedTime) * sScene.rotor.translationMatrix;
    sScene.tailRotor.translationMatrix = Matrix4D::translation(v * elapsedTime) * sScene.tailRotor.translationMatrix;

    /* move cameraFollow with helicopter */
    sScene.cameraFollow.position += v * elapsedTime;
    sScene.cameraFollow.lookAt += v * elapsedTime;
    sScene.camera = sScene.whichCamera ? sScene.cameraFollow : sScene.cameraOrigin;

    /* udpate component transformation matrices with new rotations (missing implementation for non-body components (something to do with rotation origin)) */
    sScene.body.transformationMatrix = Matrix4D::rotationX(rotationDirX) * sScene.body.transformationMatrix;
    sScene.body.transformationMatrix = Matrix4D::rotationY(rotationDirY) * sScene.body.transformationMatrix;
    sScene.body.transformationMatrix = Matrix4D::rotationZ(rotationDirZ) * sScene.body.transformationMatrix;

    /* perpetually rotate rotors */
    sScene.rotor.transformationMatrix = Matrix4D::rotationY(2) * sScene.rotor.transformationMatrix;
    sScene.tailRotor.transformationMatrix = Matrix4D::rotationZ(1.5) * sScene.tailRotor.transformationMatrix;

    /* make helicopter return to original rotation after movement is complete */
    /*if (sInput.buttonReleased[0]){
        sScene.body.transformationMatrix = Matrix4D::identity() * sScene.body.transformationMatrix;
               // Matrix4D::rotationZ(0) * sScene.body.transformationMatrix;
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
                      sScene.body.translationMatrix * sScene.body.transformationMatrix * sScene.body.scalingMatrix);
        shaderUniform(sScene.shaderColor, "checkerboard", false);
        glBindVertexArray(sScene.body.mesh.vao);
        glDrawElements(GL_TRIANGLES, sScene.body.mesh.size_ibo, GL_UNSIGNED_INT, nullptr);
        
        shaderUniform(sScene.shaderColor, "uModel", sScene.tail.translationMatrix * sScene.tail.transformationMatrix * sScene.tail.scalingMatrix);
        glBindVertexArray(sScene.tail.mesh.vao);
        glDrawElements(GL_TRIANGLES, sScene.tail.mesh.size_ibo, GL_UNSIGNED_INT, nullptr);

        shaderUniform(sScene.shaderColor, "uModel", sScene.pontoonL.translationMatrix * sScene.pontoonL.transformationMatrix * sScene.pontoonL.scalingMatrix);
        glBindVertexArray(sScene.pontoonL.mesh.vao);
        glDrawElements(GL_TRIANGLES, sScene.pontoonL.mesh.size_ibo, GL_UNSIGNED_INT, nullptr);

        shaderUniform(sScene.shaderColor, "uModel", sScene.pontoonR.translationMatrix * sScene.pontoonR.transformationMatrix * sScene.pontoonR.scalingMatrix);
        glBindVertexArray(sScene.pontoonR.mesh.vao);
        glDrawElements(GL_TRIANGLES, sScene.pontoonR.mesh.size_ibo, GL_UNSIGNED_INT, nullptr);

        shaderUniform(sScene.shaderColor, "uModel", sScene.rotor.translationMatrix * sScene.rotor.transformationMatrix * sScene.rotor.scalingMatrix);
        glBindVertexArray(sScene.rotor.mesh.vao);
        glDrawElements(GL_TRIANGLES, sScene.rotor.mesh.size_ibo, GL_UNSIGNED_INT, nullptr);

        shaderUniform(sScene.shaderColor, "uModel", sScene.tailRotor.translationMatrix * sScene.tailRotor.transformationMatrix * sScene.tailRotor.scalingMatrix);
        glBindVertexArray(sScene.tailRotor.mesh.vao);
        glDrawElements(GL_TRIANGLES, sScene.tailRotor.mesh.size_ibo, GL_UNSIGNED_INT, nullptr);
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
    meshDelete(sScene.body.mesh);

    /* cleanup glfw/glcontext */
    windowDelete(window);

    return EXIT_SUCCESS;
}
