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

const int RangeTerrain::floatsPerVertex         = 8;
const int RangeTerrain::floatsPerTriangle       = floatsPerVertex * 3;
const int RangeTerrain::floatsPerTrianglePair   = floatsPerTriangle * 2;
const int RangeTerrain::floatsPerRow            = floatsPerTrianglePair * (X_INTERVAL - 1);

RangeTerrain::RangeTerrain() {

    controlPointChangeRequiresHMapRegeneration = false;
    
    changedControlPoints    = new ChangeManager(X_INTERVAL, Y_INTERVAL);
    changedHMapCoords       = new ChangeManager(X_INTERVAL, Y_INTERVAL);
    changedVertices         = new ChangeManager(X_INTERVAL, Y_INTERVAL);
    
//    hmapInternalChanges     = new HMapChangeManager();
    
//    changedControlPoints.reserve(Y_INTERVAL * X_INTERVAL);
//    changedHMapCoords.reserve(Y_INTERVAL * X_INTERVAL);
//    changedVertices.reserve(Y_INTERVAL * X_INTERVAL);
    
    changedVertexIndices.reserve(Y_INTERVAL * X_INTERVAL * 6); // Each vertex appears in 6 different triangles
    
    FlattenHMap();
    
    SetControlPoint(0, 0, 4, 4, FUNC_COS);
    SetControlPoint(8, 8, 3, 7, FUNC_LINEAR);
    SetControlPoint(10, 12, 6, 2, FUNC_SIN);
    SetControlPoint(11, 12, 6, 2, FUNC_SIN);
    SetControlPoint(10, 13, 6, 2, FUNC_SIN);
    SetControlPoint(11, 13, 6, 2, FUNC_SIN);
    
    GenerateAll();
}

RangeTerrain::~RangeTerrain() {
    
    delete changedControlPoints;
    delete changedHMapCoords;
    delete changedVertices;
    
//    delete hmapInternalChanges;
    
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
        if(controlPointChangeRequiresHMapRegeneration || h < old_h || spread != old_spread || functype != old_functype)
            controlPointChangeRequiresHMapRegeneration = true;
        
        // delete old pointer
        delete controlPoints[y][x];
    }
    
    // perform change
    controlPoints[y][x] = new ControlPoint(x, y, h, spread, functype);
    
    // remember change
    changedControlPoints->SetChanged(x, y);
}

void RangeTerrain::FlattenHMap() {
    
    for ( int y=0; y<Y_INTERVAL; y++ )
        for ( int x=0; x<X_INTERVAL; x++ )
            hmap[y][x] = 0;
}

void RangeTerrain::UpdateAll() {

    if (controlPointChangeRequiresHMapRegeneration)
        GenerateHMap();
    else
        UpdateHMap();

    UpdateNormals();
    UpdateTrianglePairs();
    UpdateVertexData();
    
    changedControlPoints->Reset();
    controlPointChangeRequiresHMapRegeneration = false;
}

void RangeTerrain::GenerateAll() {
    
    GenerateHMap();
    GenerateNormals();
    GenerateTrianglePairs();
    GenerateVertexData();
    
    changedControlPoints->Reset();
    controlPointChangeRequiresHMapRegeneration = false;
}

void RangeTerrain::UpdateHMap() { // Changes only upwards
    
    changedHMapCoords->Reset();

    int x, y;
    ControlPoint* cp;
    for ( xy &xy : changedControlPoints->identifiers ) {
        // UpdateHMap(*controlPoints[xy.y][xy.x]);

        x = xy.x;
        y = xy.y;
        cp = controlPoints[y][x];
        
        // Height of control point itself
        if (cp->h > hmap[y][x]) {
            hmap[y][x] = cp->h;
            changedHMapCoords->SetChanged(x, y);
        } else {
            std::cout << "WARNING: Control point height (" << cp->h << ") ignored at (" << x << ", " << y << ")" << endl;
        }
        
        // Height of surrounding points
        int min_x = ceil(std::max(x - cp->spread / GRID_RES, 0.0f));
        int max_x = floor(std::min(x + cp->spread / GRID_RES, float(X_INTERVAL - 1)));
        int min_y = ceil(std::max(y - cp->spread / GRID_RES, 0.0f));
        int max_y = floor(std::min(y + cp->spread / GRID_RES, float(Y_INTERVAL - 1)));
        for (int yy=min_y; yy<=max_y; yy++) {
            for (int xx=min_x; xx<=max_x; xx++) {
                float h = cp->lift(xx, yy);
                if (h > hmap[yy][xx]) {
                    hmap[yy][xx] = h;
                    changedHMapCoords->SetChanged(xx, yy);
                }
            }
        }
    }
}

