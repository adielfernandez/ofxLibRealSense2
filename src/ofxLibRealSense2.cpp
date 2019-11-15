//
//  ofxLibRealSense2.cpp
//  example
//
//  Created by shiyamon on 2018/05/25.
//

#include "ofxLibRealSense2.hpp"
#include "ofSystemUtils.h"
#include "ofLog.h"

using namespace::std;
using namespace::rs2;

int ofxLibRealSense2::getDeviceCount() {
	// query device
	rs2::context ctx;
	return ctx.query_devices().size();
}

void ofxLibRealSense2::setupDevice(int deviceID)
{
    // query device 
    rs2::context ctx;
    _deviceList = ctx.query_devices();
    cout << "RealSense device count: " << _deviceList.size() << endl;
    
    if(_deviceList.size() <= 0) {
        ofSystemAlertDialog("RealSense device not found!");
        std::exit(0);
    }
    if (deviceID >= _deviceList.size()) {
        ofSystemAlertDialog("Requested device id is invalid");
        std::exit(0);
    }

	cout << "Devices List:\n-------------" << endl;
	for (int i = 0; i < _deviceList.size(); i++) {
		cout << i << ": " << _deviceList[i].get_info(RS2_CAMERA_INFO_NAME) << endl;
	}
    
    string deviceSerial = _deviceList[deviceID].get_info(RS2_CAMERA_INFO_SERIAL_NUMBER);
	string name = _deviceList[deviceID].get_info(RS2_CAMERA_INFO_NAME);

    _curDeviceID = deviceID;
	cout << "Attempting to connect to device: \n\t---ID: " << deviceID << "\n\t---Serial: " << deviceSerial << "\n\t---Name: " << name << endl;
    
	_config.enable_device(deviceSerial);

    _setupFinished = true;
    
    setupGUI();


}


void ofxLibRealSense2::setupColor(int width, int height, int fps)
{
    _colorWidth = width;
    _colorHeight = height;
    _colTex.allocate(_colorWidth, _colorHeight, GL_RGB);
    _config.enable_stream(RS2_STREAM_COLOR, -1, _colorWidth, _colorHeight, RS2_FORMAT_RGB8, fps);
    _colorEnabled = true;
}


void ofxLibRealSense2::setupIR(int width, int height, int fps)
{
    _irWidth = width;
    _irHeight = height;
    _irTex.allocate(_irWidth, _irHeight, GL_LUMINANCE);
    _config.enable_stream(RS2_STREAM_INFRARED, -1, _irWidth, _irHeight, RS2_FORMAT_Y8, fps);
    _irEnabled = true;
}


void ofxLibRealSense2::setupDepth(int width, int height, int fps)
{
    _depthWidth = width;
    _depthHeight = height;
    _depthTex.allocate(_depthWidth, _depthHeight, GL_LUMINANCE16);
    _config.enable_stream(RS2_STREAM_DEPTH, -1, _depthWidth, _depthHeight, RS2_FORMAT_Z16, fps);
    _depthEnabled = true;

	mappedDepthPix.allocate(_depthWidth, _depthHeight, OF_IMAGE_GRAYSCALE);

	//go 5 levels up from data folder, back down to addon folder
	string addonPath = ofFilePath::getAbsolutePath("../../../../../addons/ofxLibRealSense2/shaders/");
	string shaderName = "map";
	mapShader.load( addonPath + shaderName );

	mapFbo.allocate(_depthWidth, _depthHeight, GL_LUMINANCE);

	mapFbo.begin();
	ofClear(0);
	mapFbo.end();

}

void ofxLibRealSense2::setupPointcloud() {

	mesh.setMode(OF_PRIMITIVE_POINTS);
	
	_pointcloudEnabled = true;

}


void ofxLibRealSense2::startPipeline(bool useThread, Preset p )
{
    _pipeline.start(_config);
    _pipelineStarted=true;
	
	_preset = p;
	setSensorPreset( _preset );
    
    _useThread = useThread;
    if(_useThread)
        startThread();
}

void ofxLibRealSense2::setSensorPreset(Preset p) {

	rs2_rs400_visual_preset preset;

	switch (p) {
	case ofxLibRealSense2::Preset::DEFAULT:
		preset = rs2_rs400_visual_preset::RS2_RS400_VISUAL_PRESET_DEFAULT;
		break;
	case ofxLibRealSense2::Preset::HIGH_ACCURACY:
	preset = rs2_rs400_visual_preset::RS2_RS400_VISUAL_PRESET_HIGH_ACCURACY;
		break;
	case ofxLibRealSense2::Preset::HIGH_DENSITY:
	preset = rs2_rs400_visual_preset::RS2_RS400_VISUAL_PRESET_HIGH_DENSITY;
		break;
	case ofxLibRealSense2::Preset::MEDIUM_DENSITY:
	preset = rs2_rs400_visual_preset::RS2_RS400_VISUAL_PRESET_MEDIUM_DENSITY;
		break;
	case ofxLibRealSense2::Preset::HAND:
	preset = rs2_rs400_visual_preset::RS2_RS400_VISUAL_PRESET_HAND;
		break;
	case ofxLibRealSense2::Preset::REMOVE_IR_PATTERN:
	preset = rs2_rs400_visual_preset::RS2_RS400_VISUAL_PRESET_REMOVE_IR_PATTERN;
		break;
	default:
		break;
	}

	auto sensor = _pipeline.get_active_profile().get_device().first<rs2::depth_sensor>();
	sensor.set_option(rs2_option::RS2_OPTION_VISUAL_PRESET, preset);

	_preset = p;

}

