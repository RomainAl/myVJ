#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup() {
    ofSetBackgroundColor(0);
    // ofSetLogLevel("ofxSyphonClient", OF_LOG_FATAL_ERROR);
    glEnable(GL_PROGRAM_POINT_SIZE); // Pour que gl_PointSize = uPointSize; marche dans le shader, sinon ça reste dans le context d'OF !
    ofAddListener(dir.events.serverAnnounced, this, &ofApp::serverAnnounced);
    ofAddListener(dir.events.serverRetired, this, &ofApp::serverRetired);
    dir.setup();
    syphonClient.setup();
    syphonServer.setName("OF Particle Cloud");

    meshShader.load("shaders/mesh");
    renderFbo.allocate(1920, 1080, GL_RGBA);

    // --- Setup GUI ---
    gui.setup("Parametres Cloud");
    onoff.setName("Affichage");
    onoff.add(showVideo.set("Afficher Video", true));
    onoff.add(videoOpacity.set("Video Opacity", 255, 0, 255));
    onoff.add(showMesh.set("Afficher Mesh", true));
    onoff.add(smoothFactor.set("Lissage Global", 0.01, 0.01, 0.5));
    onoff.add(feedback.set("Feedback", 255, 0, 255));
    points.setName("Points");
    points.add(extrusionAmount.set("Extrusion", 0.0, 0.0, 10.0));
    points.add(pointSize.set("Taille Points", 2.0, 0.5, 50.0));
    points.add(meshDensity.set("Densite", 200, 10, 500));
    points.add(turbulence.set("Turbulence", 0, 0, 10));
    points.add(turbulenceXY.set("TurbulenceXY", 0, 0, 10));
    points.add(stretchX.set("Etirement X", 1.0, 0.1, 1.0));
    points.add(stretchY.set("Etirement Y", 1.0, 0.1, 1.0));
    points.add(colorTint.set("Teinte", ofColor(255), ofColor(0,0), ofColor(255,255)));
    points.add(zOffset.set("Recul (Z)", 0, -1000, 1000));
    gui.add(onoff);
    gui.add(points);
    gui.setHeaderBackgroundColor(ofColor(0,0,0));
    gui.getGroup("Affichage").setHeaderBackgroundColor(ofColor(40, 0, 0));
    gui.getGroup("Points").setHeaderBackgroundColor(ofColor(0, 40, 0));
    gui.loadFromFile("settings.xml");

    int d = meshDensity;
    onDensityChanged(d);
    meshDensity.addListener(this, &ofApp::onDensityChanged);

    // --- Init Var ---
    serverName = ""; 
    appName = "";
}

void ofApp::onDensityChanged(int & val) {
    mainMesh.clear();
    mainMesh.setMode(OF_PRIMITIVE_POINTS);
    
    float aspect = texCopy.isAllocated() ? (texCopy.getWidth() / texCopy.getHeight()) : (16.0 / 9.0);
    
    int numY = val;
    int numX = (int)(val * aspect);

    for(int y=0; y<numY; y++) {
        for(int x=0; x<numX; x++) {
            // POSITION : On crée un rectangle qui va de -0.88 à +0.88 sur X
            // et de -0.5 à +0.5 sur Y. Taille totale = 1.0 de haut.
            float posX = ((float)x / (numX - 1) - 0.5) * aspect;
            float posY = ((float)y / (numY - 1) - 0.5);
            mainMesh.addVertex(glm::vec3(posX, posY, 0));
            
            // TEXTURE : On reste simple, 0.0 à 1.0
            float tx = (float)x / (numX - 1);
            float ty = (float)y / (numY - 1); // On gérera l'inversion dans le shader !
            mainMesh.addTexCoord(glm::vec2(tx, ty));
        }
    }
}

void ofApp::serverAnnounced(ofxSyphonServerDirectoryEventArgs &arg){
    for( auto& dir : arg.servers ){
        ofLogNotice("ofxSyphonServerDirectory Server Announced")<<" Server Name: "<<dir.serverName <<" | App Name: "<<dir.appName;
    }
    dirIdx = 0;
}

void ofApp::serverUpdated(ofxSyphonServerDirectoryEventArgs &arg){
    for( auto& dir : arg.servers ){
        ofLogNotice("ofxSyphonServerDirectory Server Updated")<<" Server Name: "<<dir.serverName <<" | App Name: "<<dir.appName;
    }
    dirIdx = 0;
}

