#if defined (__APPLE__)
    #define GLFW_INCLUDE_GLCOREARB
    #define GL_SILENCE_DEPRECATION
#else
    #define GLEW_STATIC
    #include <GL/glew.h>
#endif

#include <GLFW/glfw3.h>

#include <glm/glm.hpp> //core glm functionality
#include <glm/gtc/matrix_transform.hpp> //glm extension for generating common transformation matrices
#include <glm/gtc/matrix_inverse.hpp> //glm extension for computing inverse matrices
#include <glm/gtc/type_ptr.hpp> //glm extension for accessing the internal data structure of glm types

#include "Window.h"
#include "Shader.hpp"
#include "Camera.hpp"
#include "Model3D.hpp"
#include "SkyBox.hpp"

#include <iostream>

const unsigned int SHADOW_WIDTH = 8192;
const unsigned int SHADOW_HEIGHT = 8192;
const unsigned int RAIN_DROPS = 17000;

// window
gps::Window myWindow;
int retina_width, retina_height;
//GLFWwindow* glWindow = NULL;

// matrices
glm::mat4 model;
glm::mat4 boatModel;
glm::mat4 view;
glm::mat4 projection;
glm::mat3 normalMatrix;
glm::mat4 lightRotation;

// light parameters
glm::vec3 lightDir;
glm::vec3 lightColor;

glm::vec3 lightPos;
glm::vec3 lightPos1;


// shader uniform locations
GLint modelLoc;
GLint viewLoc;
GLint projectionLoc;
GLint normalMatrixLoc;
GLint lightDirLoc;
GLint lightColorLoc;
GLint ambientStrengthLoc;
GLuint lightPosLoc;
GLuint lightPosLoc1;

// camera
gps::Camera myCamera(
    glm::vec3(0.0f, 0.0f, 3.0f),
    glm::vec3(0.0f, 0.0f, -10.0f),
    glm::vec3(0.0f, 1.0f, 0.0f));

GLfloat cameraSpeed = 0.1f;

GLboolean pressedKeys[1024];

// models
gps::Model3D teapot;
gps::Model3D scene;
gps::Model3D boat;
gps::Model3D screenQuad;
gps::Model3D Rain;
GLfloat angle = -90.0f;

// shaders
gps::Shader myBasicShader;
gps::Shader depthMapShader;
gps::Shader screenQuadShader;

GLuint shadowMapFBO;
GLuint depthMapTexture;
bool showDepthMap;

//SkyBox
gps::SkyBox mySkyBox;
gps::SkyBox mySkyBoxNight;
gps::Shader skyboxShader;

//fog
GLfloat fogDensity;
GLuint fogDensityLoc;

//light
GLfloat lightOn = 1.0f;
GLuint lightOnLoc;

float lastX = 400, lastY = 300;
float pitch = 0.0f, yaw = -90.0f;
float cameraSensit = 0.05;
bool firstMouse = true;
float fov = 45.0f;
bool polygon = false;

float boatAngleY = 0;
float boatAngleZ = 0;
float boatAngleIncrement = 0.0001;

bool enabledCameraAnimation = true;
int camAnim = 1;
int camTime = 0;

std::vector<glm::vec3> raindrops;

bool rainOn;

float lightAngle = 0.0f;





GLenum glCheckError_(const char *file, int line)
{
	GLenum errorCode;
	while ((errorCode = glGetError()) != GL_NO_ERROR) {
		std::string error;
		switch (errorCode) {
            case GL_INVALID_ENUM:
                error = "INVALID_ENUM";
                break;
            case GL_INVALID_VALUE:
                error = "INVALID_VALUE";
                break;
            case GL_INVALID_OPERATION:
                error = "INVALID_OPERATION";
                break;
            case GL_OUT_OF_MEMORY:
                error = "OUT_OF_MEMORY";
                break;
            case GL_INVALID_FRAMEBUFFER_OPERATION:
                error = "INVALID_FRAMEBUFFER_OPERATION";
                break;
        }
		std::cout << error << " | " << file << " (" << line << ")" << std::endl;
	}
	return errorCode;
}
#define glCheckError() glCheckError_(__FILE__, __LINE__)

