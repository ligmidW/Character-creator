#pragma once

#include <maya/MPxCommand.h>
#include <maya/MString.h>
#include <maya/MArgList.h>
#include <string>
#include <vector>

#ifdef _WIN32
    #ifdef CharacterFactory_EXPORTS
        #define CHARACTER_FACTORY_EXPORT __declspec(dllexport)
    #else
        #define CHARACTER_FACTORY_EXPORT __declspec(dllimport)
    #endif
#else
    #define CHARACTER_FACTORY_EXPORT
#endif

namespace cf {

class CHARACTER_FACTORY_EXPORT characterfactoryfbxhandle : public MPxCommand {
public:
    characterfactoryfbxhandle();
    virtual ~characterfactoryfbxhandle();

    static void* creator();
    MStatus doIt(const MArgList& args) override;
    virtual bool isUndoable() const override { return false; }

    static const char* commandName;

private:
    // 使用原始指针或智能指针来避免STL容器的警告
    class Implementation;
    Implementation* pImpl;
};

}
