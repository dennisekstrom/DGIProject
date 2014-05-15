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
#include <GLUT/glut.h>

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
#include "rangedrawer.h"
#include "rangetweakbar.h"
#include "text.h"
#include "callbacks.h"

#define SCREEN_W                1024
#define SCREEN_H                768

#define ORTHO_RELATIVE_MARGIN   0.1
#define SKYBOX_SCALE 1

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
    tdogl::Texture* skyboxTextures[6];
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
 Defines intersection with triangle
 */
struct Intersection
{
    vec3 position;
    float distance;
    ModelAsset* asset;
};

// globals
bool gLeftCameraUseColor = false;
bool gRightCameraUseColor = true;
bool gLeftCameraFullscreen = false;
bool gLockCameraOnHole = true;

bool gMouseButtonDown = false;
int gPrevCursorPosX, gPrevCursorPosY;

int gWindowId;

tdogl::Camera gCamera1; //Left camera
tdogl::Camera gCamera2; //Right camera, overview

ModelAsset gTerrainModelAsset;
ModelAsset gSkyboxAsset;
ModelInstance gSkyBoxInstance;
std::list<ModelInstance> gInstances;
GLfloat gDegreesRotated = 0.0f;
//vec3 gCurrentHolePos;
//vec3 gCurrentMatPos;
//bool gHolePositionSet = false;
//bool gMatPositionSet = false;

glm::vec3 gLightPosition;
glm::vec3 gLightIntensities; //a.k.a. the color of the light
float gLightAttenuation;
float gLightAmbientCoefficient;

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

static bool ClosestIntersection(vec3 start, vec3 dir,Intersection& closestIntersection) {
    
    float closest_t = std::numeric_limits<float>::max();
    float closest_index = -1;
    
    // iterate through all terrain vertices and check for intersection
    // X_INTERVAL * Y_INTERVAL * FLOATS_PER_TRIANGLE * 2 - 36
    for (int i = 0; i < X_INTERVAL * Y_INTERVAL * FLOATS_PER_TRIANGLE * 2 - 36 ; i++) {
        vec3 v0 = vec3(gTerrain.vertexData[i], gTerrain.vertexData[i+1], gTerrain.vertexData[i+2]);
        vec3 v1 = vec3(gTerrain.vertexData[i+12],gTerrain.vertexData[i+13],gTerrain.vertexData[i+14]);
        vec3 v2 = vec3(gTerrain.vertexData[i+24],gTerrain.vertexData[i+25],gTerrain.vertexData[i+26]);
        i = i+36;
        //cout << sizeof(gTerrain.vertexData) << endl;
        //cout << i << endl;
        vec3 e1 = v1 - v0;
        vec3 e2 = v2 - v0;
        vec3 b = start - v0;
        mat3 A( -dir, e1, e2 );
        vec3 x = glm::inverse( A ) * b; // x = (t, u, v)
        
        if (x.x < closest_t && x.x > 0 && x.y >= 0 && x.z >= 0 && x.y + x.z <= 1) {
            closest_t = x.x;
            closest_index = i;
        }
    }
    
    if (closest_index >= 0) {
        closestIntersection.position = start + closest_t * dir;
        closestIntersection.distance = closest_t;
        //closestIntersection.triangleIndex = closest_index;
        return true;
    }
    return false;
}

