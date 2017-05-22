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
            rgba[1] = currentPixelColor.b;
            rgba[2] = currentPixelColor.g;
            rgba[3] = currentPixelColor.r;
            memcpy(&m_TextureBytes[(pixelY * FRAMEBUFFER_WIDTH + pixelX) * sizeof(rgba)], rgba, sizeof(rgba));
            
        }
    }
}

void SDLBufferRenderer::render(){   
    SDL_UnlockTexture(m_OutputTexture);
    
	int renderWidth = 0;
	int renderHeight = 0;
	SDL_GetRendererOutputSize(m_Renderer, &renderWidth, &renderHeight);

	//Set up a rectangle scaled and positioned so that frame fills window while preserving aspect ratio
	SDL_Rect scaledOutputRect;
	scaledOutputRect.x = 0;
	scaledOutputRect.y = 0;
	if (renderWidth < renderHeight) {
		scaledOutputRect.w = renderWidth;
		scaledOutputRect.h = (int)((float)renderWidth * ((float)FRAMEBUFFER_HEIGHT / FRAMEBUFFER_WIDTH));
		scaledOutputRect.y = (int)((renderHeight / 2.0f) - (scaledOutputRect.h / 2.0f));
	} else {
		scaledOutputRect.h = renderHeight;
		scaledOutputRect.w = (int)((float)renderHeight * ((float)FRAMEBUFFER_WIDTH / FRAMEBUFFER_HEIGHT));
		scaledOutputRect.x = (int)((renderWidth/ 2.0f) - (scaledOutputRect.w / 2.0f));
	}

    //Clear renderer before copy. Otherwise, artifacts might persist on Linux
    SDL_RenderClear(m_Renderer);

    if(SDL_RenderCopy(m_Renderer, m_OutputTexture, NULL, &scaledOutputRect) < 0){
        std::cout << "Failed to copy texture to video renderer: " << SDL_GetError();
    }
    SDL_RenderPresent(m_Renderer);
    
    SDL_LockTexture(m_OutputTexture, NULL, reinterpret_cast<void**>(&m_TextureBytes), &m_TexturePitch);
    if(m_TextureBytes == NULL){
        std::cout << "Bytes are null! " << SDL_GetError() << std::endl;
    }
    
}
