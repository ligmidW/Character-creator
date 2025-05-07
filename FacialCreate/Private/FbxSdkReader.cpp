#include "FbxSdkReader.h"
#include "fbxsdk.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "DnaReader.h"

TArray<FString> UFbxSdkReader::meshNames;
FbxNode* UFbxSdkReader::RootNode = nullptr;
FbxScene* UFbxSdkReader::Scene = nullptr;
FbxManager* UFbxSdkReader::SdkManager = nullptr;
FString UFbxSdkReader::FbxFilePath;
TArray<FbxAMatrix> UFbxSdkReader::SavedBoneTransforms;

void UFbxSdkReader::ReadFbxFile(const FString& FilePath)
{
    FbxFilePath = FilePath;
    if (SdkManager)
    {
        SdkManager->Destroy();
    }
    SdkManager = FbxManager::Create();

    FbxIOSettings* IOSettings = FbxIOSettings::Create(SdkManager, IOSROOT);
    SdkManager->SetIOSettings(IOSettings);

    FbxImporter* Importer = FbxImporter::Create(SdkManager, "");
    if (!Importer->Initialize(TCHAR_TO_UTF8(*FilePath), -1, SdkManager->GetIOSettings()))
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to initialize FBX importer."));
        return;
    }

    Scene = FbxScene::Create(SdkManager, "My Scene");
    Importer->Import(Scene);

    Importer->Destroy();

    GetFbxData();
    AddSkeleton();

    FString NewFilePath = FilePath;
    NewFilePath = NewFilePath.Replace(TEXT(".fbx"), TEXT("_rig01.fbx"));
    SaveFbxFile(NewFilePath);
}

void UFbxSdkReader::GetFbxData()
{	
	meshNames.Empty();

    RootNode = Scene->GetRootNode();

	getChildMesh(RootNode, meshNames);
	DnaReader::DnaReader();
}

void UFbxSdkReader::getChildMesh(FbxNode* ParentNode, TArray<FString>& OutMeshNames)
{
	for (int i = 0; i < ParentNode->GetChildCount(); ++i)
	{
		FbxNode* ChildNode = ParentNode->GetChild(i);
		if (!ChildNode)
		{
			continue;;
		}
		FbxNodeAttribute* NodeAttribute = ChildNode->GetNodeAttribute();
		if (NodeAttribute && NodeAttribute->GetAttributeType() == FbxNodeAttribute::eMesh)
		{
			const char* meshName = ChildNode->GetName();
			OutMeshNames.Add(UTF8_TO_TCHAR(meshName));
            UE_LOG(LogTemp, Log, TEXT("Mesh name %s"), UTF8_TO_TCHAR(meshName));
		}
       
        getChildMesh(ChildNode, OutMeshNames);
	}
}

TArray<FVector> UFbxSdkReader::GetMeshVertex(const FString& MeshName)
{
    TArray<FVector> vertices;
    FbxNode* ChildNode = FindNode(TCHAR_TO_UTF8(*MeshName));
    if (!ChildNode || !ChildNode->GetNodeAttribute())
    {
        return vertices;
    }

    FbxNodeAttribute::EType attributeType = ChildNode->GetNodeAttribute()->GetAttributeType();
    if (attributeType == FbxNodeAttribute::eMesh)
    {
        FbxMesh* pMesh = static_cast<FbxMesh*>(ChildNode->GetNodeAttribute());
        if (!pMesh)
        {
            return vertices;
        }

        int vertexCount = pMesh->GetControlPointsCount();
        //UE_LOG(LogTemp, Log, TEXT("Vertex count: %d"), vertexCount);

        FbxVector4* controlPoints = pMesh->GetControlPoints();
        if (!controlPoints || vertexCount <= 0)
        {
            return vertices;
        }

        vertices.Reserve(vertexCount);
        
        //Get and set Y-up or Z-up
        if (controlPoints[0][1] < controlPoints[0][2])
        {
            for (int j = 0; j < vertexCount; j++)
            {
                vertices.Add(FVector(
                    controlPoints[j][0],
                    controlPoints[j][2],
                    -controlPoints[j][1]
                ));
            }
        }
        else
        {
            for (int j = 0; j < vertexCount; j++)
            {
                vertices.Add(FVector(
                    controlPoints[j][0],
                    controlPoints[j][1],
                    controlPoints[j][2]
                ));
            }
        }
    }

    return vertices;
}

