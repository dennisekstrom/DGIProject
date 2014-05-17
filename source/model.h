//
//  model.h
//  DGIProject
//
//  Created by Dennis Ekström on 16/05/14.
//  Copyright (c) 2014 Dennis Ekström. All rights reserved.
//

#ifndef DGIProject_model_h
#define DGIProject_model_h

#include "tdogl/Program.h"
#include "tdogl/Texture.h"

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

#endif
