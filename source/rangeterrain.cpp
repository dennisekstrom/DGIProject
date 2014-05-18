//
//  rangeterrain.cpp
//  DGIProject
//
//  Created by Dennis Ekström on 08/05/14.
//  Copyright (c) 2014 Dennis Ekström. All rights reserved.
//

#include "rangeterrain.h"
#include <iostream>
#include <glm/glm.hpp>

//using namespace std;
using glm::vec3;

RangeTerrain gTerrain;
//PerlinNoise perlinNoise;

RangeTerrain::RangeTerrain() {

    assert(X_INTERVAL == Y_INTERVAL);
    assert(TERRAIN_WIDTH == TERRAIN_DEPTH);
    
    memset(controlPoints, NULL, Y_INTERVAL*X_INTERVAL*sizeof(ControlPoint*));
    regenerationRequired = false;
    
    changedControlPoints    = new ChangeManager(X_INTERVAL, Y_INTERVAL);
    changedHMapCoords       = new ChangeManager(X_INTERVAL, Y_INTERVAL);
    changedVertices         = new ChangeManager(X_INTERVAL, Y_INTERVAL);
    
    changedVertexIndices.reserve(Y_INTERVAL * X_INTERVAL * 6); // Each vertex appears in 6 different triangles
    
    FlattenHMap();
    FlattenNoise();
    
//    SetControlPoint(0, 0, 4, 4, FUNC_COS);
//    SetControlPoint(8, 8, 3, 7, FUNC_LINEAR);
//    SetControlPoint(10, 12, 6, 2, FUNC_SIN);
//    SetControlPoint(11, 12, 6, 2, FUNC_SIN);
//    SetControlPoint(10, 13, 6, 2, FUNC_SIN);
//    SetControlPoint(11, 13, 6, 2, FUNC_SIN);
    
    GenerateAll();
    
    changedVertexIndices.clear();
}

RangeTerrain::~RangeTerrain() {
    
    delete changedControlPoints;
    delete changedHMapCoords;
    delete changedVertices;
    
    for (int y=0; y<Y_INTERVAL; y++)
        for (int x=0; x<X_INTERVAL; x++)
            delete controlPoints[y][x];
}

void RangeTerrain::SetControlPoint(int x, int y, float h, float spread, ControlPointFuncType functype) {
    
    if (controlPoints[y][x]) {
        
        // old values for this control point
        float old_h = controlPoints[y][x]->h;
        float old_spread = controlPoints[y][x]->spread;
        ControlPointFuncType old_functype = controlPoints[y][x]->functype;
        
        // did control point change at all?
        if (h == old_h && spread == old_spread && functype == old_functype)
            return;
        
        // is hmap regeneration necessary
        if(regenerationRequired || abs(h) < abs(old_h) || spread != old_spread || functype != old_functype)
            regenerationRequired = true;
        
        // delete old pointer
        delete controlPoints[y][x];
    }
    
    // perform change
    controlPoints[y][x] = new ControlPoint(x, y, h, spread, functype);
    
    // remember change
    changedControlPoints->SetChanged(x, y);
}

void RangeTerrain::SetControlPointSpread(int x, int y, float spread) {
    if (controlPoints[y][x]) {

        // old value
        float old_spread = controlPoints[y][x]->spread;
        
        // did control point change at all?
        if (spread == old_spread)
            return;
        
        // is hmap regeneration necessary
        if(regenerationRequired || spread != old_spread )
            regenerationRequired = true;
        
        // perform the update
        controlPoints[y][x]->spread = spread;
        
        // remember change
        changedControlPoints->SetChanged(x, y);
    }
}

void RangeTerrain::SetControlPointFuncType(int x, int y, ControlPointFuncType functype) {
    if (controlPoints[y][x]) {
        
        // old value
        ControlPointFuncType old_functype = controlPoints[y][x]->functype;
        
        // did control point change at all?
        if (functype == old_functype)
            return;
        
        // is hmap regeneration necessary
        if(regenerationRequired || functype != old_functype )
            regenerationRequired = true;
        
        // perform the update
        controlPoints[y][x]->SetFuncType(functype);
        
        // remember change
        changedControlPoints->SetChanged(x, y);
    }
}

