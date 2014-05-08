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
//enum ControlPointFunc {
//    FUNC_LINEAR,
//    FUNC_COS,
//    FUNC_SIN
//};
//
//struct ControlPoint {
//    int x,
//    int y,
//    ControlPointFunc func
//};
//
//class RangeTerrain {
//// private variables
//private:
//    vector<ControlPoint> controlPoints;
//    bool controlPointChanged;
//    
//// public variables
//public:
//    float hmap[1024][1024];
//    
//// public functions
//public:
//    void SetControlPoint(int x, int y, ControlPointFunc func);
//    void SetControlPoint(ControlPoint cp);
//    void GenerateHMapFromControlPoints();
//    
//    inline void ControlPointChanged() { return controlPointChanged; }
//}
//
//#endif
