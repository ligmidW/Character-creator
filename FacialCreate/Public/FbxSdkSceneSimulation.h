// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "fbxsdk.h"

/**
 * FbxSdkSceneSimulation类用于处理FBX场景中的姿势模拟和法线计算
 */
class FACIALCREATE_API FbxSdkSceneSimulation
{
public:
    FbxSdkSceneSimulation();
    ~FbxSdkSceneSimulation();

    /**
     * 处理指定索引的姿势，设置相应的骨骼变换
     * @param poseIndex - 姿势索引
     * @param poseName - 姿势名称
     */
    static void getPoseToJointMove(uint16_t poseIndex, FString poseName);

    /**
     * 执行姿势模拟，包括计算中立状态法线和处理所有姿势
     */
    static void poseSimulation();
    
    /** 存储中立状态的面法线，key为多边形索引 */
    static TMap<int32, FVector> neutralNormals;  // 存储neutral状态下的法线数据
};
