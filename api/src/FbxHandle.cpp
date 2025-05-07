#include "FbxHandle.h"
#include <maya/MGlobal.h>
#include <sstream>
#include <iomanip>
#include <nlohmann/json.hpp>
#include <maya/MFnDagNode.h>
#include <maya/MGlobal.h>
#include <maya/MSelectionList.h>
#include <maya/MDagModifier.h>
#include <maya/MStatus.h>

namespace cf {

    FbxModelHandle::FbxModelHandle() : mFbxManager(nullptr), mFbxScene(nullptr) {
        mFbxManager = FbxManager::Create();
        if (!mFbxManager) {
            MGlobal::displayError("Failed to create FBX Manager");
            return;
        }

        FbxIOSettings* ios = FbxIOSettings::Create(mFbxManager, IOSROOT);
        mFbxManager->SetIOSettings(ios);
    }

    FbxModelHandle::~FbxModelHandle() {
        if (mFbxScene) {
            mFbxScene->Destroy();
            mFbxScene = nullptr;
        }

        if (mFbxManager) {
            mFbxManager->Destroy();
            mFbxManager = nullptr;
        }
    }

    bool FbxModelHandle::createFbxScene(const char* fbxPath) {
        if (mFbxScene) {
            mFbxScene->Destroy();
            mFbxScene = nullptr;
        }

        mFbxScene = FbxScene::Create(mFbxManager, "");
        if (!mFbxScene) {
            MGlobal::displayError("Failed to create FBX Scene");
            return false;
        }

        FbxImporter* importer = FbxImporter::Create(mFbxManager, "");
        bool importStatus = importer->Initialize(fbxPath, -1, mFbxManager->GetIOSettings());

        if (!importStatus) {
            MGlobal::displayError(MString("Failed to initialize importer for: ") + fbxPath);
            importer->Destroy();
            return false;
        }

        importStatus = importer->Import(mFbxScene);
        importer->Destroy();

        if (!importStatus) {
            MGlobal::displayError(MString("Failed to import FBX file: ") + fbxPath);
            return false;
        }

        return true;
    }

    void FbxModelHandle::processNode(FbxNode* node, map<string, vector<array<double, 3>>>& meshData) {
        if (!node) return;

        FbxNodeAttribute* attr = node->GetNodeAttribute();
        if (attr && attr->GetAttributeType() == FbxNodeAttribute::eMesh) {
            FbxMesh* mesh = node->GetMesh();
            if (mesh) {
                string meshName = node->GetName();
                int controlPointsCount = mesh->GetControlPointsCount();
                FbxVector4* controlPoints = mesh->GetControlPoints();

                vector<array<double, 3>> vertices;
                for (int i = 0; i < controlPointsCount; ++i) {
                    array<double, 3> vertex = {
                        controlPoints[i][0],
                        controlPoints[i][1],
                        controlPoints[i][2]
                    };
                    vertices.push_back(vertex);
                }

                meshData[meshName] = vertices;
            }
        }

        for (int i = 0; i < node->GetChildCount(); ++i) {
            processNode(node->GetChild(i), meshData);
        }
    }

    map<string, vector<array<double, 3>>> FbxModelHandle::getModelVertices() {
        map<string, vector<array<double, 3>>> meshData;
        if (!mFbxScene) {
            MGlobal::displayError("FBX Scene is not initialized");
            return meshData;
        }

        FbxNode* rootNode = mFbxScene->GetRootNode();
        if (rootNode) {
            processNode(rootNode, meshData);
        } else {
            MGlobal::displayError("Root node is null");
        }

        return meshData;
    }