void windowResizeCallback(GLFWwindow* window, int width, int height) {
	fprintf(stdout, "Window resized! New width: %d , and height: %d\n", width, height);
	//TODO
    int w, h;
    glfwGetWindowSize(myWindow.getWindow(), &w, &h);
    myWindow.setWindowDimensions(w, h);
    glfwGetFramebufferSize(myWindow.getWindow(), &retina_width, &retina_height);

    myBasicShader.useShaderProgram();

    //set projection matrix
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)myWindow.getWindowDimensions().width / (float)myWindow.getWindowDimensions().height, 0.1f, 1000.0f);
    //send matrix data to shader
    GLint projLoc = glGetUniformLocation(myBasicShader.shaderProgram, "projection");
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

    ////set Viewport transform
    glViewport(0, 0, myWindow.getWindowDimensions().width, myWindow.getWindowDimensions().height);

    skyboxShader.useShaderProgram();
    view = myCamera.getViewMatrix();
    glUniformMatrix4fv(glGetUniformLocation(skyboxShader.shaderProgram, "view"), 1, GL_FALSE,
        glm::value_ptr(view));

    projection = glm::perspective(glm::radians(45.0f), (float)myWindow.getWindowDimensions().width / (float)myWindow.getWindowDimensions().height, 0.1f, 1000.0f);
    glUniformMatrix4fv(glGetUniformLocation(skyboxShader.shaderProgram, "projection"), 1, GL_FALSE,
        glm::value_ptr(projection));
}

void keyboardCallback(GLFWwindow* window, int key, int scancode, int action, int mode) {
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GL_TRUE);
    }

    if (key == GLFW_KEY_Q && action == GLFW_PRESS)
        showDepthMap = !showDepthMap;
  

    if (key == GLFW_KEY_R && action == GLFW_PRESS)
        rainOn = !rainOn;
  

	if (key >= 0 && key < 1024) {
        if (action == GLFW_PRESS) {
            pressedKeys[key] = true;
        } else if (action == GLFW_RELEASE) {
            pressedKeys[key] = false;
        }
    }
}

void mouseCallback(GLFWwindow* window, double xpos, double ypos) {
    //TODO
    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos;
    lastX = xpos;
    lastY = ypos;

    float sensitivity = 0.1f;
    xoffset *= sensitivity;
    yoffset *= sensitivity;

    yaw += xoffset;
    pitch += yoffset;

    if (pitch > 89.0f)
        pitch = 89.0f;
    if (pitch < -89.0f)
        pitch = -89.0f;

    myBasicShader.useShaderProgram();
    myCamera.rotate(pitch, yaw);
    view = myCamera.getViewMatrix();
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
    glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
    glUniform3fv(lightDirLoc, 1, glm::value_ptr(glm::inverseTranspose(glm::mat3(view)) * lightDir));
}

void scrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
    fov -= (float)yoffset;
    if (fov < 1.0f)
        fov = 1.0f;
    if (fov > 45.0f)
        fov = 45.0f;

    myBasicShader.useShaderProgram();
    projection = glm::perspective(glm::radians(fov),
        (float)myWindow.getWindowDimensions().width / (float)myWindow.getWindowDimensions().height,
        0.1f, 1000.0f);
    // send projection matrix to shader
    glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));
}

