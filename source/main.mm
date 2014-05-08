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
const glm::vec2 SCREEN_SIZE(1024, 800);

// globals
tdogl::Camera gCamera; //Left camera
tdogl::Camera gCamera2; //Right camera, overview
bool ONLY_LEFT_CAMERA = true;

RangeTerrain gTerrain;
ModelAsset gTerrainModelAsset;
std::list<ModelInstance> gInstances;
GLfloat gDegreesRotated = 0.0f;
Light gLight;


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

// initialises the gWoodenCrate global
static void LoadAsset(ModelAsset &asset) {
    // set all the elements of gWoodenCrate
    asset.shaders = LoadShaders("vertex-shader.txt", "fragment-shader.txt");
    asset.drawType = GL_TRIANGLES;
    asset.drawStart = 0;
    asset.drawCount = (X_INTERVAL - 1) * (Y_INTERVAL - 1) * 6;
//    asset.drawCount = 6*2*3;
    asset.texture = LoadTexture("wooden-crate.jpg");
    asset.shininess = 80.0;
    asset.specularColor = glm::vec3(1.0f, 1.0f, 1.0f);
    
    glGenBuffers(1, &asset.vbo);
    glGenVertexArrays(1, &asset.vao);

    // bind the VAO
    glBindVertexArray(asset.vao);

    // bind the VBO
    glBindBuffer(GL_ARRAY_BUFFER, asset.vbo);

    // **************** TERRAIN FROM HMAP ****************

//    GLfloat vdata[7*8];
    
    const int valuesPerVertex = 8;
    GLfloat* vdata = new GLfloat[asset.drawCount * 8];

    float h1, h2, h3, h4;
    glm::vec3 *n1, *n2, *n3, *n4;
    int v = 0;
    for (int y=1; y<Y_INTERVAL; y++) {
        for (int x=1; x<X_INTERVAL; x++) {
            h1 = gTerrain.hmap[y-1][x-1];
            h2 = gTerrain.hmap[y][x-1];
            h3 = gTerrain.hmap[y-1][x];
            h4 = gTerrain.hmap[y][x];
            
            n1 = &gTerrain.normals[y-1][x-1];
            n2 = &gTerrain.normals[y][x-1];
            n3 = &gTerrain.normals[y-1][x];
            n4 = &gTerrain.normals[y][x];
            
            vdata[v++] = (x-1) * GRID_RES;
            vdata[v++] = gTerrain.hmap[y-1][x-1];
            vdata[v++] = (y-1) * GRID_RES;

            vdata[v++] = 0;
            vdata[v++] = 0;
            
            vdata[v++] = n1->x;
            vdata[v++] = n1->y;
            vdata[v++] = n1->z;
            
            
            
            vdata[v++] = (x-1) * GRID_RES;
            vdata[v++] = h2;
            vdata[v++] = y * GRID_RES;
            
            vdata[v++] = 0;
            vdata[v++] = 1;
            
            vdata[v++] = n2->x;
            vdata[v++] = n2->y;
            vdata[v++] = n2->z;
            
            
            
            vdata[v++] = x * GRID_RES;
            vdata[v++] = h3;
            vdata[v++] = (y-1) * GRID_RES;
            
            vdata[v++] = 1;
            vdata[v++] = 0;
            
            vdata[v++] = n3->x;
            vdata[v++] = n3->y;
            vdata[v++] = n3->z;
            
            
            
            vdata[v++] = x * GRID_RES;
            vdata[v++] = h4;
            vdata[v++] = y * GRID_RES;
            
            vdata[v++] = 1;
            vdata[v++] = 1;
            
            vdata[v++] = n4->x;
            vdata[v++] = n4->y;
            vdata[v++] = n4->z;
            
            
            
            vdata[v++] = x * GRID_RES;
            vdata[v++] = h3;
            vdata[v++] = (y-1) * GRID_RES;
            
            vdata[v++] = 1;
            vdata[v++] = 0;
            
            vdata[v++] = n3->x;
            vdata[v++] = n3->y;
            vdata[v++] = n3->z;
            
            
            
            vdata[v++] = (x-1) * GRID_RES;;
            vdata[v++] = h2;
            vdata[v++] = y * GRID_RES;
            
            vdata[v++] = 0;
            vdata[v++] = 1;
            
            vdata[v++] = n2->x;
            vdata[v++] = n2->y;
            vdata[v++] = n2->z;
        }
    }
    
    // Make a cube out of triangles (two triangles per side)
//    GLfloat vdata[] = {
//        //  X     Y     Z       U     V          Normal
//        // bottom
//        /*-1.0f,-1.0f,-1.0f,   0.0f, 0.0f,   0.0f, -1.0f, 0.0f,
//         1.0f,-1.0f,-1.0f,   1.0f, 0.0f,   0.0f, -1.0f, 0.0f,
//        -1.0f,-1.0f, 1.0f,   0.0f, 1.0f,   0.0f, -1.0f, 0.0f,
//         1.0f,-1.0f,-1.0f,   1.0f, 0.0f,   0.0f, -1.0f, 0.0f,
//         1.0f,-1.0f, 1.0f,   1.0f, 1.0f,   0.0f, -1.0f, 0.0f,
//        -1.0f,-1.0f, 1.0f,   0.0f, 1.0f,   0.0f, -1.0f, 0.0f,
//
//        // top
//        -1.0f, 1.0f,-1.0f,   0.0f, 0.0f,   0.0f, 1.0f, 0.0f,
//        -1.0f, 1.0f, 1.0f,   0.0f, 1.0f,   0.0f, 1.0f, 0.0f,
//         1.0f, 1.0f,-1.0f,   1.0f, 0.0f,   0.0f, 1.0f, 0.0f,
//         1.0f, 1.0f,-1.0f,   1.0f, 0.0f,   0.0f, 1.0f, 0.0f,
//        -1.0f, 1.0f, 1.0f,   0.0f, 1.0f,   0.0f, 1.0f, 0.0f,
//         1.0f, 1.0f, 1.0f,   1.0f, 1.0f,   0.0f, 1.0f, 0.0f,*/
//
//        // front
//        -1.0f,-1.0f, 1.0f,   1.0f, 0.0f,   0.0f, 0.0f, 1.0f,
//         1.0f,-1.0f, 1.0f,   0.0f, 0.0f,   0.0f, 0.0f, 1.0f,
//        -1.0f, 1.0f, 1.0f,   1.0f, 1.0f,   0.0f, 0.0f, 1.0f,
//         1.0f,-1.0f, 1.0f,   0.0f, 0.0f,   0.0f, 0.0f, 1.0f,
//         1.0f, 1.0f, 1.0f,   0.0f, 1.0f,   0.0f, 0.0f, 1.0f,
//        -1.0f, 1.0f, 1.0f,   1.0f, 1.0f,   0.0f, 0.0f, 1.0f,
//
//        // back
//        /*-1.0f,-1.0f,-1.0f,   0.0f, 0.0f,   0.0f, 0.0f, -1.0f,
//        -1.0f, 1.0f,-1.0f,   0.0f, 1.0f,   0.0f, 0.0f, -1.0f,
//         1.0f,-1.0f,-1.0f,   1.0f, 0.0f,   0.0f, 0.0f, -1.0f,
//         1.0f,-1.0f,-1.0f,   1.0f, 0.0f,   0.0f, 0.0f, -1.0f,
//        -1.0f, 1.0f,-1.0f,   0.0f, 1.0f,   0.0f, 0.0f, -1.0f,
//         1.0f, 1.0f,-1.0f,   1.0f, 1.0f,   0.0f, 0.0f, -1.0f,*/
//
//        // left
//        /*-1.0f,-1.0f, 1.0f,   0.0f, 1.0f,   -1.0f, 0.0f, 0.0f,
//        -1.0f, 1.0f,-1.0f,   1.0f, 0.0f,   -1.0f, 0.0f, 0.0f,
//        -1.0f,-1.0f,-1.0f,   0.0f, 0.0f,   -1.0f, 0.0f, 0.0f,
//        -1.0f,-1.0f, 1.0f,   0.0f, 1.0f,   -1.0f, 0.0f, 0.0f,
//        -1.0f, 1.0f, 1.0f,   1.0f, 1.0f,   -1.0f, 0.0f, 0.0f,
//        -1.0f, 1.0f,-1.0f,   1.0f, 0.0f,   -1.0f, 0.0f, 0.0f,
//
//        // right
//         1.0f,-1.0f, 1.0f,   1.0f, 1.0f,   1.0f, 0.0f, 0.0f,
//         1.0f,-1.0f,-1.0f,   1.0f, 0.0f,   1.0f, 0.0f, 0.0f,
//         1.0f, 1.0f,-1.0f,   0.0f, 0.0f,   1.0f, 0.0f, 0.0f,
//         1.0f,-1.0f, 1.0f,   1.0f, 1.0f,   1.0f, 0.0f, 0.0f,
//         1.0f, 1.0f,-1.0f,   0.0f, 0.0f,   1.0f, 0.0f, 0.0f,
//         1.0f, 1.0f, 1.0f,   0.0f, 1.0f,   1.0f, 0.0f, 0.0f*/
//    };
    
    // **************** TERRAIN FROM HMAP ****************
    
    glBufferData(GL_ARRAY_BUFFER, asset.drawCount * valuesPerVertex * sizeof(GLfloat), vdata, GL_STATIC_DRAW);

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
    
    delete vdata;
}