    void FbxModelHandle::processMultipleFbxFiles(const vector<string>& fbxFiles, const string& outputPath, const string& jsonPath) {
        if (fbxFiles.size() < 2) {
            MGlobal::displayError("Need at least two FBX files to process");
            return;
        }
        
        vector<map<string, vector<array<double,3>>>> fbxData;
        
        for (size_t i = 0; i < fbxFiles.size(); ++i) {
            string fbxPath = fbxFiles[i];
            MGlobal::displayInfo(MString("Processing FBX file: ") + fbxPath.c_str());
            
            if (createFbxScene(fbxPath.c_str())) {
                auto meshData = getModelVertices();
                if (!meshData.empty()) {
                    fbxData.push_back(meshData);
                }
            }
        }
        vector<vector<array<double, 3>>> allHeadData;
        for (const auto& data : fbxData) {
            auto it = data.find("head_lod0_mesh");
            if (it != data.end()) {
                allHeadData.push_back(it->second);
            }
        }
    
        vector<int> correspondenceIndices;
        srand(static_cast<unsigned int>(time(nullptr)));
        for (int i = 0; i < 100; i++) {
            correspondenceIndices.push_back(rand() % 24001); // 生成0-24000范围内的随机数
        }
        vector<vector<array<double, 3>>> correspondencePoints;
        for (const auto& data : allHeadData) {
            vector<array<double, 3>> points;
            for (int idx : correspondenceIndices) {
                if (idx < data.size()) {
                    points.push_back(data[idx]);
                }
            }
            correspondencePoints.push_back(points);
        }

        vector<array<array<double,4>,4>> transforms;
        for (size_t i = 1; i < correspondencePoints.size(); ++i) {
            MGlobal::displayInfo(MString("Computing transform for model ") + static_cast<double>(i));
            transforms.push_back(calculateRigidTransformation(correspondencePoints[0], correspondencePoints[i]));
        }

        for (size_t i = 0; i < transforms.size(); ++i) {
            MString msg;
            msg += MString("Transform----------------------------- ") + (i+1) + ":\n";
            for (const auto& row : transforms[i]) {
                for (double val : row) {
                    msg += val;
                    msg += " ";
                }
                msg += "\n";
            }
            MGlobal::displayInfo(msg);
        }

        for (size_t i = 1; i < fbxData.size(); ++i) {
            const auto& transform = transforms[i-1];
            for (auto& [meshName, vertices] : fbxData[i]) {
                vector<array<double,3>> newVertices;
                for (const auto& vertex : vertices) {
                    newVertices.push_back(applyTransformation(vertex, transform));
                }
                fbxData[i][meshName] = newVertices;
            }
        }

        nlohmann::json weightJson = readWeightJson(jsonPath);
        
        vector<map<string, vector<array<double,3>>>> alignedData = fbxData;
        vector<map<string, vector<array<double,3>>>> meshGap;
        map<string, vector<array<double,3>>> vertexData;

        if (!weightJson.empty()) {
            MGlobal::displayInfo("Aligning facial features based on region centers...");
            alignedData = alignRegionsByCenter(fbxData, weightJson);
        }
        
        meshGap = getMeshGap(alignedData);
        
        for (const auto& [regionName, vertices] : alignedData[0]) {
            vector<array<double,3>> regionVertices;
            for (const auto& vertex : vertices) {
                array<double,3> v = {vertex[0], vertex[1], vertex[2]};
                regionVertices.push_back(v);
            }
            vertexData[regionName] = regionVertices;
        }
        
        createBlendShapes(fbxFiles[0], weightJson, meshGap, vertexData);
    }
    
    vector<map<string, vector<array<double, 3>>>> FbxModelHandle::alignRegionsByCenter(const vector<map<string, vector<array<double, 3>>>>& fbxData, const nlohmann::json& weightJson) {
        if (fbxData.size() < 3) {
            MGlobal::displayWarning("Need at least two meshes to perform region alignment");
            return fbxData;
        }
        
        vector<map<string, vector<array<double, 3>>>> result = fbxData;
        const auto& baseMesh = result[0];
        
        MGlobal::displayInfo("Performing region-based alignment");
        
        for (size_t i = 1; i < result.size(); ++i) {
            auto& targetMesh = result[i];
            result[i] = adjustVerticesByRegion(baseMesh, targetMesh, weightJson);
        }
        
        MGlobal::displayInfo("Region alignment completed");
        return result;
    }
    
