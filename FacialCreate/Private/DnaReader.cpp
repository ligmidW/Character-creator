#include "DnaReader.h"
#include "dnacalib/DNACalib.h"

#include "fbxsdk.h"
#include "string.h"

#include "FbxSdkReader.h"
#include "FbxSdkSceneSimulation.h"

dnac::ScopedPtr<dnac::DNACalibDNAReader> DnaReader::dnaReader;
dna::ScopedPtr<dna::BinaryStreamWriter> DnaReader::writer;
dna::ScopedPtr<dna::FileStream> DnaReader::inStream;
dna::ScopedPtr<dna::FileStream> DnaReader::outStream;
TArray<uint16_t> DnaReader::LODS;
int DnaReader::meshCount = 0;
int DnaReader::LODCount = 0;
TMap<int, FbxVector4> DnaReader::JointWorldPositionCache;
TArray<dna::Position> DnaReader::jointPositions;
TArray<dna::Position> DnaReader::oldJointPositions;
TArray<dna::Position> DnaReader::newVertexPositions;
TArray<dna::Position> DnaReader::oldVertexPositions;

const char* DnaReader::eyeLeftJoints[6] = { "FACIAL_L_EyelidUpperA",
											"FACIAL_L_EyelidLowerB",
											"FACIAL_L_EyelidLowerA",
											"FACIAL_L_EyelidUpperB",
											"FACIAL_L_EyeParallel",
											"FACIAL_L_Eye" };
const char* DnaReader::eyeRightJoints[6] = { "FACIAL_R_EyelidUpperA",
											"FACIAL_R_EyelidLowerB",
											"FACIAL_R_EyelidLowerA",
											"FACIAL_R_EyelidUpperB",
											"FACIAL_R_EyeParallel",
											"FACIAL_R_Eye" };

struct MeshDimensions {
    float xDiff;
    float yDiff;
    float zDiff;
};

void DnaReader::readDna()
{
	const char* inputDNA = "D:/plugins/MetaHuman-DNA-Calibration-main/data/dna_files/Ada.dna";
	inStream = dna::makeScoped<dna::FileStream>(inputDNA,
		dna::FileStream::AccessMode::Read,
		dna::FileStream::OpenMode::Binary);

	auto reader = dna::makeScoped<dna::BinaryStreamReader>(inStream.get());

	reader->read();
	if (!dna::Status::isOk()) {
		UE_LOG(LogTemp, Error, TEXT("An error occurred while loading the DNA file"));
		return;
	}
	dnaReader = dnac::makeScoped<dnac::DNACalibDNAReader>(reader.get());

	UE_LOG(LogTemp, Log, TEXT("Successfully read DNA file"));
}

void DnaReader::saveDna()
{
	const char* outputDNA = "D:/plugins/MetaHuman-DNA-Calibration-main/data/dna_files/Ada01.dna";

	outStream = dna::makeScoped<dna::FileStream>(outputDNA,
		dna::FileStream::AccessMode::Write,
		dna::FileStream::OpenMode::Binary);

	writer = dna::makeScoped<dna::BinaryStreamWriter>(outStream.get());
	writer->setFrom(dnaReader.get());
}

void DnaReader::setDnaLod()
{
	GetFbxLOD();
	dnac::SetLODsCommand setLodsCmd(nullptr);
	setLodsCmd.setLODs(dnac::ConstArrayView<std::uint16_t>(LODS.GetData(), LODS.Num()));
	setLodsCmd.run(dnaReader.get());

	UE_LOG(LogTemp, Log, TEXT("Successfully amended DNA with %d LODs"), LODS.Num());
}
void DnaReader::GetFbxLOD()
	//: LODCount(0)
{	
	LODS.Empty();
	LODCount = dnaReader->getLODCount();

	for (int lod_index = 0; lod_index < LODCount; ++lod_index)
	{
		trust::ArrayView<const uint16_t> meshIndicesForLOD = dnaReader->getMeshIndicesForLOD(lod_index);


		FString meshIndicesString;
		for (int index : meshIndicesForLOD)
		{
			const char* meshName = dnaReader->getMeshName(index);
			FString currentMeshName = UTF8_TO_TCHAR(meshName);

			if (UFbxSdkReader::meshNames.Contains(currentMeshName))
			{
				if (!LODS.Contains(lod_index))
				{
					LODS.Add(lod_index);
				}
			}
		}
	}
}

