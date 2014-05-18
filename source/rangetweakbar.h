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
    int _screenWidth;
    int _screenHeight;
    
    int                     objectCounter;
    TerrainObject*          currentObject;
    vector<TerrainObject*>  objects;
    
    bool RemoveTerrainObject(TerrainObject* obj);
    
public:
    RangeTweakBar();
    ~RangeTweakBar();
    
    void Init(const int &screenWidth, const int &screenHeight);
    void Draw();
    void Update(const float &dt);
    void TakeAction(const float &dt);

    void TerrainFromFile();
    void NewTerrainObject();
    void UpdateObjectToCurrent(TerrainObject* object);
};
extern RangeTweakBar gTweakBar;

#endif /* defined(__DGIProject__rangetweakbar__) */