FbxNode* UFbxSdkReader::FindChildNode(FbxNode* ParentNode, const char* Name)
{
    if (!ParentNode)
    {
        return nullptr;
    }

    for (int i = 0; i < ParentNode->GetChildCount(); i++)
    {
        FbxNode* ChildNode = ParentNode->GetChild(i);
        if (!ChildNode)
        {
            continue;
        }

        if (strcmp(ChildNode->GetName(), Name) == 0)
        {
            return ChildNode;
        }

        FbxNode* Result = FindChildNode(ChildNode, Name);
        if (Result)
        {
            return Result;
        }
    }
    return nullptr;
}

FbxNode* UFbxSdkReader::FindNode(const char* Name)
{
    if (!RootNode || !Name)
    {
        return nullptr;
    }

    // 在根节点中查找骨骼
    if (RootNode->GetNodeAttribute() && 
        strcmp(RootNode->GetName(), Name) == 0)
    {
        return RootNode;
    }

    return FindChildNode(RootNode, Name);
}

void UFbxSdkReader::SetSkeletonOrient(const char* Name, double RotX, double RotY, double RotZ)
{
    // 查找骨骼节点
    FbxNode* SkeletonNode = FindNode(Name);
    if (SkeletonNode)
    {
		// 设置Joint Orient和Rotation
		FbxVector4 jointOrient(RotX, RotY, RotZ);
		FbxVector4 Rotation(0, 0, 0);

		// 设置旋转为0和Joint Orient
		
		SkeletonNode->SetPreRotation(FbxNode::eSourcePivot, jointOrient);
		SkeletonNode->SetRotationOrder(FbxNode::eSourcePivot, FbxEuler::eOrderXYZ);
		SkeletonNode->SetRotationActive(true);
        SkeletonNode->LclRotation.Set(Rotation);
    }



}

FbxNode* UFbxSdkReader::CreateSkeletonNode(const char* Name, const char* parentjointName,
    double TransX, double TransY, double TransZ,
    double RotX, double RotY, double RotZ)
{
    FbxSkeleton* Skeleton = FbxSkeleton::Create(Scene, Name);

    if (!parentjointName)
    {
        Skeleton->SetSkeletonType(FbxSkeleton::eRoot);
    }
    else
    {
        Skeleton->SetSkeletonType(FbxSkeleton::eLimbNode);
    }

    FbxNode* SkeletonNode = FbxNode::Create(Scene, Name);
    SkeletonNode->SetNodeAttribute(Skeleton);

    FbxNode* Parent = nullptr;
    if (parentjointName)
    {
        Parent = FindNode(parentjointName);
        if (Parent)
        {
            Parent->AddChild(SkeletonNode);
        }
    }
    else
    {
        Scene->GetRootNode()->AddChild(SkeletonNode);
    }

    FbxVector4 Translation(TransX, TransY, TransZ);
    
    FbxVector4 Rotation(RotX, RotY, RotZ);  // 确保旋转为0
    FbxVector4 Scaling(1, 1, 1);

    // 设置位置和缩放

    // 设置Joint Orient



	SkeletonNode->LclTranslation.Set(Translation);
    SkeletonNode->LclRotation.Set(Rotation);
	SkeletonNode->LclScaling.Set(Scaling);
    // 显式设置Rotation为0
    

    // 设置骨骼基本属性
    if (Skeleton)
    {
        Skeleton->SetSkeletonType(FbxSkeleton::eLimbNode);
        Skeleton->Size.Set(3.0);
    }

    return SkeletonNode;
}

void UFbxSdkReader::AddSkeleton()
{
    // 保留这个函数，因为它在ReadFbxFile中被引用
    // 如果不需要添加骨骼，可以保持函数体为空
}

