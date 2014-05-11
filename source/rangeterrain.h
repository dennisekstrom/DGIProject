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

#define PI                  3.14159265359
#define X_INTERVAL          64
#define Y_INTERVAL          64
#define GRID_RES            0.5f // 0.5 meter between points
#define TERRAIN_WIDTH       (X_INTERVAL - 1) * GRID_RES
#define TERRAIN_DEPTH       (Y_INTERVAL - 1) * GRID_RES

using namespace std;
using namespace glm;

struct xy {
    int x, y;
};

struct xyh {
    int x, y;
    float h;
};

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
    ControlPointFuncType functype;
    float (*liftfunc)(float, float, float);
    
    ControlPoint(int x, int y, float h, float spread, ControlPointFuncType functype) {
        this->x = x;
        this->y = y;
        this->h = h;
        this->spread = spread;
        this->functype = functype;
        
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

struct Triangle {
    vec3 v0, v1, v2;
    vec3 n0, n1, n2;
    vec3 c0, c1, c2;
    vec2 t0, t1, t2;
};

/**
 Two triangles making up a square with side GRID_RES.
 The vertices are assumed to be at descrete positions in the grid.
 
 The coordinate system for the figures:
 
 ^ -z
 |
 |     x
 +----->
 
 The vertex/normal/color ordering:
 
  1         3
    +-----+
    |     |
    |     |
    +-----+
  2         4
 
 The texture mapping:
 
 0,0       1,0
    +-----+
    |     |
    |     |
    +-----+
 0,1       1,1
 
 The diagonalUp parameter works like this:
 
 Diagonal up:       Diagonal down:
 +-----+            +-----+
 |   / |            | \   |
 | /   |            |   \ |
 +-----+            +-----+
 */
struct TrianglePair {
    bool diagonalUp;
    Triangle t1, t2;
    
    void SetVertices(vec3 &v1, vec3 &v2, vec3 & v3, vec3 &v4) {
        diagonalUp = true; // [TODO: Set appropriate condition on this]
        vec2 t00(0,0), t01(0,1), t10(1,0), t11(1,1);
        if (diagonalUp) {
            t1.v0 = v1;     t1.v1 = v2;     t1.v2 = v3;     // vertices
            t2.v0 = v4;     t2.v1 = v3;     t2.v2 = v2;
            
            t1.t0 = t00;    t1.t1 = t01;    t1.t2 = t10;    // texture coordinates
            t2.t0 = t11;    t2.t1 = t10;    t2.t2 = t01;
        } else {
            t1.v0 = v1;     t1.v1 = v4;     t1.v2 = v3;
            t2.v0 = v4;     t2.v1 = v1;     t2.v2 = v2;
            
            t1.t0 = t00;    t1.t1 = t11;    t1.t2 = t10;
            t2.t0 = t11;    t2.t1 = t00;    t2.t2 = t01;

        }
    }
    
    
    void SetNormals(vec3 &n1, vec3 &n2, vec3 & n3, vec3 &n4) {
        if (diagonalUp) {
            t1.n0 = n1;     t1.n1 = n2;     t1.n2 = n3;     // normals
            t2.n0 = n4;     t2.n1 = n3;     t2.n2 = n2;
        } else {
            t1.n0 = n1;     t1.n1 = n4;     t1.n2 = n3;
            t2.n0 = n4;     t2.n1 = n1;     t2.n2 = n2;
        }
    }
    
    void SetColors(vec3 &c1, vec3 &c2, vec3 & c3, vec3 &c4) {
        if (diagonalUp) {
            t1.c0 = c1;     t1.c1 = c2;     t1.c2 = c3;     // colors
            t2.c0 = c4;     t2.c1 = c3;     t2.c2 = c2;
        } else {
            t1.c0 = c1;     t1.c1 = c4;     t1.c2 = c3;
            t2.c0 = c4;     t2.c1 = c1;     t2.c2 = c2;
        }
    }

    // [TODO: The following functions assumes diagonalUp]
    void SetV1(vec3 v1, vec3 n1, vec3 c1) { t1.v0 = v1;     t1.n0 = n1;     t1.c0 = c1; }
    
    void SetV2(vec3 v2, vec3 n2, vec3 c2) { t1.v1 = v2;     t1.n1 = n2;     t1.c1 = c2;
                                            t2.v2 = v2;     t2.n2 = n2;     t2.c2 = c2; }
    
    void SetV3(vec3 v3, vec3 n3, vec3 c3) { t1.v2 = v3;     t1.n2 = n3;     t1.c2 = c3;
                                            t2.v1 = v3;     t2.n1 = n3;     t2.c1 = c3; }
    
    void SetV4(vec3 v4, vec3 n4, vec3 c4) { t2.v0 = v4;     t2.n0 = n4;     t2.c0 = c4; }
};

struct ChangeManager {
private:
    const int size;
    const int y_interval;
    bool* identifierInVector;

public:
    vector<xy> identifiers;
    
    ChangeManager(const int &x_intvl, const int &y_intvl) :
    size(y_intvl * x_intvl),
    y_interval(y_intvl) {
        identifiers.reserve(size);
        identifierInVector = new bool[size];
        Reset();
    }
    
    ~ChangeManager() {
        delete identifierInVector;
    }
    
    void SetChanged(const int &x, const int &y) {
        int idx = y * y_interval + x;
        if (!identifierInVector[idx]) { // This xy hasn't previously been changed
            identifierInVector[idx] = true;
            identifiers.push_back( { x, y } );
        }
    }
    
    inline bool DidChange(const int &x, const int &y) {
        return identifierInVector[y * y_interval + x];
    }
    
    void Reset() {
        identifiers.clear();
        memset(identifierInVector, 0, size * sizeof(bool)); // Reset to zeros
    }
};

/**
 In between calls to Reset(), the highest point will be chosen.
 */
/*struct HMapChangeManager {
private:
    const int size;
    const int y_interval;
    size_t* identifierIndexInVector;
    
public:
    vector<xyh> identifiers;
    
    HMapChangeManager() :
    size(Y_INTERVAL * X_INTERVAL),
    y_interval(Y_INTERVAL) {
        identifiers.reserve(size);
        identifierIndexInVector = new size_t[size];
        Reset();
    }
    
    ~HMapChangeManager() {
        delete identifierIndexInVector;
    }
    
    void SetHeight(const int &x, const int &y, const float &h) {
        int idx = y * y_interval + x;
        if (identifierIndexInVector[idx] == -1) { // This xy hasn't previously been changed
            identifierIndexInVector[idx] = identifiers.size();
            identifiers.push_back( { x, y, h } );
        } else {
            float &old_h = identifiers[identifierIndexInVector[idx]].h;
            if (h > old_h)
                identifiers[identifierIndexInVector[idx]].h = h;
        }
    }
    
    inline bool HeightChanged(const int &x, const int &y) {
        int idx = y * y_interval + x;
        return identifierIndexInVector[idx] != -1;
    }
    
    void Reset() {
        identifiers.clear();
        memset(identifierIndexInVector, -1, size * sizeof(size_t)); // Reset all to -1
    }
};*/

class RangeTerrain {
private:
    ControlPoint*   controlPoints[Y_INTERVAL][X_INTERVAL];
    bool            controlPointChangeRequiresHMapRegeneration;
    
    ChangeManager*  changedControlPoints;
    ChangeManager*  changedHMapCoords;
    ChangeManager*  changedVertices;
    
//    HMapChangeManager*  hmapInternalChanges;
    
//    vector<xy>      changedControlPoints;
//    vector<xy>      changedHMapCoords;
//    vector<xy>      changedVertices;

//    vector<xy> changedTrianglePairs;
    
//    bool vertexNeedsUpdate[Y_INTERVAL][X_INTERVAL];
    
public:
    static const int floatsPerVertex;
    static const int floatsPerTriangle;
    static const int floatsPerTrianglePair;
    static const int floatsPerRow;
    
    
    float hmap[Y_INTERVAL][X_INTERVAL];
    vec3 normals[X_INTERVAL][Y_INTERVAL];
    
    TrianglePair trianglePairs[(Y_INTERVAL - 1)][(X_INTERVAL - 1)];
    
    GLfloat vertexData[(X_INTERVAL - 1) * (Y_INTERVAL - 1) * 2 * 3 * 8];
    vector<int> changedVertexIndices;

private:
    void FlattenHMap();
    
    void UpdateHMap();                          // Update from changed control points
    void UpdateNormals();                       // Requires hmap
    void UpdateTrianglePairs();                 // Requires hmap and normals [TODO: REMOVE TRIANGLE PAIRS]
    void UpdateVertexData();                    // Requires hmap and normals (and for the moment that changedVertices is updated by running UpdateTrianglePairs)

    void GenerateHMap();                        // Generate from control points
    void GenerateNormals();                     // Requires hmap
    void GenerateTrianglePairs();               // Requires hmap and normals [TODO: REMOVE TRIANGLE PAIRS]
    void GenerateVertexData();                  // Requires hmap and normals

    void UpdateHMap(ControlPoint &cp);                      // Updates hmap from the given control point
    void UpdateNormal(const int &x, const int &y);          // Requires hmap
    void UpdateTrianglePair(const int &x, const int &y);    // Requires hmap and normal
    void UpdateVertexData(const int &x, const int &y);      // Requires hmap and normal

    inline void SetVertexData(int idx, const vec3 &v, const vec2 &t, const vec3 &n) {
        vertexData[idx++] = v.x;
        vertexData[idx++] = v.y;
        vertexData[idx++] = v.z;
        vertexData[idx++] = t.x;
        vertexData[idx++] = t.y;
        vertexData[idx++] = n.x;
        vertexData[idx++] = n.y;
        vertexData[idx++] = n.z;
    }

public:
    RangeTerrain();
    ~RangeTerrain();
    
    void SetControlPoint(int x, int y, float h, float spread, ControlPointFuncType func);
    
    void UpdateAll();                           // Update everything from changed control points
    void GenerateAll();                         // Generate everything from control points
    
    
    inline ControlPoint* GetControlPoint(const int &x, const int &y) {
        return controlPoints[y][x];
    }
};

#endif
