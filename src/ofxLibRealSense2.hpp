//
//  ofxLibRealSense2.hpp
//  example
//
//  Created by shiyamon on 2018/05/25.
//

#pragma once
#include "librealsense2/rs.hpp"
#include "ofThread.h"
#include "ofTexture.h"
#include "ofxGui.h"

#include "ofxAutoReloadedShader.h"

class ofxLibRealSense2 : public ofThread
{
    
public:

	enum Preset {
		DEFAULT = 0,
		HIGH_ACCURACY = 1,
		HIGH_DENSITY = 2,
		MEDIUM_DENSITY = 3,
		HAND = 4,
		REMOVE_IR_PATTERN = 5
	};


    void setupDevice(int deviceID);
    void setupColor(int width, int height, int fps=60);
    void setupIR(int width, int height, int fps=60);
    void setupDepth(int width, int height, int fps=60);
	void setupPointcloud();
    void startPipeline(bool useThread, Preset p = Preset::DEFAULT );
    void update();
    void exit();

	int getDeviceCount();
    
    ofTexture*  getColorTex()   { return &_colTex; }
    ofTexture*  getIrTex()      { return &_irTex; }
    ofTexture*  getDepthTex()   { return &_depthTex; }
	ofTexture*  getMappedDepthTex() { return &mappedDepthTex; }
	ofVboMesh*	getPointCloudRef() { return &mesh; }

    int getColorWidth() { return _colorWidth; }
    int getColorHeight(){ return _colorHeight; }
    int getIrWidth()    { return _irWidth; }
    int getIrHeight()   { return _irHeight; }
    int getDepthWidth() { return _depthWidth; }
    int getDepthHeight(){ return _depthHeight; }
    bool isFrameNew()   { return _hasNewFrame; }
	float getFrameRate() { return framerate; };

    ofxGuiGroup *getGui();
    
    ofxLibRealSense2() : _setupFinished(false), _colorEnabled(false), _irEnabled(false), _depthEnabled(false), _pipelineStarted(false), _useThread(false) {}


	ofPixels mappedDepthPix;
	ofShortPixels rawDepthPix;
	ofVboMesh mesh;

	ofEvent<ofPixels> newMappedPixelsEvt;
	ofEvent<void> newDepthEvent;

	void useDepthShader(bool use) { _bUseDepthShader = use; };
	void setSensorPreset( Preset p );
	void setSensorPreset( int i );
	int getNumPresets() { return NUM_PRESETS; };
	void setNextPreset();
	void setPreviousPreset();
	Preset getCurrentPreset() { return _preset; };
	string getCurrentPresetName();

private:


	ofxAutoReloadedShader mapShader;
	ofFbo mapFbo;

	Preset			_preset;
	const int NUM_PRESETS = 6;


	bool				bValidCameraFound;
    rs2::device_list    _deviceList;
    int             _curDeviceID;
    
    rs2::config     _config;
    rs2::pipeline   _pipeline;
    bool            _useThread;
    bool            _setupFinished;
    bool            _pipelineStarted;
    bool            _colorEnabled, _irEnabled, _depthEnabled, _pointcloudEnabled;
    int             _colorWidth, _irWidth, _depthWidth;
    int             _colorHeight, _irHeight, _depthHeight;
    
    uint8_t         *_colBuff, *_irBuff;
    uint16_t        *_depthBuff;
    ofTexture       _colTex, _irTex, _depthTex;
    bool            _hasNewColor, _hasNewIr, _hasNewDepth, _hasNewFrame;
    
    ofxGuiGroup     _D400Params;
    ofxToggle       _autoExposure;
    ofxToggle       _enableEmitter;
    ofxIntSlider    _irExposure;
	ofxToggle       _mapDepth;
	ofxToggle		_bUseDepthShader;
	ofxIntSlider    _nearLimit;
	ofxIntSlider    _farLimit;
    
    void threadedFunction();
    void updateFrameData();
    void setupGUI();
    void onD400BoolParamChanged(bool &value);
    void onD400IntParamChanged(int &value);

	//-----------------------
	//----------NEW----------
	//-----------------------

	double lastFrameTime = 0;
	
	float framerate;

	ofTexture mappedDepthTex;

	rs2::pointcloud pointcloud;

};
