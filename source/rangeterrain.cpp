////
////  rangeterrain.cpp
////  DGIProject
////
////  Created by Dennis Ekström on 08/05/14.
////  Copyright (c) 2014 Dennis Ekström. All rights reserved.
////
//
//#include "rangeterrain.h"
//
//void RangeTerrain::SetControlPoint(int x, int y, ControlPointFunc func) {
//    SetControlPoint( ControlPoint { x, y, func } );
//}
// 

#include "rangeterrain.h"
#include <iostream>
#include <glm/glm.hpp>

using namespace std;
using glm::vec3;

const int RangeTerrain::floatsPerVertex = 8;

RangeTerrain::RangeTerrain() {
    
    /*
    SetControlPoint(0, 0, 4, 4, FUNC_COS);
    SetControlPoint(8, 8, 3, 7, FUNC_LINEAR);
    SetControlPoint(10, 12, 6, 2, FUNC_SIN);
    SetControlPoint(11, 12, 6, 2, FUNC_SIN);
    SetControlPoint(10, 13, 6, 2, FUNC_SIN);
    SetControlPoint(11, 13, 6, 2, FUNC_SIN);
    */
    
    GenerateEverythingFromControlPoints();
}

RangeTerrain::~RangeTerrain() {
    for (int y=0; y<Y_INTERVAL; y++)
        for (int x=0; x<X_INTERVAL; x++)
            delete controlPoints[y][x];
}

void RangeTerrain::SetControlPoint(int x, int y, float h, float spread, ControlPointFuncType typefunc) {
    delete controlPoints[y][x];
    controlPoints[y][x] = new ControlPoint(x, y, h, spread, typefunc);
}


void RangeTerrain::GenerateEverythingFromControlPoints() {
    FlattenHMap();
    GenerateHMapFromControlPoints();
    GenerateNormalsFromHMap();
    GenerateVertexData();
}


void RangeTerrain::FlattenHMap() {
    // Flatten hmap
    for (int y=0; y<Y_INTERVAL; y++)
        for (int x=0; x<X_INTERVAL; x++)
            hmap[y][x] = 0;
}


void RangeTerrain::GenerateHMapFromControlPoints() {
    
    for (int y=0; y<Y_INTERVAL; y++) {
        for (int x=0; x<X_INTERVAL; x++) {
            
            if (controlPoints[y][x]) {
                ControlPoint &cp = *controlPoints[y][x];
                
                // Height of control point itself
                if (cp.h >= hmap[y][x])
                    hmap[y][x] = cp.h;
                else
                    cout << "WARNING: Control point height (" << cp.h << ") ignored at (" << x << ", " << y << ")" << endl;
                
                // Height of surrounding points
                int min_x = ceil(max(cp.x - cp.spread / GRID_RES, 0.0f));
                int max_x = floor(min(cp.x + cp.spread / GRID_RES, float(X_INTERVAL - 1)));
                int min_y = ceil(max(cp.y - cp.spread / GRID_RES, 0.0f));
                int max_y = floor(min(cp.y + cp.spread / GRID_RES, float(Y_INTERVAL - 1)));
                for (int yy=min_y; yy<=max_y; yy++) {
                    for (int xx=min_x; xx<=max_x; xx++) {
                        float h = cp.lift(xx, yy);
                        if (h > hmap[yy][xx])
                            hmap[yy][xx] = h;
                    }
                }
            }
        }
    }
    
    dataChanged = true;
}


void RangeTerrain::GenerateNormalsFromHMap() {
    
    float hn, hs, hw, he;
    vec3 n;
    for ( int y=0; y<Y_INTERVAL; y++ ) {
        for ( int x=0; x<X_INTERVAL; x++ ) {
            hn = (y == 0 ?              2*hmap[y][x]-hmap[y+1][x] : hmap[y-1][x]); // North
            hs = (y == Y_INTERVAL - 1 ? 2*hmap[y][x]-hmap[y-1][x] : hmap[y+1][x]); // South
            hw = (x == 0 ?              2*hmap[y][x]-hmap[y][x+1] : hmap[y][x-1]); // West
            he = (x == X_INTERVAL - 1 ? 2*hmap[y][x]-hmap[y][x-1] : hmap[y][x+1]); // East
            
            n.x = hw - he;
            n.y = 2 * GRID_RES;
            n.z = hn - hs;
            normals[y][x] = glm::normalize(n);
        }
    }
    
    dataChanged = true;
}

void RangeTerrain::GenerateVertexData() {
    float h1, h2, h3, h4;
    glm::vec3 *n1, *n2, *n3, *n4;
    int v = 0;
    for (int y=1; y<Y_INTERVAL; y++) {
        for (int x=1; x<X_INTERVAL; x++) {
            h1 = hmap[y-1][x-1];
            h2 = hmap[y][x-1];
            h3 = hmap[y-1][x];
            h4 = hmap[y][x];
            
            n1 = &normals[y-1][x-1];
            n2 = &normals[y][x-1];
            n3 = &normals[y-1][x];
            n4 = &normals[y][x];
            
            vdata[v++] = (x-1) * GRID_RES;
            vdata[v++] = h1;
            vdata[v++] = (y-1) * GRID_RES;
            
            vdata[v++] = 0;
            vdata[v++] = 0;
            
            vdata[v++] = n1->x;
            vdata[v++] = n1->y;
            vdata[v++] = n1->z;
            
            
            
            vdata[v++] = (x-1) * GRID_RES;
            vdata[v++] = h2;
            vdata[v++] = y * GRID_RES;
            
            vdata[v++] = 0;
            vdata[v++] = 1;
            
            vdata[v++] = n2->x;
            vdata[v++] = n2->y;
            vdata[v++] = n2->z;
            
            
            
            vdata[v++] = x * GRID_RES;
            vdata[v++] = h3;
            vdata[v++] = (y-1) * GRID_RES;
            
            vdata[v++] = 1;
            vdata[v++] = 0;
            
            vdata[v++] = n3->x;
            vdata[v++] = n3->y;
            vdata[v++] = n3->z;
            
            
            
            vdata[v++] = x * GRID_RES;
            vdata[v++] = h4;
            vdata[v++] = y * GRID_RES;
            
            vdata[v++] = 1;
            vdata[v++] = 1;
            
            vdata[v++] = n4->x;
            vdata[v++] = n4->y;
            vdata[v++] = n4->z;
            
            
            
            vdata[v++] = x * GRID_RES;
            vdata[v++] = h3;
            vdata[v++] = (y-1) * GRID_RES;
            
            vdata[v++] = 1;
            vdata[v++] = 0;
            
            vdata[v++] = n3->x;
            vdata[v++] = n3->y;
            vdata[v++] = n3->z;
            
            
            
            vdata[v++] = (x-1) * GRID_RES;;
            vdata[v++] = h2;
            vdata[v++] = y * GRID_RES;
            
            vdata[v++] = 0;
            vdata[v++] = 1;
            
            vdata[v++] = n2->x;
            vdata[v++] = n2->y;
            vdata[v++] = n2->z;
        }
    }

}