// initializes the skybox
static void initSkyBox() {
    
    gSkyboxAsset.shaders = LoadShaders("vertex-shader.txt", "fragment-shader.txt");
    gSkyboxAsset.drawType = GL_TRIANGLES;
    gSkyboxAsset.drawStart = 0;
    gSkyboxAsset.drawCount = 6*2*3;
    gSkyboxAsset.skyboxTextures[0] = LoadTexture("Up.jpg");
    gSkyboxAsset.skyboxTextures[1] = LoadTexture("Up.jpg");
    gSkyboxAsset.skyboxTextures[2] = LoadTexture("Back.jpg");
    gSkyboxAsset.skyboxTextures[3] = LoadTexture("Front.jpg");
    gSkyboxAsset.skyboxTextures[4] = LoadTexture("Left.jpg");
    gSkyboxAsset.skyboxTextures[5] = LoadTexture("Right.jpg");
    
    glGenBuffers(1, &gSkyboxAsset.vbo);
    glGenVertexArrays(1, &gSkyboxAsset.vao);
    
    // bind the VAO
    glBindVertexArray(gSkyboxAsset.vao);
    
    // bind the VBO
    glBindBuffer(GL_ARRAY_BUFFER, gSkyboxAsset.vbo);
    
    // Make a cube out of triangles (two triangles per side)
    GLfloat vertexData[] = {
        //  X     Y     Z       U     V
        // bottom
        -1.0f,-1.0f,-1.0f,   0.0f, 0.0f,
        1.0f,-1.0f,-1.0f,   1.0f, 0.0f,
        -1.0f,-1.0f, 1.0f,   0.0f, 1.0f,
        1.0f,-1.0f,-1.0f,   1.0f, 0.0f,
        1.0f,-1.0f, 1.0f,   1.0f, 1.0f,
        -1.0f,-1.0f, 1.0f,   0.0f, 1.0f,
        
        // top
        -1.0f, 1.0f,-1.0f,   0.0f, 0.0f,
        -1.0f, 1.0f, 1.0f,   0.0f, 1.0f,
        1.0f, 1.0f,-1.0f,   1.0f, 0.0f,
        1.0f, 1.0f,-1.0f,   1.0f, 0.0f,
        -1.0f, 1.0f, 1.0f,   0.0f, 1.0f,
        1.0f, 1.0f, 1.0f,   1.0f, 1.0f,
        
        // front
        -1.0f,-1.0f, 1.0f,   1.0f, 0.0f,
        1.0f,-1.0f, 1.0f,   0.0f, 0.0f,
        -1.0f, 1.0f, 1.0f,   1.0f, 1.0f,
        1.0f,-1.0f, 1.0f,   0.0f, 0.0f,
        1.0f, 1.0f, 1.0f,   0.0f, 1.0f,
        -1.0f, 1.0f, 1.0f,   1.0f, 1.0f,
        
        // back
        -1.0f,-1.0f,-1.0f,   0.0f, 0.0f,
        -1.0f, 1.0f,-1.0f,   0.0f, 1.0f,
        1.0f,-1.0f,-1.0f,   1.0f, 0.0f,
        1.0f,-1.0f,-1.0f,   1.0f, 0.0f,
        -1.0f, 1.0f,-1.0f,   0.0f, 1.0f,
        1.0f, 1.0f,-1.0f,   1.0f, 1.0f,
        
        // left
        -1.0f,-1.0f, 1.0f,   0.0f, 1.0f,
        -1.0f, 1.0f,-1.0f,   1.0f, 0.0f,
        -1.0f,-1.0f,-1.0f,   0.0f, 0.0f,
        -1.0f,-1.0f, 1.0f,   0.0f, 1.0f,
        -1.0f, 1.0f, 1.0f,   1.0f, 1.0f,
        -1.0f, 1.0f,-1.0f,   1.0f, 0.0f,
        
        // right
        1.0f,-1.0f, 1.0f,   1.0f, 1.0f,
        1.0f,-1.0f,-1.0f,   1.0f, 0.0f,
        1.0f, 1.0f,-1.0f,   0.0f, 0.0f,
        1.0f,-1.0f, 1.0f,   1.0f, 1.0f,
        1.0f, 1.0f,-1.0f,   0.0f, 0.0f,
        1.0f, 1.0f, 1.0f,   0.0f, 1.0f
    };
    
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertexData), vertexData, GL_STATIC_DRAW);
    
    // connect the xyz to the "vert" attribute of the vertex shader
    glEnableVertexAttribArray(gSkyboxAsset.shaders->attrib("vert"));
    glVertexAttribPointer(gSkyboxAsset.shaders->attrib("vert"), 3, GL_FLOAT, GL_FALSE, 5*sizeof(GLfloat), NULL);
    
    // connect the uv coords to the "vertTexCoord" attribute of the vertex shader
    glEnableVertexAttribArray(gSkyboxAsset.shaders->attrib("vertTexCoord"));
    glVertexAttribPointer(gSkyboxAsset.shaders->attrib("vertTexCoord"), 2, GL_FLOAT, GL_TRUE,  5*sizeof(GLfloat), (const GLvoid*)(3 * sizeof(GLfloat)));
    
    // unbind the VAO
    glBindVertexArray(0);
    
    // setup model instance of skybox
    ModelInstance instance;
    instance.asset = &gSkyboxAsset;
    // translate and scale skybox
    instance.transform = glm::translate(glm::mat4(), glm::vec3(0,0,0)) *
    glm::scale(glm::mat4(), glm::vec3(SKYBOX_SCALE,SKYBOX_SCALE,SKYBOX_SCALE));
    
    gSkyBoxInstance = instance;
}

