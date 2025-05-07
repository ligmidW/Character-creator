#pragma once

#include <string>
#include <vector>
#include <map>
#include <array>
#include <memory>
#include <fbxsdk.h>
#include <nlohmann/json.hpp>

using namespace std;

namespace cf {

class FbxModelHandle {
public:
    FbxModelHandle();
    ~FbxModelHandle();
    void processMultipleFbxFiles(const vector<string>& fbxFiles, const string& outputPath = "", const string& jsonPath = "");

private:
    bool createFbxScene(const char* fbxPath);
    
    map<string, vector<array<double, 3>>> getModelVertices();

    void processNode(FbxNode* node, map<string, vector<array<double, 3>>>& meshData);
    
    void saveJsonFile(const vector<map<string, vector<array<double, 3>>>>& data, const string& filePath);
    
    array<array<double,4>,4> calculateRigidTransformation(const vector<array<double,3>>& src_points, const vector<array<double,3>>& tgt_points);
    
    array<double,3> applyTransformation(const array<double,3>& point, const array<array<double,4>,4>& transform);
    
    vector<map<string, vector<array<double, 3>>>> getMeshGap(const vector<map<string, vector<array<double, 3>>>>& meshData);
    
    void createBlendShapes(const string& basefbx, const nlohmann::json& weightJson, const vector<map<string, vector<array<double,3>>>>& distance, map<string, vector<array<double,3>>>& vertexData);
    
    nlohmann::json readWeightJson(const string& weightJsonPath);

    FbxNode* findNodeByName(FbxNode* rootNode, const string& nodeName);
    
    bool saveFbxFile(const string& outputPath);
    
    // Region-based alignment functions
    vector<map<string, vector<array<double, 3>>>> alignRegionsByCenter(const vector<map<string, vector<array<double, 3>>>>& fbxData, const nlohmann::json& weightJson);
    map<string, vector<array<double, 3>>> adjustVerticesByRegion(const map<string, vector<array<double, 3>>>& baseMesh, map<string, vector<array<double, 3>>>& targetMesh, const nlohmann::json& regionWeights);
    array<double, 3> calculateRegionCenter(const vector<array<double, 3>>& vertices);
    vector<array<double, 3>> adjustRegionVertices(const vector<array<double, 3>>& baseVertices, vector<array<double, 3>>& targetVertices, double weight);
    
private:
    FbxManager* mFbxManager;
    FbxScene* mFbxScene;
};

}
