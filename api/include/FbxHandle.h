#pragma once

#include <string>
#include <vector>
#include <map>
#include <array>
#include <memory>
#include <fbxsdk.h>
#include <nlohmann/json.hpp>
#include <maya/MStatus.h>
#include <maya/MString.h>


using namespace std;

namespace cf {

class FbxModelHandle {
public:
    FbxModelHandle();
    ~FbxModelHandle();
    void processMultipleFbxFiles(const vector<string>& fbxFiles, const string& outputPath = "", const string& jsonPath = "");
    static MStatus deleteModelByName(const MString& modelName);

    // Delete all DAG models (transforms/meshes) in the current scene that match the given name
    // Exact and namespaced matches are considered. Non-DAG nodes are ignored.
    MStatus deleteModelsByName(const MString& name);
    MStatus deleteModelsByName(const std::string& name) { return deleteModelsByName(MString(name.c_str())); }

    // Collect-and-clean helpers for temporary duplicated DAG models used as blendshape targets
    void markNodeForCleanup(const MString& nodeName);
    void cleanupMarkedNodes();

private:
    bool createFbxScene(const char* fbxPath);

    map<string, vector<array<double, 3>>> getModelVertices();

    void processNode(FbxNode* node, map<string, vector<array<double, 3>>>& meshData);

    void saveJsonFile(const vector<map<string, vector<array<double, 3>>>>& data, const string& filePath);

    array<array<double,4>,4> calculateRigidTransformation(const vector<array<double,3>>& src_points, const vector<array<double,3>>& tgt_points);

    array<double,3> applyTransformation(const array<double,3>& point, const array<array<double,4>,4>& transform);

    vector<map<string, vector<array<double, 3>>>> getMeshGap(const vector<map<string, vector<array<double, 3>>>>& meshData);

    void createBlendShapes(const string& basefbx, const nlohmann::json& weightJson, const vector<map<string, vector<array<double,3>>>>& distance, map<string, vector<array<double,3>>>& vertexData, const map<string, map<string, array<double, 3>>>& alignmentOffsets = {});

    // Helpers to build blendshape channels
    void addSourceBlendShapes(FbxMesh* mesh,
                              const string& meshName,
                              const map<string, map<string, double>>& blendShapes,
                              const vector<map<string, vector<array<double,3>>>>& distance,
                              FbxBlendShape* blendShape);

    void addAlignmentBlendShapes(FbxMesh* mesh,
                                 const string& meshName,
                                 const nlohmann::json& weightJson,
                                 const map<string, map<string, array<double, 3>>>& alignmentOffsets,
                                 FbxBlendShape* blendShape);

    nlohmann::json readWeightJson(const string& weightJsonPath);

    FbxNode* findNodeByName(FbxNode* rootNode, const string& nodeName);

    bool saveFbxFile(const string& outputPath);

    // Region-based alignment functions
    vector<map<string, vector<array<double, 3>>>> alignRegionsByCenter(const vector<map<string, vector<array<double, 3>>>>& fbxData, const nlohmann::json& weightJson, map<string, map<string, array<double, 3>>>& alignmentOffsets);
    map<string, vector<array<double, 3>>> adjustVerticesByRegion(const map<string, vector<array<double, 3>>>& baseMesh, map<string, vector<array<double, 3>>>& targetMesh, const nlohmann::json& regionWeights, map<string, array<double, 3>>& regionOffsets);
    array<double, 3> calculateRegionCenter(const vector<array<double, 3>>& vertices);
    vector<array<double, 3>> adjustRegionVertices(const vector<array<double, 3>>& baseVertices, vector<array<double, 3>>& targetVertices, double weight);


private:

    // Names of scene DAG nodes to delete at the end (temporary duplicates for target shapes)
    std::vector<MString> m_nodesToCleanup;

    FbxManager* mFbxManager;
    FbxScene* mFbxScene;
};

}