    map<string, vector<array<double, 3>>> FbxModelHandle::adjustVerticesByRegion(
        const map<string, vector<array<double, 3>>>& baseMesh, 
        map<string, vector<array<double, 3>>>& targetMesh, 
        const nlohmann::json& regionWeights) {
        
        map<string, vector<array<double, 3>>> result = targetMesh;
        map<string, vector<string>> regionToModelMap = {
            {"eyes_blendshape", {"saliva_lod0_mesh", "eyeRight_lod0_mesh", "eyeLeft_lod0_mesh", "eyeshell_lod0_mesh", "eyelashes_lod0_mesh", "eyeEdge_lod0_mesh", "cartilage_lod0_mesh"}},
            {"mouth_blendshape", {"teeth_lod0_mesh"}}
        };
        map<string, array<double, 3>> offsetMap;
        for (auto& [regionName, regions] : regionWeights.items()) {
            if (regionName != "head_lod0_mesh") {
                continue;
            }
            
            if (baseMesh.find(regionName) == baseMesh.end() || targetMesh.find(regionName) == targetMesh.end()) {
                continue;
            }
            
            const auto& baseVertices = baseMesh.at(regionName);
            auto& targetVertices = targetMesh.at(regionName);
            for (auto& [subRegionName, indices] : regions.items()) {
                vector<array<double, 3>> baseRegionVertices;
                vector<array<double, 3>> targetRegionVertices;
                vector<size_t> vertexIndices;
                vector<double> weights;
                
                for (auto& [indexStr, weight] : indices.items()) {
                    size_t index = std::stoul(indexStr);
                    if (index < baseVertices.size() && index < targetVertices.size()) {
                        vertexIndices.push_back(index);
                        baseRegionVertices.push_back(baseVertices[index]);
                        targetRegionVertices.push_back(targetVertices[index]);
                        weights.push_back(weight);
                    }
                }
                
                if (baseRegionVertices.empty() || targetRegionVertices.empty()) {
                    continue;
                }
                
                array<double, 3> baseCenter = calculateRegionCenter(baseRegionVertices);
                array<double, 3> targetCenter = calculateRegionCenter(targetRegionVertices);
                
                array<double, 3> offset = {
                    targetCenter[0] - baseCenter[0],
                    targetCenter[1] - baseCenter[1],
                    targetCenter[2] - baseCenter[2]
                };

                offsetMap[subRegionName] = offset;

                for (size_t i = 0; i < vertexIndices.size(); ++i) {
                    size_t index = vertexIndices[i];
                    double weight = weights[i];
                    
                    for (int j = 0; j < 3; ++j) {
                        result[regionName][index][j] -= offset[j] * weight;
                    }
                }
            }
        }
        for (const auto& [subRegionName, offset] : offsetMap) {
            for (const auto& modelName : regionToModelMap[subRegionName]) {
                for (size_t i = 0; i < result[modelName].size(); ++i) {
                    for (int j = 0; j < 3; ++j) {
                        result[modelName][i][j] -= offset[j];
                    }
                }
            }
        }
        
        return result;
    }
    
    array<double, 3> FbxModelHandle::calculateRegionCenter(const vector<array<double, 3>>& vertices) {
        array<double, 3> center = {0.0, 0.0, 0.0};
        
        if (vertices.empty()) {
            return center;
        }
        
        for (const auto& vertex : vertices) {
            for (int i = 0; i < 3; ++i) {
                center[i] += vertex[i];
            }
        }
        
        for (int i = 0; i < 3; ++i) {
            center[i] /= static_cast<double>(vertices.size());
        }
        
        return center;
    }
    
