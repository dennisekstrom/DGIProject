//
//  rangetweakbar.h
//  DGIProject
//
//  Created by Dennis Ekström on 14/05/14.
//  Copyright (c) 2014 Dennis Ekström. All rights reserved.
//

#ifndef __DGIProject__rangetweakbar__
#define __DGIProject__rangetweakbar__

#include <iostream>
#include <vector>
#include <AntTweakBar.h>
#include "rangedrawer.h"

using namespace std;

struct TerrainObject {
    string name;
    float height;
    float xtilt;
    float ytilt;
    float cp_spread;
    ControlPointFuncType cp_functype;
    set<xy, xy_comparator> marking;
};

class RangeTweakBar {
    
private:
    
    int                     objectCounter;
    TerrainObject*          currentObject;
    vector<TerrainObject*>  objects;
    
    void SelectObject(TerrainObject* to);
    
    void TerrainFromFile();
    void NewTerrainObject();
    bool RemoveTerrainObject(TerrainObject* &obj);
    
    void TakeAction(const float &dt);
    
public:
    RangeTweakBar();
    ~RangeTweakBar();
    
    void Init(const int &screenWidth, const int &screenHeight);
    void Update(const float &dt);
    void Draw();
};
extern RangeTweakBar gTweakBar;

#endif /* defined(__DGIProject__rangetweakbar__) */