/*static void SendDataToBuffer(GLfloat* vdata, ModelAsset &asset, int floatsPerVertex) {
 // bind the VAO
 glBindVertexArray(asset.vao);
 
 // bind the VBO
 glBindBuffer(GL_ARRAY_BUFFER, asset.vbo);
 
 // write the data
 glBufferData(GL_ARRAY_BUFFER, asset.drawCount * floatsPerVertex * sizeof(GLfloat), vdata, GL_STATIC_DRAW);
 
 // unbind the VAO
 glBindVertexArray(0);
 }*/

static void UpdateUsingMapBuffer(const ModelAsset &asset, GLfloat* data, vector<int> &indices, const int &floatsPerVertex) {
    
    // bind the VBO
    glBindBuffer(GL_ARRAY_BUFFER, asset.vbo);
    GLfloat* buf = (GLfloat*) glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
    
    for (int &idx : indices)
        memcpy(buf + idx, data + idx, floatsPerVertex * sizeof(GLfloat));
    
    // bind the VBO
    glUnmapBuffer(GL_ARRAY_BUFFER);
}

// initialises the gWoodenCrate global [TODO: WRONG COMMENT]
static void LoadAsset(ModelAsset &asset, const int &floatsPerVertex) {
    // set all the elements of gWoodenCrate [TODO: WRONG COMMENT]
    asset.shaders = LoadShaders("vertex-shader.txt", "fragment-shader.txt");
    asset.drawType = GL_TRIANGLES;
    asset.drawStart = 0;
    asset.drawCount = (X_INTERVAL - 1) * (Y_INTERVAL - 1) * 6;
    asset.texture = LoadTexture("grass.png");
    asset.shininess = 80.0;
    asset.specularColor = glm::vec3(1.0f, 1.0f, 1.0f);
    
    glGenBuffers(1, &asset.vbo);
    glGenVertexArrays(1, &asset.vao);
    
    // bind the VAO
    glBindVertexArray(asset.vao);
    
    // bind the VBO
    glBindBuffer(GL_ARRAY_BUFFER, asset.vbo);
    
    // write initial data
    glBufferData(GL_ARRAY_BUFFER, asset.drawCount * floatsPerVertex*sizeof(GLfloat), gTerrain.vertexData, GL_DYNAMIC_DRAW);
    
    // connect the xyz to the "vert" attribute of the vertex shader
    glEnableVertexAttribArray(asset.shaders->attrib("vert"));
    glVertexAttribPointer(asset.shaders->attrib("vert"), 3, GL_FLOAT, GL_FALSE, floatsPerVertex*sizeof(GLfloat), NULL);
    
    // connect the uv coords to the "vertTexCoord" attribute of the vertex shader
    glEnableVertexAttribArray(asset.shaders->attrib("vertTexCoord"));
    glVertexAttribPointer(asset.shaders->attrib("vertTexCoord"), 2, GL_FLOAT, GL_FALSE, floatsPerVertex*sizeof(GLfloat), (const GLvoid*)(3 * sizeof(GLfloat)));
    
    // connect the normal to the "vertNormal" attribute of the vertex shader
    glEnableVertexAttribArray(asset.shaders->attrib("vertNormal"));
    glVertexAttribPointer(asset.shaders->attrib("vertNormal"), 3, GL_FLOAT, GL_TRUE, floatsPerVertex*sizeof(GLfloat), (const GLvoid*)(5 * sizeof(GLfloat)));
    
    // connect the normal to the "vertNormal" attribute of the vertex shader
    glEnableVertexAttribArray(asset.shaders->attrib("vertColor"));
    glVertexAttribPointer(asset.shaders->attrib("vertColor"), 4, GL_FLOAT, GL_FALSE,  floatsPerVertex*sizeof(GLfloat), (const GLvoid*)(8 * sizeof(GLfloat)));
    
    // unbind the VAO
    glBindVertexArray(0);
}


