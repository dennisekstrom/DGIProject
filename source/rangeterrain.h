////
////  rangeterrain.h
////  DGIProject
////
////  Created by Dennis Ekström on 08/05/14.
////  Copyright (c) 2014 Dennis Ekström. All rights reserved.
////
//
//#ifndef DGIProject_rangeterrain_h
//#define DGIProject_rangeterrain_h
//
//#include <vector>
//

#ifndef DGIProject_rangeterrain_h
#define DGIProject_rangeterrain_h

#include <vector>
#include <math.h>
#include <glm/glm.hpp>
#include <GL/glfw.h>

#define PI              3.14159265359
#define X_INTERVAL      64
#define Y_INTERVAL      64
#define GRID_RES        0.5f // 0.5 meter between points

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
        float dist = sqrt(dx * dx + dy * dy) * GRID_RES;
        return this->liftfunc(this->h, this->spread, dist);
    }
};

class RangeTerrain {
private:
    ControlPoint* controlPoints[X_INTERVAL][Y_INTERVAL];
    bool dataChanged;
    
public:
    float hmap[X_INTERVAL][Y_INTERVAL];
    glm::vec3 normals[X_INTERVAL][Y_INTERVAL];
    GLfloat vdata[(X_INTERVAL - 1) * (Y_INTERVAL - 1) * 2 * 3 * 8];
    
private:
//    void InitiateModel();
//    void GenerateModelFromHMap();

public:
    RangeTerrain();
    ~RangeTerrain();
    
    void SetControlPoint(int x, int y, float h, float spread, ControlPointFuncType func);
    void FlattenHMap();
    void GenerateHMapFromControlPoints();
    void GenerateNormalsFromHMap();
    void GenerateVertexData();
    void GenerateEverythingFromControlPoints();
    
    inline ControlPoint* GetControlPoint(const int &x, const int &y) {
        return controlPoints[y][x];
    }
    
    inline bool DataChanged() { return dataChanged; }
    inline void SetDataChanged(bool dataChanged) { this->dataChanged = dataChanged; }
};

#endif