void ofxLibRealSense2::setSensorPreset( int i ) {
	if( i >= 0 && i <= 5 ) {
		setSensorPreset( (Preset)i );
	} else {
		return;
		cout << "Not a valid preset" << endl;
	}
}

void ofxLibRealSense2::setNextPreset() {
	if ((int)_preset >= NUM_PRESETS - 1) {
		setSensorPreset( 0 );
	} else {
		setSensorPreset(_preset + 1);
	}
}

void ofxLibRealSense2::setPreviousPreset() {
	if ((int)_preset <= 0) {
		setSensorPreset( NUM_PRESETS - 1 );
	} else {
		setSensorPreset(_preset - 1);
	}
}

string ofxLibRealSense2::getCurrentPresetName() {
	string s;

	switch (_preset) {
	case ofxLibRealSense2::DEFAULT:
		s = "DEFAULT";
		break;
	case ofxLibRealSense2::HIGH_ACCURACY:
		s = "HIGH_ACCURACY"; 
		break;
	case ofxLibRealSense2::HIGH_DENSITY:
		s = "HIGH_DENSITY"; 
		break;
	case ofxLibRealSense2::MEDIUM_DENSITY:
		s = "MEDIUM_DENSITY"; 
		break;
	case ofxLibRealSense2::HAND:
		s = "HAND"; 
		break;
	case ofxLibRealSense2::REMOVE_IR_PATTERN:
		s = "REMOVE_IR_PATTERN"; 
		break;
	default:
		break;
	}

	return s;

}

void ofxLibRealSense2::threadedFunction()
{
    while(isThreadRunning()) {
        
        if(lock()) {
            updateFrameData();
            unlock();
        }
    }
}


void ofxLibRealSense2::updateFrameData()
{
    rs2::frameset frameset;
    if(_pipeline.poll_for_frames(&frameset)) {
        
		rs2::video_frame colFrame = frameset.get_color_frame();
        rs2::video_frame irFrame = frameset.get_infrared_frame();
        rs2::depth_frame depthFrame = frameset.get_depth_frame();
        
		if(_colorEnabled) {
            _colBuff = (uint8_t*)colFrame.get_data();
            _colorWidth = colFrame.get_width();
            _colorHeight = colFrame.get_height();
            _hasNewColor = true;
        }
        if(_irEnabled) {
            _irBuff = (uint8_t*)irFrame.get_data();
            _irWidth = irFrame.get_width();
            _irHeight = irFrame.get_height();
            _hasNewIr = true;
        }
        if(_depthEnabled) {
            _depthBuff = (uint16_t*)depthFrame.get_data();
            _depthWidth = depthFrame.get_width();
            _depthHeight = depthFrame.get_height();
            _hasNewDepth = true;
        }
		
		if (_pointcloudEnabled && _colorEnabled && _depthEnabled) {

			auto points = pointcloud.calculate(depthFrame);

			// Tell pointcloud object to map to this color frame
			pointcloud.map_to(colFrame);

			auto vertices = points.get_vertices();
			auto tex_coords = points.get_texture_coordinates();

			mesh.clear();

			for (int i = 0; i < points.size(); i++) {
				if (vertices[i].z) {

					ofVec3f p(vertices[i].x, vertices[i].y, vertices[i].z);
					ofVec2f t(tex_coords[i].u, tex_coords[i].v);

					mesh.addVertex( p );
					mesh.addTexCoord( t );

				}
			}

			ofNotifyEvent(newDepthEvent, this);
		}

		//do a little averaging for a more readable framerate
		double frameTime = ofGetElapsedTimef();
		double dt = frameTime - lastFrameTime;
		framerate = 1.0f/dt;
		lastFrameTime = frameTime;
    }
}


