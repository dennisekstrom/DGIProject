//
//  rangetweakbar.cpp
//  DGIProject
//
//  Created by Dennis Ekström on 14/05/14.
//  Copyright (c) 2014 Dennis Ekström. All rights reserved.
//

#include "difficultyanalyzer.h"
#include "rangetweakbar.h"
#include "protracerinputhandler.h"
#include "tdogl/Camera.h"
#include <glm/glm.hpp>
#include <GLUT/glut.h>

RangeTweakBar gTweakBar;

TwBar* generalBar;
TwBar* noiseBar;
TwBar* controlBar;
TwBar* objectBar;
TwBar* difficultyBar;

// terrain parameters
float       height      = 0,    heightPrev      = 0;
float       xtilt       = 0,    xtiltPrev       = 0;
float       ytilt       = 0,    ytiltPrev       = 0;
float       spread      = 5,    spreadPrev      = 5;
ControlPointFuncType functype = FUNC_LINEAR, functypePrev = FUNC_LINEAR;

// noise parameters
float       persistance = 0.3;
float       frequency   = 0.05;
float       amplitude   = 15;
float       octaves     = 10;

// difficulty parameters
string shotDistance = "";
string difficulty = "";
string difficultyReadable = "";

RangeTweakBar::RangeTweakBar() {
    objectCounter = 0;
    currentObject = NULL;
}

RangeTweakBar::~RangeTweakBar() {}

