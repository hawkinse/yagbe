#pragma once
#include <QApplication>
#include <QMainWindow>

#include "../gb/gameboy.h"

QtYagbeMain::QtYagbeMain(Gameboy* gameboy, bool doTick, bool doWindow, bool doAudio, bool doInput, int argc, char** argv){
    float windowScale = 1.0f;
    
    m_gameboy = gameboy;
    m_doTick = doTick;
    m_doWindow = doWindow;
    m_doAudio = doAudio;
    m_doInput = doInput;
    
    //Parse Qt specific arguments
    parseArgs(argc, argv, windowScale);
    
    //Initialize SDL
    //init_sdl(windowScale);
    
    if(doWindow){
        /*
        //Connect SDL to the emulator display
        m_mainBufferRenderer = new SDLBufferRenderer(m_SDLWindowRenderer);
        
        //Connect SDL to LCD emulation
        m_gameboy->getLCD()->setMainRenderer(m_mainBufferRenderer);
         */
    }
    
    /*
    if(doAudio){
        m_audioPlayer = new SDLAudioPlayer();
        m_gameboy->getAPU()->setPlayer(m_audioPlayer);
        
        if(USE_THREADED_AUDIO){
            m_audioThread = new thread(&QtYagbeMain::audioThreadLoop, this);
            
            //Thread needs to be detached or main thread won't continue execution
            m_audioThread->detach();
        }
    }*/
    
    /*
    if(doInput){
        m_inputChecker = new SDLInputChecker(m_gameboy->getPad());
        m_gameboy->getCPU()->setInputChecker(m_inputChecker);
    }
    */
    
    if(doTick){
        mainThreadLoop();
    }
}

QtYagbeMain::~QtYagbeMain(){
    if(USE_THREADED_AUDIO && (m_audioThread != nullptr)){
        delete m_audioThread;
    }
    
    /*
    //Free any texture resources here
    if(m_doWindow){
        SDL_DestroyRenderer(m_SDLWindowRenderer);
        m_SDLWindowRenderer = NULL;
        
        SDL_DestroyWindow(m_SDLWindow);
        m_SDLWindow = NULL;
    }
    
    SDL_Quit();
    
    if(m_doWindow){
        delete m_mainBufferRenderer;
    }
    
    delete m_audioPlayer;
    delete m_inputChecker;
     */
}

void QtYagbeMain::mainThreadLoop(){
    /*
    bool bRun = true;
    float speedMultiplier = 1.0f;
    float totalTime = 0;
    float deltaTime = 0;
    unsigned long long countedSDLFrames = 0;
    
    unsigned long long countedGBFrames = 0;
    
    while(bRun){
        if(m_doInput){
            m_inputChecker->checkForInput();
            
            if(m_inputChecker->exitPerformed()){
                bRun = false;
                
                //Tell cart to save if game supports it.
                m_gameboy->getCartridge()->save();
                break;
            }
            
            if(speedMultiplier != m_inputChecker->getSpeedMultiplier()){
                speedMultiplier = m_inputChecker->getSpeedMultiplier();
                std::cout << "Speed multiplier is now: " << speedMultiplier << std::endl;
                m_gameboy->getMemory()->setClockMultiplier(speedMultiplier);
            }
            
            //Update audio channel enable states
            m_gameboy->getAPU()->setSquare1Enabled(m_inputChecker->getAudioSquare1Enabled());
            m_gameboy->getAPU()->setSquare2Enabled(m_inputChecker->getAudioSquare2Enabled());
            m_gameboy->getAPU()->setWaveEnabled(m_inputChecker->getAudioWaveEnabled());
            m_gameboy->getAPU()->setNoiseEnabled(m_inputChecker->getAudioNoiseEnabled());
        }
        
        //Update gameboy framerate report
        long lastGBFrameCount = countedGBFrames;
        countedGBFrames = m_gameboy->getLCD()->getFrames();
        
        //tick CPU, only if delta time is under a second.
        m_gameboy->getCPU()->tick((deltaTime > 1.0f) ? 0 : deltaTime);
        
        if(m_doWindow){
            //Update frame data in the renderer
            m_mainBufferRenderer->update(m_gameboy->getLCD()->getCompleteFrame(), FRAMEBUFFER_WIDTH, FRAMEBUFFER_HEIGHT);
            
            //Render frame
            m_mainBufferRenderer->render();
        }
        
        //Only play audio if not using threaded audio
        if(!USE_THREADED_AUDIO && m_doAudio){
            m_audioPlayer->play(deltaTime);
        }
        
        //Update frame counting
        countedSDLFrames++;
        
        deltaTime = (SDL_GetTicks() / 1000.0f) - totalTime;
        totalTime = (SDL_GetTicks() / 1000.0f);
    }
     */
}

void QtYagbeMain::audioThreadLoop(){
    //Playback freqency is in Hz, equivalent to seconds for this purpose.
    std::chrono::nanoseconds playback_delay (1000000000 / PLAYBACK_FREQUENCY);
    
    while(true){
        //m_audioPlayer->play(0);
        
        //Sleep audio thread to save on CPU usage
        std::this_thread::sleep_for(playback_delay);
    }
}

bool QtYagbeMain::parseArgs(int argc, char** argv, float& windowScale){
    bool bSuccess = true;
    int argIndex = 1;
    while (argIndex < argc) {
        if (strcmp(argv[argIndex], "-s") == 0) { //TODO - handle this flag in QtYagbeMain or QtYagbeMain?
            windowScale = atof(argv[argIndex + 1]);
            argIndex++;
        }
        
        argIndex++;
    }
    
    return bSuccess;
}
