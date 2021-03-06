//
//  rangedrawer.cpp
//  DGIProject
//
//  Created by Dennis Ekström on 11/05/14.
//  Copyright (c) 2014 Dennis Ekström. All rights reserved.
//

#include "rangedrawer.h"

RangeDrawer gRangeDrawer;

RangeDrawer::RangeDrawer() {
    
    for ( int y=0; y<Y_INTERVAL-1; y++ )
        for ( int x=0; x<X_INTERVAL-1; x++ )
            marked[y][x] = false;
    
    markChanged = false;
    
    markedWithShift = new AreaMarkingManager(Y_INTERVAL - 1, X_INTERVAL - 1);
    
    mouseIsDown = false;
    
    markMode = NONE;
}

RangeDrawer::~RangeDrawer() {
    delete markedWithShift;
}

void RangeDrawer::ColorQuad(const int&x, const int &y, const vec4 &c) {
    // Using same methods as in RangeTerrain::UpdateVertexData() and RangeTerrain::SetVertexData()
    int idx = (y  )*FLOATS_PER_ROW + (x  )*FLOATS_PER_TRIANGLE_PAIR;
    
    for ( int i=0; i<6; i++ ) {
        gTerrain.changedVertexIndices.push_back( idx );
        idx += 8; // Skip v, n, and t
        gTerrain.vertexData[idx++] = c.r;
        gTerrain.vertexData[idx++] = c.g;
        gTerrain.vertexData[idx++] = c.b;
        gTerrain.vertexData[idx++] = c.a;
    }
}

void RangeDrawer::MarkTerrain() {
    
    gTerrain.UpdateVertexData();
    gTerrain.changedVertices->Reset();
    
    for ( auto xy : currentlyMarked ) {
        
        // x, y are coordinates of the quad
        ColorQuad(xy.x, xy.y, vec4(1,1,1,1)); // white
    }
    
    // Color the target position
    if (targetMarked)
        ColorQuad(targetMarkPos.x, targetMarkPos.y, vec4(1,0,0,1)); // red
    
    // Color the tee position
    if (teeMarked)
        ColorQuad(teeMarkPos.x, teeMarkPos.y, vec4(0,0,1,1)); // blue
    
    ResetMarkChanged();
}

float RangeDrawer::GetAverageHeight(const set<xy, xy_comparator> &marking) {
    
    if (marking.empty())
        return 0;
    
    float sum = 0;
    for (auto xy : marking)
        sum += gTerrain.hmap[xy.y][xy.x];
    return sum / marking.size();
}

glm::vec2 RangeDrawer::GetCenter(const set<xy, xy_comparator> &marking) {
    if (marking.empty())
        return glm::vec2(0, 0);
    
    float sumx = 0, sumy = 0;
    for (auto xy : marking) {
        sumx += 4*xy.x + 2;
        sumy += 4*xy.y + 2;
    }
    return glm::vec2(sumx / (4*marking.size()), sumy / (4*marking.size()));
}

void RangeDrawer::LiftMarked(const float &lift, const float &spread, const ControlPointFuncType &functype) {
    
    int x, y;
    for ( auto xy : currentlyMarked ) {
        x = xy.x;
        y = xy.y;
        
        LiftVertex(x  , y  , lift, spread, functype);     // v1
        
        // For vertices 2-4, we need to check for other coordinates being marked to avoid multiple adjustments of same controlpoint
        if (y == Y_INTERVAL - 2 || !marked[y+1][x  ])
            LiftVertex(x  , y+1, lift, spread, functype); // v2
        
        if (x == X_INTERVAL - 2 || (!marked[y  ][x+1] && (y == 0 || !marked[y-1][x+1])))
            LiftVertex(x+1, y  , lift, spread, functype); // v3
        
        if ((y == Y_INTERVAL - 2 && x == X_INTERVAL - 2) || (!marked[y+1][x  ] && !marked[y  ][x+1] && !marked[y+1][x+1]))
            LiftVertex(x+1, y+1, lift, spread, functype); // v4
    }
}

void RangeDrawer::TiltMarked(const float &xtilt, const float &ytilt, const float &spread, const ControlPointFuncType &functype) {
    
    glm::vec2 center = GetCenter(currentlyMarked);
    const float &cx = center.x, &cy = center.y;
    const float tanx = tan(DEG2RAD(xtilt)), tany = tan(DEG2RAD(ytilt));
    
    float lift;
    int x, y;
    for ( auto xy : currentlyMarked ) {
        x = xy.x;
        y = xy.y;
        
        // v1
        lift = (float(x  ) - cx) * float(GRID_RES) * tanx + (float(y  ) - cy) * float(GRID_RES) * tany;
        LiftVertex(x  , y  , lift, spread, functype);
        
        
        // For vertices 2-4, we need to check for other coordinates being marked to avoid multiple adjustments of same controlpoint
        if (y == Y_INTERVAL - 2 || !marked[y+1][x  ]) {
            lift = ((x  ) - cx) * float(GRID_RES) * tanx + ((y+1) - cy) * float(GRID_RES) * tany;
            LiftVertex(x  , y+1, lift, spread, functype);
        }
        
        if (x == X_INTERVAL - 2 || (!marked[y  ][x+1] && (y == 0 || !marked[y-1][x+1]))) {
            lift = ((x+1) - cx) * float(GRID_RES) * tanx + ((y  ) - cy) * float(GRID_RES) * tany;
            LiftVertex(x+1, y  , lift, spread, functype);
            
        }
        
        if ((y == Y_INTERVAL - 2 && x == X_INTERVAL - 2) || (!marked[y+1][x  ] && !marked[y  ][x+1] && !marked[y+1][x+1])) {
            lift = ((x+1) - cx) * float(GRID_RES) * tanx + ((y+1) - cy) * float(GRID_RES) * tany;
            LiftVertex(x+1, y+1, lift, spread, functype);
            
        }
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
    
    switch (markMode) {
    
        case MARK_CONTROL_POINT:
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
            break;
            
        case MARK_TEE:
            UnmarkAll();
            if (teeMarked) {
                // Mark and unmark previous tee position for change to be registered and vertex to be reset
                Mark(teeMarkPos.x, teeMarkPos.y);
                Unmark(teeMarkPos.x, teeMarkPos.y);
            }
            teeMarked = true;
            teeMarkPos = { x, y };
            teeTerrainPos = glm::vec2(tx, ty);
            SetMarkChanged();
            break;
        
        case MARK_TARGET:
            UnmarkAll();
            if (targetMarked) {
                // Mark and unmark previous tee position for change to be registered and vertex to be reset
                Mark(targetMarkPos.x, targetMarkPos.y);
                Unmark(targetMarkPos.x, targetMarkPos.y);
            }
            targetMarked = true;
            targetMarkPos = { x, y };
            targetTerrainPos = glm::vec2(tx, ty);
            SetMarkChanged();
            break;
        
        case NONE:
            // do nothing
            break;
    }
}

float RangeDrawer::GetHeight(float tx, float ty) {

    const int x = TerrainX2QuadX(tx), y = TerrainY2QuadY(ty);
    
    int _x = x, _y = y;
    if (_x == X_INTERVAL - 1) { _x--; };
    if (_y == X_INTERVAL - 1) { _y--; };
    assert( 0 <= _x && _x < X_INTERVAL-1 && 0 <= _y && _y < Y_INTERVAL-1 );
    
    float hsum = gTerrain.hmap[y  ][x  ]
               + gTerrain.hmap[y  ][x+1]
               + gTerrain.hmap[y+1][x  ]
               + gTerrain.hmap[y+1][x+1];
    return hsum / 4.0f;
}