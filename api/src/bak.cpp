#include "FbxHandle.h"
#include <maya/MGlobal.h>
#include <sstream>
#include <iomanip>
#include <nlohmann/json.hpp>
#include "FitVertexPosition.h"

using namespace std;
using namespace fbxsdk;
using json = nlohmann::json;

namespace cf {

    FbxModelHandle::FbxModelHandle() : sdkManager(nullptr), scene(nullptr), fbxExporter(nullptr) {
        sdkManager = FbxManager::Create();
        if (!sdkManager) {
            MGlobal::displayError("Failed to create FBX SDK manager");
            return;
        }

        FbxIOSettings* ios = FbxIOSettings::Create(sdkManager, IOSROOT);
        sdkManager->SetIOSettings(ios);

        scene = FbxScene::Create(sdkManager, "");
        if (!scene) {
            MGlobal::displayError("Failed to create FBX scene");
            sdkManager->Destroy();
            sdkManager = nullptr;
            return;
        }
    }

    FbxModelHandle::~FbxModelHandle() {
        if (fbxExporter) {
            fbxExporter->Destroy();
            fbxExporter = nullptr;
        }
        if (scene) {
            scene->Destroy();
            scene = nullptr;
        }
        if (sdkManager) {
            sdkManager->Destroy();
            sdkManager = nullptr;
        }
    }

    bool FbxModelHandle::initializeFbxScene(const char* fbxFilePath) {
        scene->Destroy();
        scene = FbxScene::Create(sdkManager, "");
        if (!scene) {
            MGlobal::displayError("Failed to create FBX scene");
            return false;
        }

        FbxImporter* importer = FbxImporter::Create(sdkManager, "");
        if (!importer) {
            MGlobal::displayError("Failed to create FBX importer");
            return false;
        }

        bool importStatus = importer->Initialize(fbxFilePath, -1, sdkManager->GetIOSettings());
        if (!importStatus) {
            MGlobal::displayError(MString("Failed to initialize importer for file: ") + fbxFilePath);
            importer->Destroy();
            return false;
        }

        importStatus = importer->Import(scene);
        importer->Destroy();

        if (!importStatus) {
            MGlobal::displayError(MString("Failed to import FBX file: ") + fbxFilePath);
            return false;
        }
        return true;
    }

    void FbxModelHandle::processNode(fbxsdk::FbxNode* node, map<string, vector<array<double, 3>>>& vertexData) {
        if (!node) return;

        FbxMesh* mesh = node->GetMesh();
        if (mesh) {
            string nodeName = node->GetName();
            vector<array<double, 3>>& vertices = vertexData[nodeName];

            FbxVector4* controlPoints = mesh->GetControlPoints();
            int vertexCount = mesh->GetControlPointsCount();

            vertices.reserve(vertexCount);

            for (int i = 0; i < vertexCount; ++i) {
                array<double, 3> vertex = {
                    controlPoints[i][0],
                    controlPoints[i][1],
                    controlPoints[i][2]
                };
                vertices.push_back(vertex);
            }
        }

        for (int i = 0; i < node->GetChildCount(); ++i) {
            processNode(node->GetChild(i), vertexData);
        }
    }

    map<string, vector<array<double, 3>>> FbxModelHandle::getVertices(const char* fbxFilePath) {
        map<string, vector<array<double, 3>>> vertexData;

        if (!initializeFbxScene(fbxFilePath)) {
            MGlobal::displayError(MString("Failed to initialize FBX scene for file: ") + fbxFilePath);
            return vertexData;
        }

        FbxNode* rootNode = scene->GetRootNode();
        if (rootNode) {
            processNode(rootNode, vertexData);
        } else {
            MGlobal::displayError(MString("No root node found in FBX file: ") + fbxFilePath);
        }

        MGlobal::displayInfo(MString("Processed FBX file: ") + fbxFilePath);
        MGlobal::displayInfo(MString("Number of models found: ") + vertexData.size());

        return vertexData;
    }

    string FbxModelHandle::processMultipleFbxFiles(const vector<string>& fbxFiles) {
        map<int, map<string, vector<array<double, 3>>>> fbxData;
        
        for (size_t i = 0; i < fbxFiles.size(); ++i) {
            auto vertexData = getVertices(fbxFiles[i].c_str());
            
            if (!vertexData.empty()) {
                fbxData[static_cast<int>(i)] = vertexData;
            } else {
                MGlobal::displayWarning(MString("No vertex data found in file: ") + fbxFiles[i].c_str());
            }
        }

        if (fbxData.empty()) {
            MGlobal::displayWarning("No data was extracted from any FBX files");
            return "";
        }

        return convertToJson(fbxData);
    }

