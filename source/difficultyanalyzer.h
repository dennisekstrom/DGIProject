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
    
//    float difficulty;
    
//    vec3 prevTeeTerrainPos;
//    vec3 prevTargetTerrainPos;
    
    
    static float HeightDifficulty(const float &extraHeight) {
        return extraHeight;
    }
    static float SideCurveDifficulty(const float &curve) { // Curve is maximum distance from of a straight
        return curve;
    }

    static bool IntersectionBetweenPoints(const vec3 &start, const vec3 &end);
    static bool ClosestIntersection(vec3 start, vec3 dir,Intersection& closestIntersection);
    
    static bool PathIsClear(const vec3 &tee, const vec3 & p1, const vec3 &p2, const vec3 &target);
    
public:
    
//    DifficultyAnalyzer();
    
    static float CalculateDifficulty(const vec3 &tee, const vec3 &target);
    
//    void Update(const float &dt);
//    float GetDifficulty() { return difficulty; }
    
};
//extern DifficultyAnalyzer gDifficultyAnalyzer;

#endif /* defined(__DGIProject__difficultyanalyzer__) */
