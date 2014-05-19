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

using glm::vec3;

RangeTerrain gTerrain;

RangeTerrain::RangeTerrain() {

    // right now, we only support quadratic terrain
    assert(X_INTERVAL == Y_INTERVAL);
    assert(TERRAIN_WIDTH == TERRAIN_DEPTH);
    
    // initially, we have no control points
    memset(controlPoints, NULL, Y_INTERVAL*X_INTERVAL*sizeof(ControlPoint*));
    
    // initialize change manager to keep track of changes
    changedControlPoints    = new ChangeManager(X_INTERVAL, Y_INTERVAL);
    changedHMapCoords       = new ChangeManager(X_INTERVAL, Y_INTERVAL);
    changedVertices         = new ChangeManager(X_INTERVAL, Y_INTERVAL);
    
    // reserve memory for vector to avoid reallocation during runtime
    changedVertexIndices.reserve(Y_INTERVAL * X_INTERVAL * 6); // Each vertex appears in 6 different triangles
    
    // initial terrain generation (a flat surface)
    FlattenHMap();
    FlattenNoise();
    Regenerate();
    
    // reset as there are now no pending changes to adjust to
    changedVertexIndices.clear();
    regenerationRequired = false;
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
    
    // only do something if the control point exists
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
    
    // only do something if the control point exists
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
    
    // only do something if the control point exists
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
    
    // clear control points
    memset(controlPoints, NULL, Y_INTERVAL*X_INTERVAL*sizeof(ControlPoint*));

    // flatten terrain
    FlattenHMap();
    Regenerate();

    regenerationRequired = false;
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
    
    regenerationRequired = true;
}

void RangeTerrain::Update() {
    
    if (regenerationRequired) {
    
        Regenerate();
        return;

    } else if (ControlPointChanged()) {
    
        UpdateHMap();
        UpdateNormals();
        UpdateChangedVertices();
        UpdateVertexData();

        changedControlPoints->Reset();
        regenerationRequired = false;
    }
    
}

void RangeTerrain::Regenerate() {
    
    GenerateHMap();
    ApplyNoise();
    GenerateNormals();
    GenerateVertexData();
    
    changedControlPoints->Reset();
    regenerationRequired = false;
}