void RangeTerrain::GenerateHMap() {
    
    changedHMapCoords->Reset();
    
    ControlPoint* cp;
    for ( int y=0; y<Y_INTERVAL; y++ ) {
        for ( int x=0; x<X_INTERVAL; x++ ) {
            if (controlPoints[y][x]) {
//                UpdateHMap(*controlPoints[y][x]);
                
                cp = controlPoints[y][x];
                
                // Height of control point itself
                if (!changedHMapCoords->DidChange(x, y)) {  // Change if not previously changed...
                    hmap[y][x] = cp->h;
                    changedHMapCoords->SetChanged(x, y);
                } else  if (cp->h > hmap[y][x]) {           // ... or if higher
                    hmap[y][x] = cp->h;
                    // No need to register change since we know it's already been registered
                } else {
                    std::cout << "WARNING: Control point height (" << cp->h << ") ignored at (" << x << ", " << y << ")" << endl;
                }
                
                // Height of surrounding points
                int min_x = ceil(std::max(x - cp->spread / GRID_RES, 0.0f));
                int max_x = floor(std::min(x + cp->spread / GRID_RES, float(X_INTERVAL - 1)));
                int min_y = ceil(std::max(y - cp->spread / GRID_RES, 0.0f));
                int max_y = floor(std::min(y + cp->spread / GRID_RES, float(Y_INTERVAL - 1)));
                for (int yy=min_y; yy<=max_y; yy++) {
                    for (int xx=min_x; xx<=max_x; xx++) {
                        float h = cp->lift(xx, yy);
                        if (!changedHMapCoords->DidChange(xx, yy)) {    // Change if not previously changed...
                            hmap[yy][xx] = h;
                            changedHMapCoords->SetChanged(xx, yy);
                        } else if (h > hmap[yy][xx]) {                  // ... or if higher
                            hmap[yy][xx] = h;
                            // No need to register change since we know it's already been registered
                        }
                    }
                }
            }
        }
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

void RangeTerrain::UpdateTrianglePairs() {
    
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
        
//        vertexNeedsUpdate[x  ][y  ] = true;
//        vertexNeedsUpdate[x  ][y+1] = true;
//        vertexNeedsUpdate[x+1][y  ] = true;
//        vertexNeedsUpdate[x+1][y+1] = true;
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
            
//            vertexNeedsUpdate[x  ][y  ] = true;
//            vertexNeedsUpdate[x  ][y+1] = true;
//            vertexNeedsUpdate[x+1][y  ] = true;
//            vertexNeedsUpdate[x+1][y+1] = true;
        }
    }
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

//void RangeTerrain::UpdateHMap(ControlPoint &cp) {
//    
//    // Height of control point itself
////    if (cp.h >= hmap[cp.y][cp.x])
//        hmapInternalChanges->SetHeight(cp.x, cp.y, cp.h);
////    else
////        std::cout << "WARNING: Control point height (" << cp.h << ") ignored at (" << cp.x << ", " << cp.y << ")" << endl;
//    
//    // Height of surrounding points
//    int min_x = ceil(std::max(cp.x - cp.spread / GRID_RES, 0.0f));
//    int max_x = floor(std::min(cp.x + cp.spread / GRID_RES, float(X_INTERVAL - 1)));
//    int min_y = ceil(std::max(cp.y - cp.spread / GRID_RES, 0.0f));
//    int max_y = floor(std::min(cp.y + cp.spread / GRID_RES, float(Y_INTERVAL - 1)));
//    for ( int yy=min_y; yy<=max_y; yy++ )
//        for ( int xx=min_x; xx<=max_x; xx++ )
//            hmapInternalChanges->SetHeight(xx, yy, cp.lift(xx, yy));
//}
/*
void RangeTerrain::UpdateHMap(ControlPoint &cp) {
    
    // Height of control point itself
    if (cp.h > hmap[cp.y][cp.x]) {
        hmap[cp.y][cp.x] = cp.h;
        changedHMapCoords->SetChanged(cp.x, cp.y);
    }
    else
        std::cout << "WARNING: Control point height (" << cp.h << ") ignored at (" << cp.x << ", " << cp.y << ")" << endl;
    
    // Height of surrounding points
    int min_x = ceil(std::max(cp.x - cp.spread / GRID_RES, 0.0f));
    int max_x = floor(std::min(cp.x + cp.spread / GRID_RES, float(X_INTERVAL - 1)));
    int min_y = ceil(std::max(cp.y - cp.spread / GRID_RES, 0.0f));
    int max_y = floor(std::min(cp.y + cp.spread / GRID_RES, float(Y_INTERVAL - 1)));
    for (int yy=min_y; yy<=max_y; yy++) {
        for (int xx=min_x; xx<=max_x; xx++) {
            float h = cp.lift(xx, yy);
            if (h > hmap[yy][xx]) {
                hmap[yy][xx] = h;
                changedHMapCoords->SetChanged(xx, yy);
            }
        }
    }
}*/
/*
void RangeTerrain::RegenerateHMap(ControlPoint &cp) {
    
    // Height of control point itself
    if (cp.h > hmap[cp.y][cp.x]) {
        hmap[cp.y][cp.x] = cp.h;
        changedHMapCoords->SetChanged(cp.x, cp.y);
    }
    else
        std::cout << "WARNING: Control point height (" << cp.h << ") ignored at (" << cp.x << ", " << cp.y << ")" << endl;
    
    // Height of surrounding points
    int min_x = ceil(std::max(cp.x - cp.spread / GRID_RES, 0.0f));
    int max_x = floor(std::min(cp.x + cp.spread / GRID_RES, float(X_INTERVAL - 1)));
    int min_y = ceil(std::max(cp.y - cp.spread / GRID_RES, 0.0f));
    int max_y = floor(std::min(cp.y + cp.spread / GRID_RES, float(Y_INTERVAL - 1)));
    for (int yy=min_y; yy<=max_y; yy++) {
        for (int xx=min_x; xx<=max_x; xx++) {
            float h = cp.lift(xx, yy);
            if (h > hmap[yy][xx]) {
                hmap[yy][xx] = h;
                changedHMapCoords->SetChanged(xx, yy);
            }
        }
    }
}*/

void RangeTerrain::UpdateNormal(const int &x, const int &y) {

    float hn = (y == 0 ?              2*hmap[y][x]-hmap[y+1][x] : hmap[y-1][x]); // North
    float hs = (y == Y_INTERVAL - 1 ? 2*hmap[y][x]-hmap[y-1][x] : hmap[y+1][x]); // South
    float hw = (x == 0 ?              2*hmap[y][x]-hmap[y][x+1] : hmap[y][x-1]); // West
    float he = (x == X_INTERVAL - 1 ? 2*hmap[y][x]-hmap[y][x-1] : hmap[y][x+1]); // East
    
    normals[y][x] = glm::normalize(vec3(hw - he, 2 * GRID_RES, hn - hs));
}

void RangeTerrain::UpdateTrianglePair(const int &x, const int &y) {
    
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
}

// [TODO: assuming diagonalUp == true]
void RangeTerrain::UpdateVertexData(const int &x, const int &y) {
    /*
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
    vec3 v = vec3(x * GRID_RES, hmap[y][x], y * GRID_RES);
    vec3 n = normals[y][x];
    
    // v1 in South East triangle pair
    if (x < X_INTERVAL - 1 && y < Y_INTERVAL - 1) {
        idx = (y  )*floatsPerRow + (x  )*floatsPerTrianglePair;
        changedVertexIndices.push_back( idx );
        SetVertexData(idx, v, vec2(0, 0), n);
    }
    
    // v2 in North East triangle pair
    if (x < X_INTERVAL - 1 && y > 0) {
        idx = (y-1)*floatsPerRow + (x  )*floatsPerTrianglePair + 1*floatsPerVertex;
        changedVertexIndices.push_back( idx );
        SetVertexData(idx, v, vec2(0, 1), n);
        
        idx = (y-1)*floatsPerRow + (x  )*floatsPerTrianglePair + 5*floatsPerVertex;
        changedVertexIndices.push_back( idx );
        SetVertexData(idx, v, vec2(0, 1), n);
    }
    
    // v3 in South West triangle pair
    if (x > 0 && y < Y_INTERVAL - 1) {
        idx = (y  )*floatsPerRow + (x-1)*floatsPerTrianglePair + 2*floatsPerVertex;
        changedVertexIndices.push_back( idx );
        SetVertexData(idx, v, vec2(1, 0), n);
        
        idx = (y  )*floatsPerRow + (x-1)*floatsPerTrianglePair + 4*floatsPerVertex;
        changedVertexIndices.push_back( idx );
        SetVertexData(idx, v, vec2(1, 0), n);
    }
    
    // v4 in North West triangle pair
    if (x > 0 && y > 0) {
        idx = (y-1)*floatsPerRow + (x-1)*floatsPerTrianglePair + 3*floatsPerVertex;
        changedVertexIndices.push_back( idx );
        SetVertexData(idx, v, vec2(1, 1), n);
    }
}






