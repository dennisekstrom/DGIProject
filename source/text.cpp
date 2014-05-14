//
//  text.cpp
//  DGIProject
//
//  Created by Tobias Wikström on 13/05/14.
//  Copyright (c) 2014 Tobias Wikström. All rights reserved.
//


#include "text.h"
#include <iostream>

GLuint program;
GLuint textureID;
GLuint texture;
GLuint VertexArrayID;
int nbFrames;

int initText( void )
{

	glGenVertexArrays(1, &VertexArrayID);
	glBindVertexArray(VertexArrayID);

	// Create and compile our GLSL program from the shaders
    std::string file = __FILE__;
    file = file.erase(file.size()-9, file.size()-1);
    std::string file1 = file + "/text_utils/StandardShading.vertexshader";
    std::string file2 = file + "/text_utils/StandardShading.fragmentshader";
    program = LoadShaders2(file1.c_str(), file2.c_str());

	// Load the texture
    file = __FILE__;
    file = file.erase(file.size()-9, file.size()-1);
    file = file + "/text_utils/uvmap.DDS";
	texture = loadDDS(file.c_str());
	
	// Get a handle for our "myTextureSampler" uniform
    textureID = glGetUniformLocation(program, "myTextureSampler");
    

	// Read our .obj file
	std::vector<glm::vec3> vertices;
	std::vector<glm::vec2> uvs;
	std::vector<glm::vec3> normals;

	// Get a handle for our "LightPosition" uniform
	glUseProgram(program);

	// Initialize our little text library with the Holstein font
    file = __FILE__;
    file = file.erase(file.size()-9, file.size()-1);
    file = file + "/text_utils/Holstein.DDS";
    
	initText2D(file.c_str());
    // For speed computation
    nbFrames = 0;
    
    
    return 1;
}

void drawText(const std::string in_text, int x, int y, int size) {
    glBindVertexArray(VertexArrayID);

    // Use our shader
    glUseProgram(program);
    
    // Bind our texture in Texture Unit 0
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, 0);
    // Set our "myTextureSampler" sampler to user Texture Unit 0
    glUniform1i(textureID, texture);
    
    
    char text[512];
    strcpy(text, in_text.c_str());
    
//    printText2D(text, x, y, size);
    
}