void RangeTerrain::UpdateHMap() { // Changes only away from y = 0
    
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
    
    regenerationRequired = true;
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

void RangeTerrain::UpdateVertexData(const int &x, const int &y) {
    /*
     x,y are vertex coordinates.
     
     Vertex order in memory of TrianglePairs:
     diagonalUp == True:    v1  v2  v3  v4  v3  v2
     diagonalUp == False:   v1  v4  v2  v4  v3  v1
     
     Orientation:
     North =  z
     South = -z
     East  =  x
     West  = -x
     */
    
    int idx;
    vec3 v = vec3(x * GRID_RES, hmap[y][x], -y * GRID_RES);
    vec3 n = normals[y][x];
    vec4 c = ColorFromHeight(hmap[y][x]);
    
    // v1 in South East triangle pair
    if (x < X_INTERVAL - 1 && y < Y_INTERVAL - 1) {
        
        bool diagonalUp = abs(hmap[y][x] - hmap[y+1][x+1]) > abs(hmap[y+1][x] - hmap[y][x+1]);
        
        if (diagonalUp) {
            idx = (y  )*FLOATS_PER_ROW + (x  )*FLOATS_PER_TRIANGLE_PAIR + 0*FLOATS_PER_VERTEX;
            changedVertexIndices.push_back( idx );
            SetVertexData(idx, v, vec2(0, 0), n, c);
        } else {
            idx = (y  )*FLOATS_PER_ROW + (x  )*FLOATS_PER_TRIANGLE_PAIR + 0*FLOATS_PER_VERTEX;
            changedVertexIndices.push_back( idx );
            SetVertexData(idx, v, vec2(0, 0), n, c);
            
            idx = (y  )*FLOATS_PER_ROW + (x  )*FLOATS_PER_TRIANGLE_PAIR + 5*FLOATS_PER_VERTEX;
            changedVertexIndices.push_back( idx );
            SetVertexData(idx, v, vec2(0, 0), n, c);
        }
    }
    
    // v2 in North East triangle pair
    if (x < X_INTERVAL - 1 && y > 0) {
        
        bool diagonalUp = abs(hmap[y-1][x] - hmap[y][x+1]) > abs(hmap[y][x] - hmap[y-1][x+1]);
        
        if (diagonalUp) {
            idx = (y-1)*FLOATS_PER_ROW + (x  )*FLOATS_PER_TRIANGLE_PAIR + 1*FLOATS_PER_VERTEX;
            changedVertexIndices.push_back( idx );
            SetVertexData(idx, v, vec2(0, 1), n, c);
            
            idx = (y-1)*FLOATS_PER_ROW + (x  )*FLOATS_PER_TRIANGLE_PAIR + 5*FLOATS_PER_VERTEX;
            changedVertexIndices.push_back( idx );
            SetVertexData(idx, v, vec2(0, 1), n, c);
        } else {
            idx = (y-1)*FLOATS_PER_ROW + (x  )*FLOATS_PER_TRIANGLE_PAIR + 2*FLOATS_PER_VERTEX;
            changedVertexIndices.push_back( idx );
            SetVertexData(idx, v, vec2(0, 1), n, c);
        }
    }
    
    // v3 in South West triangle pair
    if (x > 0 && y < Y_INTERVAL - 1) {
        
        bool diagonalUp = abs(hmap[y][x-1] - hmap[y+1][x]) > abs(hmap[y+1][x-1] - hmap[y][x]);
        
        if (diagonalUp) {
            idx = (y  )*FLOATS_PER_ROW + (x-1)*FLOATS_PER_TRIANGLE_PAIR + 2*FLOATS_PER_VERTEX;
            changedVertexIndices.push_back( idx );
            SetVertexData(idx, v, vec2(1, 0), n, c);
            
            idx = (y  )*FLOATS_PER_ROW + (x-1)*FLOATS_PER_TRIANGLE_PAIR + 4*FLOATS_PER_VERTEX;
            changedVertexIndices.push_back( idx );
            SetVertexData(idx, v, vec2(1, 0), n, c);
        } else {
            idx = (y  )*FLOATS_PER_ROW + (x-1)*FLOATS_PER_TRIANGLE_PAIR + 4*FLOATS_PER_VERTEX;
            changedVertexIndices.push_back( idx );
            SetVertexData(idx, v, vec2(1, 0), n, c);
        }
    }
    
    
    // v4 in North West triangle pair
    if (x > 0 && y > 0) {
        
        bool diagonalUp = abs(hmap[y-1][x-1] - hmap[y][x]) > abs(hmap[y][x-1] - hmap[y-1][x]);
        
        if (diagonalUp) {
            idx = (y-1)*FLOATS_PER_ROW + (x-1)*FLOATS_PER_TRIANGLE_PAIR + 3*FLOATS_PER_VERTEX;
            changedVertexIndices.push_back( idx );
            SetVertexData(idx, v, vec2(1, 1), n, c);
        } else {
            idx = (y-1)*FLOATS_PER_ROW + (x-1)*FLOATS_PER_TRIANGLE_PAIR + 1*FLOATS_PER_VERTEX;
            changedVertexIndices.push_back( idx );
            SetVertexData(idx, v, vec2(1, 1), n, c);
            
            idx = (y-1)*FLOATS_PER_ROW + (x-1)*FLOATS_PER_TRIANGLE_PAIR + 3*FLOATS_PER_VERTEX;
            changedVertexIndices.push_back( idx );
            SetVertexData(idx, v, vec2(1, 1), n, c);
        }
    }
}

inline void RangeTerrain::SetVertexData(int idx, const vec3 &v, const vec2 &t, const vec3 &n, const vec4 &c) {
    vertexData[idx++] = v.x;
    vertexData[idx++] = v.y;
    vertexData[idx++] = v.z;
    vertexData[idx++] = t.x;
    vertexData[idx++] = t.y;
    vertexData[idx++] = n.x;
    vertexData[idx++] = n.y;
    vertexData[idx++] = n.z;
    vertexData[idx++] = c.r;
    vertexData[idx++] = c.g;
    vertexData[idx++] = c.b;
    vertexData[idx++] = c.a;
}

vec4 RangeTerrain::ColorFromHeight(const float &h) const {
    vector<vec4> levels;
    levels.push_back( vec4( 0.5,      0,      0,      1) ); // dark red
    levels.push_back( vec4(   1,      0,      0,      1) ); // red
    levels.push_back( vec4(   1,    0.5,      0,      1) ); // orange
    levels.push_back( vec4(   1,      1,      0,      1) ); // yellow
    levels.push_back( vec4(   0,      1,      0,      1) ); // green
    levels.push_back( vec4(   0,      1,      1,      1) ); // cyan
    levels.push_back( vec4(   0,      0,      1,      1) ); // blue
    levels.push_back( vec4( 0.5,    0.5,      1,      1) ); // light blue
    
    float h_min = -5, h_max = 5;
    
    if (h <= h_min)
        return levels.front();
    
    if (h >= h_max)
        return levels.back();
    
    float idx = (levels.size() - 1) * (h - h_min) / (h_max - h_min);
    float idx_low = floor( idx );
    float idx_high = ceil( idx );
    
    vec4 level_low = levels[idx_low];
    vec4 level_high = levels[idx_high];
    
    return level_low + (idx - idx_low) * (level_high - level_low);
}


