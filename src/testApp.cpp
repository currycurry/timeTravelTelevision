#include "testApp.h"

//--------------------------------------------------------------
void testApp::setup(){
    
    ofSetVerticalSync( true );
    ofHideCursor();
	
    //live video setup
	camWidth 		= 640;	// try to grab at this size.
	camHeight 		= 480;
    rImage          = 0.0;    //initialize red, green, blue pixel values to zero
    gImage          = 0.0;
    bImage          = 0.0;
    numBlackFrames  = 0;    //number of black frames in a transition
    numTimesToBlack = 0;    //number of transitions (numBlackFrames with edge detection)
    setHowLong      = 120.0;  //how long must a continuous sequence be before commercial
    

    fullscreen      = true;
    blackFrame      = false;
    lastVideoFrame  = false;
    goOldAds        = false;
    goLiveFeed      = true;
    bypass          = false; //keep analyzing but just draw the live feed
    debugMode       = false;
    maybeCommercialVideo = false;
    maybeCommercialSound = false;
    maybeCommercial = false;
    
    blackFrameTime = ofGetUnixTime();
    silentFrameTime = ofGetUnixTime();
    transitionThreshold = 5.0;
    
    topOfTheHour = 0;
    lastHour = 0;
    
    lastBlackFrameTime = ofGetUnixTime();
    timeSinceLastBlackFrame = 0.0;
	
	vidGrabber.setVerbose(true);
	vidGrabber.initGrabber(camWidth,camHeight);
	
	//videoInverted 	= new unsigned char[camWidth*camHeight*3];
	videoTexture.allocate(camWidth,camHeight, GL_RGB);
    
    //old ads setup
    oldAds[0].loadMovie("AD1.mov");
    oldAds[1].loadMovie("AD2.mov");
    oldAds[2].loadMovie("AD3.mov");
    oldAds[3].loadMovie("AD4.mov");
    oldAds[4].loadMovie("AD5.mov");
    oldAds[5].loadMovie("AD6.mov");
    
    randomizeAds    = true;
    
    totalAds        = 6;
    adNum           = 0;
    adsPlayedCounter= 0;
    numCommercialBreaks = 0;
    setNumAdsInBreak = 10;
    setHowLongAds   = 65.0;   //how long must we be without a black frame to return from commercial
    
    //audio analysis setup
    soundStream.listDevices();
    int bufferSize = 256;
    
    left.assign(bufferSize, 0.0);
    right.assign(bufferSize, 0.0);
    volHistory.assign(400, 0.0);
    
    bufferCounter   = 0;
    drawCounter     = 0;
    smoothedVol     = 0.0;
    scaledVol       = 0.0;
    volumeSum       = 0.0;
    averageVolume   = 0.0;
    samplingPeriod  = 5.5;
    samplingFrequency = 500;
    silenceLevelCutoff   = 2.3;
    blackLevel      = 5.0;
    
    silentFrame     = true;
    lastSoundFrame  = true;
    silenceLength   = 0;
    numSilentFrames = 0;
    lastSilentFrameTime = ofGetUnixTime();
    timeSinceLastSilentFrame = 0;
    
    soundStream.setup(this, 0, 2, 44100, bufferSize, 4);
    
    
    panel.setup(0, 400, 420, 400);
	panel.addPanel("Detection Settings");
    panel.addSlider("How long must a sequence be?", 120, 10, 300);
    panel.addSlider("sampling frequency", 4.9, 0, 20);
    panel.addSlider("sampling period", 9.4, 0, 50);
    panel.addSlider("silence level cutoff", 1.2, 0, 10);
    panel.addSlider("black level cutoff", 5.0, 0, 10);
    panel.addSlider("threshold difference audio and video", 5.0, 0, 8.0);
    
    panel.addPanel("Commercial Break Settings");
    panel.addSlider("how many ads per break?", 10, 0, 10);
    panel.addSlider("How long without break for return?", 90, 10, 300);
    panel.addToggle("randomize ad order", true);
    
    ////logging
    ofSetLogLevel("logChannel", OF_LOG_VERBOSE);
    startTimeSeconds = ofGetUnixTime();
    logTimerSeconds = 0;
    elapsedTimeSeconds = 0;
    loggingInterval = 15;
    logToFile = false;
    currentLogToFile = false;
    timeStamp = ofToString(ofGetMonth())+"/"+ofToString(ofGetDay())+"/"+ofToString(ofGetYear())+"."+ofToString(ofGetHours())+"."+ofToString(ofGetMinutes())+"."+ofToString(ofGetSeconds());
    
    
    //setup Arduino
    oneAdButtonState = "play one ad button state: ";
    adButtonState = "trigger commercial state button: ";
    liveButtonState = "trigger live state button: ";
    bypassSwitchState = "bypass effect switch state: ";
    
    oneAdButton = 0;
    lastOneAdButton = 0;
    adButton = 0;
    lastAdButton = 0;
    liveButton = 0;
    lastLiveButton = 0;
    bypassSwitch = 0;
    lastBypassSwitch = 0;
    playOne = false;
    
    ard.connect("/dev/tty.usbserial-FTFO9G0L", 57600); //connect arduino at 57600 baud
    ofAddListener(ard.EInitialized, this, &testApp::setupArduino); //make sure arduino is ready before calling setupArduino()
	bSetupArduino	= false;	// flag so we setup arduino when its ready, you don't need to touch this :)


}


