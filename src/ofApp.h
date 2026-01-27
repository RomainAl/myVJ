#pragma once

#include "ofMain.h"
#include "ofxSyphon.h"
#include "ofxGui.h" // On ajoute l'addon GUI

class ofApp : public ofBaseApp {
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

    void serverAnnounced(ofxSyphonServerDirectoryEventArgs &arg);
    void serverUpdated(ofxSyphonServerDirectoryEventArgs &args);
    void serverRetired(ofxSyphonServerDirectoryEventArgs &arg);

    ofxSyphonServerDirectory dir;
    int dirIdx = -1;
    ofxSyphonClient syphonClient;
    ofxSyphonServer syphonServer;
    
    ofTexture texCopy;
    ofShader meshShader;
    ofVboMesh mainMesh;
    ofFbo renderFbo;
    ofShader diffShader;
    ofFbo motionFbo;
    ofFbo prevFbo;


    // --- Var ---
    string serverName;
    string appName;

    // --- Interface GUI ---
    ofxPanel gui;
    ofParameterGroup onoff;
    ofParameter<bool> showVideo;
    ofParameter<float> videoOpacity;
    ofParameter<float> smoothFactor;
    ofParameter<float> motionThreshold;
    ofParameterGroup points;
    ofParameter<bool> showMesh;
    ofParameter<float> extrusionAmount;
    ofParameter<float> pointSize;
    ofParameter<int> meshDensity;
    ofParameter<ofColor> colorTint;
    ofParameter<float> turbulence;
    ofParameter<float> turbulenceXY;
    ofParameter<float> stretchX;
    ofParameter<float> stretchY;
    ofParameter<float> zOffset;
    std::map<string, float> smoothedParams;
    ofXml settings;
    void onDensityChanged(int & val);
};