DnaReader::DnaReader()
{
	readDna();
	setDnaLod();
	saveDna();
	FormDNAAddSkeleton();
	amendDna();
    
    setDNASkinToFbx();
    //FbxSdkSceneSimulation::poseSimulation();

	FbxSdkSceneSimulation::getPoseToJointMove(0, "Pose0");
	FbxSdkSceneSimulation::getPoseToJointMove(10, "Pose10");

	writer->write();
}
void DnaReader::setDNASkinToFbx()
{	
	meshCount = dnaReader->getMeshCount();
	for (int i = 0; i < meshCount; ++i)
	{
		int meshVertexCount = dnaReader->getVertexPositionCount(i);
		const char* meshName = dnaReader->getMeshName(i);

		for (int j = 0; j < meshVertexCount; ++j)
		{
			auto skinWeightValues = dnaReader->getSkinWeightsValues(i, j);
			auto skinWeightJointIndices = dnaReader->getSkinWeightsJointIndices(i, j);

			TArray<double> weights;
			weights.Reserve(skinWeightValues.size());
			for (const float& weight : skinWeightValues)
			{
				weights.Add(static_cast<double>(weight));
			}

			TArray<int32> vertexIndices;
			vertexIndices.Add(j);

			for (int k = 0; k < skinWeightJointIndices.size(); ++k)
			{
				if (weights[k] > 0.0)
				{
					const char* jointName = dnaReader->getJointName(skinWeightJointIndices[k]);
					TArray<double> singleWeight;
					singleWeight.Add(weights[k]);

					UFbxSdkReader::SetMeshSkinning(meshName, jointName, vertexIndices, singleWeight);
				}
			}
		}
	}
}

int DnaReader::FindNearestVertex(const TArray<dna::Position>& positions, const FbxVector4& targetPosition)
{
    if (positions.Num() == 0)
    {
        return -1;
    }

    int nearestIndex = 0;
    double minDistance = DBL_MAX;

    for (int i = 0; i < positions.Num(); ++i)
    {
        FbxVector4 posVector(positions[i].x, positions[i].y, positions[i].z);
        double distance = (posVector - targetPosition).Length();
        if (distance < minDistance)
        {
            minDistance = distance;
            nearestIndex = i;
        }
    }

    return nearestIndex;
}