//--------------------------------------------------------------
void testApp::update(){
	
	ofBackground(0);
    ofSetFullscreen( fullscreen );
    
    
    setHowLong          = panel.getValueF("How long must a sequence be?");
    samplingFrequency   = panel.getValueI("sampling frequency");
    samplingPeriod      = panel.getValueI("sampling period");
    silenceLevelCutoff  = panel.getValueF("silence level cutoff");
    blackLevel          = panel.getValueF("black level cutoff");
    setNumAdsInBreak    = panel.getValueI("how many ads per break?");
    setHowLongAds       = panel.getValueF("How long without break for return?");
    randomizeAds        = panel.getValueB("randomize ad order");
    transitionThreshold = panel.getValueF("threshold difference audio and video");
    
    //live video update
    vidGrabber.update();
	
    if (vidGrabber.isFrameNew()){
        
        // average r, g, b values
		int totalPixels = camWidth*camHeight*3;
		unsigned char * pixels = vidGrabber.getPixels();
        int numPixels = 0;
        
        for (int y=0; y<camHeight; y++){
            for(int x=0; x<camWidth; x++){
                // the index of the pixel:
                int index = y*camWidth*3 + x*3;
                int red = pixels[index];
                int green = pixels[index+1];
                int blue = pixels[index+2];
                
                rImage += red;
                gImage += green;
                bImage += blue;
                numPixels ++;
            }
        }
        rImage /= numPixels;
        gImage /= numPixels;
        bImage /= numPixels;
        
        if ( rImage < blackLevel && gImage < blackLevel && bImage < blackLevel ) {
            blackFrame = true;
            numBlackFrames ++;
            ofLog(OF_LOG_WARNING, "RGB at black detected: ( %f, %f, %f )", rImage, gImage, bImage);
            if (blackFrame != lastVideoFrame) {
                numTimesToBlack ++;
                if (timeSinceLastBlackFrame >= setHowLong){
                    if (!maybeCommercialVideo) {
                        maybeCommercialVideo = true;
                        blackFrameTime = ofGetUnixTime(); //use this for comparison with audio
                    }
                //lastBlackFrameTime = ofGetUnixTime();   //use this for every detected black frame
                }
            lastBlackFrameTime = ofGetUnixTime();   //use this for every detected black frame
            }
        else {
            blackFrame = false;
            if (blackFrame != lastVideoFrame) {
                blackLength = numBlackFrames;
                numBlackFrames = 0;
            }
        }

        }
        lastVideoFrame = blackFrame;
        timeSinceLastBlackFrame = ofGetUnixTime() - lastBlackFrameTime;
        if ( timeSinceLastBlackFrame >  transitionThreshold ) {
            maybeCommercialVideo = false;
        }
        if ( timeSinceLastBlackFrame < 0 ) {//if timeSinceLastBlackFrame becomes negative
            ofLog(OF_LOG_WARNING, "-------------------------------------");
            ofLog(OF_LOG_WARNING, "%i/%i/%i, %i:%i:%i", ofGetMonth(), ofGetDay(), ofGetYear(), ofGetHours(), ofGetMinutes(), ofGetSeconds());
            ofLog(OF_LOG_WARNING, "timeSinceLastBlackFrame: %i", timeSinceLastBlackFrame);
            ofLog(OF_LOG_WARNING, "timer became negative at %f seconds since start", ofGetUnixTime() - startTimeSeconds);
            ofLog(OF_LOG_WARNING, "timer became negative at %i", ofGetSystemTime());
            
            //lastBlackFrameTime = ofGetUnixTime() - setHowLong; //dumb failsafe
        }
	}
    
    

    ////------------------------------------------------------
    //audio analysis update
    //scale the vol up to a 0-1 range
	scaledVol = ofMap(smoothedVol, 0.0, 0.17, 0.0, 1.0, true);
    
	//record the volume into an array
	volHistory.push_back( scaledVol );
	
	//if we are bigger than the size we want to record - drop the oldest value
	if( volHistory.size() >= 400 ){
		volHistory.erase(volHistory.begin(), volHistory.begin()+1);
	}
    
    //find average volume in sampling period
    if ( ofGetSystemTime() % samplingFrequency > 0 && ofGetSystemTime() % samplingFrequency <= 50 ) {
        for ( int i = volHistory.size() - samplingPeriod; i < volHistory.size(); i ++ ) {
            volumeSum += volHistory[i];
        }
        averageVolume = volumeSum/100;
        volumeSum = 0;
    }
    
    //check for silence
    if ( averageVolume * 100.0 < silenceLevelCutoff ) { 
        silentFrame = true;
        numSilentFrames ++;
        if (silentFrame != lastSoundFrame) {
            numTimesToSilence ++;
                if (!maybeCommercialSound) {
                    silentFrameTime = ofGetUnixTime();
                    maybeCommercialSound = true;
                }
                /*else {
                    maybeCommercialSound = false;
                }*/
            //lastSilentFrameTime = ofGetUnixTime();
        }
    }
    else {
        silentFrame = false;
        if (silentFrame != lastSoundFrame) {
            silenceLength = numSilentFrames;
            numSilentFrames = 0;
        }
    }
    lastSoundFrame = silentFrame;
    timeSinceLastSilentFrame = ofGetUnixTime() - silentFrameTime;
    if ( timeSinceLastSilentFrame >  transitionThreshold ) {
        maybeCommercialSound = false;
    }
    
    /////---------------------------------------------------------------
    if ( maybeCommercialSound && maybeCommercialVideo ) { //combine silence and black frame analysis
        if ( !maybeCommercial ) {
            maybeCommercial = true;
            setHowLong = 120;
            numCommercialBreaks ++;
            if(randomizeAds){
                //random selection
                adNum = floor(ofRandom(totalAds));
            }
        
            else {
                //incremental selection
                adNum ++;
                if ( adNum == totalAds ) {
                    adNum = 0;
                }
            }
            oldAds[adNum].setPosition(0.0);
            oldAds[adNum].play();
            goOldAds = true;
            goLiveFeed = false;
            blackFrameTime = ofGetUnixTime();
            silentFrameTime = ofGetUnixTime();
            
            ofLog(OF_LOG_NOTICE, "-------------------------------------");
            ofLog(OF_LOG_NOTICE, "%i/%i/%i, %i:%i:%i", ofGetMonth(), ofGetDay(), ofGetYear(), ofGetHours(), ofGetMinutes(), ofGetSeconds());
            ofLog(OF_LOG_WARNING, "COMMERCIAL DETECTED");
            ofLog(OF_LOG_NOTICE, "Run time is %i seconds", elapsedTimeSeconds);
            ofLog(OF_LOG_NOTICE, "Number of times to black: %i", numTimesToBlack);
            ofLog(OF_LOG_NOTICE, "Number of black frames detected total: %i", numBlackFrames);
            ofLog(OF_LOG_NOTICE, "Number of times to silence: %i", numTimesToSilence);
            ofLog(OF_LOG_NOTICE, "Number of silent frames detected total: %i", numSilentFrames);
            ofLog(OF_LOG_NOTICE, "Number of commercials detected: %i", numCommercialBreaks);
            ofLog(OF_LOG_NOTICE, "Time since last black frame: %i", timeSinceLastBlackFrame);
            ofLog(OF_LOG_NOTICE, "Time since last silent frame: %i", timeSinceLastSilentFrame);
            ofLog(OF_LOG_NOTICE, "Black level cutoff if %f", blackLevel);
            ofLog(OF_LOG_NOTICE, "Silence level cutoff if %f", silenceLevelCutoff);
            
            if ( maybeCommercial ) {
                ofLog(OF_LOG_NOTICE, "Are we in a commercial break: yes");
                ofLog(OF_LOG_NOTICE, "Number of ads played this break: %i", adsPlayedCounter);
            }
            else {
                ofLog(OF_LOG_NOTICE, "Are we in a commercial break: no");
            }
            
        }
    }
    
    //////--------------------------------------------------------------------
    //ads playing update
    //check if ad is finished and load the next
    if ( goOldAds ){
        oldAds[adNum].update();
        if ( oldAds[adNum].getDuration() - oldAds[adNum].getPosition() <= 29.07) {
            adsPlayedCounter++;
                oldAds[adNum].stop();
                if(randomizeAds){
                    //random selection
                    adNum = floor(ofRandom(totalAds));
                }
                else {
                    //incremental selection
                    adNum ++;
                    if ( adNum == totalAds ) {
                        adNum = 0;
                    }
                }
                oldAds[adNum].setPosition(0.0);
                oldAds[adNum].update();
                oldAds[adNum].play();
            
            if (playOne && adsPlayedCounter >= 1) { //if we're just playing one, switch back after one
                maybeCommercial = false;
                goOldAds = false;
                goLiveFeed = true;
                oldAds[adNum].setPosition(0.0);
                for ( int i = 0; i < totalAds; i ++ ) {
                    oldAds[i].stop();
                }
                //oldAds[adNum].stop();
                adsPlayedCounter = 0;
                lastBlackFrameTime = ofGetUnixTime();
                ofLog(OF_LOG_NOTICE, "-------------------------------------");
                ofLog(OF_LOG_NOTICE, "%i/%i/%i, %i:%i:%i", ofGetMonth(), ofGetDay(), ofGetYear(), ofGetHours(), ofGetMinutes(), ofGetSeconds());
                ofLog(OF_LOG_NOTICE, "Back to live programming");
                playOne = false;
            }
            
            else if (timeSinceLastBlackFrame > setHowLongAds || adsPlayedCounter >= setNumAdsInBreak) { //otherwise return back to live after timeout or set number of ads
                maybeCommercial = false;
                goOldAds = false;
                goLiveFeed = true;
                oldAds[adNum].setPosition(0.0);
                for ( int i = 0; i < totalAds; i ++ ) {
                    oldAds[i].stop();
                }
                adsPlayedCounter = 0;
                lastBlackFrameTime = ofGetUnixTime();
                ofLog(OF_LOG_NOTICE, "-------------------------------------");
                ofLog(OF_LOG_NOTICE, "%i/%i/%i, %i:%i:%i", ofGetMonth(), ofGetDay(), ofGetYear(), ofGetHours(), ofGetMinutes(), ofGetSeconds());
                ofLog(OF_LOG_NOTICE, "Back to live programming");
            }
        }
    }
    
    topOfTheHour = ofGetHours();
    //sanity check at top of the hour
    if ( topOfTheHour != lastHour ) {
        maybeCommercial = false;
        goOldAds = false;
        goLiveFeed = true;
        oldAds[adNum].setPosition(0.0);
        oldAds[adNum].stop();
        adsPlayedCounter = 0;
        lastBlackFrameTime = ofGetUnixTime();
        ofLog(OF_LOG_NOTICE, "-------------------------------------");
        ofLog(OF_LOG_NOTICE, "%i/%i/%i, %i:%i:%i", ofGetMonth(), ofGetDay(), ofGetYear(), ofGetHours(), ofGetMinutes(), ofGetSeconds());
        ofLog(OF_LOG_NOTICE, "Top of the hour, back to programming");

    }
    lastHour = topOfTheHour;
    
    //periodic logging
    elapsedTimeSeconds = ofGetUnixTime() - startTimeSeconds;
    if ( elapsedTimeSeconds >= logTimerSeconds ) {
        ofLog(OF_LOG_NOTICE, "-------------------------------------");
        ofLog(OF_LOG_NOTICE, "%i/%i/%i, %i:%i:%i", ofGetMonth(), ofGetDay(), ofGetYear(), ofGetHours(), ofGetMinutes(), ofGetSeconds());
        ofLog(OF_LOG_NOTICE, "Run time is %i seconds", elapsedTimeSeconds);
        ofLog(OF_LOG_NOTICE, "Number of times to black: %i", numTimesToBlack);
        ofLog(OF_LOG_NOTICE, "Number of black frames detected total: %i", numBlackFrames);
        ofLog(OF_LOG_NOTICE, "Number of times to silence: %i", numTimesToSilence);
        ofLog(OF_LOG_NOTICE, "Number of silent frames detected total: %i", numSilentFrames);
        ofLog(OF_LOG_NOTICE, "Number of commercials detected: %i", numCommercialBreaks);
        ofLog(OF_LOG_NOTICE, "Time since last black frame: %i", timeSinceLastBlackFrame);
        ofLog(OF_LOG_NOTICE, "Time since last silent frame: %i", timeSinceLastSilentFrame);
        ofLog(OF_LOG_NOTICE, "Black level cutoff if %f", blackLevel);
        ofLog(OF_LOG_NOTICE, "Silence level cutoff if %f", silenceLevelCutoff);
        
        if ( maybeCommercial ) {
            ofLog(OF_LOG_NOTICE, "Are we in a commercial break: yes");
            ofLog(OF_LOG_NOTICE, "Number of ads played this break: %i", adsPlayedCounter);
        }
        else {
            ofLog(OF_LOG_NOTICE, "Are we in a commercial break: no");
        }
		logTimerSeconds += loggingInterval; // triggers logging periodically
    }
    
    if(logToFile != currentLogToFile){ // if the logging channel has just changed
		currentLogToFile = logToFile;
        if(logToFile){
            // Module-specific logging
            // These print always, even if global log level is Notice
            // They also start with the module name
            timeStamp = ofToString(ofGetMonth())+"/"+ofToString(ofGetDay())+"/"+ofToString(ofGetYear())+"."+ofToString(ofGetHours())+"."+ofToString(ofGetMinutes())+"."+ofToString(ofGetSeconds());
            ofLogVerbose("logChannel") << "Switching to file logging";
            ofLogToFile("logfile-"+timeStamp+".log", true); //set channel to log file. log file resides in the /bin/data folder
            ofLogVerbose("logChannel") << "Logging to file now";
        }
        else{
            ofLogVerbose("logChannel") << "Switching to console logging";
            ofLogToConsole(); //set channel to console
            ofLogVerbose("logChannel") << "Logging to console now";
        }
    }

    

    //arduino
    updateArduino();


}

