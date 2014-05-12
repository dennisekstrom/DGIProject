//
//  text.cpp
//  DGIProject
//
//  Created by Tobias Wikström on 12/05/14.
//  Copyright (c) 2014 Tobias Wikström. All rights reserved.
//

#include "text.h"

FT_Library ft;
//FT_Face face; // render text
//FT_Face

int init() {
    
    if(FT_Init_FreeType(&ft)) {
        fprintf(stderr, "Could not init freetype library\n");
        return 1;
    }
    return 0;
}