void DnaReader::setJointxpostion()
{
    auto vertexX = dnaReader->getVertexPositionXs(0);
    auto vertexY = dnaReader->getVertexPositionYs(0);
    auto vertexZ = dnaReader->getVertexPositionZs(0);
	auto jointX = dnaReader->getNeutralJointTranslationXs();
	auto jointY = dnaReader->getNeutralJointTranslationYs();
	auto jointZ = dnaReader->getNeutralJointTranslationZs();
    const char* _meshName = dnaReader->getMeshName(0);

    uint16_t vertexCount = dnaReader->getVertexPositionCount(0);
	newVertexPositions.SetNum(vertexCount);
    oldVertexPositions.SetNum(vertexCount);
	oldJointPositions.SetNum(jointX.size());
	for (uint16_t i = 0; i < jointX.size(); ++i)
	{
		oldJointPositions[i].x = jointX[i];
		oldJointPositions[i].y = jointY[i];
		oldJointPositions[i].z = jointZ[i];
	}

    for (uint16_t i = 0; i < vertexCount; ++i)
    {
        oldVertexPositions[i].x = vertexX[i];
        oldVertexPositions[i].y = vertexY[i];
        oldVertexPositions[i].z = vertexZ[i];

		FbxVector4 newWorldPosition = UFbxSdkReader::GetMeshVertexWorldPosition(_meshName, i);
		newVertexPositions[i].x = static_cast<float>(newWorldPosition[0]);
		newVertexPositions[i].y = static_cast<float>(newWorldPosition[1]);
		newVertexPositions[i].z = static_cast<float>(newWorldPosition[2]);
    }

    uint16_t jointCount = dnaReader->getJointCount();
    jointPositions.SetNum(jointCount);

    for (uint16_t jointIndex = 0; jointIndex < jointCount; ++jointIndex)
    {
        const char* jointName = dnaReader->getJointName(jointIndex);
        FbxVector4 dnaWorldPosition = UFbxSdkReader::GetJointWorldPosition(jointName);
        int nearestVertexIndex = FindNearestVertex(oldVertexPositions, dnaWorldPosition);

        if (nearestVertexIndex >= 0)
        {
            FbxVector4 vertexPos(oldVertexPositions[nearestVertexIndex].x,
                               oldVertexPositions[nearestVertexIndex].y,
                               oldVertexPositions[nearestVertexIndex].z);
			FbxVector4 fbxWorldPosition(newVertexPositions[nearestVertexIndex].x,
				                newVertexPositions[nearestVertexIndex].y,
				                newVertexPositions[nearestVertexIndex].z
			);
            FbxVector4 distance = fbxWorldPosition - vertexPos;
            FbxVector4 originalPosition = UFbxSdkReader::GetJointWorldPosition(jointName);
            FbxVector4 newPosition = originalPosition + distance;

            UFbxSdkReader::SetJointWorldPosition(jointName, newPosition);

            FbxNode* childNode = UFbxSdkReader::FindNode(jointName);
            if (childNode)
            {
                for (int childIndex = 0; childIndex < childNode->GetChildCount(); childIndex++)
                {
                    const char* childJointName = childNode->GetChild(childIndex)->GetName();
                    FbxVector4 childOriginalPos = UFbxSdkReader::GetJointWorldPosition(childJointName);
                    FbxVector4 relativeOffset = childOriginalPos - distance;
                    UFbxSdkReader::SetJointWorldPosition(childJointName, relativeOffset);
                }

                FbxNode* jointNode = UFbxSdkReader::FindNode(jointName);
                FbxAMatrix localMatrix = jointNode->EvaluateLocalTransform();
                FbxVector4 translation = localMatrix.GetT();

                jointPositions[jointIndex].x = static_cast<float>(translation[0]);
                jointPositions[jointIndex].y = static_cast<float>(translation[1]);
                jointPositions[jointIndex].z = static_cast<float>(translation[2]);
            }

            dna::Vector3 jointRotation = dnaReader->getNeutralJointRotation(jointIndex);
            UFbxSdkReader::SetSkeletonOrient(jointName, jointRotation.x, jointRotation.y, jointRotation.z);
        }
    }
	FString eyeLeftMeshNameStr = FString::Printf(TEXT("eyeLeft_lod%d_mesh"), LODS[0]);
	const char* eyeLeftMeshName = TCHAR_TO_ANSI(*eyeLeftMeshNameStr);
	FString eyeRightMeshNameStr = FString::Printf(TEXT("eyeRight_lod%d_mesh"), LODS[0]);
	const char* eyeRightMeshName = TCHAR_TO_ANSI(*eyeRightMeshNameStr);


	setEyeJointPosition(eyeLeftMeshName, eyeLeftJoints);
	setEyeJointPosition(eyeRightMeshName, eyeRightJoints);

	//processMouthNode("FACIAL_C_MouthUpper", "FACIAL_C_MouthLower");

    writer->setNeutralJointTranslations(jointPositions.GetData(), jointPositions.Num());

}
float DnaReader::getEyeDistance(const char* eyeJoints[])
{
    float maxY = -FLT_MAX;
    float minY = FLT_MAX;
    float maxZ = -FLT_MAX;
    float minZ = FLT_MAX;
    bool foundAnyJoint = false;

    for (int i = 0; i < 6; ++i)
    {

        UE_LOG(LogTemp, Log, TEXT("eyeJoints[%d]: %s"), i, UTF8_TO_TCHAR(eyeJoints[i]));
        FbxNode* jointNode = UFbxSdkReader::FindNode(eyeJoints[i]);
        if (!jointNode) continue;
        foundAnyJoint = true;
        const char* jointName = jointNode->GetName();
        FbxVector4 _jointPosition = UFbxSdkReader::GetJointWorldPosition(jointName);

        maxY = FMath::Max(maxY, static_cast<float>(_jointPosition[1]));
        minY = FMath::Min(minY, static_cast<float>(_jointPosition[1]));
        maxZ = FMath::Max(maxZ, static_cast<float>(_jointPosition[2]));
        minZ = FMath::Min(minZ, static_cast<float>(_jointPosition[2]));

        int childJointCount = jointNode->GetChildCount();
        for (int j = 0; j < childJointCount; ++j)
        {
            FbxNode* childJointNode = jointNode->GetChild(j);
            if (!childJointNode) continue;
            const char* childJointName = childJointNode->GetName();
            FbxVector4 _childJointPosition = UFbxSdkReader::GetJointWorldPosition(childJointName);

            maxY = FMath::Max(maxY, static_cast<float>(_childJointPosition[1]));
            minY = FMath::Min(minY, static_cast<float>(_childJointPosition[1]));
            maxZ = FMath::Max(maxZ, static_cast<float>(_childJointPosition[2]));
            minZ = FMath::Min(minZ, static_cast<float>(_childJointPosition[2]));
        }
    }

    if (!foundAnyJoint) {
        UE_LOG(LogTemp, Warning, TEXT("No eye joints found"));
        return 0.0f;
    }

    return (maxY - minY) / (maxZ - minZ);
}


