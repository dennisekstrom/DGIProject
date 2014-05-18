//
//  protracerinputhandler.cpp
//  DGIProject
//
//  Created by Dennis Ekström on 17/05/14.
//  Copyright (c) 2014 Dennis Ekström. All rights reserved.
//

#include "protracerinputhandler.h"
#include "rangedrawer.h"
#include <glm/glm.hpp>
#include <fstream>

using namespace glm;

//ProtracerInputHandler gInputHandler;

vector<GreenInfo> ProtracerInputHandler::LoadFromFile(const string &filename) {
    
    vector<GreenInfo> greens;
    
    std::ifstream infile(string("/Users/dennis/Programming/kth/dgi/DGIProject_new/resources/") + filename);
    string name;
    float x, y, z, offset_x, offset_y, offset_z, radius, xtilt, ytilt, spread;
    int functype;
    while (infile >> name >> x >> y >> z >> offset_x >> offset_y >> offset_z >> radius >> xtilt >> ytilt >> spread >> functype) {
        
        ControlPointFuncType ft;
        switch (functype) {
            case 0:     ft = FUNC_LINEAR;   break;
            case 1:     ft = FUNC_COS;      break;
            case 2:     ft = FUNC_SIN;      break;
            default:
                cout << "ERROR: Illegal functype." << endl;
                assert(0);
        }
        
        GreenInfo green_info = {
            name,
            vec3(x, y, z),
            vec3(offset_x, offset_y, offset_z),
            radius,
            xtilt, ytilt,
            spread,
            ft
        };
        
        greens.push_back(green_info);
    }
    
    return greens;
}

inline float dist(const float &x1, const float &y1, const float &x2, const float &y2) {
    float dx = x2 - x1;
    float dy = y2 - y1;
    return sqrt(dx * dx + dy * dy);
}

TerrainObject ProtracerInputHandler::GreenInfo2TerrainObject(const GreenInfo &green) {
    
    TerrainObject to;
    to.name = green.name;
    to.height = 0;               // TODO
    to.xtilt = green.xtilt;
    to.ytilt = green.ytilt;
    to.cp_spread = green.slopeSpread;
    to.cp_functype = green.slopeFunc;
    
    
    float x = green.targetPos.x;
    float y = -green.targetPos.z;
    
    int min_x = ceil(std::max(green.targetPos.x - green.radius, 0.0f));
    int max_x = floor(std::min(green.targetPos.x + green.radius, float(X_INTERVAL - 1)));
    int min_y = ceil(std::max(-green.targetPos.z - green.radius, 0.0f));
    int max_y = floor(std::min(-green.targetPos.z + green.radius, float(Y_INTERVAL - 1)));
    
    for (int yy=min_y; yy<=max_y; yy++) {
        for (int xx=min_x; xx<=max_x; xx++) {
            if (dist(x, y, xx, yy) <= green.radius) {
                to.marking.insert( { xx, yy } );
            }
        }
    }
    
    to.height = green.targetPos.y
              - green.targetCenterOffset.x * tan(DEG2RAD(green.xtilt))
              + green.targetCenterOffset.x * tan(DEG2RAD(green.ytilt));
    
    return to;
}