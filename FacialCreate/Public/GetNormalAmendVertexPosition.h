#pragma once

#include "CoreMinimal.h"
#include "IImageWrapper.h"
#include "IImageWrapperModule.h"
#include "Modules/ModuleManager.h"

/**
 * 从法线贴图中读取顶点位置修改信息的类
 */
class FACIALCREATE_API GetNormalAmendVertexPosition
{
public:
	GetNormalAmendVertexPosition();
	~GetNormalAmendVertexPosition();

	// 获取指定行的非透明RGB值
	static bool GetRowNonTransparentRGB(int32 rowIndex, TArray<TPair<int32, FVector>>& outValues);

	// 从DNA的PNG文件中读取指定像素的RGB值
	static bool GetPixelValueFromDNAPng(const FString& pngPath, int32 x, int32 y, FVector& outValue);
};
