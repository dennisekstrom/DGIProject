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

using namespace glm;

struct GreenInfo {
    vec3 targetPos;
    vec3 targetCenterOffset;
    float radius;
    ControlPointFuncType slopeFunc;
};

class ProtracerInputHandler {
    
private:
    vector<GreenInfo> greens;
    
public:
    void LoadFromFile(const string &filename);
};

extern ProtracerInputHandler gInputHandler;

#endif /* defined(__DGIProject__protracerinputhandler__) */