// convenience function that returns a translation matrix
glm::mat4 translate(GLfloat x, GLfloat y, GLfloat z) {
    return glm::translate(glm::mat4(), glm::vec3(x,y,z));
}


// convenience function that returns a scaling matrix
glm::mat4 scale(GLfloat x, GLfloat y, GLfloat z) {
    return glm::scale(glm::mat4(), glm::vec3(x,y,z));
}

static void RenderSkyBox() {
    
    gSkyBoxInstance.transform = glm::translate(glm::mat4(), gCamera1.position()) *
    glm::scale(glm::mat4(), glm::vec3(SKYBOX_SCALE,SKYBOX_SCALE,SKYBOX_SCALE));
    
    //glDisable(GL_DEPTH_TEST);
    ModelAsset* asset = &gSkyboxAsset;
    tdogl::Program* shaders = asset->shaders;
    
    //bind the shaders
    shaders->use();
    
    //set the shader uniforms
    
    shaders->setUniform("camera", gCamera1.matrix());
    shaders->setUniform("useColor", gLeftCameraUseColor);
    shaders->setUniform("monotoneLight", false);
    
    shaders->setUniform("model", gSkyBoxInstance.transform);
    shaders->setUniform("materialTex", 0); //set to 0 because the texture will be bound to GL_TEXTURE0
    shaders->setUniform("light.position", gLightPosition);
    shaders->setUniform("light.intensities", gLightIntensities);
    //    shaders->setUniform("light.attenuation", gLightAttenuation);
    shaders->setUniform("light.ambientCoefficient", gLightAmbientCoefficient*15);
    shaders->setUniform("cameraPosition", gCamera1.position());
    
    for (int i = 1; i < 6; i++) { //TODO set i=0 to also render the bottom skybox texture
        //bind the texture
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, asset->skyboxTextures[i]->object());
        //
        //    //bind VAO and draw
        glBindVertexArray(asset->vao);
        glDrawArrays(asset->drawType, i*6, 6);
        //
        //    //unbind everything
        glBindVertexArray(0);
        glBindTexture(GL_TEXTURE_2D, 0);
        
    }
    
    shaders->stopUsing();
}