void processMovement() {
	if (pressedKeys[GLFW_KEY_W]) {
		myCamera.move(gps::MOVE_FORWARD, cameraSpeed);

        glm::vec3 cPosition = myCamera.getCameraPosition();
        if (-2.454 < cPosition.x && cPosition.x < 2.335 &&
             1.708 < cPosition.y && cPosition.y < 4.95  &&
            -2.647 < cPosition.z && cPosition.z < 1.759) 
        {
            myCamera.move(gps::MOVE_BACKWARD, cameraSpeed);
            printf(" ------------- COLLISION -----------\n");
        }

		//update view matrix
        view = myCamera.getViewMatrix();
        myBasicShader.useShaderProgram();
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        // compute normal matrix for teapot
        normalMatrix = glm::mat3(glm::inverseTranspose(view*model));
	}

	if (pressedKeys[GLFW_KEY_S]) {
		myCamera.move(gps::MOVE_BACKWARD, cameraSpeed);

        glm::vec3 cPosition = myCamera.getCameraPosition();
        if (-2.454 < cPosition.x && cPosition.x < 2.335 &&
            1.708 < cPosition.y && cPosition.y < 4.95 &&
            -2.647 < cPosition.z && cPosition.z < 1.759)
        {
            myCamera.move(gps::MOVE_FORWARD, cameraSpeed);
            printf(" ------------- COLLISION -----------\n");
        }

        //update view matrix
        view = myCamera.getViewMatrix();
        myBasicShader.useShaderProgram();
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        // compute normal matrix for teapot
        normalMatrix = glm::mat3(glm::inverseTranspose(view*model));
	}

	if (pressedKeys[GLFW_KEY_A]) {
		myCamera.move(gps::MOVE_LEFT, cameraSpeed);

        glm::vec3 cPosition = myCamera.getCameraPosition();
        if (-2.454 < cPosition.x && cPosition.x < 2.335 &&
            1.708 < cPosition.y && cPosition.y < 4.95 &&
            -2.647 < cPosition.z && cPosition.z < 1.759)
        {
            myCamera.move(gps::MOVE_RIGHT, cameraSpeed);
            printf(" ------------- COLLISION -----------\n");
        }

        //update view matrix
        view = myCamera.getViewMatrix();
        myBasicShader.useShaderProgram();
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        // compute normal matrix for teapot
        normalMatrix = glm::mat3(glm::inverseTranspose(view*model));
	}

	if (pressedKeys[GLFW_KEY_D]) {
		myCamera.move(gps::MOVE_RIGHT, cameraSpeed);

        glm::vec3 cPosition = myCamera.getCameraPosition();
        if (-2.454 < cPosition.x && cPosition.x < 2.335 &&
            1.708 < cPosition.y && cPosition.y < 4.95 &&
            -2.647 < cPosition.z && cPosition.z < 1.759)
        {
            myCamera.move(gps::MOVE_LEFT, cameraSpeed);
            printf(" ------------- COLLISION -----------\n");
        }

        //update view matrix
        view = myCamera.getViewMatrix();
        myBasicShader.useShaderProgram();
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        // compute normal matrix for teapot
        normalMatrix = glm::mat3(glm::inverseTranspose(view*model));
	}


    if (pressedKeys[GLFW_KEY_M]) {
        myCamera.move(gps::MOVE_UP, cameraSpeed);

        glm::vec3 cPosition = myCamera.getCameraPosition();
        if (-2.454 < cPosition.x && cPosition.x < 2.335 &&
            1.708 < cPosition.y && cPosition.y < 4.95 &&
            -2.647 < cPosition.z && cPosition.z < 1.759)
        {
            myCamera.move(gps::MOVE_DOWN, cameraSpeed);
            printf(" ------------- COLLISION -----------\n");
        }

        //update view matrix
        view = myCamera.getViewMatrix();
        myBasicShader.useShaderProgram();
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        // compute normal matrix for teapot
        normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
    }

    if (pressedKeys[GLFW_KEY_N]) {
        myCamera.move(gps::MOVE_DOWN, cameraSpeed);

        glm::vec3 cPosition = myCamera.getCameraPosition();
        if (-2.454 < cPosition.x && cPosition.x < 2.335 &&
            1.708 < cPosition.y && cPosition.y < 4.95 &&
            -2.647 < cPosition.z && cPosition.z < 1.759)
        {
            myCamera.move(gps::MOVE_UP, cameraSpeed);
            printf(" ------------- COLLISION -----------\n");
        }

        //update view matrix
        view = myCamera.getViewMatrix();
        myBasicShader.useShaderProgram();
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        // compute normal matrix for teapot
        normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
    }

    if (pressedKeys[GLFW_KEY_F]) {
        fogDensity -= 0.004;
        if (fogDensity < 0.0) {
            fogDensity = 0.0;
        }
        myBasicShader.useShaderProgram();
        glUniform1fv(fogDensityLoc, 1, &fogDensity);
    }

    if (pressedKeys[GLFW_KEY_G]) {
        fogDensity += 0.004;
        if (fogDensity > 0.3) {
            fogDensity = 0.3;
        }
        myBasicShader.useShaderProgram();
        glUniform1fv(fogDensityLoc, 1, &fogDensity);
    }

    if (pressedKeys[GLFW_KEY_L]) {
        lightOn += 0.008f;
        if (lightOn > 1.0f) {
            lightOn = 1.0f;
        }

        myBasicShader.useShaderProgram();
        glUniform1fv(lightOnLoc, 1, &lightOn);

        myBasicShader.useShaderProgram();

        lightColor = lightOn * glm::vec3(1.0f, 1.0f, 1.0f); //white light
        glUniform3fv(lightColorLoc, 1, glm::value_ptr(lightColor));

    }

    if (pressedKeys[GLFW_KEY_X]) {
        lightAngle += 0.02;
    }

    if (pressedKeys[GLFW_KEY_Z]) {
        lightAngle -= 0.02;
    }


    if (pressedKeys[GLFW_KEY_K]) {
        lightOn -= 0.008f;
        if (lightOn < 0.00f) {
            lightOn = 0.00f;
        }

        myBasicShader.useShaderProgram();
        glUniform1fv(lightOnLoc, 1, &lightOn);

        myBasicShader.useShaderProgram();

        lightColor = lightOn * glm::vec3(1.0f, 1.0f, 1.0f); //white light
        glUniform3fv(lightColorLoc, 1, glm::value_ptr(lightColor));

    }

    if (pressedKeys[GLFW_KEY_P]) {
        
        camTime = 0;
        camAnim = 1;
        enabledCameraAnimation = true;
    }

    

    if (pressedKeys[GLFW_KEY_1]) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }
    if (pressedKeys[GLFW_KEY_2]) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    }
    if (pressedKeys[GLFW_KEY_3]) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
    }
}