void RangeTweakBar::Init(const int &screenWidth, const int &screenHeight) {
    
    // Initialize anttweakbar
    TwInit(TW_OPENGL_CORE, NULL);
    TwWindowSize(screenWidth, screenHeight);
    
    // The tweak bar needs the callbacks
    TwGLUTModifiersFunc(glutGetModifiers);
    
    //----------------------------------------------------
    // The General Bar
    //----------------------------------------------------
    generalBar = TwNewBar("General");
    TwDefine("General label=GENERAL");
    TwDefine("General position='0 0'");
    TwDefine("General size='205 256'");
    TwDefine("General resizable=false");
    TwDefine("General movable=false");
    TwDefine("General fontresizable=false");
    TwDefine("General color='255 255 255' alpha=63 ");
    TwDefine("General text=light");
    
    // We need these globals (defined in main.mm)
    extern bool gLeftCameraFullscreen;
    extern bool gLeftCameraUseColor;
    extern bool gRightCameraUseColor;
    extern tdogl::Camera gCamera1;
    extern glm::vec3 gLightPosition;
    
    TwAddButton(generalBar,
                "Toggle fullscreen",
                (TwButtonCallback) [] (void* clientData) {
                    gLeftCameraFullscreen = !gLeftCameraFullscreen;
                    gCamera1.setViewportAspectRatio(gLeftCameraFullscreen ? 2 : 1);
                },
                NULL,
                "key=F help='Turn left view fullscreen on/off.' ");
    
    TwAddButton(generalBar,
                "Toggle left color mode",
                (TwButtonCallback) [] (void* clientData) {
                    gLeftCameraUseColor = !gLeftCameraUseColor;
                },
                NULL,
                "key=1 help='Toggle the color mode of the left view.' ");
    
    TwAddButton(generalBar,
                "Toggle right color mode",
                (TwButtonCallback) [] (void* clientData) {
                    gRightCameraUseColor = !gRightCameraUseColor;
                },
                NULL,
                "key=2 help='Toggle the color mode of the right view.' ");
    
    TwAddSeparator(generalBar, NULL, NULL);
    
    TwAddButton(generalBar,
                "Light at camera",
                (TwButtonCallback) [] (void* clientData) {
                    gLightPosition = gCamera1.position();
                },
                NULL,
                "key=L help='Position the light at the camera.' ");
    
    TwAddSeparator(generalBar, NULL, NULL);
    
    TwAddButton(generalBar,
                "Place camera on tee",
                (TwButtonCallback) [] (void* clientData) {
                    if (gRangeDrawer.teeMarked && gRangeDrawer.targetMarked) {
                        
                        vec3 xzdir = gRangeDrawer.TargetTerrainPos() - gRangeDrawer.TeeTerrainPos();
                        xzdir.y = 0;
                        
                        vec3 cam_offset = glm::normalize(-xzdir);
                        cam_offset *= 5;        // Place camera 5 meters behind tee
                        cam_offset.y += 1.8;    // Place camera at height of average player (180 cm)
                        
                        gCamera1.setPosition(gRangeDrawer.TeeTerrainPos() + cam_offset);
                        gCamera1.lookAt(gRangeDrawer.TargetTerrainPos());
                    }
                },
                NULL,
                "key=T help='Position the camera at the tee looking at the target. Both tee and target must be set.' ");
    
    //----------------------------------------------------
    // The Noise Bar
    //----------------------------------------------------
    noiseBar = TwNewBar("Noise");
    TwDefine("Noise label=NOISE");
    TwDefine("Noise position='205 0'");
    TwDefine("Noise size='205 256'");
    TwDefine("Noise resizable=false");
    TwDefine("Noise movable=false");
    TwDefine("Noise fontresizable=false");
    TwDefine("Noise color='255 255 255' alpha=63 ");
    TwDefine("Noise text=light");
    
    TwAddVarRW(noiseBar, "Persistance", TW_TYPE_FLOAT, &persistance,
               "min=0 max=20 step=0.01 keyIncr=+ keyDecr=- help='Set the persistance of perlin noise.' ");
    
    TwAddVarRW(noiseBar, "Frequency", TW_TYPE_FLOAT, &frequency,
               "min=0 max=20 step=0.01 keyIncr=+ keyDecr=- help='Set the frequency of perlin noise.' ");
    
    TwAddVarRW(noiseBar, "Amplitude", TW_TYPE_FLOAT, &amplitude,
               "min=0 max=20 step=0.01 keyIncr=+ keyDecr=- help='Set the amplitude of perlin noise' ");
    
    TwAddVarRW(noiseBar, "Octaves", TW_TYPE_FLOAT, &octaves,
               "min=0 max=20 step=1 keyIncr=+ keyDecr=- help='Set the octaves of perlin noise' ");
    
    
    TwAddSeparator(noiseBar, NULL, NULL);
    
    TwAddButton(noiseBar,
                "Generate perlin noise",
                (TwButtonCallback) [] (void* clientData) {
                    gTerrain.SetNoise(persistance, frequency, amplitude, octaves, rand() % 100000);
                },
                NULL,
                "key=V help='Apply perlin noise to terrain.'");
    
    TwAddButton(noiseBar,
                "Remove noise",
                (TwButtonCallback) [] (void* clientData) {
                    gTerrain.FlattenNoise();
                },
                NULL,
                "help='Remove the noise' ");
    
    //----------------------------------------------------
    // The Control Bar
    //----------------------------------------------------
    controlBar = TwNewBar("Controls");
    TwDefine("Controls label=CONTROLS");
    TwDefine("Controls position='410 0'");
    TwDefine("Controls size='205 256'");
    TwDefine("Controls resizable=false");
    TwDefine("Controls movable=false");
    TwDefine("Controls fontresizable=false");
    TwDefine("Controls color='255 255 255' alpha=63 ");
    TwDefine("Controls text=light");
    
    TwAddVarRW(controlBar, "Height", TW_TYPE_FLOAT, &height,
               "min=-100 max=100 step=1 precision=2 keyIncr=Y keyDecr=I help='Raise/lower selected segments.' ");
    
    TwAddVarRW(controlBar, "X-Tilt", TW_TYPE_FLOAT, &xtilt,
               "min=-85 max=85 step=2 keyIncr=H keyDecr=K help='Tilt selected segments along x-axis.' ");
    
    TwAddVarRW(controlBar, "Y-Tilt", TW_TYPE_FLOAT, &ytilt,
               "min=-85 max=85 step=2 keyIncr=J keyDecr=U help='Tilt selected segments along y-axis.' ");
    
    TwAddVarRW(controlBar, "Spread", TW_TYPE_FLOAT, &spread,
               "min=0 max=20 step=0.25 keyIncr=+ keyDecr=- help='Set the spread for the selected segments.' ");
    
    TwType twCPFuncType = TwDefineEnum("CPFuncType", NULL, 0);
    
    TwAddVarRW(controlBar, "Functype", twCPFuncType, &functype,
               "enum='0 {Linear}, 1 {Cos}, 2 {Sin}' help='Set the spread for the selected segments.' ");
    
    TwAddSeparator(controlBar, NULL, NULL);
    
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
                    float h = gRangeDrawer.GetAverageHeight(gRangeDrawer.currentlyMarked);
                    gRangeDrawer.FlattenMarked(h, spread, functype);
                    
                    xtilt = 0;
                    ytilt = 0;
                    height = h;
                    
                    xtiltPrev = 0;
                    ytiltPrev = 0;
                    heightPrev = h;
                },
                NULL,
                "key=SPACE help='Flatten the current selection.' ");
    
    TwAddButton(controlBar,
                "Flatten terrain",
                (TwButtonCallback) [] (void* clientData) {
                    gTerrain.Reset();
                    
                    xtilt = 0;
                    ytilt = 0;
                    height = 0;
                    
                    xtiltPrev = 0;
                    ytiltPrev = 0;
                    heightPrev = 0;
                    
                    for (auto o : gTweakBar.objects) {
                        o->xtilt = 0;
                        o->ytilt = 0;
                        o->height = 0;
                    }
                },
                NULL,
                "key=R help='Flatten the entire terrain.' ");
    
    TwAddSeparator(controlBar, NULL, NULL);
    
    TwAddButton(controlBar,
                "Terrain from file",
                (TwButtonCallback) [] (void* clientData) {
                    set<xy, xy_comparator> tmp;
                    gTweakBar.TerrainFromFile();
                },
                NULL,
                "help='Generates terrain from a protracer formatted terrain file.' ");

    //----------------------------------------------------
    // The Green Bar
    //----------------------------------------------------
    objectBar = TwNewBar("Greens");
    TwDefine("Greens label=GREENS");
    TwDefine("Greens position='615 0'");
    TwDefine("Greens size='205 256'");
    TwDefine("Greens resizable=false");
    TwDefine("Greens movable=false");
    TwDefine("Greens fontresizable=false");
    TwDefine("Greens color='255 255 255' alpha=63 ");
    TwDefine("Greens text=light");
    
    TwAddButton(objectBar,
                "Mark tee position",
                (TwButtonCallback) [] (void* clientData) {
                    
                    // reset selection
                    gTweakBar.SelectObject(NULL);
                    
                    // we're about to mark the tee
                    gRangeDrawer.SetMarkMode(MARK_TEE);
                },
                NULL,
                "help='Mark the tee position - it will appear blue.' ");
    
    TwAddButton(objectBar,
                "Mark target position",
                (TwButtonCallback) [] (void* clientData) {
                    
                    // reset selection
                    gTweakBar.SelectObject(NULL);
                    
                    // we're about to mark the target
                    gRangeDrawer.SetMarkMode(MARK_TARGET);
                },
                NULL,
                "help='Mark the target position - it will appear red.' ");

    TwAddSeparator(objectBar, NULL, NULL);
    
    TwAddButton(objectBar,
                "New green",
                (TwButtonCallback) [] (void* clientData) { gTweakBar.NewTerrainObject(); },
                NULL,
                "key=R help='Flatten the entire terrain.' ");
    
    TwAddButton(objectBar,
                "Delete green",
                (TwButtonCallback) [] (void* clientData) {
                    if (gTweakBar.currentObject) {
                        
                        // remove the current object
                        gTweakBar.RemoveTerrainObject(gTweakBar.currentObject);
                        
                        // reset selection
                        gTweakBar.SelectObject(NULL);
                    }
                },
                NULL,
                "key=R help='Flatten the entire terrain.' ");
    
    TwAddSeparator(objectBar, NULL, NULL);
    
    difficultyBar = TwNewBar("Difficulty");
    TwDefine("Difficulty label=DIFFICULTY");
    TwDefine("Difficulty position='820 0'");
    TwDefine("Difficulty size='205 256'");
    TwDefine("Difficulty resizable=false");
    TwDefine("Difficulty movable=false");
    TwDefine("Difficulty fontresizable=false");
    TwDefine("Difficulty color='255 255 255' alpha=63 ");
    TwDefine("Difficulty text=light");
    TwDefine("Difficulty valueswidth=100");
    
    extern bool gPathShouldBeDrawn;
    extern bool gPathChanged;
    extern vec3 gPathTee;
    extern vec3 gPathP1;
    extern vec3 gPathP2;
    extern vec3 gPathTarget;
    
    TwAddButton(difficultyBar,
                "Calculate difficulty",
                (TwButtonCallback) [] (void* clientData) {
                    vec3 p1, p2;
                    float dist;
                    float d = DifficultyAnalyzer::CalculateDifficulty(gRangeDrawer.TeeTerrainPos(), gRangeDrawer.TargetTerrainPos(), p1, p2, dist);
                    shotDistance = to_string(int(round(dist)));
                    if (d >= 0) {
                        difficulty = to_string(int(round(d)));
                        
                        if (d < 50) {
                            difficultyReadable = "Very Easy";
                        } else if (d < 100) {
                            difficultyReadable = "Easy";
                        } else if (d < 150) {
                            difficultyReadable = "Medium";
                        } else if (d < 200) {
                            difficultyReadable = "Hard";
                        } else if (d < 250) {
                            difficultyReadable = "Very Hard";
                        } else {
                            difficultyReadable = "Ridiculously Hard";
                        }
                        
                        gPathTee = gRangeDrawer.TeeTerrainPos();
                        gPathP1 = p1;
                        gPathP2 = p2;
                        gPathTarget = gRangeDrawer.TargetTerrainPos();
                        gPathChanged = true;
                        gPathShouldBeDrawn = true;
                    } else {
                        difficulty = "";
                        difficultyReadable = "Impossible!";
                        gPathShouldBeDrawn = false;
                    }
                },
                NULL,
                "key=RETURN help='Calculate the difficulty from the current tee and target.' ");

    TwAddSeparator(difficultyBar, NULL, NULL);
    
    TwAddVarRO(difficultyBar, "Distance", TW_TYPE_STDSTRING, &shotDistance,
               "help='Distance that the calculated difficulty is based on.'");
    
    TwAddVarRO(difficultyBar, "Dif. value", TW_TYPE_STDSTRING, &difficulty,
               "help='Difficulty of a shot hit from tee to target.' ");
    
    TwAddVarRO(difficultyBar, "Dif. expl.", TW_TYPE_STDSTRING, &difficultyReadable,
               "help='Difficulty in human readable format.' ");
}

