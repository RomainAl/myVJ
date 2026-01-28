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
    onoff.add(persistence.set("Persistence", 2, 1, 255));
    onoff.add(moshIntensity.set("Mosh Intensity", 0.0, 0.0, 1.0));
    onoff.add(blockSizeSpeed.set("blockSizeSpeed", 0.0, 0.0, 100.0));
    onoff.add(moshScale.set("moshScale", 8, 1, 9));
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
    wireframeMesh.clear();
    wireframeMesh.setMode(OF_PRIMITIVE_TRIANGLES);

    float aspect = texCopy.isAllocated() ? (texCopy.getWidth() / texCopy.getHeight()) : (16.0 / 9.0);
    int numY = val;
    int numX = (int)(val * aspect);

    for(int y=0; y<numY; y++) {
        for(int x=0; x<numX; x++) {
            float posX = ((float)x / (numX - 1) - 0.5) * aspect;
            float posY = ((float)y / (numY - 1) - 0.5);
            glm::vec3 pos(posX, posY, 0);
            glm::vec2 uv((float)x / (numX - 1), (float)y / (numY - 1));

            mainMesh.addVertex(pos);
            mainMesh.addTexCoord(uv);

            wireframeMesh.addVertex(pos);
            wireframeMesh.addTexCoord(uv);

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
        &pointSize, &zOffset, &stretchX, &stretchY,
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
    if(!syphonClient.isSetup()) return;

    // 1. Récupération Syphon standard
    syphonClient.lockTexture();
    if(syphonClient.getTexture().isAllocated()){
        texCopy = syphonClient.getTexture();
    }
    syphonClient.unlockTexture();
    if(!texCopy.isAllocated()) return;

    // 2. Initialisation du FBO de mémoire si besoin
    if(!prevFbo.isAllocated()){
        prevFbo.allocate(texCopy.getWidth(), texCopy.getHeight(), GL_RGBA);
        prevFbo.begin(); ofClear(0, 255); prevFbo.end();
    }

    // --- PASSE 1 : CALCUL DU MOUVEMENT ---
    motionFbo.begin();
        if(persistence >= 255) {
            ofClear(0, 255);
        } else {
            ofEnableAlphaBlending();
            ofSetColor(0, 0, 0, persistence); 
            ofDrawRectangle(0, 0, motionFbo.getWidth(), motionFbo.getHeight());
            ofDisableAlphaBlending();
        }
        ofEnableBlendMode(OF_BLENDMODE_ADD);
        diffShader.begin();
            diffShader.setUniformTexture("tex0", texCopy, 0);
            diffShader.setUniformTexture("texPrev", prevFbo.getTexture(), 1);
            diffShader.setUniformTexture("texFeedback", motionFbo.getTexture(), 2);
            diffShader.setUniform1f("uThreshold", motionThreshold);
            diffShader.setUniform1f("uOpacity", videoOpacity);
            diffShader.setUniform1f("uBlockSizeSpeed", blockSizeSpeed);
            diffShader.setUniform1i("uMoshScale", moshScale);
            diffShader.setUniform1f("uMoshIntensity", moshIntensity);
            diffShader.setUniform1f("uTime", ofGetElapsedTimef());
            texCopy.draw(0, 0, motionFbo.getWidth(), motionFbo.getHeight());
        diffShader.end();
        ofDisableBlendMode();
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
                    // On passe le motionFbo comme texture principale !
                    meshShader.setUniformTexture("tex0", motionFbo.getTexture(), 0);
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
                        wireframeMesh.setMode(OF_PRIMITIVE_TRIANGLES);
                        wireframeMesh.drawFaces(); 
                    }

                    // 3. LE WIREFRAME (Les arrêtes)
                    if(showWireframe){
                        meshShader.setUniform1f("uAlphaMult", 0.6);
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

    prevFbo.begin();
        ofClear(0, 255);
        texCopy.draw(0, 0);
    prevFbo.end();


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
