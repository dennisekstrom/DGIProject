//
//  callbacks.h
//  DGIProject
//
//  Created by Dennis Ekström on 14/05/14.
//  Copyright (c) 2014 Dennis Ekström. All rights reserved.
//

#ifndef DGIProject_callbacks_h
#define DGIProject_callbacks_h

#include "rangetweakbar.h"

// Keyboard state
bool gShiftDown = false;
bool keys[256] = {false};
bool special[256] = {false};

// Mouse state
bool gMouseBtnDown = false;
int gMouseX, gMouseY;

void CBKey(unsigned char key, int x, int y) {
    assert(0 <= key && key < 255);
    key = toupper(key);
    if ( !TwEventKeyboardGLUT(key, x, y) ) {
        keys[key] = true;
        gShiftDown = glutGetModifiers() & GLUT_ACTIVE_SHIFT;
    }
}

void CBKeyUp(unsigned char key, int x, int y) {
    assert(0 <= key && key < 255);
    key = toupper(key);
    keys[key] = false;
    gShiftDown = glutGetModifiers() & GLUT_ACTIVE_SHIFT;
}

void CBSpecial(int key, int x, int y) {
    assert(0 <= key && key < 255);
    if ( !TwEventSpecialGLUT(key, x, y) ) {
        special[key] = true;
        gShiftDown = glutGetModifiers() & GLUT_ACTIVE_SHIFT;
    }
}

void CBSpecialUp(int key, int x, int y) {
    assert(0 <= key && key < 255);
    special[key] = false;
    gShiftDown = glutGetModifiers() & GLUT_ACTIVE_SHIFT;
}

void CBMouse(int button, int state, int x, int y) {
    if ( !TwEventMouseButtonGLUT(button, state, x, y) ) {
        gMouseBtnDown = (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN);
        gMouseX = x;
        gMouseY = y;
        gShiftDown = glutGetModifiers() & GLUT_ACTIVE_SHIFT;
    }
}

void CBMotion(int x, int y) {
    if ( !TwEventMouseMotionGLUT(x, y) ) {
        gMouseX = x;
        gMouseY = y;
    }
}

// --------------------------------------------------------------------------
// Helpers
// --------------------------------------------------------------------------
/*
bool KeyOnce(unsigned char key) {
    assert(0 <= key && key < 255);
    bool ret = keys[key];
    keys[key] = false;
    return ret;
}
 */
#endif
