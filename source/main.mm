/*
 main

 Copyright 2012 Thomas Dalling - http://tomdalling.com/

 Licensed under the Apache License, Version 2.0 (the "License");
 you may not use this file except in compliance with the License.
 You may obtain a copy of the License at

     http://www.apache.org/licenses/LICENSE-2.0

 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 See the License for the specific language governing permissions and
 limitations under the License.
 */

// third-party libraries
#import <Foundation/Foundation.h>
#include <GL/glew.h>
#include <GL/glfw.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

// standard C++ libraries
#include <cassert>
#include <iostream>
#include <stdexcept>
#include <cmath>
#include <list>
#include <array>

// tdogl classes
#include "tdogl/Program.h"
#include "tdogl/Texture.h"
#include "tdogl/Camera.h"

#include "rangeterrain.h"
//AntTweakBar
//#include <AntTweakBar.h>


/*
 Represents a textured geometry asset
 
 Contains everything necessary to draw arbitrary geometry with a single texture:
 
 - shaders
 - a texture
 - a VBO
 - a VAO
 - the parameters to glDrawArrays (drawType, drawStart, drawCount)
 */
struct ModelAsset {
    tdogl::Program* shaders;
    tdogl::Texture* texture;
    GLuint vbo;
    GLuint vao;
    GLenum drawType;
    GLint drawStart;
    GLint drawCount;
    GLfloat shininess;
    glm::vec3 specularColor;
    
    ModelAsset() :
    shaders(NULL),
    texture(NULL),
    vbo(0),
    vao(0),
    drawType(GL_TRIANGLES),
    drawStart(0),
    drawCount(0),
    shininess(0.0f),
    specularColor(1.0f, 1.0f, 1.0f)
    {}
};

/*
 Represents an instance of an `ModelAsset`
 
 Contains a pointer to the asset, and a model transformation matrix to be used when drawing.
 */
struct ModelInstance {
    ModelAsset* asset;
    glm::mat4 transform;
    
    ModelInstance() :
    asset(NULL),
    transform()
    {}
};


/*
 Represents a point light
 */
struct Light {
    glm::vec3 position;
    glm::vec3 intensities; //a.k.a. the color of the light
    float attenuation;
    float ambientCoefficient;
};

// constants
const glm::vec2 SCREEN_SIZE(1024, 512);

// globals

tdogl::Camera gCamera1; //Left camera
tdogl::Camera gCamera2; //Right camera, overview
bool LEFT_CAMERA_FULLSCREEN = false;

RangeTerrain gTerrain;
ModelAsset gTerrainModelAsset;
std::list<ModelInstance> gInstances;
GLfloat gDegreesRotated = 0.0f;
Light gLight;

bool mouseButtonDown = false;
int prevCursorPosX, prevCursorPosY;



// returns the full path to the file `fileName` in the resources directory of the app bundle
static std::string ResourcePath(std::string fileName) {
    NSString* fname = [NSString stringWithCString:fileName.c_str() encoding:NSUTF8StringEncoding];
    NSString* path = [[[NSBundle mainBundle] resourcePath] stringByAppendingPathComponent:fname];
    return std::string([path cStringUsingEncoding:NSUTF8StringEncoding]);
}


// returns a new tdogl::Program created from the given vertex and fragment shader filenames
static tdogl::Program* LoadShaders(const char* vertFilename, const char* fragFilename) {
    std::vector<tdogl::Shader> shaders;
    shaders.push_back(tdogl::Shader::shaderFromFile(ResourcePath(vertFilename), GL_VERTEX_SHADER));
    shaders.push_back(tdogl::Shader::shaderFromFile(ResourcePath(fragFilename), GL_FRAGMENT_SHADER));
    return new tdogl::Program(shaders);
}


// returns a new tdogl::Texture created from the given filename
static tdogl::Texture* LoadTexture(const char* filename) {
    tdogl::Bitmap bmp = tdogl::Bitmap::bitmapFromFile(ResourcePath(filename));
    bmp.flipVertically();
    return new tdogl::Texture(bmp);
}