//--------------------------------------------------------------
void testApp::draw(){
	ofSetHexColor(0xffffff);
    ofBackground(0);
    
    if ( !bypass ) {
        
    
        if ( goLiveFeed ) {
            vidGrabber.draw(0, -110, 1124, 1000);
        }
    
        if ( goOldAds ) {
            oldAds[adNum].draw(0, 0, 1024, 768);
        }
    
        if ( debugMode ) {
            ofEnableAlphaBlending();
            ofSetColor(0, 0, 0, 150);
            ofFill();
            ofRect(0, 0, 420, 400);
            ofDisableAlphaBlending();
        
            ofSetColor(255);
            ofDrawBitmapString("frame rate: " + ofToString(ofGetFrameRate(),2),20,160);
            ofDrawBitmapString("average RGB: ( " + ofToString( rImage ) + ", " + ofToString( gImage ) + ", " + ofToString( bImage ) + " )",20,180);
            ofDrawBitmapString("Number of Black Frames in Transition: " + ofToString( blackLength ),20,220);
            ofDrawBitmapString("Number of Times to Black: " + ofToString( numTimesToBlack ),20,240);
            ofDrawBitmapString("Number of Silent Frames in Transition: " + ofToString( silenceLength ),20,260);
            ofDrawBitmapString("Number of Times to Silence: " + ofToString( numTimesToSilence ),20,280);
        
        
            ofDrawBitmapString("How many seconds since last black frame? " + ofToString( timeSinceLastBlackFrame ),20,300);
            ofDrawBitmapString("How many seconds since last silent frame? " + ofToString( timeSinceLastSilentFrame ),20,320);
        
        
            if ( maybeCommercial ) {
                ofDrawBitmapString("Do I think we're in commercial? Yes" ,20,340);
            }
            else {
                ofDrawBitmapString("Do I think we're in commercial? No" ,20,340);
            }
        
            ofDrawBitmapString("Number of old ads played this break: " + ofToString( adsPlayedCounter ),20,360);
            ofDrawBitmapString("Number of commercial breaks so far: " + ofToString( numCommercialBreaks ),20,380);
        
                
            // draw the average volume:
            ofPushStyle();
            ofPushMatrix();
            ofTranslate(20, 20);
        
            ofSetColor(0);
            ofFill();
            ofRect(0, 0, 400, 100);
            ofSetColor(225);
            ofDrawBitmapString("Scaled average vol (0-100): " + ofToString(scaledVol * 100.0, 0), 4, 18);
            ofDrawBitmapString("Average volume per second: " + ofToString(averageVolume * 100.0), 4, 28);
        
		
				
            //draw the volume history as a graph
            ofSetColor(245, 58, 135);
            ofFill();
            ofBeginShape();
            for (int i = 0; i < volHistory.size(); i++){
                if( i == 0 ) ofVertex(i, 100);
            
                ofVertex(i, 100 - volHistory[i] * 70);
			
                if( i == volHistory.size() -1 ) ofVertex(i, 100);
            }
            ofEndShape(false);
        
            ofPopMatrix();
            ofPopStyle();
        
            drawCounter++;
            
        }
    }
    
    else {
        vidGrabber.draw(0, -110, 1124, 1000);
    }
    
    
}

