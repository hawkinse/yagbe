#include "SDLBufferRenderer.h"
#include "gb/gblcd.h"

SDLBufferRenderer::SDLBufferRenderer(SDL_Renderer* renderer){
    m_Renderer = renderer;
    
    if(renderer == NULL){
        std::cout << "Passed in renderer is null" << std::endl;
    }
    
    m_OutputTexture = SDL_CreateTexture(m_Renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, FRAMEBUFFER_WIDTH, FRAMEBUFFER_HEIGHT);
    
    if(!m_OutputTexture){
        std::cout << "Failed to create texture: " << SDL_GetError() << std::endl;
        std::cin.get();
    }
    
    m_TexturePitch = 0;
    
    SDL_LockTexture(m_OutputTexture, NULL, reinterpret_cast<void**>(&m_TextureBytes), &m_TexturePitch);
    if(m_TextureBytes == NULL){
        std::cout << "Bytes are null! " << SDL_GetError() << std::endl;
    }
}

void SDLBufferRenderer::update(RGBColor** buffer, int width, int height){    
    unsigned char rgba[4];
    rgba[0] = 0xFF;
    for(int pixelX = 0; pixelX < FRAMEBUFFER_WIDTH; pixelX++){
        for(int pixelY = 0; pixelY < FRAMEBUFFER_HEIGHT; pixelY++){
            RGBColor currentPixelColor = buffer[pixelX][pixelY];
            rgba[1] = currentPixelColor.r;
            rgba[2] = currentPixelColor.b;
            rgba[3] = currentPixelColor.b;
            memcpy(&m_TextureBytes[(pixelY * FRAMEBUFFER_WIDTH + pixelX) * sizeof(rgba)], rgba, sizeof(rgba));
            
        }
    }
}

void SDLBufferRenderer::render(){   
    SDL_UnlockTexture(m_OutputTexture);
    
    if(SDL_RenderCopy(m_Renderer, m_OutputTexture, NULL, NULL) < 0){
        std::cout << "Failed to copy texture to video renderer: " << SDL_GetError();
    }
    SDL_RenderPresent(m_Renderer);
    
    SDL_LockTexture(m_OutputTexture, NULL, reinterpret_cast<void**>(&m_TextureBytes), &m_TexturePitch);
    if(m_TextureBytes == NULL){
        std::cout << "Bytes are null! " << SDL_GetError() << std::endl;
    }
    
}