void DnaReader::setEyeJointPosition(const char* meshName, const char* eyeJoints[])
{
    FbxVector4 eyeMeshPosition = UFbxSdkReader::GetMeshPosition(meshName);

    FbxVector4 _newPositionMax{ -FLT_MAX, -FLT_MAX, -FLT_MAX };
    FbxVector4 _newPositionMin{ FLT_MAX, FLT_MAX, FLT_MAX };

    FbxVector4 _oldPositionMax{ -FLT_MAX, -FLT_MAX, -FLT_MAX };
    FbxVector4 _oldPositionMin{ FLT_MAX, FLT_MAX, FLT_MAX };

    for (int i = 0; i < 6; ++i)
    {
        FbxNode* jointNode = UFbxSdkReader::FindNode(eyeJoints[i]);
        if (!jointNode) continue;

        int childJointCount = jointNode->GetChildCount();
        TArray<FbxVector4> _jointPosition;
        _jointPosition.SetNum(childJointCount);

        for (int j = 0; j < childJointCount; ++j)
        {
            const char* childJointName = jointNode->GetChild(j)->GetName();
            FbxVector4 childJointPosition = UFbxSdkReader::GetJointWorldPosition(childJointName);
            _jointPosition[j] = childJointPosition;
        }

        UFbxSdkReader::SetJointWorldPosition(eyeJoints[i], eyeMeshPosition);
        FbxVector4 Rotation(0, 0, 0);
        jointNode->LclRotation.Set(Rotation);

        for (int j = 0; j < childJointCount; ++j)
        {
            FbxNode* childJointNode = jointNode->GetChild(j);
            const char* childJointName = childJointNode->GetName();
            
            UFbxSdkReader::SetJointWorldPosition(childJointName, _jointPosition[j]);
            childJointNode->LclRotation.Set(Rotation);
        }
		custonjointpositionUpdateDna(jointNode);
	}
}