void initOpenGLWindow() {
    if (!glfwInit()) {
        fprintf(stderr, "ERROR: could not start GLFW3\n");
        return ;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);


    //window scaling for HiDPI displays
    glfwWindowHint(GLFW_SCALE_TO_MONITOR, GLFW_TRUE);

    //for sRBG framebuffer
    glfwWindowHint(GLFW_SRGB_CAPABLE, GLFW_TRUE);

    //for antialising
    glfwWindowHint(GLFW_SAMPLES, 4);

    myWindow.Create(1024, 600, "OpenGL Shader Example");
    if (!myWindow.getWindow()) {
        fprintf(stderr, "ERROR: could not open window with GLFW3\n");
        glfwTerminate();
        return ;
    }

    glfwSetWindowSizeCallback(myWindow.getWindow(), windowResizeCallback);
    glfwSetKeyCallback(myWindow.getWindow(), keyboardCallback);
    glfwSetCursorPosCallback(myWindow.getWindow(), mouseCallback);
    glfwSetScrollCallback(myWindow.getWindow(), scrollCallback);
    glfwSetInputMode(myWindow.getWindow(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    glfwMakeContextCurrent(myWindow.getWindow());

    glfwSwapInterval(1);

#if not defined (__APPLE__)
    // start GLEW extension handler
    glewExperimental = GL_TRUE;
    glewInit();
#endif

    // get version info
    const GLubyte* renderer = glGetString(GL_RENDERER); // get renderer string
    const GLubyte* version = glGetString(GL_VERSION); // version as a string
    printf("Renderer: %s\n", renderer);
    printf("OpenGL version supported %s\n", version);

    //for RETINA display
    int w, h;
    glfwGetWindowSize(myWindow.getWindow(), &w, &h);
    myWindow.setWindowDimensions(w, h);
    glfwGetFramebufferSize(myWindow.getWindow(), &retina_width, &retina_height);

    return ;
}

void setWindowCallbacks() {
	glfwSetWindowSizeCallback(myWindow.getWindow(), windowResizeCallback);
    glfwSetKeyCallback(myWindow.getWindow(), keyboardCallback);
    glfwSetCursorPosCallback(myWindow.getWindow(), mouseCallback);
}

void initOpenGLState() {
	glClearColor(0.7f, 0.7f, 0.7f, 1.0f);
	glViewport(0, 0, myWindow.getWindowDimensions().width, myWindow.getWindowDimensions().height);
    glEnable(GL_FRAMEBUFFER_SRGB);
	glEnable(GL_DEPTH_TEST); // enable depth-testing
	glDepthFunc(GL_LESS); // depth-testing interprets a smaller value as "closer"
	glEnable(GL_CULL_FACE); // cull face
	glCullFace(GL_BACK); // cull back face
	glFrontFace(GL_CCW); // GL_CCW for counter clock-wise

}

void initModels() {
    //teapot.LoadModel("models/teapot/teapot20segUT.obj");
    scene.LoadModel("models/scene/scene.obj");
    boat.LoadModel("models/boat/boat.obj");
    screenQuad.LoadModel("models/quad/quad.obj"); 
    Rain.LoadModel("models/rain/rain.obj");
}

void initShaders() {
	myBasicShader.loadShader(
        "shaders/basic.vert",
        "shaders/basic.frag");
    myBasicShader.useShaderProgram();

    skyboxShader.loadShader("shaders/skyboxShader.vert", "shaders/skyboxShader.frag");
    skyboxShader.useShaderProgram();

    depthMapShader.loadShader("shaders/depthMap.vert", "shaders/depthMap.frag");
    depthMapShader.useShaderProgram();

    screenQuadShader.loadShader("shaders/screenQuad.vert", "shaders/screenQuad.frag");
    screenQuadShader.useShaderProgram();
}

void initSkyBox() {
    std::vector<const GLchar*> faces;
    faces.push_back("skybox/front.tga");
    faces.push_back("skybox/back.tga");
    faces.push_back("skybox/top.tga");
    faces.push_back("skybox/bottom.tga");
    faces.push_back("skybox/right.tga");
    faces.push_back("skybox/left.tga");
    mySkyBox.Load(faces);
}

void initSkyBoxNight() {
    std::vector<const GLchar*> faces;
    faces.push_back("skybox1/front.tga");
    faces.push_back("skybox1/back.tga");
    faces.push_back("skybox1/top.tga");
    faces.push_back("skybox1/bottom.tga");
    faces.push_back("skybox1/right.tga");
    faces.push_back("skybox1/left.tga");
    mySkyBoxNight.Load(faces);
}

void presentation() {

    if (enabledCameraAnimation) {
        if (camAnim == 1) {
            if (camTime > 700) {
                camTime = 0;
                camAnim = 2;
            }
            else
            {
                camTime++;
                myCamera = gps::Camera(glm::vec3(-10.0f + camTime/70.0f, 3.0f + camTime/700.0f , 8.0f + camTime/31.81f), glm::vec3(0, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
            }
        }
        if (camAnim == 2) {
            if (camTime > 500) {
                camTime = 0;
                camAnim = 3;
                
            }
            else
            {
                camTime++;

                float camZ = sin(camTime / 100.0f) * 20;
                float camX = cos(camTime / 100.0f) * 10;

                myCamera = gps::Camera(glm::vec3(camX-10.0f, 4.0f, camZ+30.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
            }
        }
        if (camAnim == 3) {
            if (camTime > 400) {
                camTime = 0;
                camAnim = 1;
                enabledCameraAnimation = false;
            }
            else
            {
                camTime++;
                myCamera = gps::Camera(glm::vec3(-10.0f - camTime/20.0f, 4.0f-camTime/300.0f, 10.0f - camTime/8.0f), glm::vec3(0.0f, 0.0f+camTime/250.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));

            }
        }
    }
}

void initUniforms() {
	myBasicShader.useShaderProgram();

    // create model matrix for teapot
    model = glm::mat4(1.0f);
    model = glm::rotate(glm::mat4(1.0f), glm::radians(angle), glm::vec3(0.0f, 1.0f, 0.0f));
	modelLoc = glGetUniformLocation(myBasicShader.shaderProgram, "model");

	// get view matrix for current camera
	view = myCamera.getViewMatrix();
	viewLoc = glGetUniformLocation(myBasicShader.shaderProgram, "view");
	// send view matrix to shader
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));

    // compute normal matrix for teapot
    normalMatrix = glm::mat3(glm::inverseTranspose(view*model));
	normalMatrixLoc = glGetUniformLocation(myBasicShader.shaderProgram, "normalMatrix");
    glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));

	// create projection matrix
	projection = glm::perspective(glm::radians(45.0f),
                            (float)myWindow.getWindowDimensions().width / (float)myWindow.getWindowDimensions().height,
                            0.1f, 1000.0f);
	projectionLoc = glGetUniformLocation(myBasicShader.shaderProgram, "projection");
	// send projection matrix to shader
	glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));	

	//set the light direction (direction towards the light)
	lightDir = glm::vec3(-15.0f, 10.0f, 0.0f);

    lightRotation = glm::rotate(glm::mat4(1.0f), lightAngle, glm::vec3(0.0f, 1.0f, 0.0f));
	lightDirLoc = glGetUniformLocation(myBasicShader.shaderProgram, "lightDir");
	// send light dir to shader
    glUniform3fv(lightDirLoc, 1, glm::value_ptr(glm::inverseTranspose(glm::mat3(view * lightRotation)) * lightDir));

	//set light color
	lightColor = lightOn * glm::vec3(1.0f, 1.0f, 1.0f); //white light
	lightColorLoc = glGetUniformLocation(myBasicShader.shaderProgram, "lightColor");
	// send light color to shader
	glUniform3fv(lightColorLoc, 1, glm::value_ptr(lightColor));

    fogDensity = 0.0f;
    fogDensityLoc = glGetUniformLocation(myBasicShader.shaderProgram, "fogDensity");
    glUniform1fv(fogDensityLoc, 1, &fogDensity);

    lightOn = 1.0f;
    lightOnLoc = glGetUniformLocation(myBasicShader.shaderProgram, "lightOn");
    glUniform1fv(lightOnLoc, 1, &lightOn);
}