void ofApp::serverRetired(ofxSyphonServerDirectoryEventArgs &arg){
    for( auto& dir : arg.servers ){
        ofLogNotice("ofxSyphonServerDirectory Server Retired")<<" Server Name: "<<dir.serverName <<" | App Name: "<<dir.appName;
    }
    dirIdx = 0;
}

//--------------------------------------------------------------
void ofApp::update() {
    static float lastAspect = 0;
    float currentAspect = texCopy.getWidth() / texCopy.getHeight();
    
    if(currentAspect != lastAspect) {
        int d = meshDensity;
        onDensityChanged(d); // On force la régénération du mesh
        lastAspect = currentAspect;
    }
    if (syphonClient.isSetup()){
        serverName = syphonClient.getServerName();
        appName = syphonClient.getApplicationName();
    } else {
        serverName = ""; 
        appName = "";
    }
    std::string title = serverName + " : " + appName + " | FPS: " + ofToString(ofGetFrameRate(), 0);
    ofSetWindowTitle(title);

    vector<ofParameter<float>*> toSmooth = { 
        &extrusionAmount, &turbulence, &turbulenceXY, 
        &pointSize, &zOffset, &stretchX, &stretchY 
    };

    for(auto p : toSmooth) {
        string name = p->getName();
        // Si le paramètre n'existe pas encore dans la map, on l'initialise
        if(smoothedParams.find(name) == smoothedParams.end()) {
            smoothedParams[name] = p->get();
        }
        // Calcul du Lerp : ValeurCourante += (Cible - ValeurCourante) * vitesse
        smoothedParams[name] += (p->get() - smoothedParams[name]) * smoothFactor;
    }
}

//--------------------------------------------------------------
void ofApp::draw() {
    // 1. On "réveille" Syphon sans rien dessiner
    if(syphonClient.isSetup()){
        syphonClient.lockTexture();
        if(syphonClient.getTexture().isAllocated()){
            texCopy = syphonClient.getTexture(); // On récupère l'image
        }
        syphonClient.unlockTexture();
    }

    renderFbo.begin();
    ofClear(0, 0, 0, 255);
    // ofEnableAlphaBlending();
    // ofSetColor(0, 0, 0, feedback); 
    // ofDrawRectangle(0, 0, renderFbo.getWidth(), renderFbo.getHeight());

    if(texCopy.isAllocated()){
        ofEnableDepthTest();
        ofEnableAlphaBlending();

        // 1. On utilise les dimensions du FBO (1920x1080)
        float fboW = renderFbo.getWidth();
        float fboH = renderFbo.getHeight();
        float aspect = texCopy.getWidth() / texCopy.getHeight();

        ofPushMatrix();
            ofTranslate(fboW / 2.0, fboH / 2.0, zOffset);
            ofScale(fboH, fboH, fboH);

            if(showVideo){
                ofSetColor(255, 255, 255, videoOpacity);
                texCopy.draw(-aspect/2.0, -0.5, -0.01, aspect, 1.0);
            }
            if(showMesh){
                meshShader.begin();
                    meshShader.setUniformTexture("tex0", texCopy, 0);
                    meshShader.setUniform1f("extrusion", smoothedParams["Extrusion"]);
                    meshShader.setUniform1f("uPointSize", smoothedParams["Taille Points"]);
                    meshShader.setUniform1f("uTurbulence", smoothedParams["Turbulence"]);
                    meshShader.setUniform1f("uTurbulenceXY", smoothedParams["TurbulenceXY"]);
                    meshShader.setUniform1f("uStretchX", smoothedParams["Etirement X"]);
                    meshShader.setUniform1f("uStretchY", smoothedParams["Etirement Y"]);
                    meshShader.setUniform1f("uTime", ofGetElapsedTimef());
                    meshShader.setUniform4f("uColorTint", colorTint->r/255.0, colorTint->g/255.0, colorTint->b/255.0, colorTint->a/255.0);
                    mainMesh.draw();
                meshShader.end();
            }
        ofPopMatrix();
        ofDisableDepthTest();
    }
    renderFbo.end();

    ofSetColor(255);
    renderFbo.draw(0, 0, ofGetWidth(), ofGetHeight());
    syphonServer.publishTexture(&renderFbo.getTexture());

    gui.draw();
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){

}

//--------------------------------------------------------------
void ofApp::keyReleased(int key){
    //press any key to move through all available Syphon servers
    if (dir.size() > 0)
    {
        dirIdx++;
        if(dirIdx > dir.size() - 1)
            dirIdx = 0;

        syphonClient.set(dir.getDescription(dirIdx));
    }
}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y ){}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button){}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button){}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button){}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h){}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg){}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo){}