static void SendDataToBuffer(GLfloat* vdata, ModelAsset &asset, int floatsPerVertex) {
    // bind the VAO
    glBindVertexArray(asset.vao);
    
    // bind the VBO
    glBindBuffer(GL_ARRAY_BUFFER, asset.vbo);
    
    // write the data
    glBufferData(GL_ARRAY_BUFFER, asset.drawCount * floatsPerVertex * sizeof(GLfloat), vdata, GL_STATIC_DRAW);
    
    // unbind the VAO
    glBindVertexArray(0);
}

//void* gMappedBuffer;
//static void ChangeBufferData(GLfloat* data, int* idx, int &length) {
//    for (int i=0; i<length; i++) {
//        gMappedBuffer[
//    }
//}

// initialises the gWoodenCrate global
static void LoadAsset(ModelAsset &asset) {
    // set all the elements of gWoodenCrate
    asset.shaders = LoadShaders("vertex-shader.txt", "fragment-shader.txt");
    asset.drawType = GL_TRIANGLES;
    asset.drawStart = 0;
    asset.drawCount = (X_INTERVAL - 1) * (Y_INTERVAL - 1) * 6;
//    asset.drawCount = 6*2*3;
    asset.texture = LoadTexture("grass.png");
    asset.shininess = 80.0;
    asset.specularColor = glm::vec3(1.0f, 1.0f, 1.0f);
    
    glGenBuffers(1, &asset.vbo);
    glGenVertexArrays(1, &asset.vao);
    
    // bind the VAO
    glBindVertexArray(asset.vao);

    // bind the VBO
    glBindBuffer(GL_ARRAY_BUFFER, asset.vbo);

    // connect the xyz to the "vert" attribute of the vertex shader
    glEnableVertexAttribArray(asset.shaders->attrib("vert"));
    glVertexAttribPointer(asset.shaders->attrib("vert"), 3, GL_FLOAT, GL_FALSE, 8*sizeof(GLfloat), NULL);

    // connect the uv coords to the "vertTexCoord" attribute of the vertex shader
    glEnableVertexAttribArray(asset.shaders->attrib("vertTexCoord"));
    glVertexAttribPointer(asset.shaders->attrib("vertTexCoord"), 2, GL_FLOAT, GL_TRUE,  8*sizeof(GLfloat), (const GLvoid*)(3 * sizeof(GLfloat)));

    // connect the normal to the "vertNormal" attribute of the vertex shader
    glEnableVertexAttribArray(asset.shaders->attrib("vertNormal"));
    glVertexAttribPointer(asset.shaders->attrib("vertNormal"), 3, GL_FLOAT, GL_TRUE,  8*sizeof(GLfloat), (const GLvoid*)(5 * sizeof(GLfloat)));

    // unbind the VAO
    glBindVertexArray(0);
    
    // Send data to buffer
    SendDataToBuffer(gTerrain.vdata, gTerrainModelAsset, RangeTerrain::floatsPerVertex);
}


// convenience function that returns a translation matrix
glm::mat4 translate(GLfloat x, GLfloat y, GLfloat z) {
    return glm::translate(glm::mat4(), glm::vec3(x,y,z));
}


// convenience function that returns a scaling matrix
glm::mat4 scale(GLfloat x, GLfloat y, GLfloat z) {
    return glm::scale(glm::mat4(), glm::vec3(x,y,z));
}