void DnaReader::processMouthNode(const char* UpperjointName, const char* LowerjointName)
{
    FbxNode* UpperjointNode = UFbxSdkReader::FindNode(UpperjointName);
    FbxNode* LowerjointNode = UFbxSdkReader::FindNode(LowerjointName);
    if (UpperjointNode && LowerjointNode)
    {
        int UpperchildCount = UpperjointNode->GetChildCount();
        int LowerchildCount = LowerjointNode->GetChildCount();
        if (UpperchildCount > 0 && LowerchildCount > 0)
        {
            TArray<FbxVector4> UpperoriginalPositions;
            UpperoriginalPositions.SetNum(UpperchildCount);
            for (int i = 0; i < UpperchildCount; ++i)
            {
                const char* childName = UpperjointNode->GetChild(i)->GetName();
                UpperoriginalPositions[i] = UFbxSdkReader::GetJointWorldPosition(childName);
            }

            TArray<FbxVector4> LoweroriginalPositions;
            LoweroriginalPositions.SetNum(LowerchildCount);
            for (int i = 0; i < LowerchildCount; ++i)
            {
                const char* childName = LowerjointNode->GetChild(i)->GetName();
                LoweroriginalPositions[i] = UFbxSdkReader::GetJointWorldPosition(childName);
            }

            float _oldMax = -FLT_MAX;
            float _oldMin = FLT_MAX;
            float _oldZ = 0;
            float _newMax = -FLT_MAX;
            float _newMin = FLT_MAX;
            float _newZ = 0;

            for (int i = 0; i < UpperchildCount; ++i)
            {
                const char* childName = UpperjointNode->GetChild(i)->GetName();
                int jointIndex = getJointIndexFromName(childName);
                if (jointIndex != -1)
                {   
                    if (oldJointPositions[jointIndex].x > _oldMax)
                    {
                        _oldMax = oldJointPositions[jointIndex].x;
                        _oldZ = oldJointPositions[jointIndex].z;
                    }
                    if (oldJointPositions[jointIndex].x < _oldMin)
                    {
                        _oldMin = oldJointPositions[jointIndex].x;
                    }
                    if (jointPositions[jointIndex].x > _newMax)
                    {
                        _newMax = jointPositions[jointIndex].x;
                        _newZ = jointPositions[jointIndex].z;
                    }
                    if (jointPositions[jointIndex].x < _newMin)
                    {
                        _newMin = jointPositions[jointIndex].x;
                    }
                }
                else
                {
                    UE_LOG(LogTemp, Error, TEXT("Failed to find joint index for upper child: %s"), UTF8_TO_TCHAR(childName));
                }
            }

            if (_oldZ != 0)
            {
                float _m = (_oldMax - _oldMin) / _oldZ;
                float newMouthPositionDistance = _newZ - ((_newMax - _newMin) / _m);
                float _v = (_newMax - _newMin) / (_oldMax - _oldMin);
                FbxVector4 mouthUpperPosition = UFbxSdkReader::GetJointWorldPosition(UpperjointName);
                FbxVector4 mouthLowerPosition = UFbxSdkReader::GetJointWorldPosition(LowerjointName);
                
                mouthUpperPosition[2] += newMouthPositionDistance;
                mouthLowerPosition[2] += newMouthPositionDistance;
                
                UFbxSdkReader::SetJointWorldPosition(UpperjointName, mouthUpperPosition);
                UFbxSdkReader::SetJointWorldPosition(LowerjointName, mouthLowerPosition);

                FbxVector4 Rotation(0, 0, 0);
                UpperjointNode->LclRotation.Set(Rotation);
                LowerjointNode->LclRotation.Set(Rotation);

                for (int j = 0; j < UpperchildCount; j++)
                {
                    FbxNode* childNode = UpperjointNode->GetChild(j);
                    const char* childName = childNode->GetName();
                    UFbxSdkReader::SetJointWorldPosition(childName, UpperoriginalPositions[j]);
                    childNode->LclRotation.Set(Rotation);
                    
                    if (childNode->GetChildCount() > 1)
                    {
                        UE_LOG(LogTemp, Warning, TEXT("Processing sub-children for upper joint: %s"), UTF8_TO_TCHAR(childName));
                        custonjointposition(childNode);
                    }
                    int _jointIndex = getJointIndexFromName(childName);
                    amendJointGroupAsValue(_jointIndex, _v, _v, _v);
                }

                for (int j = 0; j < LowerchildCount; j++)
                {
                    FbxNode* childNode = LowerjointNode->GetChild(j);
                    const char* childName = childNode->GetName();
                    UFbxSdkReader::SetJointWorldPosition(childName, LoweroriginalPositions[j]);
                    childNode->LclRotation.Set(Rotation);

                    if (childNode->GetChildCount() > 1)
                    {
                        UE_LOG(LogTemp, Warning, TEXT("Processing sub-children for lower joint: %s"), UTF8_TO_TCHAR(childName));
                        custonjointposition(childNode);
                    }

                    int _jointIndex = getJointIndexFromName(childName);
                    amendJointGroupAsValue(_jointIndex, _v, _v, _v);
                }

                int _upperjointIndex = getJointIndexFromName(UpperjointName);
                amendJointGroupAsValue(_upperjointIndex, _v, _v, _v);
                int _lowerjointIndex = getJointIndexFromName(LowerjointName);
                amendJointGroupAsValue(_lowerjointIndex, _v, _v, _v);

            }
            else
            {
                UE_LOG(LogTemp, Error, TEXT("_oldZ is 0, skipping position updates"));
            }
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("Node has no children, skipping processing"));
        }
        
        // Update DNA for both joints
        custonjointpositionUpdateDna(UpperjointNode);
        custonjointpositionUpdateDna(LowerjointNode);
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to find joint nodes: %s, %s"), UTF8_TO_TCHAR(UpperjointName), UTF8_TO_TCHAR(LowerjointName));
    }
}


