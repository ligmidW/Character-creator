// Fill out your copyright notice in the Description page of Project Settings.


#include "GetNormalAmendVertexPosition.h"

GetNormalAmendVertexPosition::GetNormalAmendVertexPosition()
{
}

GetNormalAmendVertexPosition::~GetNormalAmendVertexPosition()
{
}

bool GetNormalAmendVertexPosition::GetPixelValueFromDNAPng(const FString& pngPath, int32 x, int32 y, FVector& outValue)
{
    // 读取PNG文件
    TArray<uint8> FileData;
    if (!FFileHelper::LoadFileToArray(FileData, *pngPath))
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to load PNG file: %s"), *pngPath);
        return false;
    }

    // 获取ImageWrapper模块
    IImageWrapperModule& ImageWrapperModule = FModuleManager::LoadModuleChecked<IImageWrapperModule>(FName("ImageWrapper"));
    TSharedPtr<IImageWrapper> ImageWrapper = ImageWrapperModule.CreateImageWrapper(EImageFormat::PNG);

    if (!ImageWrapper.IsValid() || !ImageWrapper->SetCompressed(FileData.GetData(), FileData.Num()))
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to create image wrapper for PNG file"));
        return false;
    }

    // 获取原始像素数据
    TArray<uint8> RawData;
    if (!ImageWrapper->GetRaw(ERGBFormat::RGBA, 8, RawData))
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to get raw data from PNG file"));
        return false;
    }

    // 获取图像尺寸
    const int32 Width = ImageWrapper->GetWidth();
    const int32 Height = ImageWrapper->GetHeight();

    // 检查坐标是否有效
    if (x < 0 || x >= Width || y < 0 || y >= Height)
    {
        UE_LOG(LogTemp, Error, TEXT("Invalid pixel coordinates: x=%d, y=%d"), x, y);
        return false;
    }

    // 计算像素索引（每个像素4个字节：RGBA）
    const int32 PixelIndex = (y * Width + x) * 4;

    // 读取RGB值（这里假设值已经被正规化到0-255范围）
    outValue.X = RawData[PixelIndex] / 255.0f;     // R
    outValue.Y = RawData[PixelIndex + 1] / 255.0f; // G
    outValue.Z = RawData[PixelIndex + 2] / 255.0f; // B

    return true;
}
