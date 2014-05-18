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
    string dummyLine;
    getline(infile, dummyLine);
    
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