    string FbxModelHandle::convertToJson(const map<int, map<string, vector<array<double, 3>>>>& fbxData) {
        json j;
        
        for (const auto& modelPair : fbxData) {
            json modelJson;
            for (const auto& meshPair : modelPair.second) {
                json verticesJson = json::array();
                for (const auto& vertex : meshPair.second) {
                    verticesJson.push_back({vertex[0], vertex[1], vertex[2]});
                }
                modelJson[meshPair.first] = verticesJson;
            }
            j[to_string(modelPair.first)] = modelJson;
        }
        
        return j.dump(4);
    }
    
    FbxNode* FbxModelHandle::findNode(const char* nodeName, FbxNode* rootNode) {
        if (!scene) {
            MGlobal::displayError("Scene is not initialized");
            return nullptr;
        }

        if (!rootNode) {
            rootNode = scene->GetRootNode();
        }

        if (!rootNode) {
            MGlobal::displayError("Root node is null");
            return nullptr;
        }

        if (strcmp(rootNode->GetName(), nodeName) == 0) {
            return rootNode;
        }

        for (int i = 0; i < rootNode->GetChildCount(); i++) {
            FbxNode* childResult = findNode(nodeName, rootNode->GetChild(i));
            if (childResult) {
                return childResult;
            }
        }

        return nullptr;
    }

    void FbxModelHandle::createBlendShapes(FbxNode* meshNode, const std::vector<vector<vector<double>>>& distance, nlohmann::json weightJson, map<string, vector<vector<double>>>& vertexData) {
        if (!meshNode) {
            MGlobal::displayError("Mesh node is null");
            return;
        }
        std::vector<std::string> regionNames;
        for (auto vdf = weightJson.begin(); vdf != weightJson.end(); ++vdf) {
            regionNames.push_back(vdf.key());
        }
        FbxMesh* mesh = meshNode->GetMesh();
        if (!mesh) {
            MGlobal::displayError("Cannot get mesh from node");
            return;
        }

        FbxBlendShape* blendShape = FbxBlendShape::Create(this->scene, (std::string(meshNode->GetName()) + "_blendshape").c_str());
        
        for (auto vdf = weightJson.begin(); vdf != weightJson.end(); ++vdf) {
            std::string regionName = vdf.key();
            std::vector<std::string> suffixes = {"source_L", "source_R"};
            for (int suffix_index = 0; suffix_index < suffixes.size(); ++suffix_index){
                std::string channelName = regionName + "_" + suffixes[suffix_index];
                FbxBlendShapeChannel* channel = FbxBlendShapeChannel::Create(this->scene, channelName.c_str());

                FbxShape* shape = FbxShape::Create(this->scene, channelName.c_str());
                shape->InitControlPoints(mesh->GetControlPointsCount());
                FbxVector4* shapePoints = shape->GetControlPoints();
                FbxVector4* meshPoints = mesh->GetControlPoints();

                for (int i = 0; i < mesh->GetControlPointsCount(); i++) {
                    shapePoints[i] = meshPoints[i];
                }

                for (auto vertex_index = vdf.value().begin(); vertex_index != vdf.value().end(); ++vertex_index) {
                    int index = std::stoi(vertex_index.key());
                    double vertex = vertex_index.value();
                    vector<double> vecPos = {
                        vertexData["0"][index][0] + distance[suffix_index][index][0] * vertex,
                        vertexData["0"][index][1] + distance[suffix_index][index][1] * vertex,
                        vertexData["0"][index][2] + distance[suffix_index][index][2] * vertex
                    };
                    
                    shapePoints[index].Set(vecPos[0], vecPos[1], vecPos[2]);
                }

                blendShape->AddBlendShapeChannel(channel);
                channel->AddTargetShape(shape);
            }
        }

        mesh->AddDeformer(blendShape);
    }

    bool FbxModelHandle::exportScene(const char* fbxFilePath) {
        if (!scene || !sdkManager) {
            MGlobal::displayError("Scene or SDK Manager is not initialized");
            return false;
        }

        string dirPath = string(fbxFilePath);
        size_t lastSlash = dirPath.find_last_of("/\\");
        if (lastSlash != string::npos) {
            dirPath = dirPath.substr(0, lastSlash);
            MString createDirCmd = "if (`file -q -exists \"" + MString(dirPath.c_str()) + "\"` == 0) { sysFile -makeDir \"" + MString(dirPath.c_str()) + "\"; }";
            MGlobal::executeCommand(createDirCmd);
        }

        fbxExporter = FbxExporter::Create(sdkManager, "");
        if (!fbxExporter) {
            MGlobal::displayError("Failed to create FBX exporter");
            return false;
        }

        int fileFormat = sdkManager->GetIOPluginRegistry()->FindWriterIDByDescription("FBX ascii (*.fbx)");
        bool initResult = fbxExporter->Initialize(fbxFilePath, fileFormat, sdkManager->GetIOSettings());
        if (!initResult) {
            MString errorMsg = "Failed to initialize FBX exporter. Error: ";
            errorMsg += fbxExporter->GetStatus().GetErrorString();
            MGlobal::displayError(errorMsg);
            fbxExporter->Destroy();
            fbxExporter = nullptr;
            return false;
        }

        bool success = fbxExporter->Export(scene);

        fbxExporter->Destroy();
        fbxExporter = nullptr;

        if (!success) {
            MString errorMsg = "Failed to export FBX file. Error: ";
            errorMsg += fbxExporter->GetStatus().GetErrorString();
            MGlobal::displayError(errorMsg);
            return false;
        }

        MString msg = "Successfully exported FBX file to: ";
        msg += fbxFilePath;
        MGlobal::displayInfo(msg);
        return true;
    }

