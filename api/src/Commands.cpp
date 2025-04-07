#include "Commands.h"
#include "FbxHandle.h"
#include <maya/MFnPlugin.h>
#include <maya/MGlobal.h>
#include <maya/MArgList.h>
#include <maya/MStringArray.h>
#include <string>
#include <vector>

using namespace std;

namespace cf {

class characterfactoryfbxhandle::Implementation {
public:
    vector<string> fbxFiles;
};

const char* characterfactoryfbxhandle::commandName = "characterfactoryfbxhandle";

characterfactoryfbxhandle::characterfactoryfbxhandle() : pImpl(new Implementation()) {}
characterfactoryfbxhandle::~characterfactoryfbxhandle() {
    delete pImpl;
}

void* characterfactoryfbxhandle::creator() {
    return new characterfactoryfbxhandle();
}

MStatus characterfactoryfbxhandle::doIt(const MArgList& args) {
    MStatus status = MS::kSuccess;
    if (args.length() < 2 || args.length() > 3) {
        MGlobal::displayError("Expected 2-3 arguments: fbx_files_list, output_path, [json_path]");
        return MS::kFailure;
    }

    MString fbxListStr;
    status = args.get(0, fbxListStr);
    if (!status) {
        MGlobal::displayError("Failed to get fbx files list");
        return status;
    }

    MStringArray fbxFiles;
    status = fbxListStr.split(';', fbxFiles);  
    if (!status) {
        MGlobal::displayError("Failed to split fbx files list");
        return status;
    }

    pImpl->fbxFiles.clear();
    for (unsigned int i = 0; i < fbxFiles.length(); ++i) {
        pImpl->fbxFiles.push_back(fbxFiles[i].asChar());
    }

    MString outputPath;
    status = args.get(1, outputPath);
    if (!status) {
        MGlobal::displayError("Failed to get output path argument");
        return status;
    }
    
    MString jsonPath;
    if (args.length() == 3) {
        status = args.get(2, jsonPath);
        if (!status) {
            MGlobal::displayError("Failed to get json path argument");
            return status;
        }
    }

    FbxModelHandle fbxHandle;
    fbxHandle.processMultipleFbxFiles(pImpl->fbxFiles, outputPath.asChar(), jsonPath.asChar());

    clearResult();
    setResult("FBX files processed successfully");
    return MS::kSuccess;
}

}

CHARACTER_FACTORY_EXPORT MStatus initializePlugin(MObject obj) {
    MStatus status;
    MFnPlugin plugin(obj, "CharacterFactory", "1.0", "Any");

    status = plugin.registerCommand(cf::characterfactoryfbxhandle::commandName,
                                  cf::characterfactoryfbxhandle::creator);
    if (!status) {
        status.perror("Failed to register command: characterfactoryfbxhandle");
        return status;
    }
    return status;
}

CHARACTER_FACTORY_EXPORT MStatus uninitializePlugin(MObject obj) {
    MStatus status;
    MFnPlugin plugin(obj);

    status = plugin.deregisterCommand(cf::characterfactoryfbxhandle::commandName);
    if (!status) {
        status.perror("Failed to deregister command: characterfactoryfbxhandle");
        return status;
    }
    return status;
}