    vector<array<double, 3>> FbxModelHandle::adjustRegionVertices(
        const vector<array<double, 3>>& baseVertices, 
        vector<array<double, 3>>& targetVertices, 
        double weight) {
        
        if (baseVertices.size() != targetVertices.size() || baseVertices.empty()) {
            return targetVertices;
        }
        
        array<double, 3> baseCenter = calculateRegionCenter(baseVertices);
        array<double, 3> targetCenter = calculateRegionCenter(targetVertices);
        
        array<double, 3> offset = {
            targetCenter[0] - baseCenter[0],
            targetCenter[1] - baseCenter[1],
            targetCenter[2] - baseCenter[2]
        };
        
        vector<array<double, 3>> result = targetVertices;
        for (size_t i = 0; i < result.size(); ++i) {
            for (int j = 0; j < 3; ++j) {
                result[i][j] -= offset[j] * weight;
            }
        }
        
        return result;
    }

    array<array<double,4>,4> FbxModelHandle::calculateRigidTransformation(const vector<array<double,3>>& src_points, const vector<array<double,3>>& tgt_points) {
        MGlobal::displayInfo(MString("Computing transform with source points: ") + static_cast<double>(src_points.size()) + " target points: " + static_cast<double>(tgt_points.size()));
        
        if (src_points.size() != tgt_points.size() || src_points.empty()) {
            MGlobal::displayWarning("Invalid points for transform computation");
            return array<array<double,4>,4>{{{0.0, 0.0, 0.0, 0.0}, {0.0, 0.0, 0.0, 0.0}, {0.0, 0.0, 0.0, 0.0}, {0.0, 0.0, 0.0, 0.0}}};
        }

        array<double,3> src_centroid = {0.0, 0.0, 0.0};
        array<double,3> tgt_centroid = {0.0, 0.0, 0.0};
        for (const auto& p : src_points) {
            for (int i = 0; i < 3; ++i) src_centroid[i] += p[i];
        }
        for (const auto& p : tgt_points) {
            for (int i = 0; i < 3; ++i) tgt_centroid[i] += p[i];
        }
        for (int i = 0; i < 3; ++i) {
            src_centroid[i] /= src_points.size();
            tgt_centroid[i] /= tgt_points.size();
        }

        vector<array<double,3>> src_centered(src_points.size());
        vector<array<double,3>> tgt_centered(tgt_points.size());
        for (size_t i = 0; i < src_points.size(); ++i) {
            for (int j = 0; j < 3; ++j) {
                src_centered[i][j] = src_points[i][j] - src_centroid[j];
                tgt_centered[i][j] = tgt_points[i][j] - tgt_centroid[j];
            }
        }

        double src_scale = 0.0, tgt_scale = 0.0;
        for (const auto& p : src_centered) {
            src_scale += p[0]*p[0] + p[1]*p[1] + p[2]*p[2];
        }
        for (const auto& p : tgt_centered) {
            tgt_scale += p[0]*p[0] + p[1]*p[1] + p[2]*p[2];
        }
        src_scale = sqrt(src_scale);
        tgt_scale = sqrt(tgt_scale);
        double scale = src_scale / (tgt_scale + 1e-10);

        array<array<double,3>,3> H = {{{0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}}};
        for (size_t i = 0; i < src_points.size(); ++i) {
            for (int j = 0; j < 3; ++j) {
                for (int k = 0; k < 3; ++k) {
                    H[j][k] += tgt_centered[i][j] * src_centered[i][k];
                }
            }
        }

        array<array<double,3>,3> R = {{{1.0, 0.0, 0.0}, {0.0, 1.0, 0.0}, {0.0, 0.0, 1.0}}};

        array<double,3> t;
        for (int i = 0; i < 3; ++i) {
            t[i] = src_centroid[i];
            double Rt = 0;
            for (int j = 0; j < 3; ++j) {
                Rt += scale * R[i][j] * tgt_centroid[j];
            }
            t[i] -= Rt;
        }

        array<array<double,4>,4> transform = {{{0.0, 0.0, 0.0, 0.0}, {0.0, 0.0, 0.0, 0.0}, {0.0, 0.0, 0.0, 0.0}, {0.0, 0.0, 0.0, 0.0}}};
        for (int i = 0; i < 3; ++i) {
            array<double,3> row;
            for (int j = 0; j < 3; ++j) {
                row[j] = scale * R[i][j];
                transform[i][j] = row[j];
            }
            transform[i][3] = t[i];
        }
        transform[3][3] = 1.0;

        return transform;
    }