//--------------------------------------------------------------
void testApp::audioIn(float * input, int bufferSize, int nChannels){
	
	float curVol = 0.0;
	
	// samples are "interleaved"
	int numCounted = 0;
    
	//go through each sample and calculate the root mean square which is a rough way to calculate volume
	for (int i = 0; i < bufferSize; i++){
		left[i]		= input[i*2]*0.5;
		right[i]	= input[i*2+1]*0.5;
        
		curVol += left[i] * left[i];
		curVol += right[i] * right[i];
		numCounted+=2;
	}
	
	//this is how we get the mean of rms 
	curVol /= (float)numCounted;
	
	// this is how we get the root of rms 
	curVol = sqrt( curVol );
	
	smoothedVol *= 0.93;
	smoothedVol += 0.07 * curVol;
	
	bufferCounter++;
	
}


//--------------------------------------------------------------
void testApp::keyPressed  (int key){ 
	
	// in fullscreen mode, on a pc at least, the 
	// first time video settings the come up
	// they come up *under* the fullscreen window
	// use alt-tab to navigate to the settings
	// window. we are working on a fix for this...
	
	// Video settings no longer works in 10.7
	// You'll need to compile with the 10.6 SDK for this
    // For Xcode 4.4 and greater, see this forum post on instructions on installing the SDK
    // http://forum.openframeworks.cc/index.php?topic=10343        
	if (key == 's' || key == 'S'){
		vidGrabber.videoSettings();
	}
    
    if ( key == 'f' || key == 'F') {
        if (!fullscreen) {
            fullscreen = true;
            ofHideCursor();
        }
        else {
            fullscreen = false;
        }
    }
    
    if ( key == '1' ) { //ad state
        maybeCommercial = true;
        
        if(randomizeAds){
            //random selection
            adNum = floor(ofRandom(totalAds));
        }
        
        else {
            //incremental selection
            adNum ++;
            if ( adNum == totalAds ) {
                adNum = 0;
            }
        }
        panel.setValueB("switch to old ads", true);
        oldAds[adNum].setPosition(0.0);
        oldAds[adNum].play();
        goOldAds = true;
        goLiveFeed = false;
        lastBlackFrameTime = ofGetUnixTime();
        ofLog(OF_LOG_NOTICE, "-------------------------------------");
        ofLog(OF_LOG_NOTICE, "%i/%i/%i, %i:%i:%i", ofGetMonth(), ofGetDay(), ofGetYear(), ofGetHours(), ofGetMinutes(), ofGetSeconds());
        ofLog(OF_LOG_NOTICE, "Switch to commercial via switch");
        
    }
    if ( key == '2' ) { //live mode
        maybeCommercial = false;
        goOldAds = false;
        goLiveFeed = true;
        oldAds[adNum].setPosition(0.0);
        for ( int i = 0; i < totalAds; i ++ ) {
            oldAds[i].stop();
        }
        adsPlayedCounter = 0;
        panel.setValueB("switch to old ads", false);
        lastBlackFrameTime = ofGetUnixTime();
        ofLog(OF_LOG_NOTICE, "-------------------------------------");
        ofLog(OF_LOG_NOTICE, "%i/%i/%i, %i:%i:%i", ofGetMonth(), ofGetDay(), ofGetYear(), ofGetHours(), ofGetMinutes(), ofGetSeconds());
        ofLog(OF_LOG_NOTICE, "Back to live programming via switch");
        
    }
    
    if ( key == 'd' ) {
        if (!debugMode) {
            debugMode = true;
            ofShowCursor();
        }
        else {
            debugMode = false;
            ofHideCursor();
        }
    }
    
    if ( key == 'l' ) {
        if (!logToFile) {
            logToFile = true;
        }
        else {
            logToFile = false;
        }
    }
    
    if ( key == 'n') { //press n to log current state of variables
        ofLog(OF_LOG_NOTICE, "-------------------------------------");
        ofLog(OF_LOG_NOTICE, "%i/%i/%i, %i:%i:%i", ofGetMonth(), ofGetDay(), ofGetYear(), ofGetHours(), ofGetMinutes(), ofGetSeconds());
        ofLog(OF_LOG_NOTICE, "Run time is %i seconds", elapsedTimeSeconds);
        ofLog(OF_LOG_NOTICE, "Number of times to black: %i", numTimesToBlack);
        ofLog(OF_LOG_NOTICE, "Number of black frames detected total: %i", numBlackFrames);
        ofLog(OF_LOG_NOTICE, "Number of times to silence: %i", numTimesToSilence);
        ofLog(OF_LOG_NOTICE, "Number of silent frames detected total: %i", numSilentFrames);
        ofLog(OF_LOG_NOTICE, "Number of commercials detected: %i", numCommercialBreaks);
        ofLog(OF_LOG_NOTICE, "Time since last black frame: %i", timeSinceLastBlackFrame);
        ofLog(OF_LOG_NOTICE, "Time since last silent frame: %i", timeSinceLastSilentFrame);
        ofLog(OF_LOG_NOTICE, "Black level cutoff if %f", blackLevel);
        ofLog(OF_LOG_NOTICE, "Silence level cutoff if %f", silenceLevelCutoff);
        
        if ( maybeCommercial ) {
            ofLog(OF_LOG_NOTICE, "Are we in a commercial break: yes");
            ofLog(OF_LOG_NOTICE, "Number of ads played this break: %i", adsPlayedCounter);
        }
        else {
            ofLog(OF_LOG_NOTICE, "Are we in a commercial break: no");
        }
    }
    
    if ( key == 'b' || key == 'B') {
        if (!bypass) {
            bypass = true;
            for (int i = 0; i < totalAds; i ++ ) {
                oldAds[i].setVolume(0.0);
            }
        }
        else {
            bypass = false;
            for (int i = 0; i < totalAds; i ++ ) {
                oldAds[i].setVolume(100.0);
            }
        }
    }

	
}