void RangeTerrain::Reset() {
    memset(controlPoints, NULL, Y_INTERVAL*X_INTERVAL*sizeof(ControlPoint*));
    regenerationRequired = false;

    FlattenHMap();
    GenerateAll();
}

void RangeTerrain::FlattenHMap() {
    
    for ( int y=0; y<Y_INTERVAL; y++ )
        for ( int x=0; x<X_INTERVAL; x++ )
            hmap[y][x] = 0;
}

void RangeTerrain::FlattenNoise() {
    
    for ( int y=0; y<Y_INTERVAL; y++ )
        for ( int x=0; x<X_INTERVAL; x++ )
            noise[y][x] = 0;
}

void RangeTerrain::UpdateAll() {
    
    if (regenerationRequired) {
        GenerateAll();
        return;
    }
    
    UpdateHMap();
    UpdateNormals();
    UpdateChangedVertices();
    UpdateVertexData();

//    UpdateTrianglePairs();
    
    changedControlPoints->Reset();
    regenerationRequired = false;
}

void RangeTerrain::GenerateAll() {
    
//    FlattenHMap();
    GenerateHMap();
    ApplyNoise();
    GenerateNormals();
//    GenerateTrianglePairs();
    GenerateVertexData();
    
    changedControlPoints->Reset();
    regenerationRequired = false;
}

// [TODO: THIS FUNCTION IS NOT UP TO DATE]
void RangeTerrain::UpdateHMap() { // Changes only upwards
    
    changedHMapCoords->Reset();

    for ( xy &xy : changedControlPoints->identifiers )
        UpdateHMap(*controlPoints[xy.y][xy.x]);
}

void RangeTerrain::GenerateHMap() {
    
    changedHMapCoords->Reset();
    FlattenHMap();
    
    for ( int y=0; y<Y_INTERVAL; y++ )
        for ( int x=0; x<X_INTERVAL; x++ )
            if (controlPoints[y][x])
                UpdateHMap(*controlPoints[y][x]);

    ApplyNoise();
}

void RangeTerrain::UpdateChangedVertices() {
    
    changedVertices->Reset();
    
    int x, y;
    for ( xy &xy : changedHMapCoords->identifiers ) {
        // The grid of triangles is one less than the hmap, adjustment
        // at boundary causes adjustment of the correct triangles
        x = xy.x;
        y = xy.y;
        if (x == X_INTERVAL - 1) { x--; }
        if (y == Y_INTERVAL - 1) { y--; }
        
        changedVertices->SetChanged( x  , y   );
        changedVertices->SetChanged( x  , y+1 );
        changedVertices->SetChanged( x+1, y   );
        changedVertices->SetChanged( x+1, y+1 );
    }
}
void RangeTerrain::UpdateNormals() {
    
    for ( xy &xy : changedHMapCoords->identifiers )
        UpdateNormal(xy.x, xy.y);
}

void RangeTerrain::GenerateNormals() {
    
    for ( int y=0; y<Y_INTERVAL; y++ )
        for ( int x=0; x<X_INTERVAL; x++ )
            UpdateNormal(x, y);
}

