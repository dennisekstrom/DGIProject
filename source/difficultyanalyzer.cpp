//
//  difficultyanalyzer.cpp
//  DGIProject
//
//  Created by Dennis Ekström on 16/05/14.
//  Copyright (c) 2014 Dennis Ekström. All rights reserved.
//

#include "difficultyanalyzer.h"
#include "rangeterrain.h"
#include "rangedrawer.h"

//DifficultyAnalyzer gDifficultyAnalyzer;

const float p1_relative = 0.5;
const float p2_relative = 0.7;
const float H = 2;

//const float extraHeightSteps = 1;
//const float curveSteps = 1;
//const int maxExtraHeightSteps = 20;
//const int maxCurveSteps = 20;
//const int maxSteps = 20;
const int maxHeightSteps = 30;
const int maxCurveSteps = 30;
const int maxComboSteps = 20;
const float heightPerStep = 1;
const float curvePerStep = 1;

//DifficultyAnalyzer::DifficultyAnalyzer() {
//    difficulty = 0;
//}

bool DifficultyAnalyzer::PathIsClear(const vec3 &tee, const vec3 & p1, const vec3 &p2, const vec3 &target) {
    return !IntersectionBetweenPoints(tee, p1) && !IntersectionBetweenPoints(p1, p2) && !IntersectionBetweenPoints(p2, target);
}

//void DifficultyAnalyzer::Update(const float &dt) {
//    
//    // Don't do any new calculations if nothing changed
//    if (prevTeeTerrainPos != gRangeDrawer.TeeTerrainPos() || prevTargetTerrainPos != gRangeDrawer.TargetTerrainPos()) {
//        difficulty = CalculateDifficulty(gRangeDrawer.TeeTerrainPos(), gRangeDrawer.TargetTerrainPos());
//        prevTeeTerrainPos = gRangeDrawer.TeeTerrainPos();
//        prevTargetTerrainPos = gRangeDrawer.TargetTerrainPos();
//    }
//    
//    
//    /*
//    // ***** TEMP BELOW *****
//    if (gRangeDrawer.TeeMarked() && gRangeDrawer.TargetMarked()) {
//        vec3 start_offset(0,1,0);
//        vec3 start = gRangeDrawer.TeeTerrainPos();
//        vec3 dir = gRangeDrawer.TargetTerrainPos() - start;
//        Intersection intersection;
//        bool intersected = ClosestIntersection(start + start_offset, dir, intersection);
//        if (intersected) {
//            cout << "Position x,y,z: " << intersection.position.x << " "  << intersection.position.y << " " << intersection.position.z << endl;
//            cout << "Distance: " << intersection.distance << endl;
//        } else {
//            cout << "No intersection!" << endl;
//        }
//    }
//    // ***** TEMP ABOVE *****
//   */
//}

