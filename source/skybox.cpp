//
//  skybox.cpp
//  DGIProject
//
//  Created by Tobias Wikström on 11/05/14.
//  Copyright (c) 2014 Tobias Wikström. All rights reserved.
//

//#include "skybox.h"
//
//
//Skybox::Skybox()
//{
//	//initial operations
//	gltMakeCube(skyCube, 20.0f);
//	
//	filenames[0]="Textures/Skybox/pos_x.tga";
//	filenames[1]="Textures/Skybox/neg_x.tga";
//	filenames[2]="Textures/Skybox/pos_y.tga";
//	filenames[3]="Textures/Skybox/neg_y.tga";
//	filenames[4]="Textures/Skybox/pos_z.tga";
//	filenames[5]="Textures/Skybox/neg_z.tga";
//    
//	//load textures using loaders library.
//    //SkyBox::cubemap(filenames,&skyTexture);
//    
//	//load&compile shaders
//	skyboxShader = gltLoadShaderPairWithAttributes("Shaders/texonly.vert", "Shaders/texonly.frag", 2,
//                                                   GLT_ATTRIBUTE_VERTEX, "vVertex",
//                                                   GLT_ATTRIBUTE_NORMAL, "vNormal");
//	
//	//get location of combined modelview/projecton matrix
//	locMVPSkyBox = glGetUniformLocation(skyboxShader, "mvpMatrix");
//    
//}
//
//void Skybox::draw(GLGeometryTransform *pGLGT){
//	glDisable(GL_DEPTH_TEST);
//	glUseProgram(skyboxShader);
//	glUniformMatrix4fv(locMVPSkyBox, 1, GL_FALSE, pGLGT->GetModelViewProjectionMatrix());
//	skyCube.Draw();
//	glEnable(GL_DEPTH_TEST);
//}
//
//Skybox::~Skybox()
//{
//	glDeleteTextures(1, &skyTexture);
//}
//
//void Skybox::cubemap(char **files,GLuint *cubeTexture){
//    
//    GLenum  cube[6] = {  GL_TEXTURE_CUBE_MAP_POSITIVE_X,
//        GL_TEXTURE_CUBE_MAP_NEGATIVE_X,
//        GL_TEXTURE_CUBE_MAP_POSITIVE_Y,
//        GL_TEXTURE_CUBE_MAP_NEGATIVE_Y,
//        GL_TEXTURE_CUBE_MAP_POSITIVE_Z,
//        GL_TEXTURE_CUBE_MAP_NEGATIVE_Z };
//
//    GLbyte *pBytes;
//    GLint iWidth, iHeight, iComponents;
//    GLenum eFormat;
//    int i;
//    
//    // Cull backs of polygons
//    glCullFace(GL_BACK);
//    glFrontFace(GL_CCW);
//    glEnable(GL_DEPTH_TEST);
//    
//    glGenTextures(1, cubeTexture);
//    glBindTexture(GL_TEXTURE_CUBE_MAP, *cubeTexture);
//    
//    // Set up texture maps
//    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
//    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
//    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
//    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
//    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
//    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
//    
//    // Load Cube Map images
//    for(i = 0; i < 6; i++)
//    {
//        // Load this texture map
//        pBytes = gltReadTGABits(files[i], &iWidth, &iHeight, &iComponents, &eFormat);
//        glTexImage2D(cube[i], 0, iComponents, iWidth, iHeight, 0, eFormat, GL_UNSIGNED_BYTE, pBytes);
//        if(pBytes==NULL){
//            cerr << "Something went wrong loading texture: " << files[i] << endl;
//            MessageBox(NULL, "Skybox textures did not load correctly!\nGame's a bogey!", "Fail!", MB_OK);
//            glutLeaveMainLoop();
//        }
//        free(pBytes);
//    }
//    glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
//    
//    cout << "Skybox textures loaded." << endl;
//}