void RangeTweakBar::Draw() {
    TwDraw();
}

void RangeTweakBar::Update(const float &dt) {
    TakeAction(dt);
}

void RangeTweakBar::TakeAction(const float &dt) {

    // Don't allow changes if nothing
    if (gRangeDrawer.currentlyMarked.empty()) {
        height = heightPrev;
        xtilt = xtiltPrev;
        ytilt = ytiltPrev;
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
        int x, y;
        for (auto xy : gRangeDrawer.currentlyMarked) {
            x = xy.x;
            y = xy.y;
            gTerrain.SetControlPointSpread(x  , y  , spread);
            gTerrain.SetControlPointSpread(x  , y+1, spread);
            gTerrain.SetControlPointSpread(x+1, y  , spread);
            gTerrain.SetControlPointSpread(x+1, y+1, spread);
        }
        
        spreadPrev = spread;
    }
    
    // functype
    if (functype != functypePrev) {
        int x, y;
        for (auto xy : gRangeDrawer.currentlyMarked) {
            x = xy.x;
            y = xy.y;
            gTerrain.SetControlPointFuncType(x  , y  , functype);
            gTerrain.SetControlPointFuncType(x  , y+1, functype);
            gTerrain.SetControlPointFuncType(x+1, y  , functype);
            gTerrain.SetControlPointFuncType(x+1, y+1, functype);
        }
        
        functypePrev = functype;
    }
}

