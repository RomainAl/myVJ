#include "ofMain.h"
#include "ofApp.h"

//========================================================================
int main(){
    ofGLWindowSettings settings;
    settings.setGLVersion(3, 2); 
    settings.setSize(1280, 720);
    settings.windowMode = OF_WINDOW; //  OF_FULLSCREEN pour le live
    ofCreateWindow(settings);
    ofRunApp(new ofApp());
}