//renders a single `ModelInstance`
static void RenderInstance(const ModelInstance& inst, tdogl::Camera& camera, bool ortho) {
    ModelAsset* asset = inst.asset;
    tdogl::Program* shaders = asset->shaders;
    
    //bind the shaders
    shaders->use();
    
    //set the shader uniforms
    if (ortho) {
        shaders->setUniform("camera", camera.orthoMatrix());
        shaders->setUniform("useColor", gRightCameraUseColor);
        shaders->setUniform("monotoneLight", gRightCameraUseColor);
    } else {
        shaders->setUniform("camera", camera.matrix());
        shaders->setUniform("useColor", gLeftCameraUseColor);
        shaders->setUniform("monotoneLight", false);
    }
    shaders->setUniform("model", inst.transform);
    shaders->setUniform("materialTex", 0); //set to 0 because the texture will be bound to GL_TEXTURE0
    //    shaders->setUniform("materialShininess", asset->shininess);
    //    shaders->setUniform("materialSpecularColor", asset->specularColor);
    shaders->setUniform("light.position", gLightPosition);
    shaders->setUniform("light.intensities", gLightIntensities);
    shaders->setUniform("light.attenuation", gLightAttenuation);
    shaders->setUniform("light.ambientCoefficient", gLightAmbientCoefficient);
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
        
        // Viewports 1:1 (two screens) or 2:1 (left camera fullscreen)
        if (i == 0 && !gLeftCameraFullscreen)
            glViewport(0, 0, SCREEN_W/2, SCREEN_W/2);
        else if (!gLeftCameraFullscreen)
            glViewport(SCREEN_W/2, 0, SCREEN_W/2, SCREEN_W/2);
        else
            glViewport(0, 0, SCREEN_W, SCREEN_W/2);
        
        std::list<ModelInstance>::const_iterator it;
        for(it = gInstances.begin(); it != gInstances.end(); ++it){
            if (i == 0) {
                // render skybox, should always be rendered behind everything else
                glDisable(GL_DEPTH_TEST);
                RenderSkyBox();
                glEnable(GL_DEPTH_TEST);
                
            }
            if (i == 0 || gLeftCameraFullscreen) {
                RenderInstance(*it, gCamera1, false);
                //                drawText("DGI Project Alpha", 0, 0, 30);
            } else {
                RenderInstance(*it, gCamera2, true); // Render second viewport with 2D projection matrix
            }
        }
    }
        
    // draw the tweakbar
    gTweakBar.Draw();
    
    // swap the display buffers (displays what was just drawn)
    glutSwapBuffers();
}

static void Quit() {
    glutDestroyWindow(gWindowId);
    TwTerminate();
    exit(0);
}

static void TakeKeyAction(const float &dt) {
    if (keys[0x1b]) // Escape
        Quit();
    
    // move position of camera based on WASD keys, and QE keys for up and down
    const float moveSpeed = gShiftDown ? 50.0 : 10.0; //units per second
    if (keys['S'])
        gCamera1.offsetPosition(dt * moveSpeed * -gCamera1.forward());
    if (keys['W'])
        gCamera1.offsetPosition(dt * moveSpeed * gCamera1.forward());
    if (keys['A'])
        gCamera1.offsetPosition(dt * moveSpeed * -gCamera1.right());
    if (keys['D'])
        gCamera1.offsetPosition(dt * moveSpeed * gCamera1.right());
    if (keys['E'])
        gCamera1.offsetPosition(dt * moveSpeed * -gCamera1.up());
    if (keys['Q'])
        gCamera1.offsetPosition(dt * moveSpeed * gCamera1.up());
    
    // rotate the camera based on arrow keys
    const float rotSpeed = 45.0; //degrees per second
    if (special[GLUT_KEY_UP])
        gCamera1.offsetOrientation(dt * -rotSpeed, 0);
    if (special[GLUT_KEY_DOWN])
        gCamera1.offsetOrientation(dt * rotSpeed, 0);
    if (special[GLUT_KEY_RIGHT])
        gCamera1.offsetOrientation(0, dt * rotSpeed);
    if (special[GLUT_KEY_LEFT])
        gCamera1.offsetOrientation(0, dt * -rotSpeed);
}