bool UFbxSdkReader::SetJointWorldPosition(const char* JointName, const FbxVector4& WorldPosition)
{
    FbxNode* jointNode = FindNode(JointName);
    if (!jointNode)
    {
        UE_LOG(LogTemp, Warning, TEXT("Joint not found in FBX: %s"), UTF8_TO_TCHAR(JointName));
        return false;
    }

    // 获取当前的全局变换
    FbxAMatrix globalTransform = jointNode->EvaluateGlobalTransform();
    
    // 保持原有的旋转和缩放
    FbxVector4 rotation = globalTransform.GetR();
    FbxVector4 scaling = globalTransform.GetS();

    // 创建新的全局变换矩阵
    FbxAMatrix newGlobalTransform;
    newGlobalTransform.SetIdentity();
    newGlobalTransform.SetT(WorldPosition);  // 设置新的位置
    newGlobalTransform.SetR(rotation);       // 保持原有旋转
    newGlobalTransform.SetS(scaling);        // 保持原有缩放

    // 如果有父节点，需要计算局部变换
    FbxNode* parentNode = jointNode->GetParent();
    if (parentNode)
    {
        FbxAMatrix parentGlobal = parentNode->EvaluateGlobalTransform();
        FbxAMatrix localTransform = parentGlobal.Inverse() * newGlobalTransform;
        
        // 设置局部变换
        jointNode->LclTranslation.Set(localTransform.GetT());
        jointNode->LclRotation.Set(localTransform.GetR());
        jointNode->LclScaling.Set(localTransform.GetS());
    }
    else
    {
        // 如果是根节点，直接设置全局变换为局部变换
        jointNode->LclTranslation.Set(WorldPosition);
        jointNode->LclRotation.Set(rotation);
        jointNode->LclScaling.Set(scaling);
    }

    //UE_LOG(LogTemp, Log, TEXT("Set Joint [%s] World Position to: X=%.3f, Y=%.3f, Z=%.3f"),
    //    UTF8_TO_TCHAR(JointName),
    //    WorldPosition[0], WorldPosition[1], WorldPosition[2]);

    return true;
}

void UFbxSdkReader::SaveFbxFile(const FString& FilePath)
{
    if (!Scene || !SdkManager)
    {
        UE_LOG(LogTemp, Error, TEXT("Scene or SdkManager is null when saving FBX"));
        return;
    }
    FbxExporter* Exporter = FbxExporter::Create(SdkManager, "");
    if (!Exporter->Initialize(TCHAR_TO_UTF8(*FilePath), -1, SdkManager->GetIOSettings()))
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to initialize FBX exporter."));
        Exporter->Destroy();
        return;
    }

    bool bSuccess = Exporter->Export(Scene);
    if (bSuccess)
    {
        UE_LOG(LogTemp, Log, TEXT("Successfully saved rigged FBX to: %s"), *FilePath);
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to save rigged FBX"));
    }

    Exporter->Destroy();
}

FbxVector4 UFbxSdkReader::GetJointWorldPosition(const char* JointName)
{
    FbxNode* jointNode = FindNode(JointName);
    if (!jointNode)
    {
        UE_LOG(LogTemp, Warning, TEXT("Joint not found in FBX: %s"), UTF8_TO_TCHAR(JointName));
        return FbxVector4(0, 0, 0);
    }
    FbxAMatrix globalTransform = jointNode->EvaluateGlobalTransform();

    FbxVector4 globalPosition = globalTransform.GetT();

    return globalPosition;
}

