//
//  rangedrawer.cpp
//  DGIProject
//
//  Created by Dennis Ekström on 11/05/14.
//  Copyright (c) 2014 Dennis Ekström. All rights reserved.
//

#include "rangedrawer.h"

#define PI              3.14159265359
#define RAD2DEG(X)      X*180.0f/PI
#define DEG2RAD(X)      X*PI/180.0f

RangeDrawer gRangeDrawer;
vec4 white(1,1,1,1);
vec4 red(1,0,0,1);
vec4 blue(0,0,1,1);
vec2 gHolePosition;
vec2 gMatPosition;
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

void RangeDrawer::MarkTerrain(bool holePositionSet, bool matPositionSet) {
    
    gTerrain.UpdateVertexData();
    gTerrain.changedVertices->Reset();
    
    vec4 c = white;
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
    
    // Color the hole and mat positions
    if (holePositionSet) {
        idx = (gHolePosition.y  )*FLOATS_PER_ROW + (gHolePosition.x  )*FLOATS_PER_TRIANGLE_PAIR;
        c = red;
        for ( int i=0; i<6; i++ ) {
            gTerrain.changedVertexIndices.push_back( idx );
            idx += 8; // Skip v, n, and t
            vertexData[idx++] = c.r; vertexData[idx++] = c.g;
            vertexData[idx++] = c.b; vertexData[idx++] = c.a;
        }
    }
    if (matPositionSet) {
        idx = (gMatPosition.y  )*FLOATS_PER_ROW + (gMatPosition.x  )*FLOATS_PER_TRIANGLE_PAIR;
        c = blue;
        for ( int i=0; i<6; i++ ) {
            gTerrain.changedVertexIndices.push_back( idx );
            idx += 8; // Skip v, n, and t
            vertexData[idx++] = c.r; vertexData[idx++] = c.g;
            vertexData[idx++] = c.b; vertexData[idx++] = c.a;
        }
    }
    
}

inline void RangeDrawer::Lift(const int &x, const int &y, const float &lift, const float &spread, const ControlPointFuncType &functype) {
    ControlPoint* cp = gTerrain.GetControlPoint(x, y);
    float h = cp ? cp->h : gTerrain.hmap[y][x];
    gTerrain.SetControlPoint(x, y, h + lift, spread, functype);
}

float RangeDrawer::GetAverageHeightOfMarked() {
    
    if (currentlyMarked.empty())
        return 0;
    
    float sum = 0;
    for (auto xy : currentlyMarked)
        sum += gTerrain.hmap[xy.y][xy.x];
    return sum / currentlyMarked.size();
}

glm::vec2 RangeDrawer::GetCenterOfMarked() {
    if (currentlyMarked.empty())
        return glm::vec2(0, 0);
    
    float sumx = 0, sumy = 0;
    for (auto xy : currentlyMarked) {
        sumx += 4*xy.x + 2;
        sumy += 4*xy.y + 2;
    }
    return glm::vec2(sumx / (4*currentlyMarked.size()), sumy / (4*currentlyMarked.size()));
}

//const int spread = 4;// [TODO: THIS IS TEMP]
//const ControlPointFuncType functype = FUNC_SIN;// [TODO: THIS IS TEMP]
void RangeDrawer::LiftMarked(const float &lift, const float &spread, const ControlPointFuncType &functype) {
    
    int x, y;
    for ( auto xy : currentlyMarked ) {
        x = xy.x;
        y = xy.y;
        
        Lift(x  , y  , lift, spread, functype);     // v1
        
        // For vertices 2-4, we need to check for other coordinates being marked to avoid multiple adjustments of same controlpoint
        if (y == Y_INTERVAL - 2 || !marked[y+1][x  ])
            Lift(x  , y+1, lift, spread, functype); // v2
        
        if (x == X_INTERVAL - 2 || (!marked[y  ][x+1] && (y == 0 || !marked[y-1][x+1])))
            Lift(x+1, y  , lift, spread, functype); // v3
        
        if ((y == Y_INTERVAL - 2 && x == X_INTERVAL - 2) || (!marked[y+1][x  ] && !marked[y  ][x+1] && !marked[y+1][x+1]))
            Lift(x+1, y+1, lift, spread, functype); // v4
    }
}

void RangeDrawer::FlattenMarked(const float &h, const float &spread, const ControlPointFuncType &functype) {
    
    int x, y;
    for ( auto xy : currentlyMarked ) {
        x = xy.x;
        y = xy.y;
        
        gTerrain.SetControlPoint(x  , y  , h, spread, functype);
        gTerrain.SetControlPoint(x  , y+1, h, spread, functype);
        gTerrain.SetControlPoint(x+1, y  , h, spread, functype);
        gTerrain.SetControlPoint(x+1, y+1, h, spread, functype);
    }
}