void DnaReader::amendJointGroupAsValue(int jointIndex, float valueX, float valueY, float valueZ)
{
    int jointGroupCount = dnaReader->getJointGroupCount();

    for (int i = 0; i < jointGroupCount; i++)
    {
        TArray<uint16_t> jointGroupJointIndices;
        auto arrayViewIndices = dnaReader->getJointGroupJointIndices(i);
        for (size_t j = 0; j < arrayViewIndices.size(); ++j) {
            jointGroupJointIndices.Add(arrayViewIndices[j]);
        }

        if (jointGroupJointIndices.Contains(jointIndex))
        {
			auto arrayViewInputIndices = dnaReader->getJointGroupInputIndices(i);

            TArray<uint16_t> jointGroupOutputIndices;
            auto arrayViewOutputIndices = dnaReader->getJointGroupOutputIndices(i);
            for (size_t j = 0; j < arrayViewOutputIndices.size(); ++j) 
            {
                jointGroupOutputIndices.Add(arrayViewOutputIndices[j]);
            }

            TArray<float> jointGroupValues;
            auto arrayViewValues = dnaReader->getJointGroupValues(i);
            for (size_t j = 0; j < arrayViewValues.size(); ++j) {
                jointGroupValues.Add(arrayViewValues[j]);
            }
            float _valueX = valueX * valueX;
            TArray<float> XYZMult = { _valueX, valueY, valueZ, valueX, valueY, valueZ, valueX, valueY, valueZ };
            for (int k = jointIndex * 9; k < jointIndex * 9 + 9; k++)
            {
                if (jointGroupOutputIndices.Contains(k))
                {
                    for (int p = 0; p < jointGroupOutputIndices.Num(); p++)
                    {
                        if (jointGroupOutputIndices[p] == k)
                        {
                            for (int l = p * arrayViewInputIndices.size(); l < p * arrayViewInputIndices.size() + arrayViewInputIndices.size(); l++)
                            {
                                jointGroupValues[l] *= XYZMult[l % 9];
                            }
                        }
                    }
                }
            }

            writer->setJointGroupValues(i, jointGroupValues.GetData(), jointGroupValues.Num());

            UE_LOG(LogTemp, Log, TEXT("Modified joint group %d for joint %s with multiplier %f,%f,%f"), i, UTF8_TO_TCHAR(dnaReader->getJointName(jointIndex)), _valueX, valueY, valueZ);
        }
    }
}