    array<double,3> FbxModelHandle::applyTransformation(const array<double,3>& point, const array<array<double,4>,4>& transform) {
        array<double,3> result = {0.0, 0.0, 0.0};
        for (int i = 0; i < 3; ++i) {
            result[i] = transform[i][3];
            for (int j = 0; j < 3; ++j) {
                result[i] += transform[i][j] * point[j];
            }
        }
        return result;
    }

    vector<map<string, vector<array<double, 3>>>> FbxModelHandle::getMeshGap(const vector<map<string, vector<array<double, 3>>>>& meshData) {
        if (meshData.size() < 2) {
            MGlobal::displayWarning("Need at least two meshes to calculate gap");
            return {};
        }
        
        vector<map<string, vector<array<double, 3>>>> result;
        result.reserve(meshData.size() - 1);
        
        const auto& baseMesh = meshData[0];
        
        map<string, size_t> vertexCounts;
        for (const auto& [regionName, vertices] : baseMesh) {
            vertexCounts[regionName] = vertices.size();
        }
        
        for (size_t i = 1; i < meshData.size(); ++i) {
            const auto& currentMesh = meshData[i];
            map<string, vector<array<double, 3>>> regionGap;
            
            for (const auto& [regionName, baseVertices] : baseMesh) {
                auto currentIt = currentMesh.find(regionName);
                if (currentIt == currentMesh.end()) {
                    continue;
                }
                
                const auto& currentVertices = currentIt->second;
                const size_t expectedSize = vertexCounts[regionName];
                
                if (currentVertices.size() != expectedSize) {
                    continue;
                }
                
                vector<array<double, 3>> meshGap;
                meshGap.reserve(expectedSize);
                
                for (size_t j = 0; j < expectedSize; ++j) {
                    const auto& baseVert = baseVertices[j];
                    const auto& currentVert = currentVertices[j];
                    meshGap.push_back(array<double, 3>{baseVert[0] - currentVert[0],
                                                           baseVert[1] - currentVert[1],
                                                           baseVert[2] - currentVert[2]});
                }
                
                regionGap[regionName] = std::move(meshGap);
            }
            
            result.push_back(std::move(regionGap));
        }
        
        return result;
    }