// convenience function that returns a translation matrix
glm::mat4 translate(GLfloat x, GLfloat y, GLfloat z) {
    return glm::translate(glm::mat4(), glm::vec3(x,y,z));
}


// convenience function that returns a scaling matrix
glm::mat4 scale(GLfloat x, GLfloat y, GLfloat z) {
    return glm::scale(glm::mat4(), glm::vec3(x,y,z));
}

//static void BuildRangeModel(ModelInstance &instance) {
//    
//    // set all the elements of gWoodenCrate
//    instance.asset->shaders = LoadShaders("vertex-shader.txt", "fragment-shader.txt");
//    instance.asset->drawType = GL_TRIANGLES;
//    instance.asset->drawStart = 0;
//    instance.asset->drawCount = 6*2*3;
//    instance.asset->texture = LoadTexture("wooden-crate.jpg");
//    instance.asset->shininess = 80.0;
//    instance.asset->specularColor = glm::vec3(1.0f, 1.0f, 1.0f);
//    
//    // TODO build vertex data
//}

//create all the `instance` structs for the 3D scene, and add them to `gInstances`
//static void CreateInstances() {
//    ModelInstance dot;
//    dot.asset = &gWoodenCrate;
//    dot.transform = glm::mat4();
//    gInstances.push_back(dot);
//
//    ModelInstance i;
//    i.asset = &gWoodenCrate;
//    i.transform = translate(0,-4,0) * scale(1,2,1);
//    gInstances.push_back(i);
//
//    ModelInstance hLeft;
//    hLeft.asset = &gWoodenCrate;
//    hLeft.transform = translate(-8,0,0) * scale(1,6,1);
//    gInstances.push_back(hLeft);
//
//    ModelInstance hRight;
//    hRight.asset = &gWoodenCrate;
//    hRight.transform = translate(-4,0,0) * scale(1,6,1);
//    gInstances.push_back(hRight);
//
//    ModelInstance hMid;
//    hMid.asset = &gWoodenCrate;
//    hMid.transform = translate(-6,0,0) * scale(2,1,0.8f);
//    gInstances.push_back(hMid);
//}


