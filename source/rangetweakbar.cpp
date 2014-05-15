//
//  rangetweakbar.cpp
//  DGIProject
//
//  Created by Dennis Ekström on 14/05/14.
//  Copyright (c) 2014 Dennis Ekström. All rights reserved.
//

#include "rangetweakbar.h"
#include "tdogl/Camera.h"
#include <glm/glm.hpp>
#include <GLUT/glut.h>

RangeTweakBar gTweakBar;

TwBar* controlBar;
TwBar* objectBar;

ControlPointFuncType functype = FUNC_LINEAR, functypePrev = FUNC_LINEAR;

float       height      = 0,    heightPrev      = 0;
float       xtilt       = 0,    xtiltPrev       = 0;
float       ytilt       = 0,    ytiltPrev       = 0;
float       spread      = 5,    spreadPrev      = 5;

RangeTweakBar::RangeTweakBar() {
    
    currentObject = NULL;
}

RangeTweakBar::~RangeTweakBar() {}

void RangeTweakBar::Init(const int &screenWidth, const int &screenHeight) {
    
    _screenWidth = screenWidth;
    _screenHeight = screenHeight;
    
    TwInit(TW_OPENGL_CORE, NULL);
    TwWindowSize(screenWidth, screenHeight);
    
    controlBar = TwNewBar("Controls");
    TwDefine("Controls label=CONTROLS");
    TwDefine("Controls position='0 0'");
    TwDefine("Controls size='512 256'");
    TwDefine("Controls resizable=false");
    TwDefine("Controls movable=false");
    TwDefine("Controls fontresizable=false");
    TwDefine("Controls color='255 255 255' alpha=63 ");
    TwDefine("Controls text=light");
    
    TwAddVarRW(controlBar, "Height", TW_TYPE_FLOAT, &height,
               "min=-20 max=20 step=0.25 precision=2 keyIncr=Y keyDecr=I help='Raise/lower selected segments.' ");
    
    TwAddVarRW(controlBar, "X-Tilt", TW_TYPE_FLOAT, &xtilt,
               "min=-85 max=85 step=2 keyIncr=K keyDecr=H help='Tilt selected segments along x-axis.' ");
    
    TwAddVarRW(controlBar, "Y-Tilt", TW_TYPE_FLOAT, &ytilt,
               "min=-85 max=85 step=2 keyIncr=U keyDecr=J help='Tilt selected segments along y-axis.' ");
    
    TwAddVarRW(controlBar, "Spread", TW_TYPE_FLOAT, &spread,
               "min=0 max=20 step=0.25 keyIncr=+ keyDecr=- help='Set the spread for the selected segments.' ");
    
    TwType twCPFuncType = TwDefineEnum("CPFuncType", NULL, 0);
    
    TwAddVarRW(controlBar, "Functype", twCPFuncType, &functype,
               "enum='0 {Linear}, 1 {Cos}, 2 {Sin}' help='Set the spread for the selected segments.' ");
    
    TwAddSeparator(controlBar, NULL, NULL);
    
    TwAddButton(controlBar,
                "Flatten terrain",
                (TwButtonCallback) [] (void* clientData) {
                    gTerrain.Reset();
                },
                NULL,
                "key=R help='Flatten the entire terrain.' ");
    
    TwAddButton(controlBar,
                "Clear selection",
                (TwButtonCallback) [] (void* clientData) {
                    gRangeDrawer.UnmarkAll();
                },
                NULL,
                "key=C help='Clear the current selection.' ");
    
    TwAddButton(controlBar,
                "Flatten selection",
                (TwButtonCallback) [] (void* clientData) {
                    float h = gRangeDrawer.GetAverageHeightOfMarked();
                    gRangeDrawer.FlattenMarked(h, spread, functype);
                },
                NULL,
                "key=SPACE help='Flatten the current selection.' ");
    
    extern bool gLeftCameraFullscreen;
    extern bool gLeftCameraUseColor;
    extern bool gRightCameraUseColor;
    extern tdogl::Camera gCamera1;
    extern glm::vec3 gLightPosition;
    
    TwAddSeparator(controlBar, NULL, NULL);
    
    TwAddButton(controlBar,
                "Toggle fullscreen",
                (TwButtonCallback) [] (void* clientData) {
                    gLeftCameraFullscreen = !gLeftCameraFullscreen;
                    gCamera1.setViewportAspectRatio(gLeftCameraFullscreen ? 2 : 1);
                },
                NULL,
                "key=F help='Turn left view fullscreen on/off.' ");
    
    TwAddButton(controlBar,
                "Toggle left color mode",
                (TwButtonCallback) [] (void* clientData) {
                    gLeftCameraUseColor = !gLeftCameraUseColor;
                },
                NULL,
                "key=1 help='Toggle the color mode of the left view.' ");
    
    TwAddButton(controlBar,
                "Toggle right color mode",
                (TwButtonCallback) [] (void* clientData) {
                    gRightCameraUseColor = !gRightCameraUseColor;
                },
                NULL,
                "key=2 help='Toggle the color mode of the right view.' ");
    
    TwAddButton(controlBar,
                "Light at camera",
                (TwButtonCallback) [] (void* clientData) {
                    gLightPosition = gCamera1.position();
                },
                NULL,
                "key=L help='Position the light at the camera.' ");
    
    objectBar = TwNewBar("Greens");
    TwDefine("Greens label=GREENS");
    TwDefine("Greens position='512 0'");
    TwDefine("Greens size='512 256'");
    TwDefine("Greens resizable=false");
    TwDefine("Greens movable=false");
    TwDefine("Greens fontresizable=false");
    TwDefine("Greens color='255 255 255' alpha=63 ");
    TwDefine("Greens text=light");
    
    TwAddButton(objectBar,
                "New green",
                (TwButtonCallback) [] (void* clientData) { gTweakBar.NewTerrainObject(); },
                NULL,
                "key=R help='Flatten the entire terrain.' ");
    
    TwAddSeparator(objectBar, NULL, NULL);
    
    // tweak bar needs the callbacks
    TwGLUTModifiersFunc(glutGetModifiers);
}