FbxVector4 UFbxSdkReader::GetMeshVertexWorldPosition(const char* MeshName, int VertexIndex)
{
    FbxNode* meshNode = FindChildNode(RootNode, MeshName);
    if (!meshNode)
    {
        UE_LOG(LogTemp, Warning, TEXT("Mesh not found: %s"), UTF8_TO_TCHAR(MeshName));
        return FbxVector4(0, 0, 0, 1);
    }

    FbxMesh* mesh = meshNode->GetMesh();
    if (!mesh)
    {
        UE_LOG(LogTemp, Warning, TEXT("No mesh found on node: %s"), UTF8_TO_TCHAR(MeshName));
        return FbxVector4(0, 0, 0, 1);
    }

    if (VertexIndex < 0 || VertexIndex >= mesh->GetControlPointsCount())
    {
        UE_LOG(LogTemp, Warning, TEXT("Invalid vertex index %d for mesh %s (max: %d)"), 
            VertexIndex, UTF8_TO_TCHAR(MeshName), mesh->GetControlPointsCount());
        return FbxVector4(0, 0, 0, 1);
    }

    FbxVector4 localPosition = mesh->GetControlPointAt(VertexIndex);

    FbxAMatrix globalTransform = meshNode->EvaluateGlobalTransform();

    FbxVector4 worldPosition = globalTransform.MultT(localPosition);

    //UE_LOG(LogTemp, Log, TEXT("Mesh [%s] Vertex [%d] World Position: (%.3f, %.3f, %.3f)"),
    //    UTF8_TO_TCHAR(MeshName), VertexIndex,
    //    worldPosition[0], worldPosition[1], worldPosition[2]);

    return worldPosition;
}

bool UFbxSdkReader::SetMeshSkinning(const char* MeshName, const char* BoneName, const TArray<int32>& VertexIndices, const TArray<double>& Weights)
{
    if (!Scene || !RootNode)
    {
        UE_LOG(LogTemp, Error, TEXT("Scene or RootNode is null"));
        return false;
    }
    FbxNode* MeshNode = FindNode(MeshName);
    FbxNode* BoneNode = FindNode(BoneName);

    if (!MeshNode || !BoneNode)
    {
        UE_LOG(LogTemp, Error, TEXT("Could not find mesh or bone node"));
        return false;
    }

    FbxMesh* Mesh = MeshNode->GetMesh();
    if (!Mesh)
    {
        UE_LOG(LogTemp, Error, TEXT("Could not get mesh from node"));
        return false;
    }

    FbxSkin* Skin = nullptr;
    int DeformerCount = Mesh->GetDeformerCount();
    for (int i = 0; i < DeformerCount; ++i)
    {
        FbxDeformer* Deformer = Mesh->GetDeformer(i);
        if (Deformer->GetDeformerType() == FbxDeformer::eSkin)
        {
            Skin = static_cast<FbxSkin*>(Deformer);
            break;
        }
    }

    if (!Skin)
    {
        Skin = FbxSkin::Create(Scene, "");
        Mesh->AddDeformer(Skin);
    }

    FbxCluster* Cluster = nullptr;
    int ClusterCount = Skin->GetClusterCount();
    for (int i = 0; i < ClusterCount; ++i)
    {
        if (Skin->GetCluster(i)->GetLink() == BoneNode)
        {
            Cluster = Skin->GetCluster(i);
            break;
        }
    }

    if (!Cluster)
    {
        Cluster = FbxCluster::Create(Scene, "");
        Cluster->SetLink(BoneNode);
        Cluster->SetLinkMode(FbxCluster::eTotalOne);
        Skin->AddCluster(Cluster);
    }

    FbxAMatrix MeshTransform = MeshNode->EvaluateGlobalTransform();
    FbxAMatrix BoneTransform = BoneNode->EvaluateGlobalTransform();

    Cluster->SetTransformMatrix(MeshTransform);
    Cluster->SetTransformLinkMatrix(BoneTransform);

    for (int i = 0; i < VertexIndices.Num(); ++i)
    {
        if (i < Weights.Num())
        {
            Cluster->AddControlPointIndex(VertexIndices[i], Weights[i]);
        }
    }

    return true;
}