// update the scene based on the time elapsed since last update
static void Update(const float &dt) {
    
    // Act on key events
    TakeKeyAction(dt);
    
    // Terrain needs update if control point changed
    if (gTerrain.ControlPointChanged())
        gTerrain.UpdateAll();
    
    // Adjust to terrain and marking changes
    if (gTerrain.ControlPointChanged() || gTerrain.VertexChanged() || gRangeDrawer.MarkChanged()) {
        gRangeDrawer.MarkTerrain();
        UpdateUsingMapBuffer(gTerrainModelAsset, gTerrain.vertexData, gTerrain.changedVertexIndices, FLOATS_PER_VERTEX);
        gTerrain.changedVertexIndices.clear();
        gRangeDrawer.ResetMarkChanged();
    }
    
    //Mouse click
    if(gMouseBtnDown) {
        
        if (!gLeftCameraFullscreen && gMouseX > SCREEN_W/2) { // right viewport
            
            float x = gMouseX - SCREEN_W / 2, y = SCREEN_H - gMouseY;
            float screen_side_px = SCREEN_W / 2;
            float terrain_side_px = (screen_side_px / (1 + 2*ORTHO_RELATIVE_MARGIN));
            float margin_px = (screen_side_px - terrain_side_px) / 2;
            
            if (x < margin_px)                  { x = margin_px; };
            if (x > screen_side_px - margin_px) { x = screen_side_px - margin_px; }
            if (y < margin_px)                  { y = margin_px; };
            if (y > screen_side_px - margin_px) { y = screen_side_px - margin_px; }
            
            x -= margin_px;
            y -= margin_px;
            
            float terrain_x = TERRAIN_WIDTH * x / terrain_side_px;
            float terrain_y = TERRAIN_DEPTH * y / terrain_side_px;

            gRangeDrawer.TerrainCoordClicked(terrain_x, terrain_y, gShiftDown);
            
        } /*else { // left viewport
           
           glfwDisable(GLFW_MOUSE_CURSOR);
           
           //rotate camera based on mouse movement
           const float mouseSensitivity = 0.1f;
           int mouseX, mouseY;
           glfwGetMousePos(&mouseX, &mouseY);
           
           if (!gMouseButtonDown) {
           gPrevCursorPosX = mouseX;
           gPrevCursorPosY = mouseY;
           glfwSetMousePos(0, 0);
           mouseX = 0; mouseY = 0;
           }
           gMouseButtonDown = true;
           
           gCamera1.offsetOrientation(mouseSensitivity * mouseY, mouseSensitivity * mouseX);
           glfwSetMousePos(0, 0); //reset the mouse, so it doesn't go out of the window
           */
    }
    
    /*
     } else if (gMouseButtonDown) {
     // Remember mouse position before mouse orientation
     glfwEnable(GLFW_MOUSE_CURSOR);
     glfwSetMousePos(gPrevCursorPosX, gPrevCursorPosY);
     gMouseButtonDown = false;
     }
     */
    
    if(!gMouseBtnDown) {
        // For marking in camera 2
        gRangeDrawer.MouseReleased();
    }
    
    /*if (gHolePositionSet) {
    // lock camera on hole from mat, if both points are set
    if (gLockCameraOnHole && gMatPositionSet && gHolePositionSet) {
        float h_hole = gRangeDrawer.GetHeight(gCurrentHolePos.x, gCurrentHolePos.z);
        float h_mat = gRangeDrawer.GetHeight(gCurrentMatPos.x, gCurrentMatPos.z);
        gCurrentHolePos.y = h_hole;
        gCurrentMatPos.y = h_mat;
        
        vec3 newCameraPos = vec3(gCurrentMatPos.x, gCurrentMatPos.y+1, gCurrentMatPos.z);
        gCamera1.setPosition(newCameraPos);
        gCamera1.lookAt(gCurrentHolePos);
     
        Intersection inter;
        vec3 flagPosition = vec3(gCurrentHolePos.x, gCurrentHolePos.y+2, gCurrentHolePos.z);
        vec3 matPosition = vec3(gCurrentMatPos.x, gCurrentMatPos.y+1, gCurrentMatPos.z);
        bool intersected = ClosestIntersection(matPosition, flagPosition, inter);
        cout << "Intersected: " << intersected << endl;
        cout << "Position x,y,z: " << inter.position.x << " "  << inter.position.y << " " << inter.position.z << endl;
        cout << "Distance: " << inter.distance << endl;
        
    }*/

    //FOR TESTING camera raytracing toward the hole
    
//    if (gHolePositionSet) {
//        Intersection inter;
//        bool intersected = ClosestIntersection(gCamera1.position(), gCurrentHolePos, inter);
//        if (intersected) {
//            cout << "Intersected! " << intersected << endl;
//            cout << "Position x,y,z: " << inter.position.x << " "  << inter.position.y << " " << inter.position.z << endl;
//            cout << "Distance: " << inter.distance << endl;
//            cout << gMouseX << " " << gMouseY << endl;
//        }
//    }

    
    
}

