#pragma once

#include "ofMain.h"
#include "ofxAutoControlPanel.h"
#include "ofEvents.h"


class testApp : public ofBaseApp{
	
	public:
		
		void setup();
		void update();
		void draw();
		
		void keyPressed(int key);
		void keyReleased(int key);
		void mouseMoved(int x, int y );
		void mouseDragged(int x, int y, int button);
		void mousePressed(int x, int y, int button);
		void mouseReleased(int x, int y, int button);
		void windowResized(int w, int h);
		void dragEvent(ofDragInfo dragInfo);
		void gotMessage(ofMessage msg);
	
        //live video analysis
		ofVideoGrabber 		vidGrabber;
		unsigned char * 	videoInverted;
		ofTexture			videoTexture;
		int 				camWidth;
		int 				camHeight;
    
        float               rImage;
        float               gImage;
        float               bImage;
    
        bool                fullscreen;
        bool                blackFrame;
        bool                lastVideoFrame;
        bool                debugMode;
    
        int                 numBlackFrames;
        int                 blackLength;
        int                 numTimesToBlack;
        int                 adNum;
        int                 adsPlayedCounter;
        int                 setNumAdsInBreak;
        float               setHowLongAds;
        int                 numCommercialBreaks;
        int                 lastBlackFrameTime;
        int                 timeSinceLastBlackFrame;
        float               setHowLong;
        bool                maybeCommercialVideo;
        bool                maybeCommercialSound;
        bool                maybeCommercial;
    
        int                 blackFrameTime;
        int                 silentFrameTime;
        float               transitionThreshold;
    
        int                 topOfTheHour;
        int                 lastHour;
    
    
        //old ads
        int                 totalAds;
        ofVideoPlayer 		oldAds[6];
    
        bool                randomizeAds;
        bool                goOldAds;
        bool                goLiveFeed;
        bool                bypass;
    
        //sound analysis
        void audioIn(float * input, int bufferSize, int nChannels);
	
        vector <float>      left;
        vector <float>      right;
        vector <float>      volHistory;
    
        int                 bufferCounter;
        int                 drawCounter;
    
        float               smoothedVol;
        float               scaledVol;
        float               volumeSum;
        float               averageVolume;
    
        int                 samplingFrequency;
        int                 samplingPeriod;
        float                 silenceLevelCutoff;
        float               blackLevel;
        bool                silentFrame;
        bool                lastSoundFrame;
        int                 silenceLength;
        int                 numSilentFrames;
        int               lastSilentFrameTime;
        int               timeSinceLastSilentFrame;
        int                 numTimesToSilence;
    
        ofSoundStream       soundStream;

        ofxAutoControlPanel panel;
    
    
        //logging
        bool logToFile;
        bool currentLogToFile;
        int logTimerSeconds;
        int elapsedTimeSeconds;
        int startTimeSeconds;
        int loggingInterval;
        string timeStamp;
    
        //Arduino
    ofArduino   ard;
    bool    bSetupArduino;
    
private:
    
    void setupArduino(const int & version);
    void digitalPinChanged(const int & pinNum);
    void updateArduino();
    
    string oneAdButtonState;
    string adButtonState;
    string liveButtonState;
    string bypassSwitchState;
    
    int oneAdButton;
    int lastOneAdButton;
    bool playOne;
    int adButton;
    int lastAdButton;
    int liveButton;
    int lastLiveButton;
    int bypassSwitch;
    int lastBypassSwitch;
    
    
};
