#include "OccCreate.h"
#include "ImageUtils.h"
#include "IImageWrapper.h"
#include "IImageWrapperModule.h"
#include "Modules/ModuleManager.h"
#include "Json.h"
#include "JsonObjectConverter.h"
#include "Serialization/JsonWriter.h"
#include "Serialization/JsonSerializer.h"
#include "Policies/CondensedJsonPrintPolicy.h"

// 定义静态成员变量
TMap<uint16_t, TMap<int32, FVector>> OccCreate::PoseVertexAngleDiffs;

OccCreate::OccCreate()
{
}

OccCreate::~OccCreate()
{
}

void OccCreate::getFbxSdkMesh(FString poseName, uint16_t poseIndex)
{
    FbxNode* meshNode = UFbxSdkReader::FindNode("head_lod0_mesh");
    if (!meshNode)
    {
        return;
    }

    CreateOcclusionMap(meshNode, UFbxSdkReader::FbxFilePath, poseName, poseIndex);

    FString JsonPath = FPaths::Combine(FPaths::GetPath(UFbxSdkReader::FbxFilePath), TEXT("angle_diffs.json"));
    SaveAngleDiffsToJson(JsonPath);
}

void OccCreate::CreateOcclusionMap(FbxNode* meshNode, const FString& fbxPath, const FString& poseName, uint16_t poseIndex)
{
    if (!meshNode)
    {
        return;
    }

    FbxMesh* originalMesh = meshNode->GetMesh();
    if (!originalMesh)
    {
        UE_LOG(LogTemp, Error, TEXT("Invalid mesh"));
        return;
    }

    FbxNode* deformedNode = FbxNode::Create(meshNode->GetScene(), TCHAR_TO_ANSI(*poseName));
    meshNode->GetScene()->GetRootNode()->AddChild(deformedNode);

    FbxMesh* deformedMesh = static_cast<FbxMesh*>(originalMesh->Clone(FbxObject::eDeepClone));
    if (!deformedMesh)
    {
        UE_LOG(LogTemp, Warning, TEXT("Failed to clone mesh"));
        return;
    }

    deformedNode->SetNodeAttribute(deformedMesh);


    FbxSkin* skin = static_cast<FbxSkin*>(originalMesh->GetDeformer(0, FbxDeformer::eSkin));
    if (skin)
    {
        FbxVector4* controlPoints = deformedMesh->GetControlPoints();
        const int32 numVertices = deformedMesh->GetControlPointsCount();

        TArray<FbxVector4> deformedPositions;
        deformedPositions.SetNum(numVertices);
        TArray<double> totalWeights;
        totalWeights.SetNum(numVertices);
        
        for (int32 i = 0; i < numVertices; ++i)
        {
            deformedPositions[i] = FbxVector4(0, 0, 0, 0);
            totalWeights[i] = 0.0;
        }
        
        int32 clusterCount = skin->GetClusterCount();
        for (int32 clusterIndex = 0; clusterIndex < clusterCount; ++clusterIndex)
        {
            FbxCluster* cluster = skin->GetCluster(clusterIndex);
            FbxNode* boneNode = cluster->GetLink();
            
            if (!boneNode)
                continue;
                
            FbxAMatrix currentTransform = boneNode->EvaluateGlobalTransform();
            FbxAMatrix bindPoseMatrix;
            cluster->GetTransformMatrix(bindPoseMatrix);
            FbxAMatrix bindPoseLinkMatrix;
            cluster->GetTransformLinkMatrix(bindPoseLinkMatrix);
            FbxAMatrix transformMatrix = currentTransform * bindPoseLinkMatrix.Inverse() * bindPoseMatrix;
            
            int32 vertexIndexCount = cluster->GetControlPointIndicesCount();
            int32* vertexIndices = cluster->GetControlPointIndices();
            double* weights = cluster->GetControlPointWeights();
            
            for (int32 i = 0; i < vertexIndexCount; ++i)
            {
                int32 vertexIndex = vertexIndices[i];
                double weight = weights[i];
                
                if (weight > 0)
                {
                    totalWeights[vertexIndex] += weight;
                    FbxVector4 transformedPosition = transformMatrix.MultT(controlPoints[vertexIndex]);
                    deformedPositions[vertexIndex] += transformedPosition * weight;
                }
            }
        }
		while (deformedMesh->GetDeformerCount() > 0)
		{
			deformedMesh->RemoveDeformer(0);
		}

        for (int32 i = 0; i < numVertices; ++i)
        {
            if (totalWeights[i] > 0)
            {
                deformedPositions[i] = deformedPositions[i] * (1.0 / totalWeights[i]);
            }
            else
            {
                deformedPositions[i] = controlPoints[i];
            }
            controlPoints[i] = deformedPositions[i];
        }

        FbxLayerElementUV* uvLayer = deformedMesh->GetElementUV(0);
        if (!uvLayer)
        {
            return;
        }

        int32 vertexCount = deformedMesh->GetControlPointsCount();
        const int32 imageHeight = poseIndex + 1;
        const int32 imageWidth = vertexCount;
        static TArray<uint16> imageData;
        if (imageData.Num() < imageWidth * imageHeight * 4)
        {
            imageData.SetNumZeroed(imageWidth * imageHeight * 4);
        }

        TArray<TTuple<FVector, FVector, FVector, FVector>> trianglesAndNormals;
        for (int32 polyIndex = 0; polyIndex < deformedMesh->GetPolygonCount(); polyIndex++)
        {
            FVector v0(deformedMesh->GetControlPoints()[deformedMesh->GetPolygonVertex(polyIndex, 0)][0],
                      deformedMesh->GetControlPoints()[deformedMesh->GetPolygonVertex(polyIndex, 0)][1],
                      deformedMesh->GetControlPoints()[deformedMesh->GetPolygonVertex(polyIndex, 0)][2]);
            FVector v1(deformedMesh->GetControlPoints()[deformedMesh->GetPolygonVertex(polyIndex, 1)][0],
                      deformedMesh->GetControlPoints()[deformedMesh->GetPolygonVertex(polyIndex, 1)][1],
                      deformedMesh->GetControlPoints()[deformedMesh->GetPolygonVertex(polyIndex, 1)][2]);
            FVector v2(deformedMesh->GetControlPoints()[deformedMesh->GetPolygonVertex(polyIndex, 2)][0],
                      deformedMesh->GetControlPoints()[deformedMesh->GetPolygonVertex(polyIndex, 2)][1],
                      deformedMesh->GetControlPoints()[deformedMesh->GetPolygonVertex(polyIndex, 2)][2]);
            
            FVector normal = FVector::CrossProduct(v1 - v0, v2 - v0);
            normal.Normalize();
            
            trianglesAndNormals.Add(MakeTuple(v0, v1, v2, normal));
        }

        FCriticalSection CriticalSection;

        ParallelFor(deformedMesh->GetPolygonCount(), [&](int32 polyIndex)
        {
            for (int32 vertIndex = 0; vertIndex < 3; vertIndex++)
            {
                int32 controlPointIndex = deformedMesh->GetPolygonVertex(polyIndex, vertIndex);

                int32 pixelIndex = (poseIndex * imageWidth + controlPointIndex) * 4;
                
                FVector normal = trianglesAndNormals[polyIndex].Get<3>();
                
                uint16 poseAngleX = FMath::Clamp(FMath::RoundToInt(FMath::RadiansToDegrees(FMath::Acos(normal.X)) * 360.0f), 0, 65535);
                uint16 poseAngleY = FMath::Clamp(FMath::RoundToInt(FMath::RadiansToDegrees(FMath::Acos(normal.Y)) * 360.0f), 0, 65535);
                uint16 poseAngleZ = FMath::Clamp(FMath::RoundToInt(FMath::RadiansToDegrees(FMath::Acos(normal.Z)) * 360.0f), 0, 65535);

                uint16 angleX = poseAngleX;
                uint16 angleY = poseAngleY;
                uint16 angleZ = poseAngleZ;

                if (FbxSdkSceneSimulation::neutralNormals.Contains(polyIndex))
                {
                    FVector neutralNormal = FbxSdkSceneSimulation::neutralNormals[polyIndex];
                    uint16 neutralAngleX = FMath::Clamp(FMath::RoundToInt(FMath::RadiansToDegrees(FMath::Acos(neutralNormal.X)) * 360.0f), 0, 65535);
                    uint16 neutralAngleY = FMath::Clamp(FMath::RoundToInt(FMath::RadiansToDegrees(FMath::Acos(neutralNormal.Y)) * 360.0f), 0, 65535);
                    uint16 neutralAngleZ = FMath::Clamp(FMath::RoundToInt(FMath::RadiansToDegrees(FMath::Acos(neutralNormal.Z)) * 360.0f), 0, 65535);

                    angleX = FMath::Abs(poseAngleX - neutralAngleX);
                    angleY = FMath::Abs(poseAngleY - neutralAngleY);
                    angleZ = FMath::Abs(poseAngleZ - neutralAngleZ);

                    if (angleX + angleY + angleZ < 100)
                    {
                        angleX = angleY = angleZ = 0;
                    }
                }

                FScopeLock Lock(&CriticalSection);
                
                if (angleX == 0 && angleY == 0 && angleZ == 0)
                {
                    imageData[pixelIndex] = 0;
                    imageData[pixelIndex + 1] = 0;
                    imageData[pixelIndex + 2] = 0;
                    imageData[pixelIndex + 3] = 0;
                }
                else
                {
                    FVector dnaAngles;
                    FString dongPath = FPaths::ChangeExtension(TEXT("D:/plugins/MetaHuman-DNA-Calibration-main/data/dna_files/Ada.dna"), TEXT("png"));
                    if (GetNormalAmendVertexPosition::GetPixelValueFromDNAPng(dongPath, controlPointIndex % imageWidth, controlPointIndex / imageWidth, dnaAngles))
                    {
                        FVector angleDiff;
                        angleDiff.X = angleX - dnaAngles.X;
                        angleDiff.Y = angleY - dnaAngles.Y;
                        angleDiff.Z = angleZ - dnaAngles.Z;

                        if (!PoseVertexAngleDiffs.Contains(poseIndex))
                        {
                            PoseVertexAngleDiffs.Add(poseIndex, TMap<int32, FVector>());
                        }

                        PoseVertexAngleDiffs[poseIndex].Add(controlPointIndex, angleDiff);
                    }

                    imageData[pixelIndex] = angleX;
                    imageData[pixelIndex + 1] = angleY;
                    imageData[pixelIndex + 2] = angleZ;
                    imageData[pixelIndex + 3] = 65535;
                }
            }
        });

        static FCriticalSection SaveLock;
        FScopeLock SaveGuard(&SaveLock);
        auto& reader = DnaReader::dnaReader;
        if (reader)
        {
            if (poseIndex == reader->getRawControlCount())
            {
                FString NewFilePath = FPaths::GetPath(fbxPath) / TEXT("combined_normals.png");
                UE_LOG(LogTemp, Warning, TEXT("Saving combined normal map to: %s"), *NewFilePath);

                IImageWrapperModule& ImageWrapperModule = FModuleManager::LoadModuleChecked<IImageWrapperModule>(FName("ImageWrapper"));
                TSharedPtr<IImageWrapper> ImageWrapper = ImageWrapperModule.CreateImageWrapper(EImageFormat::PNG);

                if (ImageWrapper.IsValid() && ImageWrapper->SetRaw(imageData.GetData(), imageData.Num() * sizeof(uint16), imageWidth, imageHeight, ERGBFormat::RGBA, 16))
                {
                    TArray<uint8> PNGData;
                    PNGData = ImageWrapper->GetCompressed(100);
                    FFileHelper::SaveArrayToFile(PNGData, *NewFilePath);
                }

                imageData.Empty();
            }
        }
    }
    deformedNode->Destroy();
}

