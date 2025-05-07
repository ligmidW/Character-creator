#pragma once

#include "CoreMinimal.h"
#include "FbxSdkReader.h"
#include "fbxsdk.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Materials/MaterialInstanceDynamic.h"

/**
 * 
 */
class FACIALCREATE_API OccCreate
{
public:
	OccCreate();
	~OccCreate();

	static void getFbxSdkMesh(FString poseName, uint16_t poseIndex);
	static void CreateOcclusionMap(FbxNode* meshNode, const FString& fbxPath, const FString& poseName, uint16_t poseIndex);

	// 存储每个pose的顶点角度差异值：pose索引 -> (顶点索引 -> 差异值)
	static TMap<uint16_t, TMap<int32, FVector>> PoseVertexAngleDiffs;

	// 将顶点角度差异值保存为JSON文件
	static void SaveAngleDiffsToJson(const FString& OutputPath);

private:
	static bool LineTriangleIntersection(const FVector& Start, const FVector& End, const FVector& V0, const FVector& V1, const FVector& V2);
};
