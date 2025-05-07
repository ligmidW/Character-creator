#pragma once

#include <maya/MPxCommand.h>
#include <maya/MStatus.h>
#include <maya/MArgList.h>
#include <maya/MGlobal.h>
#include <maya/MStringArray.h>
#include <maya/MSelectionList.h>
#include <maya/MFileIO.h>
#include <maya/MFnDependencyNode.h>
#include <string>
#include <vector>
#include <array>
#include <map>
#include <algorithm>
#include <fbxsdk.h>

using namespace std;

namespace cf {

// 前向声明
class CHARACTER_FACTORY_EXPORT characterfactoryautorigcreate : public MPxCommand {
public:
    characterfactoryautorigcreate();
    virtual ~characterfactoryautorigcreate();

    static void* creator();
    MStatus doIt(const MArgList& args) override;
    virtual bool isUndoable() const override { return false; }
    
    // FBX导出方法
    MString FbxExporter();
    
    // 创建FBX场景方法
    bool CreateFbxScene(const char* fbxPath);
    
    // 获取全局FBX场景
    static FbxScene* GetGlobalFbxScene();

    static const char* commandName;
    
private:
    class Implementation;
    Implementation* pImpl;
    
    // 全局FBX管理器和场景
    static FbxManager* mGlobalFbxManager;
    static FbxScene* mGlobalFbxScene;
};
    
}  // namespace cf
