// Fill out your copyright notice in the Description page of Project Settings.


#include "FbxSdkSceneSimulation.h"
#include "DnaReader.h"
#include "dnacalib/DNACalib.h"
#include "OccCreate.h"
#include "FbxSdkReader.h"


TMap<int32, FVector> FbxSdkSceneSimulation::neutralNormals = TMap<int32, FVector>();


FbxSdkSceneSimulation::FbxSdkSceneSimulation()
{
}

FbxSdkSceneSimulation::~FbxSdkSceneSimulation()
{


}
void FbxSdkSceneSimulation::poseSimulation()
{
    auto& reader = DnaReader::dnaReader;
    uint16_t poseCount = reader->getRawControlCount(); //get drawControlCount
    
    // 先获取neutral状态的mesh并计算法线
    OccCreate::getFbxSdkMesh("neutral",0);
    FbxNode* meshNode = UFbxSdkReader::FindNode("head_lod0_mesh");
    if (meshNode)
    {
        FbxMesh* mesh = meshNode->GetMesh();
        if (mesh)
        {
            // 清空之前的法线数据
            neutralNormals.Empty();
            
            // 计算每个多边形的法线并存储
            for (int32 polyIndex = 0; polyIndex < mesh->GetPolygonCount(); polyIndex++)
            {
                FVector v0(mesh->GetControlPoints()[mesh->GetPolygonVertex(polyIndex, 0)][0],
                          mesh->GetControlPoints()[mesh->GetPolygonVertex(polyIndex, 0)][1],
                          mesh->GetControlPoints()[mesh->GetPolygonVertex(polyIndex, 0)][2]);
                FVector v1(mesh->GetControlPoints()[mesh->GetPolygonVertex(polyIndex, 1)][0],
                          mesh->GetControlPoints()[mesh->GetPolygonVertex(polyIndex, 1)][1],
                          mesh->GetControlPoints()[mesh->GetPolygonVertex(polyIndex, 1)][2]);
                FVector v2(mesh->GetControlPoints()[mesh->GetPolygonVertex(polyIndex, 2)][0],
                          mesh->GetControlPoints()[mesh->GetPolygonVertex(polyIndex, 2)][1],
                          mesh->GetControlPoints()[mesh->GetPolygonVertex(polyIndex, 2)][2]);
                
                FVector normal = FVector::CrossProduct(v1 - v0, v2 - v0);
                normal.Normalize();
                
                neutralNormals.Add(polyIndex, normal);
            }
        }
    }

    // 处理所有pose
    for (uint16_t i = 0; i < poseCount; ++i)
    {
        FString poseName = FString(reader->getRawControlName(i));
        FbxSdkSceneSimulation::getPoseToJointMove(i, poseName);
    }
}
void FbxSdkSceneSimulation::getPoseToJointMove(uint16_t poseIndex, FString poseName)
{
    TArray<uint16_t> returnJointIndices;
    
    auto& reader = DnaReader::dnaReader;
    if (!reader)
    {
        return;
    }

    uint16_t jointGroupCount = reader->getJointGroupCount();
    
    TArray<FString> attributes = { TEXT("TranslateX"), TEXT("TranslateY"), TEXT("TranslateZ"), 
                                 TEXT("RotateX"), TEXT("RotateY"), TEXT("RotateZ") };

    for (uint16_t i = 0; i < jointGroupCount; ++i)
    {
        TArray<uint16_t> jointGroupInputIndices;
        auto arrayViewInputIndices = reader->getJointGroupInputIndices(i);
        for (size_t j = 0; j < arrayViewInputIndices.size(); ++j)
        {
            jointGroupInputIndices.Add(arrayViewInputIndices[j]);
        }

        if (jointGroupInputIndices.Contains(poseIndex))
        {
            TArray<float> jointGroupValues;
            auto arrayViewValues = reader->getJointGroupValues(i);
            for (size_t j = 0; j < arrayViewValues.size(); ++j) {
                jointGroupValues.Add(arrayViewValues[j]);
            }

            TArray<uint16_t> jointGroupOutputIndices;
            auto arrayViewOutputIndices = reader->getJointGroupOutputIndices(i);

            for (uint16_t j = 0; j < jointGroupInputIndices.Num(); ++j)
            {
                if (jointGroupInputIndices[j] == poseIndex)
                {
                    for (size_t k = 0; k < arrayViewOutputIndices.size(); ++k)
                    {
                        uint16_t valuesindex = k * jointGroupInputIndices.Num() + j;
                        uint16_t jointOutputIndex = arrayViewOutputIndices[k];
                        
                        uint16_t quotient = jointOutputIndex / 9;
                        uint16_t remainder = jointOutputIndex % 9;

                        const char* jointName = reader->getJointName(quotient);

                        FbxNode* jointNode = UFbxSdkReader::FindNode(jointName);

                        if (jointNode)
                        {
                            const int axisIndex = remainder % 3;  // 获取xyz轴的索引
                            if (remainder < 3)  // Translation
                            {
                                FbxDouble3 translation = jointNode->LclTranslation.Get();
                                translation[axisIndex] += jointGroupValues[valuesindex];
                                jointNode->LclTranslation.Set(translation);
                            }
                            else  // Rotation
                            {
                                FbxDouble3 rotation = jointNode->LclRotation.Get();
                                rotation[axisIndex] = jointGroupValues[valuesindex];
                                jointNode->LclRotation.Set(rotation);

                            }
                        }
                    }
                }
            }
        }
    }


    if (UFbxSdkReader::Scene && UFbxSdkReader::SdkManager)
    {

        UFbxSdkReader::Scene->GetRootNode()->EvaluateGlobalTransform();
        
        FbxAnimEvaluator* evaluator = UFbxSdkReader::Scene->GetAnimationEvaluator();
        evaluator->Reset();
    }

    OccCreate::getFbxSdkMesh(poseName, poseIndex + 1);

    DnaReader::getDnaPositionSetFbx();

}
