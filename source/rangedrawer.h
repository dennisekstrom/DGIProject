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
#include <iostream>
#include "rangeterrain.h"

using namespace std;

struct xy_comparator {
    bool operator() (const xy &a, const xy &b) const {
        return a.y * (X_INTERVAL-1) + a.x < b.y * (X_INTERVAL-1) + b.x;
    }
};

class RangeDrawer {
private:
    
    bool                    marked[Y_INTERVAL-1][X_INTERVAL-1];
    set<xy, xy_comparator>  currentlyMarked;
    bool                    markChanged;

public:
    
    RangeDrawer();
    ~RangeDrawer();
    
    void MarkTerrain(RangeTerrain &terrain);
    
    void Mark(const int &x, const int &y);
    void Unmark(const int &x, const int &y);
    void ToggleMarked(const int &x, const int &y);
    void UnmarkAll();
    
    void MarkTerrainCoord(const float &tx, const float &ty);
    void UnmarkTerrainCoord(const float &tx, const float &ty);
    void ToggleMarkedTerrainCoord(const float &tx, const float &ty);
    
    inline bool MarkChanged()                           { return markChanged; }
    inline void ResetMarkChanged()                      { markChanged = false; }
    inline bool IsMarked(const int &x, const int &y)    { return marked[y][x]; }
    
    inline static int TerrainX2QuadX(const float &tx) {
        assert(tx >= 0 && tx <= TERRAIN_WIDTH);
        cout << "x: " << int(round((X_INTERVAL - 1) * tx / float(TERRAIN_WIDTH)));
        return int(floor((X_INTERVAL - 1) * tx / float(TERRAIN_WIDTH)));
    }
    
    inline static int TerrainY2QuadY(const float &ty) {
        assert(ty >= 0 && ty <= TERRAIN_DEPTH);
        cout << " y: " << int(round((Y_INTERVAL - 1) * ty / float(TERRAIN_DEPTH))) << endl << endl;
        return int(floor((Y_INTERVAL - 1) * ty / float(TERRAIN_DEPTH)));
    }
};

extern RangeDrawer gRangeDrawer;

#endif
