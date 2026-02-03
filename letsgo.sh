#!/bin/bash
# 1. On compile (on garde le silence -s pour la clarté)
make -j4 -s 2>&1

# 2. On crée le dossier et on copie le framework
mkdir -p bin/myVJ.app/Contents/Frameworks
cp -R ../../../addons/ofxSyphon/libs/Syphon/lib/osx/Syphon.framework bin/myVJ.app/Contents/Frameworks/

# 3. On lance l'app
make RunRelease