void DnaReader::custonjointpositionUpdateDna(FbxNode* childNode)
{
	
    if (!childNode) return;

	FbxAMatrix localMatrix = childNode->EvaluateLocalTransform();
	FbxVector4 translation = localMatrix.GetT();

    const char* childjointName = childNode->GetName();

	int jointIndex = getJointIndexFromName(childjointName);
	if (jointIndex == -1)
	{
		return;
	}
	jointPositions[jointIndex].x = static_cast<float>(translation[0]);
	jointPositions[jointIndex].y = static_cast<float>(translation[1]);
	jointPositions[jointIndex].z = static_cast<float>(translation[2]);

    int grandChildCount = childNode->GetChildCount();
	if (grandChildCount > 0)
	{
		for (int i = 0; i < grandChildCount; ++i)
		{
			FbxNode* grandChildNode = childNode->GetChild(i);
			custonjointpositionUpdateDna(grandChildNode);
		}
	}
}
int DnaReader::getJointIndexFromName(const char* jointName)
{
	int jointCount = dnaReader->getJointCount();
	for (int i = 0; i < jointCount; ++i)
	{
		const char* joint = dnaReader->getJointName(i);
		if (strcmp(joint, jointName) == 0)
		{
			return i;
		}
	}
	return -1; // 如果没找到返回-1
}

void DnaReader::custonjointposition(FbxNode* childNode)
{
    if (!childNode) return;

    const char* childjointName = childNode->GetName();
    int grandChildCount = childNode->GetChildCount();
    
    if (grandChildCount > 1)
    {
        FbxVector4 oldPos = UFbxSdkReader::GetJointWorldPosition(childjointName);
        FbxVector4 minBound(DBL_MAX, DBL_MAX, DBL_MAX);
        FbxVector4 maxBound(-DBL_MAX, -DBL_MAX, -DBL_MAX);
        TArray<FbxVector4> oldgrandChildPositions;
        oldgrandChildPositions.SetNum(grandChildCount);

        for (int j = 0; j < grandChildCount; ++j)
        {
            FbxNode* grandChildNode = childNode->GetChild(j);
            FbxVector4 pos = UFbxSdkReader::GetJointWorldPosition(grandChildNode->GetName());
            oldgrandChildPositions[j] = pos;
            for (int axis = 0; axis < 3; ++axis)
            {
                minBound[axis] = FMath::Min(minBound[axis], pos[axis]);
                maxBound[axis] = FMath::Max(maxBound[axis], pos[axis]);
            }
        }

        FbxVector4 newPos((minBound[0] + maxBound[0]) * 0.5,
                         (minBound[1] + maxBound[1]) * 0.5,
                         (minBound[2] + maxBound[2]) * 0.5,
                         0);

        FbxVector4 offset = oldPos - newPos;
        UFbxSdkReader::SetJointWorldPosition(childjointName, newPos);

		FbxVector4 Rotation(0, 0, 0);
		childNode->LclRotation.Set(Rotation);
        for (int j = 0; j < grandChildCount; ++j)
        {
            FbxNode* grandChildNode = childNode->GetChild(j);
            UFbxSdkReader::SetJointWorldPosition(grandChildNode->GetName(), oldgrandChildPositions[j]);
			grandChildNode->LclRotation.Set(Rotation);
        }
    }
}

void DnaReader::setVertexpostion()
{
	meshCount = dnaReader->getMeshCount();
	for (int i = 0; i < meshCount; ++i)
	{
		const char* meshName = dnaReader->getMeshName(i);
		FString currentMeshName = UTF8_TO_TCHAR(meshName);

		if (UFbxSdkReader::meshNames.Contains(currentMeshName))
		{
			int fbxForDNAindex = UFbxSdkReader::meshNames.Find(currentMeshName);
			TArray<FVector> vertices = UFbxSdkReader::GetMeshVertex(FString(currentMeshName));
			if (vertices.Num() > 0)
			{
				TArray<dna::Position> vertexPositions;
				vertexPositions.SetNum(vertices.Num());

				for (int j = 0; j < vertices.Num(); ++j)
				{
					vertexPositions[j].x = vertices[j].X;
					vertexPositions[j].y = vertices[j].Y;
					vertexPositions[j].z = vertices[j].Z;
				}
				writer->setVertexPositions(i, vertexPositions.GetData(), vertices.Num());
			}
		}
		else
		{
			writer->deleteMesh(i);
		}
	}
}