bool RangeTweakBar::RemoveTerrainObject(TerrainObject* &obj) {
    
    if (obj == NULL)
        return false;
    
    // remove from the bar
    TwRemoveVar(objectBar, obj->name.c_str());
    
    // remove from the vector of objects
    int idx = -1;
    for ( int i=0; i<objects.size(); i++ ) {
        if (objects[i] == obj) {
            idx = i;
            break;
        }
    }
    
    if (idx == -1)
        return false;
    
    objects.erase(objects.begin() + idx);
    delete obj;
    obj = NULL;
    return true;
}

inline float dist(const float &x1, const float &y1, const float &x2, const float &y2) {
    float dx = x2 - x1;
    float dy = y2 - y1;
    return sqrt(dx * dx + dy * dy);
}

void RangeTweakBar::TerrainFromFile() {
    
    // remove the existing objects
    while (!objects.empty())
        RemoveTerrainObject(objects.back());
    
    // after this, no object is selected
    gTweakBar.currentObject = NULL;
    
    // reset the current marking
    gRangeDrawer.UnmarkAll();
    
    vector<GreenInfo> greens = ProtracerInputHandler::LoadFromFile("input.txt");
    
    for (GreenInfo &green : greens) {
        
        const vec3 pivotPoint = green.targetPos + green.targetCenterOffset;
        const float &cx = pivotPoint.x, &cy = -pivotPoint.z;
        const float tanx = tan(DEG2RAD(green.xtilt)), tany = tan(DEG2RAD(green.ytilt));
        
        float x = green.targetPos.x;
        float y = -green.targetPos.z;
        
        int min_x = floor(std::max(green.targetPos.x - green.radius, 0.0f));
        int max_x = ceil(std::min(green.targetPos.x + green.radius, float(X_INTERVAL)));
        int min_y = floor(std::max(-green.targetPos.z - green.radius, 0.0f));
        int max_y = ceil(std::min(-green.targetPos.z + green.radius, float(Y_INTERVAL)));
        
        for (int yy=min_y; yy<=max_y; yy++) {
            for (int xx=min_x; xx<=max_x; xx++) {
                if (dist(x, y, xx, yy) < green.radius) {
                    float lift = (xx - cx) * float(GRID_RES) * tanx + (yy - cy) * float(GRID_RES) * tany;
                    gTerrain.SetControlPoint(xx  , yy  , pivotPoint.y + lift, green.slopeSpread, green.slopeFunc);
                }
            }
        }
    }
}

