//
//  rangeterrain.cpp
//  DGIProject
//
//  Created by Dennis Ekström on 08/05/14.
//  Copyright (c) 2014 Dennis Ekström. All rights reserved.
//

#include "rangeterrain.h"
#include <iostream>

using namespace std;

void RangeTerrain::SetControlPoint(int x, int y, float h, float spread, ControlPointFuncType typefunc) {
    SetControlPoint( ControlPoint (x, y, h, spread, typefunc) );
}


void RangeTerrain::SetControlPoint(ControlPoint cp) {
    controlPoints.push_back(cp);
}


void RangeTerrain::GenerateHMapFromControlPoints() {
    
    for (int i=0; i<controlPoints.size(); i++) {
        ControlPoint &cp = controlPoints[i];
        
        // Height of control point itself
        if (cp.h >= hmap[cp.y][cp.x])
            hmap[cp.y][cp.x] = cp.h;
        else
            cout << "WARNING: Control point height (" << cp.h << ") ignored at (" << cp.x << ", " << cp.y << ")";
        
        // Height of surrounding points
        float min_x = max(cp.x - cp.spread, 0.0f);
        float max_x = min(cp.x + cp.spread, float(X_INTERVAL));
        float min_y = max(cp.y - cp.spread, 0.0f);
        float max_y = min(cp.y + cp.spread, float(Y_INTERVAL));
        for (int y=min_y; y<max_y; i++) {
            for (int x=min_x; x<max_x; i++) {
                float h = cp.lift(x, y);
                if (h > hmap[y][x])
                    hmap[y][x] = h;
            }
        }
    }
}