    void FbxModelHandle::createBlendShapes(const string& basefbx, const nlohmann::json& weightJson, const vector<map<string, vector<array<double,3>>>>& distance, map<string, vector<array<double,3>>>& vertexData) {
        if (!createFbxScene(basefbx.c_str())) { 
            MGlobal::displayError(MString("Failed to create FBX scene from base file: ") + basefbx.c_str());
            return;
        }
        
        map<string, map<string, map<string, double>>> jsonData;
        for (auto meshIt = weightJson.begin(); meshIt != weightJson.end(); ++meshIt) {
            for (auto blendIt = meshIt.value().begin(); blendIt != meshIt.value().end(); ++blendIt) {
                for (auto weightIt = blendIt.value().begin(); weightIt != blendIt.value().end(); ++weightIt) {
                    jsonData[meshIt.key()][blendIt.key()][weightIt.key()] = weightIt.value();
                }
            }
        }
        
        for (const auto& [meshName, blendShapes] : jsonData) {
            FbxNode* rootNode = mFbxScene->GetRootNode();
            FbxNode* meshNode = findNodeByName(rootNode, meshName);
            
            if (!meshNode) {
                MGlobal::displayWarning(MString("Mesh not found in scene: ") + meshName.c_str());
                continue;
            }
            
            FbxMesh* mesh = meshNode->GetMesh();
            if (!mesh) {
                MGlobal::displayWarning(MString("Node is not a mesh: ") + meshName.c_str());
                continue;
            }
            string meshName = meshNode->GetName();

            FbxBlendShape* blendShape = FbxBlendShape::Create(mFbxScene, (meshName + "_blendshape").c_str());
            
            for (const auto& [regionName, weights] : blendShapes) {
                vector<string> suffixes = {"source_L", "source_M", "source_R"};
                
                for (int suffixIndex = 0; suffixIndex < suffixes.size(); ++suffixIndex) {
                    string channelName = meshName + regionName + "_" + suffixes[suffixIndex];
                    
                    FbxBlendShapeChannel* channel = FbxBlendShapeChannel::Create(mFbxScene, channelName.c_str());
                    
                    FbxShape* shape = FbxShape::Create(mFbxScene, channelName.c_str());
                    
                    shape->InitControlPoints(mesh->GetControlPointsCount());
                    FbxVector4* shapePoints = shape->GetControlPoints();
                    FbxVector4* meshPoints = mesh->GetControlPoints();
                    
                    for (int i = 0; i < mesh->GetControlPointsCount(); i++) {
                        string vertexIndex = std::to_string(i);
                        auto weightIt = weights.find(vertexIndex);
                        if (weightIt != weights.end() && weightIt->second != 0) {
                            if (suffixIndex < distance.size()) {
                                auto& meshDistances = distance[suffixIndex];
                                auto meshIt = meshDistances.find(meshName);
                                if (meshIt != meshDistances.end() && i < meshIt->second.size()) {
                                    double x = meshPoints[i][0] - meshIt->second[i][0] * weightIt->second;
                                    double y = meshPoints[i][1] - meshIt->second[i][1] * weightIt->second;
                                    double z = meshPoints[i][2] - meshIt->second[i][2] * weightIt->second;
                                    shapePoints[i].Set(x, y, z);
                                } else {
                                    shapePoints[i] = meshPoints[i];
                                }
                            } else {
                                shapePoints[i] = meshPoints[i];
                            }
                        } else {
                            shapePoints[i] = meshPoints[i];
                        }
                    }
                    
                    blendShape->AddBlendShapeChannel(channel);
                    channel->AddTargetShape(shape);
                    deleteModelByName(MString(channelName.c_str()));
                }
            }
            
            mesh->AddDeformer(blendShape);
        }

        string outputPath = basefbx.substr(0, basefbx.find_last_of(".")) + "_blendshape.fbx";
        saveFbxFile(outputPath);
    }


    MStatus FbxModelHandle::deleteModelByName(const MString& modelName) {
        MStatus status;
        MSelectionList selection;
        // 支持通配符查找
        status = MGlobal::getSelectionListByName("*" + modelName, selection);
        if (selection.length() == 0) {
            MGlobal::displayWarning("No model found for: " + modelName);
            return MS::kNotFound;
        }
        for (unsigned int i = 0; i < selection.length(); ++i) {
            MDagPath dagPath;
            status = selection.getDagPath(i, dagPath);
            if (status == MS::kSuccess) {
                MObject node = dagPath.node();
                MDagModifier dagMod;
                dagMod.deleteNode(node);
                dagMod.doIt();
                MGlobal::displayInfo("Deleted model: " + dagPath.fullPathName());
            }
        }
        return MS::kSuccess;
    }

    void FbxModelHandle::saveJsonFile(const vector<map<string, vector<array<double, 3>>>>& data, const string& filePath) {
        nlohmann::json j;
        
        for (size_t i = 0; i < data.size(); ++i) {
            const auto& meshes = data[i];
            nlohmann::json meshJson;
            
            for (const auto& [meshName, vertices] : meshes) {
                meshJson[meshName] = vertices;
            }
            
            j.push_back(meshJson);
        }
        
        try {
            FILE* fp = fopen(filePath.c_str(), "w");
            if (fp) {
                string jsonStr = j.dump(4);
                fwrite(jsonStr.c_str(), 1, jsonStr.size(), fp);
                fclose(fp);
            } else {
                MGlobal::displayError(MString("Failed to open file for writing: ") + filePath.c_str());
            }
        } catch (const exception& e) {
            MGlobal::displayError(MString("Error saving JSON file: ") + e.what());
        }
    }

