//
//  protracerinputhandler.h
//  DGIProject
//
//  Created by Dennis Ekström on 17/05/14.
//  Copyright (c) 2014 Dennis Ekström. All rights reserved.
//

#ifndef __DGIProject__protracerinputhandler__
#define __DGIProject__protracerinputhandler__

#include <iostream>
#include <vector>
#include <glm/glm.hpp>
#include "rangeterrain.h"
#include "rangetweakbar.h"

using namespace glm;

struct GreenInfo {
    string name;
    vec3 targetPos;
    vec3 targetCenterOffset; // y is ignored
    float radius;
    float xtilt;
    float ytilt;
    float slopeSpread;
    ControlPointFuncType slopeFunc;
};

class ProtracerInputHandler {
    
public:
    
    static vector<GreenInfo> LoadFromFile(const string &filename);
    static TerrainObject GreenInfo2TerrainObject(const GreenInfo &green);
};

//extern ProtracerInputHandler gInputHandler;

#endif /* defined(__DGIProject__protracerinputhandler__) */