//renders a single `ModelInstance` //TODO add camera as argument
static void RenderInstance(const ModelInstance& inst, tdogl::Camera& camera, bool ortho) {
    ModelAsset* asset = inst.asset;
    tdogl::Program* shaders = asset->shaders;

    //bind the shaders
    shaders->use();

    //set the shader uniforms
    if (ortho)
        shaders->setUniform("camera", camera.orthoMatrix());
    else
        shaders->setUniform("camera", camera.matrix());
    shaders->setUniform("model", inst.transform);
    shaders->setUniform("materialTex", 0); //set to 0 because the texture will be bound to GL_TEXTURE0
//    shaders->setUniform("materialShininess", asset->shininess);
//    shaders->setUniform("materialSpecularColor", asset->specularColor);
    shaders->setUniform("light.position", gLight.position);
    shaders->setUniform("light.intensities", gLight.intensities);
    shaders->setUniform("light.attenuation", gLight.attenuation);
    shaders->setUniform("light.ambientCoefficient", gLight.ambientCoefficient);
    shaders->setUniform("cameraPosition", camera.position());

    //bind the texture
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, asset->texture->object());

    //bind VAO and draw
    glBindVertexArray(asset->vao);
    glDrawArrays(asset->drawType, asset->drawStart, asset->drawCount);

    //unbind everything
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
    shaders->stopUsing();
}


// draws a single frame
static void Render() {
    // clear everything
    glClearColor(0, 0, 0, 1); // black
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    int viewports = 2;

    // render all the instances for each viewport
    for (int i = 0; i < viewports; i++) {
        
        
        if (i == 0 && !LEFT_CAMERA_FULLSCREEN)
            glViewport(0, 0, SCREEN_SIZE.x/2, SCREEN_SIZE.y);
        else if (!LEFT_CAMERA_FULLSCREEN)
            glViewport(SCREEN_SIZE.x/2, 0, SCREEN_SIZE.x/2, SCREEN_SIZE.y);
        else
            glViewport(0, 0, SCREEN_SIZE.x, SCREEN_SIZE.y);
        
        std::list<ModelInstance>::const_iterator it;
        for(it = gInstances.begin(); it != gInstances.end(); ++it){
            if (i==0 || LEFT_CAMERA_FULLSCREEN)
                RenderInstance(*it, gCamera1, false);
            else
                RenderInstance(*it, gCamera2, true); // Render second viewport with 2D projection matrix
        }
    }
    

    // swap the display buffers (displays what was just drawn)
    glfwSwapBuffers();
}