void RangeTweakBar::Draw() {
    TwDraw();
}

void RangeTweakBar::Update(const float &dt) {
    TakeAction(dt);
}

void RangeTweakBar::TakeAction(const float &dt) {
    
    
    if (!currentObject)
        gRangeDrawer.UnmarkAll();
    
    if (gRangeDrawer.currentlyMarked.empty()) {
        height = 0;
        xtilt = 0;
        ytilt = 0;
        return;
    }
    
    // height
    if (height != heightPrev) {
        gRangeDrawer.LiftMarked( height - heightPrev, spread, functype );
        heightPrev = height;
    }
    
    // x-tilt
    if (xtilt != xtiltPrev) {
        gRangeDrawer.TiltMarked( xtilt - xtiltPrev, 0, spread, functype );
        xtiltPrev = xtilt;
    }
    
    // y-tilt
    if (ytilt != ytiltPrev) {
        gRangeDrawer.TiltMarked( 0, ytilt - ytiltPrev, spread, functype );
        ytiltPrev = ytilt;
    }
    
    // spread
    if (spread != spreadPrev) {
        ControlPoint* cp;
        float h;
        int x, y;
        for (auto xy : gRangeDrawer.currentlyMarked) {
            x = xy.x;
            y = xy.y;
            cp = gTerrain.GetControlPoint(x, y);
            h = cp ? cp->h : gTerrain.hmap[y][x];
            gTerrain.SetControlPoint(x, y, h, spread, functype);
        }
        
        spreadPrev = spread;
    }
    
    // functype
    if (functype != functypePrev) {
        ControlPoint* cp;
        float h;
        int x, y;
        for (auto xy : gRangeDrawer.currentlyMarked) {
            x = xy.x;
            y = xy.y;
            cp = gTerrain.GetControlPoint(x, y);
            h = cp ? cp->h : gTerrain.hmap[y][x];
            gTerrain.SetControlPoint(x, y, h, spread, functype);
        }
        
        functypePrev = functype;
    }
}

int num_greens = 0;
void RangeTweakBar::NewTerrainObject() {
    
    
    if (gTweakBar.currentObject) {
        
        // remember current marking
        gTweakBar.currentObject->height     = height;
        gTweakBar.currentObject->xtilt      = xtilt;
        gTweakBar.currentObject->ytilt      = ytilt;
        gTweakBar.currentObject->cp_spread  = spread;
        gTweakBar.currentObject->marking    = gRangeDrawer.currentlyMarked;
        
        // visually unselect the current object
        TwDefine((string("Greens/") + gTweakBar.currentObject->name + " label='" + gTweakBar.currentObject->name + "'").c_str());
    }
    
    // reset the current marking
    gRangeDrawer.UnmarkAll();
    
    // create new object
    TerrainObject* to = new TerrainObject;
    to->name = string("green") + to_string(++num_greens);
    to->height = 0;
    to->xtilt = 0;
    to->ytilt = 0;
    to->cp_spread = spread;
    to->cp_functype = functype;
    objects.push_back(to);
    
    // select the new object
    currentObject = to;
    
    TwAddButton(objectBar,
                to->name.c_str(),
                (TwButtonCallback) [] (void* clientData) {
                    
                    if (gTweakBar.currentObject) {
                        
                        // remember current marking
                        gTweakBar.currentObject->height     = height;
                        gTweakBar.currentObject->xtilt      = xtilt;
                        gTweakBar.currentObject->ytilt      = ytilt;
                        gTweakBar.currentObject->cp_spread  = spread;
                        gTweakBar.currentObject->marking    = gRangeDrawer.currentlyMarked;
                        
                        // visually unselect the current object
                        TwDefine((string("Greens/") + gTweakBar.currentObject->name + " label='" + gTweakBar.currentObject->name + "'").c_str());
                    }
                    
                    // change current object
                    TerrainObject* to = (TerrainObject*) clientData;
                    gTweakBar.currentObject = to;
                    
                    // adjust parameters
                    height          = to->height;
                    heightPrev      = to->height;
                    ytilt           = to->ytilt;
                    spread          = to->cp_spread;
                    functype        = to->cp_functype;
                    
                    // adjust prev parameters not to cause weird initial changes
                    xtilt           = to->xtilt;
                    xtiltPrev       = to->xtilt;
                    ytiltPrev       = to->ytilt;
                    spreadPrev      = to->cp_spread;
                    functypePrev    = to->cp_functype;
                    
                    //refresg the control bar since these changes affect it too
                    TwRefreshBar(controlBar);
                    
                    // visually select the new object
                    TwDefine((string("Greens/") + gTweakBar.currentObject->name + " label='" + gTweakBar.currentObject->name + "     SELECTED'").c_str());
                    
                    // update marking
                    gRangeDrawer.UnmarkAll();
                    for (auto xy : to->marking)
                        gRangeDrawer.Mark(xy.x, xy.y);
                    
                },
                to,
                (string("label='") + to->name + "     SELECTED'").c_str());
}