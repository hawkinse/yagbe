#pragma once
#include <stdlib.h>
#include <iostream>
#include <SDL2/SDL.h>
#include "IRenderer.h"

using namespace std;

class SDLBufferRenderer : public IRenderer {
    private:
      SDL_Renderer* m_Renderer;
      SDL_Texture* m_OutputTexture;
      unsigned char* m_TextureBytes = NULL;
      int m_TexturePitch;

    public:
      SDLBufferRenderer(SDL_Renderer* renderer);
      void update(RGBColor** buffer, int width, int height);
      void render();
};