void ofxLibRealSense2::update()
{
    if(!_pipelineStarted) return;
    
    rs2::frameset frameset;
    if( !_useThread ) {
        updateFrameData();
    }
    
    _hasNewFrame = _hasNewColor || _hasNewIr || _hasNewDepth;

    if(_depthBuff && _hasNewDepth) {

		rawDepthPix.setFromPixels(_depthBuff, _depthWidth, _depthHeight, OF_IMAGE_GRAYSCALE);
		_depthTex.loadData(rawDepthPix);


		if (_mapDepth) {

			if ( !_bUseDepthShader) {

				//Map the short pixel data to an 8-bit gray scale.
				//SLOW!!! Move this to a shader
				for (int i = 0; i < _depthWidth * _depthHeight; i++) {

					if (rawDepthPix[i] == 0) {
						mappedDepthPix[i] = 0;
					} else {
						mappedDepthPix[i] = ofMap(rawDepthPix[i], _nearLimit, _farLimit, 255.0f, 0.0f, true);
					}

				}
				
				mappedDepthTex.loadData(mappedDepthPix);

			} else {

				//use shader to get mapped depth
				mapFbo.begin();

				ofClear(255);

				mapShader.begin();
				mapShader.setUniformTexture("depthTex", _depthTex, 0);
				mapShader.setUniform2f("resolution", mapFbo.getWidth(), mapFbo.getHeight());

				//the values of the raw depth are uint16_t (unsigned short), but appear as normalized 0-1 values
				//in the fragment shader, so convert the limits to normalized values too
				float nearLim = ofMap(_nearLimit, 0, USHRT_MAX, 0.0f, 1.0f);
				float farLim = ofMap(_farLimit, 0, USHRT_MAX, 0.0f, 1.0f);

				mapShader.setUniform1f("nearLimit", nearLim);
				mapShader.setUniform1f("farLimit", farLim);

				ofPushStyle();
				ofSetColor(0);
				ofFill();
				ofDrawRectangle(0, 0, mapFbo.getWidth(), mapFbo.getHeight());
				ofPopStyle();

				//_depthTex.draw(0,0);

				mapShader.end();

				mapFbo.end();



				mappedDepthTex = mapFbo.getTexture();
				mapFbo.readToPixels(mappedDepthPix);

			}


			//notify listeners of new data
			ofNotifyEvent(newMappedPixelsEvt, mappedDepthPix, this);

		}
        _hasNewDepth = false;
    }

    if(_irBuff && _hasNewIr) {
        _irTex.loadData(_irBuff, _irWidth, _irHeight, GL_LUMINANCE);
        _hasNewIr = false;
    }

    if(_colBuff && _hasNewColor) {
        _colTex.loadData(_colBuff, _colorWidth, _colorHeight, GL_RGB);
        _hasNewColor = false;
    }
}


void ofxLibRealSense2::setupGUI()
{
    rs2::sensor sensor = _deviceList[_curDeviceID].query_sensors()[0];
    rs2::option_range orExp = sensor.get_option_range(RS2_OPTION_EXPOSURE);
    rs2::option_range orGain = sensor.get_option_range(RS2_OPTION_GAIN);

    _D400Params.setup("D400");
    _D400Params.add( _autoExposure.setup("Auto exposure", true) );
    _D400Params.add( _enableEmitter.setup("Emitter", true) );
    _D400Params.add( _irExposure.setup("IR Exposure", orExp.def, orExp.min, orExp.max ));
	_D400Params.add( _mapDepth.setup("Map Depth Image", true)); 
	_D400Params.add( _bUseDepthShader.setup("Use Depth Shader", true));
	_D400Params.add( _nearLimit.setup("Near Limit", 350, 0, 8000));
	_D400Params.add( _farLimit.setup("Far Limit", 1000, 0, 8000));
    
    _autoExposure.addListener(this, &ofxLibRealSense2::onD400BoolParamChanged);
    _enableEmitter.addListener(this, &ofxLibRealSense2::onD400BoolParamChanged);
    _irExposure.addListener(this, &ofxLibRealSense2::onD400IntParamChanged);
}


void ofxLibRealSense2::onD400BoolParamChanged(bool &value)
{
    if(!_pipelineStarted) return;
    rs2::sensor sensor = _pipeline.get_active_profile().get_device().first<rs2::depth_sensor>();
    if(sensor.supports(RS2_OPTION_ENABLE_AUTO_EXPOSURE))
        sensor.set_option(RS2_OPTION_ENABLE_AUTO_EXPOSURE, _autoExposure?1.0f:0.0f);
    if(sensor.supports(RS2_OPTION_EMITTER_ENABLED))
        sensor.set_option(RS2_OPTION_EMITTER_ENABLED, _enableEmitter?1.0f:0.0f);
}


void ofxLibRealSense2::onD400IntParamChanged(int &value)
{
    if(!_pipelineStarted) return;
    rs2::sensor sensor = _pipeline.get_active_profile().get_device().first<rs2::depth_sensor>();
    if(sensor.supports(RS2_OPTION_EXPOSURE))
        sensor.set_option(RS2_OPTION_EXPOSURE, (float)_irExposure);
}


ofxGuiGroup* ofxLibRealSense2::getGui()
{
    return &_D400Params;
}


void ofxLibRealSense2::exit()
{
    waitForThread();
    stopThread();
//    _pipeline.stop();
}
