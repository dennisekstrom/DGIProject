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

DifficultyAnalyzer gDifficultyAnalyzer;

void DifficultyAnalyzer::Update(const float &dt) {
    
    // ***** TEMP BELOW *****
    if (gRangeDrawer.TeeMarked() && gRangeDrawer.TargetMarked()) {
        vec3 start_offset(0,1,0);
        vec3 start = gRangeDrawer.TeeTerrainPos();
        vec3 dir = gRangeDrawer.TargetTerrainPos() - start;
        Intersection intersection;
        bool intersected = ClosestIntersection(start + start_offset, dir, intersection);
        if (intersected) {
            cout << "Position x,y,z: " << intersection.position.x << " "  << intersection.position.y << " " << intersection.position.z << endl;
            cout << "Distance: " << intersection.distance << endl;
        } else {
            cout << "No intersection!" << endl;
        }
    }
    // ***** TEMP ABOVE *****
}

bool DifficultyAnalyzer::ClosestIntersection(vec3 start, vec3 dir,Intersection& closestIntersection) {
    
    float closest_t = std::numeric_limits<float>::max();
    float closest_index = -1;
    
    // iterate through all terrain vertices and check for intersection
    // X_INTERVAL * Y_INTERVAL * FLOATS_PER_TRIANGLE * 2 - 36
    for (int i = 0; i < (X_INTERVAL-1) * (Y_INTERVAL-1) * FLOATS_PER_TRIANGLE * 2 - 36 ; i += 36) {
        vec3 v0 = vec3(gTerrain.vertexData[i], gTerrain.vertexData[i+1], gTerrain.vertexData[i+2]);
        vec3 v1 = vec3(gTerrain.vertexData[i+12],gTerrain.vertexData[i+13],gTerrain.vertexData[i+14]);
        vec3 v2 = vec3(gTerrain.vertexData[i+24],gTerrain.vertexData[i+25],gTerrain.vertexData[i+26]);
        //        i = i+36;
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