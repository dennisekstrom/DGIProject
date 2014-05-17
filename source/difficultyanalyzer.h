//
//  difficultyanalyzer.h
//  DGIProject
//
//  Created by Dennis Ekström on 16/05/14.
//  Copyright (c) 2014 Dennis Ekström. All rights reserved.
//

#ifndef __DGIProject__difficultyanalyzer__
#define __DGIProject__difficultyanalyzer__

#include <iostream>
#include <glm/glm.hpp>
#include "model.h"

using namespace glm;


/*
 Defines an intersection with a triangle
 */
struct Intersection
{
    vec3 position;
    float distance;
    ModelAsset* asset;
};

class DifficultyAnalyzer {
    
private:
    
    bool ClosestIntersection(vec3 start, vec3 dir,Intersection& closestIntersection);
    
public:
    
    void Update(const float &dt);
    
};
extern DifficultyAnalyzer gDifficultyAnalyzer;

#endif /* defined(__DGIProject__difficultyanalyzer__) */