void initFBO() {
	//TODO - Create the FBO, the depth texture and attach the depth texture to the FBO

	//generate FBO ID
	glGenFramebuffers(1, &shadowMapFBO);

	
	//create depth texture for FBO
	glGenTextures(1, &depthMapTexture);
	glBindTexture(GL_TEXTURE_2D, depthMapTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT,
		SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

	//attach texture to FBO
	glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMapTexture, 0);

	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

glm::mat4 computeLightSpaceTrMatrix() {
    //TODO - Return the light-space transformation matrix
    lightRotation = glm::rotate(glm::mat4(1.0f), lightAngle, glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 lightView = glm::lookAt(glm::vec3(lightRotation * glm::vec4(lightDir, 1.0f)), glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    const GLfloat near_plane = -100.0f, far_plane = 100.0f;
    glm::mat4 lightProjection = glm::ortho(-100.0f, 100.0f, -50.0f, 50.0f, near_plane, far_plane);
    glm::mat4 lightSpaceTrMatrix = lightProjection * lightView;
    return lightSpaceTrMatrix;
}

void renderObjects(gps::Shader shader, bool depthPass) {
    // select active shader program
    shader.useShaderProgram();

    model = glm::rotate(glm::mat4(1.0f), 0.0f, glm::vec3(0.0f, 1.0f, 0.0f));
    glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));

    if (!depthPass) {
        normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
        glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
    }
    
    scene.Draw(shader);

    model = glm::rotate(model, boatAngleZ, glm::vec3(0, 0, 1));
    model = glm::rotate(model, boatAngleY, glm::vec3(0, 1, 0));
    glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
    boatAngleY += 0.0005;
    boatAngleZ += boatAngleIncrement;

    if (boatAngleZ > 0.007f || boatAngleZ < -0.007f) {
        boatAngleIncrement *= -1.0f;
    }

    if (!depthPass) {
        normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
        glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
    }
    
    boat.Draw(shader);

}


float yRain = 7.0f;
void renderRain(gps::Shader shader) {
    // select active shader program
    shader.useShaderProgram();

    for (int i = 0; i < RAIN_DROPS; i++) {
        glm::vec3 drop = glm::vec3(raindrops.at(i).x, yRain, raindrops.at(i).z);

        yRain -= 0.02f;
        if (yRain < 0.0f) {
            yRain = 7.0f;
        }


        glm::mat4 modelRain = glm::mat4(1.0f);
        modelRain = glm::translate(modelRain, drop);


        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelRain));


        glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));


        Rain.Draw(shader);

    }
}

