//
//  rangedrawer.h
//  DGIProject
//
//  Created by Dennis Ekström on 11/05/14.
//  Copyright (c) 2014 Dennis Ekström. All rights reserved.
//

#ifndef DGIProject_rangedrawer_h
#define DGIProject_rangedrawer_h

#include <set>
#include <map>
#include <iostream>
#include "rangeterrain.h"

using namespace std;

struct xy_comparator {
    bool operator() (const xy &a, const xy &b) const {
        return a.y * (X_INTERVAL-1) + a.x < b.y * (X_INTERVAL-1) + b.x;
    }
};

struct AreaMarkingManager {
private:
    const int size;
    const int x_interval;
    bool* identifierInVector;
    
public:
    vector<xy> identifiers;
    int x_min;
    int x_max;
    int y_min;
    int y_max;
    
    AreaMarkingManager(const int &x_intvl, const int &y_intvl) :
    size(y_intvl * x_intvl),
    x_interval(x_intvl) {
        identifiers.reserve(size);
        identifierInVector = new bool[size];
        Reset();
    }
    
    ~AreaMarkingManager() {
        delete identifierInVector;
    }
    
    void Mark(const int &x, const int &y) {
        int idx = xy2idx(x, y);
        if (!identifierInVector[idx]) { // This xy hasn't previously been changed
            identifierInVector[idx] = true;
            identifiers.push_back( { x, y } );
            x_max = std::max(x, x_max);
            x_min = std::min(x, x_min);
            y_max = std::max(y, y_max);
            y_min = std::min(y, y_min);
        }
    }
    
    inline bool DidChange(const int &x, const int &y) {
        return identifierInVector[xy2idx(x, y)];
    }
    
    void Reset() {
        identifiers.clear();
        memset(identifierInVector, 0, size * sizeof(bool)); // Reset to zeros (false) [TODO: Potential speed up - iterate over identifiers and set one at a time]
        x_min = y_min = std::numeric_limits<int>::max(); // More than possible
        x_max = y_max = std::numeric_limits<int>::min(); // Less than possible
    }
    
    inline int xy2idx(const int &x, const int &y) {
        return y * x_interval + x;
    }
};

class RangeDrawer {
    
    //    friend class TerrainObjectManager;
    friend class RangeTweakBar;
    
private:
    
    // Marking
    bool marked[Y_INTERVAL-1][X_INTERVAL-1];
    bool markChanged;
    set<xy, xy_comparator>      currentlyMarked;
    
    // Drawing
    AreaMarkingManager*     markedWithShift;
    
    // Mouse
    bool                    mouseIsDown;
    bool                    mouseDownIsMarking;     // true means mouse down for marking, false means unmarking
    
    
    inline void SetMarkChanged() { markChanged = true; }
    
public:
    
    RangeDrawer();
    ~RangeDrawer();
    
    void MarkTerrain(bool holePositionSet, bool matPositionSet);
    void Lift(const int &x, const int &y, const float &lift, const float &spread, const ControlPointFuncType &functype);
    void LiftMarked(const float &lift, const float &spread, const ControlPointFuncType &functype);
    void FlattenMarked(const float &h, const float &spread, const ControlPointFuncType &functype);
    void TiltMarked(const float &xtilt, const float &ytilt, const float &spread, const ControlPointFuncType &functype);
    
    float GetAverageHeightOfMarked();
    glm::vec2 GetCenterOfMarked();
    float GetHeight(float tx, float ty);
    
    
    void Mark(const int &x, const int &y);
    void Unmark(const int &x, const int &y);
    void ToggleMarked(const int &x, const int &y);
    void UnmarkAll();
    void MarkHole(const float &tx, const float &ty);
    void MarkMat(const float &tx, const float &ty);
    
    void MarkTerrainCoord(const float &tx, const float &ty);
    void UnmarkTerrainCoord(const float &tx, const float &ty);
    void ToggleMarkedTerrainCoord(const float &tx, const float &ty);
    
    void TerrainCoordClicked(const float &tx, const float &ty, const bool &shift_down);
    
    inline bool MarkChanged()                           { return markChanged; }
    inline void ResetMarkChanged()                      { markChanged = false; }
    inline bool IsMarked(const int &x, const int &y)    { return marked[y][x]; }
    
    inline void MouseReleased() { mouseIsDown = false; }
    
    inline static int TerrainX2QuadX(const float &tx) {
        assert(tx >= 0 && tx <= TERRAIN_WIDTH);
        return int(floor((X_INTERVAL - 1) * tx / float(TERRAIN_WIDTH)));
    }
    
    inline static int TerrainY2QuadY(const float &ty) {
        assert(ty >= 0 && ty <= TERRAIN_DEPTH);
        return int(floor((Y_INTERVAL - 1) * ty / float(TERRAIN_DEPTH)));
    }
};

extern RangeDrawer gRangeDrawer;

#endif
