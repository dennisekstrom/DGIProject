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
struct Intersection {
    vec3 position;
    float distance;
    ModelAsset* asset;
};

class DifficultyAnalyzer {
    
private:
    
    static float HeightDifficulty(const float &extraHeight) {
        // ASSUMPTION: Hitting 10 meters further is as hard as hitting one meter higher
        return 10 * extraHeight;
    }
    static float CurveDifficulty(const float &curve) { // Curve is maximum distance from of a straight
        // ASSUMPTION: Hitting 10 meters further is as hard as hitting one meter curve
        return 10 * curve;
    }
    static float ComboDifficulty(const float &extraHeight, const float &curve) {
        // ASSUMPTION: Hitting height and curve difficulty may be added when combined
        return HeightDifficulty(extraHeight) + CurveDifficulty(curve);
    }
    

    static bool IntersectionBetweenPoints(const vec3 &start, const vec3 &end);
    static bool ClosestIntersection(vec3 start, vec3 dir,Intersection& closestIntersection);
    
    static bool PathIsClear(const vec3 &tee, const vec3 & p1, const vec3 &p2, const vec3 &target);
    
public:
    
//    static const float SCALE_FACTOR; // To be able to test with smaller terrain but with realistic parameters
    static const float IMPOSSIBLE; // The maximum level of difficulty
    
    static float CalculateDifficulty(const vec3 &tee, const vec3 &target, vec3 &p1_adjusted, vec3 &p2_adjusted, float &distance);
    
};

#endif /* defined(__DGIProject__difficultyanalyzer__) */