float DifficultyAnalyzer::CalculateDifficulty(const vec3 &tee, const vec3 &target) {
    /*
     See attached description of golf shot modelling.
     */
    
    float L = length(target - tee);
    float h = target.y - tee.y;
    float l = ( L * H ) / ( H - h * (1 - p2_relative) );
    
    vec3 dir = normalize(target - tee);
    vec3 up(0, 1, 0);
    vec3 right = cross(dir, up);
    
    const vec3 p1 = tee + dir * l * p1_relative + up * H;
    const vec3 p2 = tee + dir * l * p2_relative + up * H;
    
    // Is path clear to begin with?
    if (PathIsClear(tee, p1, p2, target))
        return 0;
    
    int heightSteps = 0;
    int curveSteps = 0;
    int comboSteps = 0; // combination of height and curve
    
    // These will be adjusted as difficulty increases
    vec3 p1_adjusted = p1;
    vec3 p2_adjusted = p2;
    
    while (true) {
        
        // Take next step depending on which is easiest.
        float heightDifficulty = HeightDifficulty((heightSteps + 1) * heightPerStep);
        float curveDifficulty = HeightDifficulty((curveSteps + 1) * curvePerStep);
        float comboDifficulty = HeightDifficulty((comboSteps + 1) * heightPerStep) + HeightDifficulty((comboSteps + 1) * curvePerStep);
        
        // If number of steps have been exceeded in all directions, the difficulty is infinite
        if (heightSteps == maxHeightSteps && curveSteps == maxCurveSteps && comboSteps == maxComboSteps)
//            return std::numeric_limits<float>::max();
            return -1;
            
        // If number of steps are exceeded, make sure steps more steps aren't taking by settings difficulty to infinite
        if (heightSteps >= maxHeightSteps) { heightDifficulty = std::numeric_limits<float>::max(); }
        if (curveSteps  >= maxCurveSteps ) { curveDifficulty  = std::numeric_limits<float>::max(); }
        if (comboSteps  >= maxComboSteps ) { comboDifficulty  = std::numeric_limits<float>::max(); }
        
        float minDifficulty = std::min(heightDifficulty, std::min(curveDifficulty, comboDifficulty));
        
        vec3 diff;
        if (minDifficulty == heightDifficulty) {
            
            heightSteps += 1;
            
            diff = up * (heightSteps * heightPerStep);
            p1_adjusted = p1 + diff;
            p2_adjusted = p2 + diff;
            if (PathIsClear(tee, p1_adjusted, p2_adjusted, target))
                return heightDifficulty;
            
        } else if (minDifficulty == curveDifficulty) {
            
            curveSteps += 1;
            
            // Try curve to the right (path curves left)
            diff = right * (curveSteps * curvePerStep);
            p1_adjusted = p1 + diff;
            p2_adjusted = p2 + diff;
            if (PathIsClear(tee, p1_adjusted, p2_adjusted, target))
                return curveDifficulty;
            
            // Try curve to the left (path curves right)
            diff = -right * (curveSteps * curvePerStep);
            p1_adjusted = p1 + diff;
            p2_adjusted = p2 + diff;
            if (PathIsClear(tee, p1_adjusted, p2_adjusted, target))
                return curveDifficulty;
            
            
            
        } else if (minDifficulty == comboDifficulty) {
            
            comboSteps += 1;
            
            // Try curve to the right (path curves left)
            diff = up * (comboSteps * heightPerStep) + right * (comboSteps * curvePerStep);
            p1_adjusted = p1 + diff;
            p2_adjusted = p2 + diff;
            if (PathIsClear(tee, p1_adjusted, p2_adjusted, target))
                return comboDifficulty;
            
            // Try curve to the left (path curves right)
            diff = up * (comboSteps * heightPerStep) - right * (comboSteps * curvePerStep);
            p1_adjusted = p1 + diff;
            p2_adjusted = p2 + diff;
            if (PathIsClear(tee, p1_adjusted, p2_adjusted, target))
                return comboDifficulty;
        }
    }
}

bool DifficultyAnalyzer::IntersectionBetweenPoints(const vec3 &start, const vec3 &end) {
    
    const vec3 dir = end - start;
    
    // iterate through all terrain vertices and check for intersection
    for (int i = 0; i < (X_INTERVAL-1) * (Y_INTERVAL-1) * FLOATS_PER_TRIANGLE * 2 - 36 ; i += 36) {
        
        vec3 v0 = vec3(gTerrain.vertexData[i], gTerrain.vertexData[i+1], gTerrain.vertexData[i+2]);
        vec3 v1 = vec3(gTerrain.vertexData[i+12],gTerrain.vertexData[i+13],gTerrain.vertexData[i+14]);
        vec3 v2 = vec3(gTerrain.vertexData[i+24],gTerrain.vertexData[i+25],gTerrain.vertexData[i+26]);
        
        vec3 e1 = v1 - v0;
        vec3 e2 = v2 - v0;
        vec3 b = start - v0;
        mat3 A( -dir, e1, e2 );
        vec3 x = glm::inverse( A ) * b; // x = (t, u, v)
        
        if (x.x > 0 && x.x < 1 && x.y >= 0 && x.z >= 0 && x.y + x.z <= 1) {
//            cout << "intersection t: " << x.x << endl;
            return true;
        }
    }
    
    return false;
}

bool DifficultyAnalyzer::ClosestIntersection(vec3 start, vec3 dir, Intersection& closestIntersection) {
    
    float closest_t = std::numeric_limits<float>::max();
    float closest_index = -1;
    
    // iterate through all terrain vertices and check for intersection
    for (int i = 0; i < (X_INTERVAL-1) * (Y_INTERVAL-1) * FLOATS_PER_TRIANGLE * 2 - 36 ; i += 36) {
        
        vec3 v0 = vec3(gTerrain.vertexData[i], gTerrain.vertexData[i+1], gTerrain.vertexData[i+2]);
        vec3 v1 = vec3(gTerrain.vertexData[i+12],gTerrain.vertexData[i+13],gTerrain.vertexData[i+14]);
        vec3 v2 = vec3(gTerrain.vertexData[i+24],gTerrain.vertexData[i+25],gTerrain.vertexData[i+26]);

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
        return true;
    }
    
    return false;
}