    nlohmann::json FbxModelHandle::readWeightJson(const string& weightJsonPath) {
        nlohmann::json weightJson;
        
        try {
            FILE* fp = fopen(weightJsonPath.c_str(), "r");
            if (fp) {
                fseek(fp, 0, SEEK_END);
                long fileSize = ftell(fp);
                fseek(fp, 0, SEEK_SET);
                
                string jsonContent(fileSize, ' ');
                fread(&jsonContent[0], 1, fileSize, fp);
                fclose(fp);
                
                weightJson = nlohmann::json::parse(jsonContent);
            } else {
                MGlobal::displayError(MString("Failed to open weight JSON file: ") + weightJsonPath.c_str());
            }
        } catch (const exception& e) {
            MGlobal::displayError(MString("Error parsing weight JSON: ") + e.what());
        }
        
        return weightJson;
    }

    FbxNode* FbxModelHandle::findNodeByName(FbxNode* node, const string& name) {
        if (!node) return nullptr;

        if (node->GetName() == name) return node;

        for (int i = 0; i < node->GetChildCount(); ++i) {
            FbxNode* child = node->GetChild(i);
            FbxNode* found = findNodeByName(child, name);
            if (found) return found;
        }

        return nullptr;
    }
    
    bool FbxModelHandle::saveFbxFile(const string& outputPath) {
        if (!mFbxScene || !mFbxManager) {
            MGlobal::displayError("FBX Scene or Manager is not initialized");
            return false;
        }
        
        string dirPath = outputPath;
        size_t lastSlash = dirPath.find_last_of("/\\");
        if (lastSlash != string::npos) {
            dirPath = dirPath.substr(0, lastSlash);
            
            MString checkDirCmd = "file -q -exists \"" + MString(dirPath.c_str()) + "\"";
            int dirExists = 0;
            MGlobal::executeCommand(checkDirCmd, dirExists);
            
            if (!dirExists) {
                MString createDirCmd = "sysFile -makeDir \"" + MString(dirPath.c_str()) + "\"";
                int result = 0;
                MGlobal::executeCommand(createDirCmd, result);
                
                if (!result) {
                    MGlobal::displayError(MString("Failed to create directory: ") + dirPath.c_str());
                    return false;
                }
                
                MGlobal::displayInfo(MString("Created directory: ") + dirPath.c_str());
            }
        }
        
        FbxExporter* exporter = FbxExporter::Create(mFbxManager, "");
        if (!exporter) {
            MGlobal::displayError("Failed to create FBX exporter");
            return false;
        }
        
        int fileFormat = mFbxManager->GetIOPluginRegistry()->FindWriterIDByDescription("FBX ascii (*.fbx)");
        bool initResult = exporter->Initialize(outputPath.c_str(), fileFormat, mFbxManager->GetIOSettings());
        if (!initResult) {
            MString errorMsg = "Failed to initialize FBX exporter. Error: ";
            errorMsg += exporter->GetStatus().GetErrorString();
            MGlobal::displayError(errorMsg);
            exporter->Destroy();
            return false;
        }
        
        bool success = exporter->Export(mFbxScene);
        
        exporter->Destroy();
        
        if (!success) {
            MString errorMsg = "Failed to export FBX file. Error: ";
            MGlobal::displayError(errorMsg);
            return false;
        }
        
        MString msg = "Successfully exported FBX file to: ";
        msg += outputPath.c_str();
        MGlobal::displayInfo(msg);
        
        MString openCmd = "file -import -type \"FBX\" -ignoreVersion -ra true -mergeNamespacesOnClash false -namespace \"" + 
                          MString(outputPath.c_str()) + "\" -options \"fbx\" \"" + MString(outputPath.c_str()) + "\"";
        MGlobal::executeCommand(openCmd);
        MGlobal::displayInfo("Opened FBX file in Maya");
        
        return true;
    }
}
