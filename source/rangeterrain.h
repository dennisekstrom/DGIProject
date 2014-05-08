//
//  rangeterrain.h
//  DGIProject
//
//  Created by Dennis Ekström on 08/05/14.
//  Copyright (c) 2014 Dennis Ekström. All rights reserved.
//

#ifndef DGIProject_rangeterrain_h
#define DGIProject_rangeterrain_h

#include <vector>

enum ControlPointFunc {
    FUNC_LINEAR,
    FUNC_COS,
    FUNC_SIN
};

struct ControlPoint {
    int x,
    int y,
    ControlPointFunc func
};

class RangeTerrain {
private:
    float hmap[1024][1024];
    vector<ControlPoint> controlPoints;
    
public:
    void GenerateHMap
}

#endif