// update the scene based on the time elapsed since last update
static void Update(float dt) {

    //move position of camera based on WASD keys, and QE keys for up and down
    const float moveSpeed = glfwGetKey(GLFW_KEY_LSHIFT) ? 50.0 : 10.0; //units per second
    if(glfwGetKey('S')){
        gCamera1.offsetPosition(dt * moveSpeed * -gCamera1.forward());
    } else if(glfwGetKey('W')){
        gCamera1.offsetPosition(dt * moveSpeed * gCamera1.forward());
    }
    if(glfwGetKey('A')){
        gCamera1.offsetPosition(dt * moveSpeed * -gCamera1.right());
    } else if(glfwGetKey('D')){
        gCamera1.offsetPosition(dt * moveSpeed * gCamera1.right());
    }
    if(glfwGetKey('E')){
        gCamera1.offsetPosition(dt * moveSpeed * -gCamera1.up());
    } else if(glfwGetKey('Q')){
        gCamera1.offsetPosition(dt * moveSpeed * gCamera1.up());
    }
    
    // ************ TEMP FOR DYNAMIC TERRAIN ADJUSTMENT BELOW ************
    if(glfwGetKey('O')){
        gTerrain.SetControlPoint(32, 32, (gTerrain.GetControlPoint(32, 32) ? gTerrain.GetControlPoint(32, 32)->h : 0) + 1 * dt, 8, FUNC_COS);
        gTerrain.GenerateEverythingFromControlPoints();
        SendDataToBuffer(gTerrain.vdata, gTerrainModelAsset, RangeTerrain::floatsPerVertex);
    } else if(glfwGetKey('P')){
        gTerrain.SetControlPoint(32, 32, (gTerrain.GetControlPoint(32, 32) ? gTerrain.GetControlPoint(32, 32)->h : 0) - 1 * dt, 8, FUNC_COS);
        gTerrain.GenerateEverythingFromControlPoints();
        SendDataToBuffer(gTerrain.vdata, gTerrainModelAsset, RangeTerrain::floatsPerVertex);
    }
    // ************ TEMP FOR DYNAMIC TERRAIN ADJUSTMENT ABOVE ************
    
    // rotate the camera based on arrow keys
    const float rotSpeed = 45.0; //degrees per second
    if(glfwGetKey(GLFW_KEY_UP)){
        gCamera1.offsetOrientation(dt * -rotSpeed, 0);
    } else if(glfwGetKey(GLFW_KEY_DOWN)){
        gCamera1.offsetOrientation(dt * rotSpeed, 0);
    }
    if(glfwGetKey(GLFW_KEY_RIGHT)){
        gCamera1.offsetOrientation(0, dt * rotSpeed);
    } else if(glfwGetKey(GLFW_KEY_LEFT)){
        gCamera1.offsetOrientation(0, dt * -rotSpeed);
    }
    
    // zoom based on XZ keys
    const float zoomSpeed = 30.0; //degrees per second
    if(glfwGetKey('X')){
        float fieldOfView = gCamera1.fieldOfView() - dt * zoomSpeed;
        if(fieldOfView < 5.0f) fieldOfView = 5.0f;
        if(fieldOfView > 130.0f) fieldOfView = 130.0f;
        gCamera1.setFieldOfView(fieldOfView);
    } else if(glfwGetKey('Z')){
        float fieldOfView = gCamera1.fieldOfView() + dt * zoomSpeed;
        if(fieldOfView < 5.0f) fieldOfView = 5.0f;
        if(fieldOfView > 130.0f) fieldOfView = 130.0f;
        gCamera1.setFieldOfView(fieldOfView);
    }
    
    //Mouse click
    if(glfwGetMouseButton(GLFW_MOUSE_BUTTON_1)) {
        int xpos,ypos;
        glfwGetMousePos(&xpos, &ypos);
        
        // get the
        
        //TODO, do something with the cursor coordinates
        if (xpos >= SCREEN_SIZE.x/2) { //right viewport
            std::cout << "x: "<< xpos << " y:" << ypos << std::endl;
            
           
        } else {
            glfwDisable(GLFW_MOUSE_CURSOR);
            
            //rotate camera based on mouse movement
            const float mouseSensitivity = 0.1f;
            int mouseX, mouseY;
            glfwGetMousePos(&mouseX, &mouseY);
            
            if (!mouseButtonDown) {
                prevCursorPosX = mouseX;
                prevCursorPosY = mouseY;
                glfwSetMousePos(0, 0);
                mouseX = 0; mouseY = 0;
            }
            mouseButtonDown = true;
            
            gCamera1.offsetOrientation(mouseSensitivity * mouseY, mouseSensitivity * mouseX);
            glfwSetMousePos(0, 0); //reset the mouse, so it doesn't go out of the window
        }
        
    } else if (mouseButtonDown){
        glfwEnable(GLFW_MOUSE_CURSOR);
        glfwSetMousePos(prevCursorPosX, prevCursorPosY);
        mouseButtonDown = false;
        
    }
    
    //move light
    if(glfwGetKey('1'))
        gLight.position = gCamera1.position();

    // change light color
    if(glfwGetKey('2'))
        gLight.intensities = glm::vec3(1,0,0); //red
    else if(glfwGetKey('3'))
        gLight.intensities = glm::vec3(0,1,0); //green
    else if(glfwGetKey('4'))
        gLight.intensities = glm::vec3(0,0,1); //blue
    else if(glfwGetKey('5'))
        gLight.intensities = glm::vec3(1,1,1); //white
}

