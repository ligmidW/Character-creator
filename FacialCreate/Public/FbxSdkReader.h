// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "fbxsdk.h"
#include "FbxSdkReader.generated.h"

/**
 * 
 */

UCLASS()
class FACIALCREATE_API UFbxSdkReader : public UObject
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "FBX")
    static void ReadFbxFile(const FString& FilePath);

    UFUNCTION(BlueprintCallable, Category = "FBX")
    static void AddSkeleton();

    UFUNCTION(BlueprintCallable, Category = "FBX")
    static void SaveFbxFile(const FString& FilePath);

    static TArray<FString> meshNames;

    static FbxNode* FindNode(const char* Name);
    UFUNCTION(BlueprintCallable, Category = "FBX")
    static TArray<FVector> GetMeshVertex(const FString& MeshName);
    static FbxNode* CreateSkeletonNode(const char* Name, const char* parentjointName = nullptr,
        double TransX = 0.0, double TransY = 0.0, double TransZ = 0.0,
        double RotX = 0.0, double RotY = 0.0, double RotZ = 0.0);
    // 获取骨骼的世界位置
    static FbxVector4 GetJointWorldPosition(const char* JointName);
    // 设置骨骼的世界位置
    static bool SetJointWorldPosition(const char* JointName, const FbxVector4& WorldPosition);
    // 获取指定模型的指定顶点的世界位置
    static FbxVector4 GetMeshVertexWorldPosition(const char* MeshName, int VertexIndex);
    static void SetSkeletonOrient(const char* Name, double RotX, double RotY, double RotZ);
    static void getChildMesh(FbxNode* ParentNode, TArray<FString>& OutMeshNames);
    // Set skinning weights between mesh and bones
    static bool SetMeshSkinning(const char* MeshName, const char* BoneName, const TArray<int32>& VertexIndices, const TArray<double>& Weights);
    //static FbxVector4 GetMeshMaxVertexPosition(const char* meshNamePattern);
    static FbxVector4 GetMeshPosition(const char* meshNamePattern);

    static FString FbxFilePath;

    static FbxScene* Scene;
    static FbxManager* SdkManager;
private:
    static void GetFbxData();
    static FbxNode* RootNode;
    
    static FbxNode* FindChildNode(FbxNode* ParentNode, const char* Name);
    static TArray<FbxAMatrix> SavedBoneTransforms;  // 保存骨骼变换
    static void SaveSceneBoneTransforms();  // 保存当前场景骨骼变换
    static void RestoreSceneBoneTransforms();  // 恢复保存的骨骼变换
};