double lastTime = 0;
static void Display() {
    
    double thisTime = double(glutGet(GLUT_ELAPSED_TIME)) / 1000;
    float dt = thisTime - lastTime;
    lastTime = thisTime;
    //cout << "render time: " << round(dt * 1000) << " ms" << endl;
    
    // take tweakbar action
    gTweakBar.Update(dt);
    
    // update the scene based on the time elapsed since last update
    Update(dt);
    
    // render the scene
    Render();
    
    // check for errors
    GLenum error;
    if((error = glGetError()) != GL_NO_ERROR)
        std::cerr << "OpenGL Error " << error << ": " << (const char*)gluErrorString(error) << std::endl;
    
    glutPostRedisplay();
}

// the program starts here
void AppMain(int argc, char *argv[]) {
    
    // initialise GLUT and create window
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_3_2_CORE_PROFILE | GLUT_RGB | GLUT_SINGLE | GLUT_DEPTH);
    glutInitWindowSize(SCREEN_W, SCREEN_H);
    glutInitWindowPosition((glutGet(GLUT_SCREEN_WIDTH) - SCREEN_W) / 2, (glutGet(GLUT_SCREEN_HEIGHT) - SCREEN_H) / 2);
    gWindowId = glutCreateWindow("DGI Project");
    
    // initialise GLEW
    glewExperimental = GL_TRUE; // stops glew crashing on OSX :-/
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
    
    // initialise the terrain asset
    LoadAsset(gTerrainModelAsset, FLOATS_PER_VERTEX);
    ModelInstance instance;
    instance.asset = &gTerrainModelAsset;
    gInstances.push_back(instance);
    
    // setup gCamera1 (left camera)
    gCamera1.setPosition(glm::vec3(TERRAIN_WIDTH / 2, 10, 0));
    gCamera1.setViewportAspectRatio(gLeftCameraFullscreen ? 2 : 1);
    gCamera1.setNearAndFarPlanes(0.5f, 1000.0f);
    gCamera1.lookAt(glm::vec3(TERRAIN_WIDTH / 2, 0, -TERRAIN_DEPTH / 2));
    
    // setup gCamera2 (right camera)
    gCamera2.setPosition(glm::vec3(TERRAIN_WIDTH / 2, 20, -TERRAIN_DEPTH / 2));
    gCamera2.setOrtho(-TERRAIN_WIDTH / 2 - TERRAIN_WIDTH * ORTHO_RELATIVE_MARGIN,
                      TERRAIN_WIDTH / 2 + TERRAIN_WIDTH * ORTHO_RELATIVE_MARGIN,
                      -TERRAIN_DEPTH / 2 - TERRAIN_WIDTH * ORTHO_RELATIVE_MARGIN,
                      TERRAIN_DEPTH / 2 + TERRAIN_WIDTH * ORTHO_RELATIVE_MARGIN,
                      0.5f,
                      100.0f);
    gCamera2.SetAboveMode(true);
    
    // setup gLight
    gLightPosition = glm::vec3(TERRAIN_WIDTH / 2, 10, TERRAIN_DEPTH / 2);
    gLightIntensities = glm::vec3(1,1,1); //white
    gLightAttenuation = 0.0001f;
    gLightAmbientCoefficient = 0.080f;
    
    // setup skybox
    initSkyBox();
    
    // glut settings
    //    glutIgnoreKeyRepeat(1);
    
    // glut callbacks
    glutDisplayFunc(Display);
    glutKeyboardFunc(CBKey);
    glutKeyboardUpFunc(CBKeyUp);
    glutSpecialFunc(CBSpecial);
    glutSpecialUpFunc(CBSpecialUp);
    glutMouseFunc(CBMouse);
    glutMotionFunc(CBMotion);
    atexit(Quit);
    
    // setup tweak bar
    gTweakBar.Init(SCREEN_W, SCREEN_H);
    
    // start main loop
    glutMainLoop();
}


int main(int argc, char *argv[]) {
    try {
        AppMain(argc, argv);
    } catch (const std::exception& e){
        std::cerr << "ERROR: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    
    return EXIT_SUCCESS;
}
