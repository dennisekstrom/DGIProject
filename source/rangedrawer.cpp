//
//  rangedrawer.cpp
//  DGIProject
//
//  Created by Dennis Ekström on 11/05/14.
//  Copyright (c) 2014 Dennis Ekström. All rights reserved.
//

#include "rangedrawer.h"

RangeDrawer gRangeDrawer;

vec4 white(1,1,1,1);
//RangeTerrain gTerrain;

RangeDrawer::RangeDrawer() {
    
    for ( int y=0; y<Y_INTERVAL-1; y++ )
        for ( int x=0; x<X_INTERVAL-1; x++ )
            marked[y][x] = false;
    
    markChanged = false;
}

RangeDrawer::~RangeDrawer() {
    
}

void RangeDrawer::MarkTerrain(RangeTerrain &terrain) {

    vec4 &c = white;
    GLfloat* vertexData = terrain.vertexData;
    int x, y, idx;
    
    for ( auto xy : currentlyMarked ) {
        
        // x, y are coordinates of the quad
        x = xy.x;
        
        y = xy.y;
        
        
        // Using same methods as in RangeTerrain::UpdateVertexData() and RangeTerrain::SetVertexData()
        idx = (y  )*FLOATS_PER_ROW + (x  )*FLOATS_PER_TRIANGLE_PAIR;
        
        for ( int i=0; i<6; i++ ) {
            terrain.changedVertexIndices.push_back( idx );
            idx += 8; // Skip v, n, and t
            vertexData[idx++] = c.r;
            vertexData[idx++] = c.g;
            vertexData[idx++] = c.b;
            vertexData[idx++] = c.a;
        }
    }
    
}

void RangeDrawer::Mark(const int &x, const int &y) {
    
    int _x = x, _y = y;
    if (_x == X_INTERVAL - 1) { _x--; };
    if (_y == X_INTERVAL - 1) { _y--; };
    assert( 0 <= _x && _x < X_INTERVAL-1 && 0 <= _y && _y < Y_INTERVAL-1 );
    
    if (marked[y][x])
        return; // Already marked
    
    marked[y][x] = true;
    currentlyMarked.insert( { x, y } );
    markChanged = true;
}

void RangeDrawer::Unmark(const int &x, const int &y) {
    if (!marked[y][x])
        return; // Already unmarked
        
    marked[y][x] = false;
    currentlyMarked.erase( { x, y } );
    markChanged = true;
}

void RangeDrawer::ToggleMarked(const int &x, const int &y) {
    marked[y][x] ? Unmark(x, y) : Mark(x, y);
}

void RangeDrawer::UnmarkAll() {
    if (currentlyMarked.empty())
        return; // Already unmarked
    
    memset(marked, false, (X_INTERVAL-1) * (Y_INTERVAL-1) * sizeof(bool)); // Zero all elements in marked
    currentlyMarked.clear();
}

void RangeDrawer::MarkTerrainCoord(const float &tx, const float &ty) {
    Mark( TerrainX2QuadX(tx), TerrainY2QuadY(ty) );
}

void RangeDrawer::UnmarkTerrainCoord(const float &tx, const float &ty) {
    Unmark( TerrainX2QuadX(tx), TerrainY2QuadY(ty) );
}

void RangeDrawer::ToggleMarkedTerrainCoord(const float &tx, const float &ty) {
    ToggleMarked( TerrainX2QuadX(tx), TerrainY2QuadY(ty) );
}