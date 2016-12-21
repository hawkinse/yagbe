#pragma once
#include "constants.h"

//Forward declare RGB Color struct since we need to inclue IRenderer in GBLCD where RGBColor is defined
#ifndef RBGColor
struct RGBColor;
#endif

class IRenderer{
  public:
    virtual void update(RGBColor** buffer, int width, int height) = 0;
    virtual void render() = 0;
};