#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup() {
    ofSetBackgroundColor(0);
    ofSetFrameRate(60); // Aligne-toi sur la source standard (Resolume/HeavyM)
    // ofSetVerticalSync(true); // Évite les déchirures et stabilise le flux
    // ofSetLogLevel("ofxSyphonClient", OF_LOG_FATAL_ERROR);
    glEnable(GL_PROGRAM_POINT_SIZE); // Pour que gl_PointSize = uPointSize; marche dans le shader, sinon ça reste dans le context d'OF !
    ofAddListener(dir.events.serverAnnounced, this, &ofApp::serverAnnounced);
    ofAddListener(dir.events.serverRetired, this, &ofApp::serverRetired);
    dir.setup();
    syphonClient.setup();
    syphonServer.setName("OF Particle Cloud");

    meshShader.load("shaders/mesh");
    renderFbo.allocate(1920, 1080, GL_RGBA);
    diffShader.load("shaders/diff");
    motionFbo.allocate(1920, 1080, GL_RGBA);

    // --- Setup GUI ---
    gui.setup("SETTINGS");
    onoff.setName("Affichage");
    onoff.add(showVideo.set("Afficher Video", true));
    onoff.add(videoOpacity.set("Video Opacity", 1, 0, 1));
    onoff.add(showMesh.set("Afficher Mesh", true));
    onoff.add(showWireframe.set("Afficher Wireframe", true));
    onoff.add(showFaces.set("Afficher Faces", false));
    onoff.add(smoothFactor.set("Lissage Global", 0.01, 0.01, 0.5));
    onoff.add(motionThreshold.set("Seuil Mouvement", 0.0, 0.0, 1.0));
    onoff.add(delayFrames.set("Mosh Delay", 1, 1, 58));
    onoff.add(persistence.set("Persistence", 2, 1, 255));
    onoff.add(moshIntensity.set("Mosh Intensity", 0.0, 0.0, 1.0));
    onoff.add(blockSizeSpeed.set("blockSizeSpeed", 0.0, 0.0, 100.0));
    onoff.add(moshScale.set("moshScale", 8, 1, 10));
    onoff.add(uSaturation.set("Mosh Saturation", 1.0, 0.0, 3.0));
    onoff.add(uContrast.set("Mosh Contraste", 1.0, 0.0, 3.0));
    onoff.add(uBrightness.set("Mosh Brightness", 1.0, 0.0, 3.0));
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
    bool computeWire = (val <= 200);
    mainMesh.clear();
    mainMesh.setMode(OF_PRIMITIVE_POINTS);
    wireframeMesh.clear();
    wireframeMesh.setMode(OF_PRIMITIVE_TRIANGLES);

    // On garde le calcul de l'aspect pour savoir combien de points créer
    float aspect = texCopy.isAllocated() ? (texCopy.getWidth() / texCopy.getHeight()) : (16.0 / 9.0);
    int numY = val;
    int numX = (int)(val * aspect);

    for(int y=0; y<numY; y++) {
        for(int x=0; x<numX; x++) {
            // STRATÉGIE GPU : On envoie juste l'index x et y dans la position
            // On laisse z à 0. Les UVs servent à lire la texture.
            glm::vec3 pos(x, y, 0); 
            glm::vec2 uv((float)x / (numX - 1), (float)y / (numY - 1));

            mainMesh.addVertex(pos);
            mainMesh.addTexCoord(uv);

            wireframeMesh.addVertex(pos);
            wireframeMesh.addTexCoord(uv);
            if(computeWire) {
                if(x < numX - 1 && y < numY - 1) {
                    int i1 = x + y * numX;
                    int i2 = (x + 1) + y * numX;
                    int i3 = x + (y + 1) * numX;
                    int i4 = (x + 1) + (y + 1) * numX;
                    wireframeMesh.addIndex(i1); wireframeMesh.addIndex(i2); wireframeMesh.addIndex(i3);
                    wireframeMesh.addIndex(i2); wireframeMesh.addIndex(i4); wireframeMesh.addIndex(i3);
                }
            }
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
    float currentAspect = 16.0/9.0;
    
    if(texCopy.isAllocated() && texCopy.getHeight() > 0){
        currentAspect = texCopy.getWidth() / texCopy.getHeight();
    }
    
    // 1. Régénération du mesh si l'aspect change
    if(currentAspect != lastAspect) {
        int d = meshDensity;
        onDensityChanged(d); 
        lastAspect = currentAspect;
    }

    // 2. Infos Syphon pour le titre de la fenêtre
    if (syphonClient.isSetup()){
        serverName = syphonClient.getServerName();
        appName = syphonClient.getApplicationName();
    } else {
        serverName = ""; 
        appName = "";
    }
    string title = serverName + " : " + appName + " | FPS: " + ofToString(ofGetFrameRate(), 0);
    ofSetWindowTitle(title);

    // 3. Lissage des paramètres (Smoothing)
    vector<ofParameter<float>*> toSmooth = { 
        &extrusionAmount, &turbulence, &turbulenceXY, 
        &pointSize, &zOffset, &stretchX, &stretchY,
    };

    for(auto p : toSmooth) {
        string name = p->getName();
        if(smoothedParams.find(name) == smoothedParams.end()) {
            smoothedParams[name] = p->get();
        }
        smoothedParams[name] += (p->get() - smoothedParams[name]) * smoothFactor;
    }

    // 4. Gestion du Buffer Circulaire (Allocation et redimensionnement)
    if(texCopy.isAllocated()) {
        int w = texCopy.getWidth();
        int h = texCopy.getHeight();

        if(frameBuffer.empty() || frameBuffer[0].getWidth() != w || frameBuffer[0].getHeight() != h) {
            frameBuffer.clear();
            frameBuffer.resize(60); // 60 frames = 1 seconde de buffer à 60fps
            for(int i=0; i<60; i++) {
                frameBuffer[i].allocate(w, h, GL_RGBA);
                frameBuffer[i].begin(); 
                ofClear(0, 255); // CORRECTION WARNING : brightness, alpha
                frameBuffer[i].end();
            }
            writeIndex = 0; // On reset l'index si on change de taille
            ofLogNotice("Buffer") << "Allocated 60 frames at " << w << "x" << h;
        }
    }
}

//--------------------------------------------------------------
void ofApp::draw() {
    if(!syphonClient.isSetup()) return;

    syphonClient.lockTexture();
    if(syphonClient.getTexture().isAllocated()){
        texCopy = syphonClient.getTexture();
    }
    syphonClient.unlockTexture();
    if(!texCopy.isAllocated()) return;

    if(frameBuffer.empty() || !frameBuffer[0].isAllocated()) return;

    frameBuffer[writeIndex].begin();
        texCopy.draw(0, 0);
    frameBuffer[writeIndex].end();

    int d = ofClamp(delayFrames, 1, (int)frameBuffer.size() - 1);
    int readIndex = (writeIndex - d + (int)frameBuffer.size()) % (int)frameBuffer.size();
    writeIndex = (writeIndex + 1) % (int)frameBuffer.size();

    // --- PASSE 1 : CALCUL DU MOUVEMENT ---
    motionFbo.begin();
        if(persistence >= 255) {
            ofClear(0, 0, 0, 255);
        } else {
            ofEnableAlphaBlending();
            ofSetColor(0, 0, 0, persistence); 
            ofDrawRectangle(0, 0, motionFbo.getWidth(), motionFbo.getHeight());
            ofDisableAlphaBlending();
        }
        
        diffShader.begin();
            diffShader.setUniformTexture("tex0", texCopy, 0);
            diffShader.setUniformTexture("texPrev", frameBuffer[readIndex].getTexture(), 1);
            
            diffShader.setUniformTexture("texFeedback", motionFbo.getTexture(), 2);
            diffShader.setUniform1f("uThreshold", motionThreshold);
            diffShader.setUniform1f("uOpacity", videoOpacity);
            diffShader.setUniform1f("uBlockSizeSpeed", blockSizeSpeed);
            diffShader.setUniform1i("uMoshScale", moshScale);
            diffShader.setUniform1f("uMoshIntensity", moshIntensity);
            diffShader.setUniform1f("uTime", ofGetElapsedTimef());
            diffShader.setUniform1f("uBrightness", uBrightness);
            diffShader.setUniform1f("uContrast", uContrast);
            diffShader.setUniform1f("uSaturation", uSaturation);
            
            texCopy.draw(0, 0, motionFbo.getWidth(), motionFbo.getHeight());
        diffShader.end();
    motionFbo.end();

    // --- PASSE 2 : RENDU FINAL ---
    renderFbo.begin();
        ofClear(0, 0, 0, 255);

        float fboW = renderFbo.getWidth();
        float fboH = renderFbo.getHeight();
        float aspect = texCopy.getWidth() / texCopy.getHeight();

        ofEnableBlendMode(OF_BLENDMODE_ADD);
        ofPushMatrix();
            ofTranslate(fboW / 2.0, fboH / 2.0, smoothedParams["Recul (Z)"]);
            ofScale(fboH, fboH, fboH);

            if(showVideo){
                ofDisableDepthTest();
                ofSetColor(255);
                motionFbo.getTexture().draw(-aspect/2.0, -0.5, -0.01, aspect, 1.0);
            }

            if(showMesh){
                ofDisableDepthTest();
                meshShader.begin();
                    meshShader.setUniformTexture("tex0", motionFbo.getTexture(), 0);
                    meshShader.setUniform1f("meshDensity", (float)meshDensity);
                    meshShader.setUniform1f("extrusion", smoothedParams["Extrusion"]);
                    meshShader.setUniform1f("uPointSize", smoothedParams["Taille Points"]);
                    meshShader.setUniform1f("uTurbulence", smoothedParams["Turbulence"]);
                    meshShader.setUniform1f("uTurbulenceXY", smoothedParams["TurbulenceXY"]);
                    meshShader.setUniform1f("uStretchX", stretchX);
                    meshShader.setUniform1f("uStretchY", stretchY);
                    meshShader.setUniform1f("uTime", ofGetElapsedTimef());
                    meshShader.setUniform4f("uColorTint", colorTint->r/255.0, colorTint->g/255.0, colorTint->b/255.0, colorTint->a/255.0);
                    mainMesh.draw();

                    if(showFaces){
                        meshShader.setUniform1f("uAlphaMult", 0.2);
                        wireframeMesh.drawFaces(); 
                    }

                    if(showWireframe){
                        meshShader.setUniform1f("uAlphaMult", 0.5);
                        wireframeMesh.drawWireframe();
                    }
                    
                meshShader.end();
            }
        ofPopMatrix();
        ofDisableDepthTest();
        ofDisableAlphaBlending();
    renderFbo.end();
    renderFbo.draw(0, 0, ofGetWidth(), ofGetHeight());
    syphonServer.publishTexture(&renderFbo.getTexture());

    gui.draw();
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){
    if(key == 's'){
        diffShader.load("shaders/diff");
        meshShader.load("shaders/mesh");
        ofLogNotice() << "Shaders rechargés !";
    } else if (key == OF_KEY_SPACE){
        if (dir.size() > 0)
        {
            dirIdx++;
            if(dirIdx > dir.size() - 1)
                dirIdx = 0;

            syphonClient.set(dir.getDescription(dirIdx));
        }
    }
}
//--------------------------------------------------------------
void ofApp::keyReleased(int key){}

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