//renders a single `ModelInstance` //TODO add camera as argument
static void RenderInstance(const ModelInstance& inst, tdogl::Camera& camera) {
    ModelAsset* asset = inst.asset;
    tdogl::Program* shaders = asset->shaders;

    //bind the shaders
    shaders->use();

    //set the shader uniforms
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
        
        
        if (i == 0 && !ONLY_LEFT_CAMERA)
            glViewport(0, SCREEN_SIZE.y/2, SCREEN_SIZE.x/2, SCREEN_SIZE.y);
        else if (!ONLY_LEFT_CAMERA)
            glViewport(SCREEN_SIZE.x/2, SCREEN_SIZE.y/2, SCREEN_SIZE.x, SCREEN_SIZE.y);
        else
            glViewport(0, 0, SCREEN_SIZE.x, SCREEN_SIZE.y);
        
        std::list<ModelInstance>::const_iterator it;
        for(it = gInstances.begin(); it != gInstances.end(); ++it){
            if (i==0 || ONLY_LEFT_CAMERA)
                RenderInstance(*it, gCamera);
            else
                RenderInstance(*it, gCamera2);
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
        gCamera.offsetPosition(dt * moveSpeed * -gCamera.forward());
    } else if(glfwGetKey('W')){
        gCamera.offsetPosition(dt * moveSpeed * gCamera.forward());
    }
    if(glfwGetKey('A')){
        gCamera.offsetPosition(dt * moveSpeed * -gCamera.right());
    } else if(glfwGetKey('D')){
        gCamera.offsetPosition(dt * moveSpeed * gCamera.right());
    }
    if(glfwGetKey('E')){
        gCamera.offsetPosition(dt * moveSpeed * -gCamera.up());
    } else if(glfwGetKey('Q')){
        gCamera.offsetPosition(dt * moveSpeed * gCamera.up());
    }
    
    // rotate the camera based on arrow keys
    const float rotSpeed = 45.0; //degrees per second
    if(glfwGetKey(GLFW_KEY_UP)){
        gCamera.offsetOrientation(dt * -rotSpeed, 0);
    } else if(glfwGetKey(GLFW_KEY_DOWN)){
        gCamera.offsetOrientation(dt * rotSpeed, 0);
    }
    if(glfwGetKey(GLFW_KEY_RIGHT)){
        gCamera.offsetOrientation(0, dt * rotSpeed);
    } else if(glfwGetKey(GLFW_KEY_LEFT)){
        gCamera.offsetOrientation(0, dt * -rotSpeed);
    }
    
    // zoom based on XZ keys
    const float zoomSpeed = 30.0; //degrees per second
    if(glfwGetKey('X')){
        float fieldOfView = gCamera.fieldOfView() - dt * zoomSpeed;
        if(fieldOfView < 5.0f) fieldOfView = 5.0f;
        if(fieldOfView > 130.0f) fieldOfView = 130.0f;
        gCamera.setFieldOfView(fieldOfView);
    } else if(glfwGetKey('Z')){
        float fieldOfView = gCamera.fieldOfView() + dt * zoomSpeed;
        if(fieldOfView < 5.0f) fieldOfView = 5.0f;
        if(fieldOfView > 130.0f) fieldOfView = 130.0f;
        gCamera.setFieldOfView(fieldOfView);
    }

    //move light
    if(glfwGetKey('1'))
        gLight.position = gCamera.position();

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
    glfwDisable(GLFW_MOUSE_CURSOR);
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
    
    // create all the instances in the 3D scene based on the gWoodenCrate asset
//    CreateInstances();
    
    //mouse cursor
    glfwEnable(GLFW_MOUSE_CURSOR);
    

    // setup gCamera (left camera)
    gCamera.setPosition(glm::vec3(0,4,15));
    gCamera.setViewportAspectRatio(SCREEN_SIZE.x / SCREEN_SIZE.y);
    gCamera.setNearAndFarPlanes(0.5f, 100.0f);
    gCamera.lookAt(glm::vec3(8,4,8));
    
    // setup gCamera2 (right camera)
    gCamera2.setPosition(glm::vec3(0,20,0));
    gCamera2.setViewportAspectRatio(SCREEN_SIZE.x / SCREEN_SIZE.y);
    gCamera2.setNearAndFarPlanes(0.5f, 100.0f);

    // setup gLight
    gLight.position = glm::vec3(0,6,0);
    gLight.intensities = glm::vec3(1,1,1); //white
    gLight.attenuation = 0.2f;
    gLight.ambientCoefficient = 0.05f;

    // run while the window is open
    double lastTime = glfwGetTime();
    while(glfwGetWindowParam(GLFW_OPENED)){
        // update the scene based on the time elapsed since last update
        double thisTime = glfwGetTime();
        Update((float)(thisTime - lastTime));
        lastTime = thisTime;

        
        //setup two viewports and draw one frame
        Render();

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