void RangeDrawer::TiltMarked(const float &xtilt, const float &ytilt, const float &spread, const ControlPointFuncType &functype) {
    
    glm::vec2 center = GetCenterOfMarked();
    const float &cx = center.x, &cy = center.y;
    const float tanx = tan(DEG2RAD(xtilt)), tany = tan(DEG2RAD(ytilt));
    
    float lift;
    int x, y;
    for ( auto xy : currentlyMarked ) {
        x = xy.x;
        y = xy.y;
        
        // v1
        lift = (cx - float(x  )) * float(GRID_RES) * tanx + (cy - float(y  )) * float(GRID_RES) * tany;
        Lift(x  , y  , lift, spread, functype);
        
        
        // For vertices 2-4, we need to check for other coordinates being marked to avoid multiple adjustments of same controlpoint
        if (y == Y_INTERVAL - 2 || !marked[y+1][x  ]) {
            lift = (cx - (x  )) * float(GRID_RES) * tanx + (cy - (y+1)) * float(GRID_RES) * tany;
            Lift(x  , y+1, lift, spread, functype);
        }
        
        if (x == X_INTERVAL - 2 || (!marked[y  ][x+1] && (y == 0 || !marked[y-1][x+1]))) {
            lift = (cx - (x+1)) * float(GRID_RES) * tanx + (cy - (y  )) * float(GRID_RES) * tany;
            Lift(x+1, y  , lift, spread, functype);
            
        }
        
        if ((y == Y_INTERVAL - 2 && x == X_INTERVAL - 2) || (!marked[y+1][x  ] && !marked[y  ][x+1] && !marked[y+1][x+1])) {
            lift = (cx - (x+1)) * float(GRID_RES) * tanx + (cy - (y+1)) * float(GRID_RES) * tany;
            Lift(x+1, y+1, lift, spread, functype);
            
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
    SetMarkChanged();
    
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
    SetMarkChanged();
    
    gTerrain.changedVertices->SetChanged( x  , y   );
    gTerrain.changedVertices->SetChanged( x  , y+1 );
    gTerrain.changedVertices->SetChanged( x+1, y   );
    gTerrain.changedVertices->SetChanged( x+1, y+1 );
}

void RangeDrawer::ToggleMarked(const int &x, const int &y) {
    marked[y][x] ? Unmark(x, y) : Mark(x, y);
}

void RangeDrawer::UnmarkAll() {
    Mark(gHolePosition.x, gHolePosition.y);
    Mark(gMatPosition.x, gMatPosition.y);

    Unmark(gHolePosition.x, gHolePosition.y);
    Unmark(gMatPosition.x, gMatPosition.y);
    
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
    SetMarkChanged();
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

void RangeDrawer::MarkHole(const float &tx, const float &ty) {
    //TODO, color needs to be reset on previous, should exist a better way
    
    vec4 c = gTerrain.ColorFromHeight(gTerrain.hmap[(int)gHolePosition.y][(int)gHolePosition.x]);
    
    int idx = (gHolePosition.y  )*FLOATS_PER_ROW + (gHolePosition.x  )*FLOATS_PER_TRIANGLE_PAIR;
    for ( int i=0; i<6; i++ ) {
        gTerrain.changedVertexIndices.push_back( idx );
        idx += 8; // Skip v, n, and t
        gTerrain.vertexData[idx++] = c.r; gTerrain.vertexData[idx++] = c.g;
        gTerrain.vertexData[idx++] = c.b; gTerrain.vertexData[idx++] = c.a;
    }
    
    const int x = TerrainX2QuadX(tx), y = TerrainY2QuadY(ty);
    gHolePosition.x = x;
    gHolePosition.y = y;
    
}

void RangeDrawer::MarkMat(const float &tx, const float &ty) {
    //TODO, color needs to be reset on previous, should exist a better way
    
    vec4 c = gTerrain.ColorFromHeight(gTerrain.hmap[(int)gMatPosition.y][(int)gMatPosition.x]);
    
    int idx = (gMatPosition.y  )*FLOATS_PER_ROW + (gMatPosition.x  )*FLOATS_PER_TRIANGLE_PAIR;
    for ( int i=0; i<6; i++ ) {
        gTerrain.changedVertexIndices.push_back( idx );
        idx += 8; // Skip v, n, and t
        gTerrain.vertexData[idx++] = c.r; gTerrain.vertexData[idx++] = c.g;
        gTerrain.vertexData[idx++] = c.b; gTerrain.vertexData[idx++] = c.a;
    }
    
    const int x = TerrainX2QuadX(tx), y = TerrainY2QuadY(ty);
    gMatPosition.x = x;
    gMatPosition.y = y;
    
}

// TODO this is incorrect (only one corner taken into account)
float RangeDrawer::GetHeight(float tx, float ty) {
    const int x = TerrainX2QuadX(tx), y = TerrainY2QuadY(abs(ty));
    
    return gTerrain.hmap[(int)y][(int)x];
    
}