/*void RangeTerrain::UpdateTrianglePairs() {
    
    changedVertices->Reset();
    
    int x, y;
    for ( xy &xy : changedHMapCoords->identifiers ) {
        // The grid of triangles is one less than the hmap, adjustment
        // at boundary causes adjustment of the correct triangles
        x = xy.x;
        y = xy.y;
        if (x == X_INTERVAL - 1) { x--; }
        if (y == Y_INTERVAL - 1) { y--; }
        
        UpdateTrianglePair(x, y);
        changedVertices->SetChanged( x  , y   );
        changedVertices->SetChanged( x  , y+1 );
        changedVertices->SetChanged( x+1, y   );
        changedVertices->SetChanged( x+1, y+1 );
    }
}

void RangeTerrain::GenerateTrianglePairs() {
    
    changedVertices->Reset();
    
    for ( int y=0; y<Y_INTERVAL-1; y++ ) {
        for ( int x=0; x<X_INTERVAL-1; x++ ) {
            UpdateTrianglePair(x, y);
            
            changedVertices->SetChanged( x  , y   );
            changedVertices->SetChanged( x  , y+1 );
            changedVertices->SetChanged( x+1, y   );
            changedVertices->SetChanged( x+1, y+1 );
        }
    }
}*/

void RangeTerrain::UpdateVertexData() {
    
    for ( xy &xy : changedVertices->identifiers )
        UpdateVertexData(xy.x, xy.y);
}

void RangeTerrain::GenerateVertexData() {
    
    for ( int y=0; y<Y_INTERVAL; y++ )
        for ( int x=0; x<X_INTERVAL; x++ )
            UpdateVertexData(x, y);
}

void RangeTerrain::SetNoise(double _persistence, double _frequency, double _amplitude, int _octaves, int _randomseed) {
    PerlinNoise pn(_persistence, _frequency, _amplitude, _octaves, _randomseed);
    for( int x=0; x<X_INTERVAL; x++)
        for( int y=0; y<Y_INTERVAL; y++)
            noise[y][x] = pn.GetHeight(x, y);
}

void RangeTerrain::ApplyNoise() {
    for( int x=0; x<X_INTERVAL; x++) {
        for( int y=0; y<Y_INTERVAL; y++) {
            if (abs(noise[y][x]) > abs(hmap[y][x]))
                hmap[y][x] = noise[y][x];
        }
    }
}

void RangeTerrain::UpdateHMap(const ControlPoint &cp) {

    // Height of control point itself
    hmap[cp.y][cp.x] = cp.h;
    changedHMapCoords->SetChanged(cp.x, cp.y);
    
    // Height of surrounding points
    int min_x = ceil(std::max(cp.x - cp.spread / GRID_RES, 0.0f));
    int max_x = floor(std::min(cp.x + cp.spread / GRID_RES, float(X_INTERVAL - 1)));
    int min_y = ceil(std::max(cp.y - cp.spread / GRID_RES, 0.0f));
    int max_y = floor(std::min(cp.y + cp.spread / GRID_RES, float(Y_INTERVAL - 1)));
    for (int yy=min_y; yy<=max_y; yy++) {
        for (int xx=min_x; xx<=max_x; xx++) {
            float h = cp.lift(xx, yy);
            if (!controlPoints[yy][xx] && abs(h) > abs(hmap[yy][xx])) {
                hmap[yy][xx] = h;
                changedHMapCoords->SetChanged(xx, yy);
            }
        }
    }
}

void RangeTerrain::UpdateNormal(const int &x, const int &y) {

    float hs = (y == 0 ?              2*hmap[y][x]-hmap[y+1][x] : hmap[y-1][x]); // North
    float hn = (y == Y_INTERVAL - 1 ? 2*hmap[y][x]-hmap[y-1][x] : hmap[y+1][x]); // South
    float hw = (x == 0 ?              2*hmap[y][x]-hmap[y][x+1] : hmap[y][x-1]); // West
    float he = (x == X_INTERVAL - 1 ? 2*hmap[y][x]-hmap[y][x-1] : hmap[y][x+1]); // East
    
    normals[y][x] = glm::normalize(vec3(hw - he, 2 * GRID_RES, hn - hs));
}