//--------------------------------------------------------------
void testApp::keyReleased(int key){ 
	
}

//--------------------------------------------------------------
void testApp::mouseMoved(int x, int y ){
	
}

//--------------------------------------------------------------
void testApp::mouseDragged(int x, int y, int button){
	
}

//--------------------------------------------------------------
void testApp::mousePressed(int x, int y, int button){
	
}

//--------------------------------------------------------------
void testApp::mouseReleased(int x, int y, int button){

}

//--------------------------------------------------------------
void testApp::windowResized(int w, int h){

}

//--------------------------------------------------------------
void testApp::gotMessage(ofMessage msg){

}

//--------------------------------------------------------------
void testApp::dragEvent(ofDragInfo dragInfo){ 

}
//--------------------------------------------------------------
void testApp::setupArduino(const int & version) {
	
	// remove listener because we don't need it anymore
	ofRemoveListener(ard.EInitialized, this, &testApp::setupArduino);
    
    // it is now safe to send commands to the Arduino
    bSetupArduino = true;
    
    // print firmware name and version to the console
    cout << ard.getFirmwareName() << endl;
    cout << "firmata v" << ard.getMajorFirmwareVersion() << "." << ard.getMinorFirmwareVersion() << endl;
    

    // set pins D2, D5, D10, D13 to input
    ard.sendDigitalPinMode(2, ARD_INPUT);
    ard.sendDigitalPinMode(5, ARD_INPUT);
    ard.sendDigitalPinMode(10, ARD_INPUT);
    ard.sendDigitalPinMode(13, ARD_INPUT);

    // Listen for changes on the digital and analog pins
    ofAddListener(ard.EDigitalPinChanged, this, &testApp::digitalPinChanged);
    
}
//--------------------------------------------------------------
void testApp::updateArduino(){
    
	// update the arduino, get any data or messages.
    // the call to ard.update() is required
	ard.update();
}
//--------------------------------------------------------------
void testApp::digitalPinChanged(const int & pinNum) {
    // do something with the digital input. here we're simply going to print the pin number and
    // value to the screen each time it changes
    //buttonState = "digital pin: " + ofToString(pinNum) + " = " + ofToString(ard.getDigital(pinNum));
    oneAdButtonState = "play one ad button state: " + ofToString(ard.getDigital(2));
    adButtonState = "trigger commercial state button: " + ofToString(ard.getDigital(5));
    liveButtonState = "trigger live state button: " + ofToString(ard.getDigital(10));
    bypassSwitchState = "bypass effect switch state: " + ofToString(ard.getDigital(13));
    
    oneAdButton = ard.getDigital(2);
    if ( oneAdButton != lastOneAdButton ) {
        if ( oneAdButton == 1 ) {
            ofLog(OF_LOG_NOTICE, "-------------------------------------");
            ofLog(OF_LOG_NOTICE, "%i/%i/%i, %i:%i:%i", ofGetMonth(), ofGetDay(), ofGetYear(), ofGetHours(), ofGetMinutes(), ofGetSeconds());
            ofLog(OF_LOG_NOTICE, "Playing one ad via switch");

            if(randomizeAds){
                //random selection
                adNum = floor(ofRandom(totalAds));
            }
            else {
                //incremental selection
                adNum ++;
                if ( adNum == totalAds ) {
                    adNum = 0;
                }
            }
            panel.setValueB("switch to old ads", true);
            oldAds[adNum].setPosition(0.0);
            oldAds[adNum].play();

            playOne = true;
            goOldAds = true;
            goLiveFeed = false;
        }
    }
    lastOneAdButton = oneAdButton;
    
    adButton = ard.getDigital(5);
    if ( adButton != lastAdButton ) {
        if ( adButton == 1 ) {
            maybeCommercial = true;
            if(randomizeAds){
                //random selection
                adNum = floor(ofRandom(totalAds));
            }
            else {
                //incremental selection
                adNum ++;
                if ( adNum == totalAds ) {
                    adNum = 0;
                }
            }
            panel.setValueB("switch to old ads", true);
            oldAds[adNum].setPosition(0.0);
            oldAds[adNum].play();
            goOldAds = true;
            goLiveFeed = false;
            //lastBlackFrameTime = ofGetUnixTime();
            goOldAds = true;
            goLiveFeed = false;
            ofLog(OF_LOG_NOTICE, "-------------------------------------");
            ofLog(OF_LOG_NOTICE, "%i/%i/%i, %i:%i:%i", ofGetMonth(), ofGetDay(), ofGetYear(), ofGetHours(), ofGetMinutes(), ofGetSeconds());
            ofLog(OF_LOG_NOTICE, "Commercial break via switch");
            
        }
    }
    lastAdButton = adButton;
    
    liveButton = ard.getDigital(10);
    if ( liveButton != lastLiveButton ) {
        if ( liveButton == 1 ) {
            maybeCommercial = false;
            goOldAds = false;
            goLiveFeed = true;
            oldAds[adNum].setPosition(0.0);
            for ( int i = 0; i < totalAds; i ++ ) {
                oldAds[i].stop();
            }
            adsPlayedCounter = 0;
            panel.setValueB("switch to old ads", false);
            setHowLong = 5;
            //lastBlackFrameTime = ofGetUnixTime();
            ofLog(OF_LOG_NOTICE, "-------------------------------------");
            ofLog(OF_LOG_NOTICE, "%i/%i/%i, %i:%i:%i", ofGetMonth(), ofGetDay(), ofGetYear(), ofGetHours(), ofGetMinutes(), ofGetSeconds());
            ofLog(OF_LOG_NOTICE, "Back to live programming via switch");
        }
    }
    lastLiveButton = liveButton;
    
    bypassSwitch = ard.getDigital(13);
    if ( bypassSwitch != lastBypassSwitch ) {
        if ( bypassSwitch == 1 ) {
            bypass = true;
            for (int i = 0; i < totalAds; i ++ ) {
                oldAds[i].setVolume(0.0);
            }
        }
        else {
            bypass = false;
            for (int i = 0; i < totalAds; i ++ ) {
                oldAds[i].setVolume(100.0);
            }
        }
    }
    lastBypassSwitch = bypassSwitch;
    
}

