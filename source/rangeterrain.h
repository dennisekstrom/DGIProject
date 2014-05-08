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
#include <math.h>

#define PI              3.14159265359
#define X_INTERVAL      1024
#define Y_INTERVAL      1024

using namespace std;

enum ControlPointFuncType {
    FUNC_LINEAR,
    FUNC_COS,
    FUNC_SIN
};

struct ControlPoint {
public:
    int x;
    int y;
    float h;
    float spread;
    float (*liftfunc)(float, float, float);
    
    ControlPoint(int x, int y, float h, float spread, ControlPointFuncType functype) {
        this->x = x;
        this->y = y;
        this->h = h;
        this->spread = spread;
        
        switch (functype) {
            case FUNC_LINEAR:
                this->liftfunc = lift_linear;
                break;
                
            case FUNC_COS:
                this->liftfunc = lift_cos;
                break;
                
            case FUNC_SIN:
                this->liftfunc = lift_sin;
                break;
        }
    }
        
    inline static float lift_linear(float h, float spread, float dist) {
        return dist <= spread ? h * (1 - dist / spread) : 0;
    }
    
    inline static float lift_cos(float h, float spread, float dist) {
        return dist <= spread ? h * cos((dist * PI) / (spread * 2)) : 0;
    }
    
    inline static float lift_sin(float h, float spread, float dist) {
        return dist <= spread ? h * (1 - sin((dist * PI) / (spread * 2))) : 0;
    }
    
    float lift(int x, int y) {
        float dx = this->x - x;
        float dy = this->y - y;
        float dist = sqrt(dx * dx + dy * dy);
        return this->liftfunc(this->h, this->spread, dist);
    }
};

class RangeTerrain {
// private variables
private:
    vector<ControlPoint> controlPoints;
    bool controlPointChanged;
    
// public variables
public:
    float hmap[X_INTERVAL][Y_INTERVAL];
    
    
// private functions
private:

    
// public functions
public:
    void SetControlPoint(int x, int y, float h, float spread, ControlPointFuncType func);
    void SetControlPoint(ControlPoint cp);
    void GenerateHMapFromControlPoints();
    
    inline bool ControlPointChanged() { return controlPointChanged; }
};

#endif