void renderScene() {
    presentation();

    depthMapShader.useShaderProgram();
    glUniformMatrix4fv(glGetUniformLocation(depthMapShader.shaderProgram, "lightSpaceTrMatrix"),
        1,
        GL_FALSE,
        glm::value_ptr(computeLightSpaceTrMatrix()));
    glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
    glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO);
    glClear(GL_DEPTH_BUFFER_BIT);

    renderObjects(depthMapShader, true);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    // render depth map on screen - toggled with the M key

    if (showDepthMap) {
        glViewport(0, 0, myWindow.getWindowDimensions().width, myWindow.getWindowDimensions().height);

        glClear(GL_COLOR_BUFFER_BIT);

        screenQuadShader.useShaderProgram();

        //bind the depth map
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, depthMapTexture);
        glUniform1i(glGetUniformLocation(screenQuadShader.shaderProgram, "depthMap"), 0);

        glDisable(GL_DEPTH_TEST);
        screenQuad.Draw(screenQuadShader);
        glEnable(GL_DEPTH_TEST);
       
    }
    else {

        // final scene rendering pass (with shadows)

        glViewport(0, 0, myWindow.getWindowDimensions().width, myWindow.getWindowDimensions().height);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        myBasicShader.useShaderProgram();

        view = myCamera.getViewMatrix();
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));

        lightRotation = glm::rotate(glm::mat4(1.0f), lightAngle, glm::vec3(0.0f, 1.0f, 0.0f));
        glUniform3fv(lightDirLoc, 1, glm::value_ptr(glm::inverseTranspose(glm::mat3(view * lightRotation)) * lightDir));


        //bind the shadow map
        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_2D, depthMapTexture);
        glUniform1i(glGetUniformLocation(myBasicShader.shaderProgram, "shadowMap"), 3);

        glUniformMatrix4fv(glGetUniformLocation(myBasicShader.shaderProgram, "lightSpaceTrMatrix"),
            1,
            GL_FALSE,
            glm::value_ptr(computeLightSpaceTrMatrix()));

        renderObjects(myBasicShader, false);

        skyboxShader.useShaderProgram();

        if (lightOn < 0.2f) {
            mySkyBoxNight.Draw(skyboxShader, view, projection);
        }
        else {
            mySkyBox.Draw(skyboxShader, view, projection);
        }
        

        if (rainOn) {
            if (rainOn == true) {
                if(lightOn > 0.5f)
                    lightOn = 0.5f;
            }
            myBasicShader.useShaderProgram();
            glUniform1fv(lightOnLoc, 1, &lightOn);

            myBasicShader.useShaderProgram();

            lightColor = lightOn * glm::vec3(1.0f, 1.0f, 1.0f); //white light
            glUniform3fv(lightColorLoc, 1, glm::value_ptr(lightColor));


            myBasicShader.useShaderProgram();
            renderRain(myBasicShader);
        }
        
    }
}