    void FbxModelHandle::buildScene(const string& fbxFilePath, map<string, vector<vector<double>>>& vertexData, const string& vertexWeightpath){
        MGlobal::displayInfo("FBX File Path: " + MString(fbxFilePath.c_str()));
        MGlobal::displayInfo("Vertex Weight Path: " + MString(vertexWeightpath.c_str()));
        
        for (const auto& pair : vertexData) {
            MString msg = pair.first.c_str();
            msg += ": [";
            
            for (const auto& vertexList : pair.second) {
                msg += "[";
                for (size_t i = 0; i < vertexList.size(); ++i) {
                    msg += vertexList[i];
                    if (i < vertexList.size() - 1) {
                        msg += ",";
                    }
                }
                msg += "],";
            }
            msg += "]";
            
            MGlobal::displayInfo(msg);
        }

        if (!initializeFbxScene(fbxFilePath.c_str())) {
            return;
        }

        FbxNode* headMeshNode = findNode("head_lod0_mesh");
        if (!headMeshNode) {
            MGlobal::displayError("Could not find head_lod0_mesh node");
            return;
        }

        FitVertexPosition vertexPos;
        nlohmann::json weightJson = vertexPos.loadJsonFile(vertexWeightpath.c_str());
        if (weightJson.is_null()) {
            return;
        }

        std::vector<std::string> regionNames;
        for (auto vdf = weightJson.begin(); vdf != weightJson.end(); ++vdf) {
            MString msg = MString("Region: ") + vdf.key().c_str();
            MGlobal::displayInfo(msg);
            regionNames.push_back(vdf.key());
        }
        vector<vector<vector<double>>> distance = getMeshGap(vertexData);
        
        createBlendShapes(headMeshNode, distance, weightJson,vertexData);

        string exportDir = "D:/work/Maya/CharacterFactory/resources/exported";
        MString createDirCmd = "if (`file -q -exists \"" + MString(exportDir.c_str()) + "\"` == 0) { sysFile -makeDir \"" + MString(exportDir.c_str()) + "\"; }";
        MGlobal::executeCommand(createDirCmd);

        string fbxexportPath = exportDir + "/head_lod0_mesh.fbx";
        if (!exportScene(fbxexportPath.c_str())) {
            MGlobal::displayError("Failed to export modified scene");
            return;
        }
        
        MString importCommand = "file -import -type \"FBX\"  -ignoreVersion -ra true -mergeNamespacesOnClash false -namespace \"imported\" -options \"fbx\"  -pr \"";
        importCommand += fbxexportPath.c_str();
        importCommand += "\";";
        
        MStatus status = MGlobal::executeCommand(importCommand);
        if (status != MS::kSuccess) {
            MGlobal::displayError("Failed to import FBX into Maya");
            return;
        }
        
        MGlobal::displayInfo("Successfully imported FBX into Maya");
    }

    vector<vector<vector<double>>> FbxModelHandle::getMeshGap(const map<string, vector<vector<double>>>& meshData) {
        vector<vector<vector<double>>> result;

        if (meshData.find("0") == meshData.end() || 
            meshData.find("1") == meshData.end() || 
            meshData.find("2") == meshData.end()) {
            return result;
        }

        const vector<vector<double>>& baseList = meshData.at("0");
        const vector<vector<double>>& secondList01 = meshData.at("1");
        const vector<vector<double>>& secondList02 = meshData.at("2");

        if (baseList.empty() || secondList01.size() != baseList.size() || secondList02.size() != baseList.size()) {
            return result;
        }

        vector<vector<double>> distance01;
        vector<vector<double>> distance02;

        for (size_t i = 0; i < baseList.size(); ++i) {
            if (secondList01[i].size() != baseList[i].size() || secondList02[i].size() != baseList[i].size()) {
                continue;
            }

            vector<double> distance_d_01;
            vector<double> distance_d_02;
            
            for (size_t j = 0; j < baseList[i].size(); ++j) {
                distance_d_01.push_back(secondList01[i][j] - baseList[i][j]);
                distance_d_02.push_back(secondList02[i][j] - baseList[i][j]);
            }
            
            distance01.push_back(distance_d_01);
            distance02.push_back(distance_d_02);
        }

        result.push_back(distance01);
        result.push_back(distance02);
        
        return result;
    }

}