void RangeTweakBar::NewTerrainObject() {
    
    // create new object
    TerrainObject* to = new TerrainObject;
    to->name = string("green") + to_string(++objectCounter);
    to->height = 0;
    to->xtilt = 0;
    to->ytilt = 0;
    to->cp_spread = spread;
    to->cp_functype = functype;
    
    // add it to the collection
    objects.push_back(to);
    
    // add it to the tweakbar
    TwAddButton(objectBar,
                to->name.c_str(),
                (TwButtonCallback) [] (void* clientData) {
                    
                    // select this object
                    gTweakBar.SelectObject((TerrainObject*) clientData);
                    
                    // we're about to mark control points
                    gRangeDrawer.SetMarkMode(MARK_CONTROL_POINT);
                },
                to,
                NULL); // label will be set in SetCurrentObject()
    
    // select it (this causes label to be set too)
    SelectObject(to);
    
    // we're about to mark control points
    gRangeDrawer.SetMarkMode(MARK_CONTROL_POINT);
}

void RangeTweakBar::SelectObject(TerrainObject* to) {

    if (currentObject) {
        
        // remember current marking
        currentObject->height     = height;
        currentObject->xtilt      = xtilt;
        currentObject->ytilt      = ytilt;
        currentObject->cp_spread  = spread;
        currentObject->marking    = gRangeDrawer.currentlyMarked;
        
        // visually unselect the current object
        TwDefine((string("Greens/") + currentObject->name + " label='" + currentObject->name + "'").c_str());
    }
    
    // reset the current marking
    gRangeDrawer.UnmarkAll();

    // update current object
    gTweakBar.currentObject = to;
    
    if (to) {
        
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
    
    } else {
        
        // reset params to defaults
        height = 0;
        heightPrev = 0;
        xtilt = 0;
        xtiltPrev = 0;
        ytilt = 0;
        ytiltPrev = 0;
        spread = 5;
        spreadPrev = 5;
        functype = FUNC_LINEAR;
        functypePrev = FUNC_LINEAR;
    }
}