void cleanup() {
    glDeleteTextures(1, &depthMapTexture);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDeleteFramebuffers(1, &shadowMapFBO);
    glfwDestroyWindow(myWindow.getWindow());
    //close GL context and any other GLFW resources
    glfwTerminate();
}

int main(int argc, const char * argv[]) {

    try {
        initOpenGLWindow();
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    for (int i = 0; i < RAIN_DROPS; i++) {
        float x = (rand() % 65000) / 1000.0f;
        float z = (rand() % 65000) / 1000.0f;
        float signX = (rand() % 4 + 1);
        float signZ = (rand() % 4 + 1);
        if ((int)signX % 2 != 0) {
            x = -x;
        }
        if ((int)signZ % 2 != 0) {
            z = -z;
        }

        raindrops.push_back(glm::vec3(x, 7.0f, z));
    }


    initOpenGLState();
	initModels();
	initShaders();
    initSkyBox();
    initSkyBoxNight();
	initUniforms();
    initFBO();
    setWindowCallbacks();

	glCheckError();
	// application loop
	while (!glfwWindowShouldClose(myWindow.getWindow())) {
        processMovement();
	    renderScene();

		glfwPollEvents();
		glfwSwapBuffers(myWindow.getWindow());

		glCheckError();
	}

	cleanup();

    return EXIT_SUCCESS;
}