void DnaReader::FormDNAAddSkeleton()
{
	int jointCount = dnaReader->getJointCount();
	for (int i = 0; i < jointCount; ++i)
	{
		const char* jointName = dnaReader->getJointName(i);
		FString currentJointName = UTF8_TO_TCHAR(jointName);
		dna::Vector3 jointTranslation = dnaReader->getNeutralJointTranslation(i);
		dna::Vector3 jointRotation = dnaReader->getNeutralJointRotation(i);
		if (i == 0)
		{
			UFbxSdkReader::CreateSkeletonNode(jointName, nullptr, jointTranslation.x, jointTranslation.y, jointTranslation.z, jointRotation.x, jointRotation.y, jointRotation.z);
		}
		else
		{
			int parentIndex = dnaReader->getJointParentIndex(i);
			const char* parentjointName = dnaReader->getJointName(parentIndex);
			UFbxSdkReader::CreateSkeletonNode(jointName, parentjointName, jointTranslation.x, jointTranslation.y, jointTranslation.z, jointRotation.x, jointRotation.y, jointRotation.z);

		}
		
	}
}
void DnaReader::amendDna()
{
    //float _oldEyeLeftDistance = getEyeDistance(eyeLeftJoints);
    //float _oldRightDistance = getEyeDistance(eyeRightJoints);
	setJointxpostion();
	//float _newEyeLeftDistance = getEyeDistance(eyeLeftJoints);
	//float _newRightDistance = getEyeDistance(eyeRightJoints);
	setVertexpostion();

 //   UE_LOG(LogTemp, Log, TEXT("oldEyeLeftDistance:%f"), _oldEyeLeftDistance);
	//UE_LOG(LogTemp, Log, TEXT("oldRightDistance:%f"), _oldRightDistance);
	//UE_LOG(LogTemp, Log, TEXT("newEyeLeftDistance:%f"), _newEyeLeftDistance);
	//UE_LOG(LogTemp, Log, TEXT("newRightDistance:%f"), _newRightDistance);
}
void DnaReader::getJointPositionToAmendJointGroup()
{

}

void DnaReader::getDnaPositionSetFbx()
{
    if (!dnaReader)
    {
        return;
    }

    uint16_t jointCount = dnaReader->getJointCount();
    for (uint16_t jointIndex = 0; jointIndex < jointCount; ++jointIndex)
    {
        const char* jointName = dnaReader->getJointName(jointIndex);
        FbxNode* jointNode = UFbxSdkReader::FindNode(jointName);
        
        if (jointNode)
        {
            // 从DNA获取位移
            dna::Vector3 dnaPosition = dnaReader->getNeutralJointTranslation(jointIndex);
            FbxDouble3 translation(DnaReader::jointPositions[jointIndex].x, DnaReader::jointPositions[jointIndex].y, DnaReader::jointPositions[jointIndex].z);
            jointNode->LclTranslation.Set(translation);
            
            // 旋转设为0
            FbxDouble3 rotation(0.0, 0.0, 0.0);
            jointNode->LclRotation.Set(rotation);
        }
    }

    // 更新场景
    if (UFbxSdkReader::Scene && UFbxSdkReader::SdkManager)
    {
        UFbxSdkReader::Scene->GetRootNode()->EvaluateGlobalTransform();
        FbxAnimEvaluator* evaluator = UFbxSdkReader::Scene->GetAnimationEvaluator();
        evaluator->Reset();
    }
}

DnaReader::~DnaReader()
{
}