FbxVector4 UFbxSdkReader::GetMeshPosition(const char* meshNamePattern)
{
    if (!Scene || !RootNode)
    {
        UE_LOG(LogTemp, Error, TEXT("Scene or RootNode is null"));
        return FbxVector4(0, 0, 0, 1);
    }

    FbxNode* meshNode = FindNode(meshNamePattern);
    if (meshNode && meshNode->GetNodeAttribute() && 
        meshNode->GetNodeAttribute()->GetAttributeType() == FbxNodeAttribute::eMesh)
    {
        FbxMesh* mesh = static_cast<FbxMesh*>(meshNode->GetNodeAttribute());
        if (!mesh)
        {
            return FbxVector4(0, 0, 0, 1);
        }

        // 获取世界变换矩阵
        FbxAMatrix globalTransform = meshNode->EvaluateGlobalTransform();

        // 计算模型的边界框
        FbxVector4 min(DBL_MAX, DBL_MAX, DBL_MAX);
        FbxVector4 max(-DBL_MAX, -DBL_MAX, -DBL_MAX);
        
        int vertexCount = mesh->GetControlPointsCount();
        FbxVector4* controlPoints = mesh->GetControlPoints();

        for (int i = 0; i < vertexCount; i++)
        {
            FbxVector4 vertex = globalTransform.MultT(controlPoints[i]);
            
            // 更新边界框
            for (int j = 0; j < 3; j++)
            {
                if (vertex[j] < min[j]) min[j] = vertex[j];
                if (vertex[j] > max[j]) max[j] = vertex[j];
            }
        }

        // 计算中心点
        FbxVector4 center = (min + max) * 0.5;

        UE_LOG(LogTemp, Warning, TEXT("Found mesh: %s at position (%f, %f, %f)"), 
            UTF8_TO_TCHAR(meshNode->GetName()),
            center[0], center[1], center[2]);

        return center;
    }

    UE_LOG(LogTemp, Warning, TEXT("No matching mesh found for pattern: %s"), UTF8_TO_TCHAR(meshNamePattern));
    return FbxVector4(0, 0, 0, 1);
}

void UFbxSdkReader::SaveSceneBoneTransforms()
{
    if (!Scene || !RootNode)
    {
        return;
    }

    SavedBoneTransforms.Empty();

    // 遍历所有骨骼节点并保存它们的全局变换
    for (int i = 0; i < RootNode->GetChildCount(); i++)
    {
        FbxNode* childNode = RootNode->GetChild(i);
        if (childNode && childNode->GetNodeAttribute() && 
            childNode->GetNodeAttribute()->GetAttributeType() == FbxNodeAttribute::eSkeleton)
        {
            SavedBoneTransforms.Add(childNode->EvaluateGlobalTransform());
        }
    }
}

void UFbxSdkReader::RestoreSceneBoneTransforms()
{
    if (!Scene || !RootNode || SavedBoneTransforms.Num() == 0)
    {
        return;
    }

    int transformIndex = 0;
    // 遍历所有骨骼节点并恢复它们的全局变换
    for (int i = 0; i < RootNode->GetChildCount(); i++)
    {
        FbxNode* childNode = RootNode->GetChild(i);
        if (childNode && childNode->GetNodeAttribute() && 
            childNode->GetNodeAttribute()->GetAttributeType() == FbxNodeAttribute::eSkeleton)
        {
            if (transformIndex < SavedBoneTransforms.Num())
            {
                // 从全局变换计算局部变换
                FbxNode* parentNode = childNode->GetParent();
                FbxAMatrix parentGlobal = parentNode ? parentNode->EvaluateGlobalTransform() : FbxAMatrix();
                FbxAMatrix localTransform = parentGlobal.Inverse() * SavedBoneTransforms[transformIndex];

                // 设置局部变换
                FbxVector4 translation = localTransform.GetT();
                FbxVector4 rotation = localTransform.GetR();
                FbxVector4 scaling = localTransform.GetS();

                childNode->LclTranslation.Set(FbxDouble3(translation[0], translation[1], translation[2]));
                childNode->LclRotation.Set(FbxDouble3(rotation[0], rotation[1], rotation[2]));
                childNode->LclScaling.Set(FbxDouble3(scaling[0], scaling[1], scaling[2]));

                transformIndex++;
            }
        }
    }

    // 更新场景
    Scene->GetRootNode()->EvaluateGlobalTransform();
    FbxAnimEvaluator* evaluator = Scene->GetAnimationEvaluator();
    evaluator->Reset();
}