// the program starts here
void AppMain() {
    // initialise GLFW
    if(!glfwInit())
        throw std::runtime_error("glfwInit failed");

    // open a window with GLFW
    glfwOpenWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwOpenWindowHint(GLFW_OPENGL_VERSION_MAJOR, 3);
    glfwOpenWindowHint(GLFW_OPENGL_VERSION_MINOR, 2);
    glfwOpenWindowHint(GLFW_WINDOW_NO_RESIZE, GL_TRUE);
    if(!glfwOpenWindow((int)SCREEN_SIZE.x, (int)SCREEN_SIZE.y, 8, 8, 8, 8, 16, 0, GLFW_WINDOW))
        throw std::runtime_error("glfwOpenWindow failed. Can your hardware handle OpenGL 3.2?");

    // GLFW settings
    //glfwDisable(GLFW_MOUSE_CURSOR);
    glfwSetMousePos(0, 0);
    glfwSetMouseWheel(0);
    
    
    // initialise GLEW
    glewExperimental = GL_TRUE; //stops glew crashing on OSX :-/
    if(glewInit() != GLEW_OK)
        throw std::runtime_error("glewInit failed");

    // GLEW throws some errors, so discard all the errors so far
    while(glGetError() != GL_NO_ERROR) {}

    // print out some info about the graphics drivers
    std::cout << "OpenGL version: " << glGetString(GL_VERSION) << std::endl;
    std::cout << "GLSL version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << std::endl;
    std::cout << "Vendor: " << glGetString(GL_VENDOR) << std::endl;
    std::cout << "Renderer: " << glGetString(GL_RENDERER) << std::endl;

    // make sure OpenGL version 3.2 API is available
    if(!GLEW_VERSION_3_2)
        throw std::runtime_error("OpenGL 3.2 API is not available.");

    // OpenGL settings
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    // initialise the asset
    LoadAsset(gTerrainModelAsset);
    ModelInstance instance;
    instance.asset = &gTerrainModelAsset;
    gInstances.push_back(instance);

    // setup gCamera1 (left camera)
    gCamera1.setPosition(glm::vec3(0, 10, 0));
    gCamera1.setViewportAspectRatio(SCREEN_SIZE.x / SCREEN_SIZE.y);
    gCamera1.setNearAndFarPlanes(0.5f, 100.0f);
    gCamera1.lookAt(glm::vec3(TERRAIN_WIDTH / 2, 0, TERRAIN_DEPTH / 2));
    
    // setup gCamera2 (right camera)
    gCamera2.setPosition(glm::vec3(TERRAIN_WIDTH / 2, 20, TERRAIN_DEPTH / 2));
    gCamera2.setOrtho(-TERRAIN_WIDTH / 2, TERRAIN_WIDTH / 2, -TERRAIN_DEPTH / 2, TERRAIN_DEPTH / 2, 0.5f, 100.0f);
    gCamera2.SetAboveMode(true);

    // setup gLight
    gLight.position = glm::vec3(TERRAIN_WIDTH / 2, 10, TERRAIN_DEPTH / 2);
    gLight.intensities = glm::vec3(1,1,1); //white
    gLight.attenuation = 0.001f;
    gLight.ambientCoefficient = 0.080f;
    
    // setup AntTweakBar
//    TwInit(TW_OPENGL, NULL);
//    TwWindowSize(100, 100);
//    TwBar *tweakBar;
//    tweakBar = TwNewBar("Controls");
    // run while the window is open
    double lastTime = glfwGetTime();
    while(glfwGetWindowParam(GLFW_OPENED)){
        // update the scene based on the time elapsed since last update
        double thisTime = glfwGetTime();
        float dt = thisTime - lastTime;
        Update(dt);
        lastTime = thisTime;
        cout << "render time: " << round(dt * 1000) << " ms" << endl;

        
        //setup two viewports and draw one frame
        Render();
        
        //draw tweak bar
//        TwDraw();

        // check for errors
        GLenum error = glGetError();
        if(error != GL_NO_ERROR)
            std::cerr << "OpenGL Error " << error << ": " << (const char*)gluErrorString(error) << std::endl;

        //exit program if escape key is pressed
        if(glfwGetKey(GLFW_KEY_ESC))
            glfwCloseWindow();
    }

    // clean up and exit
    glfwTerminate();
}


int main(int argc, char *argv[]) {
    try {
        AppMain();
    } catch (const std::exception& e){
        std::cerr << "ERROR: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
// test
