#include "Commands.h"
#include "AutoRigCreate.h"
#include <maya/MGlobal.h>
#include <maya/MSelectionList.h>
#include <maya/MFileIO.h>
#include <maya/MFnDependencyNode.h>
#include <string>
#include <vector>
#include <array>
#include <map>
#include <algorithm>

using namespace std;

namespace cf {

FbxManager* characterfactoryautorigcreate::mGlobalFbxManager = nullptr;
FbxScene* characterfactoryautorigcreate::mGlobalFbxScene = nullptr;

class characterfactoryautorigcreate::Implementation {
public:
    Implementation() {}
    ~Implementation() {}

    MString FbxExporter() {
        MStatus status;

        vector<string> meshNameList = {
            "head_lod0_mesh", 
            "teeth_lod0_mesh",
            "saliva_lod0_mesh", 
            "eyeRight_lod0_mesh", 
            "eyeLeft_lod0_mesh", 
            "eyeshell_lod0_mesh", 
            "eyelashes_lod0_mesh", 
            "eyeEdge_lod0_mesh", 
            "cartilage_lod0_mesh"
        };
        
        MSelectionList selectionList;
        
        int selectedCount = 0;
        for (const auto& meshName : meshNameList) {
            status = selectionList.add(meshName.c_str());
            if (status) {
                selectedCount++;
                MGlobal::displayInfo(MString("Successfully added model to selection: ") + meshName.c_str());
            } else {
                MGlobal::displayWarning(MString("Failed to add model to selection: ") + meshName.c_str());
            }
        }
        
        status = MGlobal::setActiveSelectionList(selectionList);
        if (!status) {
            MGlobal::displayError("Failed to set active selection");
            return MString("Failed to select models");
        }
        
        if (selectedCount > 0) {
            MGlobal::displayInfo(MString("Successfully selected ") + selectedCount + " models");
            
            MString projectDir;
            MGlobal::executeCommand("workspace -q -rd", projectDir);
            
            MString exportPath = projectDir + "scenes/character_export.fbx";
            MString exportCommand = "FBXExport -f \"" + exportPath + "\" -s";
            
            status = MGlobal::executeCommand(exportCommand);
            if (status) {
                MGlobal::displayInfo(MString("Successfully exported FBX to: ") + exportPath);
                return exportPath;
            } else {
                MGlobal::displayError(MString("Failed to export FBX to: ") + exportPath);
                return MString("Failed to export FBX");
            }
        } else {
            MGlobal::displayWarning("No models found, cannot export FBX");
            return MString("No models found");
        }
    }
};

characterfactoryautorigcreate::characterfactoryautorigcreate() : pImpl(new Implementation()) {}

characterfactoryautorigcreate::~characterfactoryautorigcreate() {
    delete pImpl;
}

void* characterfactoryautorigcreate::creator() {
    return new characterfactoryautorigcreate();
}

MString characterfactoryautorigcreate::FbxExporter() {
    return pImpl->FbxExporter();
}

bool characterfactoryautorigcreate::CreateFbxScene(const char* fbxPath) {
    if (!mGlobalFbxManager) {
        mGlobalFbxManager = FbxManager::Create();
        if (!mGlobalFbxManager) {
            MGlobal::displayError("Failed to create global FBX Manager");
            return false;
        }

        FbxIOSettings* ios = FbxIOSettings::Create(mGlobalFbxManager, IOSROOT);
        mGlobalFbxManager->SetIOSettings(ios);
    }

    if (mGlobalFbxScene) {
        mGlobalFbxScene->Destroy();
        mGlobalFbxScene = nullptr;
    }

    mGlobalFbxScene = FbxScene::Create(mGlobalFbxManager, "");
    if (!mGlobalFbxScene) {
        MGlobal::displayError("Failed to create global FBX Scene");
        return false;
    }

    if (fbxPath && strlen(fbxPath) > 0) {
        FbxImporter* importer = FbxImporter::Create(mGlobalFbxManager, "");
        bool importStatus = importer->Initialize(fbxPath, -1, mGlobalFbxManager->GetIOSettings());

        if (!importStatus) {
            MGlobal::displayError(MString("Failed to initialize importer for: ") + fbxPath);
            importer->Destroy();
            return false;
        }

        importStatus = importer->Import(mGlobalFbxScene);
        importer->Destroy();

        if (!importStatus) {
            MGlobal::displayError(MString("Failed to import FBX file: ") + fbxPath);
            return false;
        }
    }

    MGlobal::displayInfo("Successfully created global FBX scene");
    return true;
}

FbxScene* characterfactoryautorigcreate::GetGlobalFbxScene() {
    return mGlobalFbxScene;
}

MStatus characterfactoryautorigcreate::doIt(const MArgList& args) {
    MStatus status = MS::kSuccess;

    MGlobal::displayInfo("Executing characterfactoryautorigcreate command");
    
    MString exportPath = FbxExporter();
    
    if (!exportPath.length() || exportPath == "Failed to export FBX" || exportPath == "No models found") {
        MGlobal::displayWarning("Failed to export FBX or no models found, cannot create FBX scene");
    } else {
        const char* fbxPathCStr = exportPath.asChar();
        
        if (CreateFbxScene(fbxPathCStr)) {
            MGlobal::displayInfo("Successfully created global FBX scene from exported file");
            
            if (mGlobalFbxScene) {
                int nodeCount = mGlobalFbxScene->GetNodeCount();
                MGlobal::displayInfo(MString("Global FBX scene contains ") + nodeCount + " nodes");
            }
        } else {
            MGlobal::displayError("Failed to create global FBX scene");
        }
    }
    
    MPxCommand::clearResult();
    MPxCommand::setResult(exportPath);
    
    return status;
}

}