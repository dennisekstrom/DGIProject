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
    
    for ( int y=0; y<Y_INTERVAL; y++ )
        for ( int x=0; x<X_INTERVAL; x++ )
            marked[y][x] = false;
    
    markChanged = false;
}


RangeDrawer::~RangeDrawer() {
    
}

void RangeDrawer::MarkTerrain(RangeTerrain &terrain) {
    for ( auto xy : currentlyMarked )
        terrain.UpdateVertexData(xy.x, xy.y, &white);
        
}

void RangeDrawer::Mark(const int &x, const int &y) {
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

void RangeDrawer::MarkTerrainCoord(const float &tx, const float &ty) {
    Mark( TerrainX2VertexX(tx), TerrainY2VertexY(ty) );
}

void RangeDrawer::UnmarkTerrainCoord(const float &tx, const float &ty) {
    Unmark( TerrainX2VertexX(tx), TerrainY2VertexY(ty) );
}

void RangeDrawer::ToggleMarkedTerrainCoord(const float &tx, const float &ty) {
    ToggleMarked( TerrainX2VertexX(tx), TerrainY2VertexY(ty) );
}