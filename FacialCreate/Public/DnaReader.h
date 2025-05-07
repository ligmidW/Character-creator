// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "fbxsdk.h"
#include "dnacalib/DNACalib.h"


class FACIALCREATE_API DnaReader
{
public:
	struct MeshDimensions
	{
		MeshDimensions(float x, float y, float z) : xDiff(x), yDiff(y), zDiff(z) {}
		float xDiff;
		float yDiff;
		float zDiff;
	};

	DnaReader();
	~DnaReader();
	static TArray<uint16_t> LODS;
	static dnac::ScopedPtr<dnac::DNACalibDNAReader> dnaReader;
	static dna::ScopedPtr<dna::BinaryStreamWriter> writer;
	static dna::ScopedPtr<dna::FileStream> inStream;
	static dna::ScopedPtr<dna::FileStream> outStream;
	static TArray<dna::Position> jointPositions;
	static TArray<dna::Position> oldJointPositions;
	static TArray<dna::Position> newVertexPositions;
	static TArray<dna::Position> oldVertexPositions;
	static const char* eyeLeftJoints[6];
	static const char* eyeRightJoints[6];


	static void readDna();
	static void saveDna();
	static void setDnaLod();
	static void amendDna();
	static void getJointPositionToAmendJointGroup();
	static void amendJointGroupAsValue(int jointIndex, float valueX, float valueY, float valueZ);
	static void getDnaPositionSetFbx();
	static float getEyeDistance(const char* eyeJoints[]);

private:
	//const char meshName;
	static void GetFbxLOD();
	static void FormDNAAddSkeleton();
	static void setVertexpostion();
	static void setJointxpostion();
    static void processMouthNode(const char* UpperjointName, const char* LowerjointName);
    static void setEyeJointPosition(const char* meshName, const char* eyeJoints[]);
    static void custonjointposition(FbxNode* childNode);
    static void custonjointpositionUpdateDna(FbxNode* childNode);
    static TMap<int, FbxVector4> JointWorldPositionCache;
    static int getJointIndexFromName(const char* jointName);
    // 获取距离目标位置最近的顶点
    static int FindNearestVertex(const TArray<dna::Position>& positions, const FbxVector4& targetPosition);
	static void setDNASkinToFbx();
	static int LODCount;
	static int meshCount;
};