/*void RangeTerrain::UpdateTrianglePair(const int &x, const int &y) {
    
    TrianglePair &tp = trianglePairs[y][x];
    
    vec3 v1 = vec3((x  ) * GRID_RES, hmap[y  ][x  ], (y  ) * GRID_RES);
    vec3 v2 = vec3((x  ) * GRID_RES, hmap[y+1][x  ], (y+1) * GRID_RES);
    vec3 v3 = vec3((x+1) * GRID_RES, hmap[y  ][x+1], (y  ) * GRID_RES);
    vec3 v4 = vec3((x+1) * GRID_RES, hmap[y+1][x+1], (y+1) * GRID_RES);
    
    tp.SetVertices(v1, v2, v3, v4);
    
    vec3 n1 = normals[y  ][x  ];
    vec3 n2 = normals[y+1][x  ];
    vec3 n3 = normals[y  ][x+1];
    vec3 n4 = normals[y+1][x+1];
    
    tp.SetNormals(n1, n2, n3, n4);
    
    vec3 c1 = vec3(1,0,0); // [TODO]
    vec3 c2 = vec3(1,0,0); // [TODO]
    vec3 c3 = vec3(1,0,0); // [TODO]
    vec3 c4 = vec3(1,0,0); // [TODO]
    
    tp.SetColors(c1, c2, c3, c4);
}*/

// [TODO: assuming diagonalUp == true]
void RangeTerrain::UpdateVertexData(const int &x, const int &y/*, const vec4* color*//*=NULL*/) {
    /*
     x,y are vertex coordinates.
     
     Vertex order in memory of TrianglePairs:
     diagonalUp == True:    v1  v2  v3  v4  v3  v2
     diagonalUp == False:   v1  v4  v3  v4  v1  v2
     
     Orientation:
     North =  z
     South = -z
     East  =  x
     West  = -x
     */
    
    int idx;
    vec3 v = vec3(x * GRID_RES, hmap[y][x], -y * GRID_RES);
//    vec2 t = vec2(x / float(X_INTERVAL), y / float(Y_INTERVAL)); // For texture covering entire ground
    vec3 n = normals[y][x];
    vec4 c = /*color ? *color : */ColorFromHeight(hmap[y][x]);
    
    // v1 in South East triangle pair
    if (x < X_INTERVAL - 1 && y < Y_INTERVAL - 1) {
        idx = (y  )*FLOATS_PER_ROW + (x  )*FLOATS_PER_TRIANGLE_PAIR;
        changedVertexIndices.push_back( idx );
        SetVertexData(idx, v, vec2(0, 0), n, c);
    }
    
    // v2 in North East triangle pair
    if (x < X_INTERVAL - 1 && y > 0) {
        idx = (y-1)*FLOATS_PER_ROW + (x  )*FLOATS_PER_TRIANGLE_PAIR + 1*FLOATS_PER_VERTEX;
        changedVertexIndices.push_back( idx );
        SetVertexData(idx, v, vec2(0, 1), n, c);
        
        idx = (y-1)*FLOATS_PER_ROW + (x  )*FLOATS_PER_TRIANGLE_PAIR + 5*FLOATS_PER_VERTEX;
        changedVertexIndices.push_back( idx );
        SetVertexData(idx, v, vec2(0, 1), n, c);
    }
    
    // v3 in South West triangle pair
    if (x > 0 && y < Y_INTERVAL - 1) {
        idx = (y  )*FLOATS_PER_ROW + (x-1)*FLOATS_PER_TRIANGLE_PAIR + 2*FLOATS_PER_VERTEX;
        changedVertexIndices.push_back( idx );
        SetVertexData(idx, v, vec2(1, 0), n, c);
        
        idx = (y  )*FLOATS_PER_ROW + (x-1)*FLOATS_PER_TRIANGLE_PAIR + 4*FLOATS_PER_VERTEX;
        changedVertexIndices.push_back( idx );
        SetVertexData(idx, v, vec2(1, 0), n, c);
    }
    
    // v4 in North West triangle pair
    if (x > 0 && y > 0) {
        idx = (y-1)*FLOATS_PER_ROW + (x-1)*FLOATS_PER_TRIANGLE_PAIR + 3*FLOATS_PER_VERTEX;
        changedVertexIndices.push_back( idx );
        SetVertexData(idx, v, vec2(1, 1), n, c);
    }
}