void OccCreate::SaveAngleDiffsToJson(const FString& OutputPath)
{
    TSharedPtr<FJsonObject> RootObject = MakeShared<FJsonObject>();

    for (const auto& PosePair : PoseVertexAngleDiffs)
    {
        TSharedPtr<FJsonObject> PoseObject = MakeShared<FJsonObject>();

        for (const auto& VertexPair : PosePair.Value)
        {
            TSharedPtr<FJsonObject> DiffObject = MakeShared<FJsonObject>();
            DiffObject->SetNumberField(TEXT("X"), VertexPair.Value.X);
            DiffObject->SetNumberField(TEXT("Y"), VertexPair.Value.Y);
            DiffObject->SetNumberField(TEXT("Z"), VertexPair.Value.Z);

            PoseObject->SetObjectField(FString::FromInt(VertexPair.Key), DiffObject);
        }

        RootObject->SetObjectField(FString::FromInt(PosePair.Key), PoseObject);
    }

    FString OutputString;
    TSharedRef<TJsonWriter<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>> Writer = TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>::Create(&OutputString);
    FJsonSerializer::Serialize(RootObject.ToSharedRef(), Writer);

    FFileHelper::SaveStringToFile(OutputString, *OutputPath);

    UE_LOG(LogTemp, Log, TEXT("Saved angle differences to: %s"), *OutputPath);
}