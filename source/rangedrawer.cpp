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
    
    markedWithShift = new AreaMarkingManager(Y_INTERVAL - 1, X_INTERVAL - 1);
    
    mouseIsDown = false;
}

RangeDrawer::~RangeDrawer() {
    delete markedWithShift;
}

void RangeDrawer::MarkTerrain() {
    
    gTerrain.UpdateVertexData();
    gTerrain.changedVertices->Reset();
    
    vec4 &c = white;
    GLfloat* vertexData = gTerrain.vertexData;
    int x, y, idx;
    
    for ( auto xy : currentlyMarked ) {
        
        // x, y are coordinates of the quad
        x = xy.x;
        y = xy.y;
        
        
        // Using same methods as in RangeTerrain::UpdateVertexData() and RangeTerrain::SetVertexData()
        idx = (y  )*FLOATS_PER_ROW + (x  )*FLOATS_PER_TRIANGLE_PAIR;
        
        for ( int i=0; i<6; i++ ) {
            gTerrain.changedVertexIndices.push_back( idx );
            idx += 8; // Skip v, n, and t
            vertexData[idx++] = c.r;
            vertexData[idx++] = c.g;
            vertexData[idx++] = c.b;
            vertexData[idx++] = c.a;
        }
    }
}

const int spread = 8;// [TODO: THIS IS TEMP]
const ControlPointFuncType functype = FUNC_SIN;// [TODO: THIS IS TEMP]
void RangeDrawer::LiftMarkedTerrain(const float &lift) {
    int x, y;
    float h;
    ControlPoint* cp;
    for ( auto xy : currentlyMarked ) {
        x = xy.x;
        y = xy.y;
        
        // v1
        cp = gTerrain.GetControlPoint(x  , y  );
        h = cp ? cp->h : gTerrain.hmap[y  ][x  ];
        gTerrain.SetControlPoint(x  , y  , h + lift, spread, functype);
        
        // For the other vertices, we need to for other coordinates being marked to avoid multiple adjustments of same controlpoint
        
        // v2
        if (!marked[y+1][x  ]) {
            cp = gTerrain.GetControlPoint(x  , y+1);
            h = cp ? cp->h : gTerrain.hmap[y+1][x  ];
            gTerrain.SetControlPoint(x  , y+1, h + lift, spread, functype);
        }
        
        // v3
        if (!marked[y  ][x+1]) {
            cp = gTerrain.GetControlPoint(x+1, y  );
            h = cp ? cp->h : gTerrain.hmap[y  ][x+1];
            gTerrain.SetControlPoint(x+1, y  , h + lift, spread, functype);
        }
        
        // v4
        if (!marked[y+1][x  ] && !marked[y  ][x+1] && !marked[y+1][x+1]) {
            cp = gTerrain.GetControlPoint(x+1, y+1);
            h = cp ? cp->h : gTerrain.hmap[y+1][x+1];
            gTerrain.SetControlPoint(x+1, y+1, h + lift, spread, functype);
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
    
    gTerrain.changedVertices->SetChanged( x  , y   );
    gTerrain.changedVertices->SetChanged( x  , y+1 );
    gTerrain.changedVertices->SetChanged( x+1, y   );
    gTerrain.changedVertices->SetChanged( x+1, y+1 );
}

void RangeDrawer::Unmark(const int &x, const int &y) {
    
    int _x = x, _y = y;
    if (_x == X_INTERVAL - 1) { _x--; };
    if (_y == X_INTERVAL - 1) { _y--; };
    assert( 0 <= _x && _x < X_INTERVAL-1 && 0 <= _y && _y < Y_INTERVAL-1 );
    
    if (!marked[y][x])
        return; // Already unmarked
        
    marked[y][x] = false;
    currentlyMarked.erase( { x, y } );
    markChanged = true;
    
    gTerrain.changedVertices->SetChanged( x  , y   );
    gTerrain.changedVertices->SetChanged( x  , y+1 );
    gTerrain.changedVertices->SetChanged( x+1, y   );
    gTerrain.changedVertices->SetChanged( x+1, y+1 );
}

void RangeDrawer::ToggleMarked(const int &x, const int &y) {
    marked[y][x] ? Unmark(x, y) : Mark(x, y);
}

void RangeDrawer::UnmarkAll() {
    
    if (currentlyMarked.empty())
        return; // Nothing is marked
    
    int x, y;
    for ( auto xy : currentlyMarked ) {
        x = xy.x;
        y = xy.y;
        
        marked[y][x] = false;
        
        gTerrain.changedVertices->SetChanged( x  , y   );
        gTerrain.changedVertices->SetChanged( x  , y+1 );
        gTerrain.changedVertices->SetChanged( x+1, y   );
        gTerrain.changedVertices->SetChanged( x+1, y+1 );
    }
    currentlyMarked.clear();
    markChanged = true;
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

void RangeDrawer::TerrainCoordClicked(const float &tx, const float &ty, const bool &shift_down) {
    
    const int x = TerrainX2QuadX(tx), y = TerrainY2QuadY(ty);
    
    if (!mouseIsDown) { // Mouse was recently pressed
        mouseDownIsMarking = !marked[y][x]; // if (x,y) was marked, only mark until mouse release
        markedWithShift->Reset();
    }
    mouseIsDown = true;
    
    if (!shift_down) {
    
        markedWithShift->Reset();
        mouseDownIsMarking ? Mark(x, y) : Unmark(x, y);

    } else {
        
        markedWithShift->Mark(x, y);
        for ( int y=markedWithShift->y_min; y<=markedWithShift->y_max; y++) {
            for ( int x=markedWithShift->x_min; x<=markedWithShift->x_max; x++) {
                mouseDownIsMarking ? Mark(x, y) : Unmark(x, y);
            }
        }
    }
}