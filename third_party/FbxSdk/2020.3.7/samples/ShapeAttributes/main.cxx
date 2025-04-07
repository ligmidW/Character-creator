/****************************************************************************************

Copyright (C) 2019 Autodesk, Inc.
All rights reserved.

Use of this software is subject to the terms of the Autodesk license agreement
provided at the time of installation or download, or which otherwise accompanies
this software in either electronic or hard copy form.

****************************************************************************************/


#include <fbxsdk.h>

#include "../Common/Common.h"

#define SAMPLE_FILENAME "ShapeAttributes.fbx"

/////////////////////////////////////////////////////////////////////////
//
// The scene created in this example is a grid with a blend shape setup 
// containing modified control points, vertex colors and normals on shapes. 
// It shows the differences between legacy style FbxShape and modern style FbxShape 
// and how it handles FbxLayerElement.
// For modern style FbxShape it also shows the difference between absolute and 
// relative value persistence.
//
// This sample illustrates how to use the following:
//      - FbxShape
//      - FbxBlendShape
//      - Normals
//      - Vertex Color on Shape
//
// Remarks: Once you run this sample inspect the content of the file ShapeAttributes.fbx
// look at the different shapes.
//
/////////////////////////////////////////////////////////////////////////

FbxMesh* CreateGrid(FbxScene* pScene, const char* pName);
FbxBlendShapeChannel* SetupBlendShapeDeformer(FbxManager* pManager, FbxMesh* pBaseGeometry);
FbxShape* CreateGridShape(FbxScene* pScene, FbxMesh* pBaseGeometry, const char* pName, bool pLegacy, bool pAbsolute = false);

// ================================================================================================================
int main(int argc, char** argv)
{
	FbxManager* lSdkManager = NULL;
	FbxScene* lScene = NULL;
	bool lResult = true;

	// Prepare the FBX SDK.
	InitializeSdkObjects(lSdkManager, lScene);

	// Create the scene.
	FbxMesh* lBaseGeometry = CreateGrid(lScene, "BaseGeometry");
	FbxBlendShapeChannel* lBlendShapeChannel = SetupBlendShapeDeformer(lSdkManager, lBaseGeometry);
	lBlendShapeChannel->AddTargetShape(CreateGridShape(lScene, lBaseGeometry, "LegacyShape", true, false));
	lBlendShapeChannel->AddTargetShape(CreateGridShape(lScene, lBaseGeometry, "ModernShape", false, false));
	lBlendShapeChannel->AddTargetShape(CreateGridShape(lScene, lBaseGeometry, "ModernAbsoluteShape", false, true));

	// Save the scene.

	// The example can take an output file name as an argument.
	const char* lSampleFileName = NULL;
	for (int i = 1; i < argc; ++i)
	{
		if (FBXSDK_stricmp(argv[i], "-test") == 0) continue;
		else if (!lSampleFileName) lSampleFileName = argv[i];
	}
	if (!lSampleFileName) lSampleFileName = SAMPLE_FILENAME;

	lResult = SaveScene(lSdkManager, lScene, lSampleFileName);
	if (lResult == false)
	{
		FBXSDK_printf("\n\nAn error occurred while saving the scene...\n");
		DestroySdkObjects(lSdkManager, lResult);
		return 0;
	}

	// Destroy all objects created by the FBX SDK.
	DestroySdkObjects(lSdkManager, lResult);

	return 0;
}


FbxMesh* CreateGrid(FbxScene* pScene, const char* pName)
{
	FbxMesh* lMesh = FbxMesh::Create(pScene, pName);

	static const int lNbControlPoints = 6;
	static const int lNbPolygonVertices = 8;
	// Create control points.
	lMesh->InitControlPoints(lNbControlPoints);
	FbxVector4* lControlPoints = lMesh->GetControlPoints();
	lControlPoints[0].Set(- 50, 0, 0);
	lControlPoints[1].Set(0, 0, 0);
	lControlPoints[2].Set(50, 0, 0);
	lControlPoints[3].Set(50, 0, -50);
	lControlPoints[4].Set(0, 0, -50);
	lControlPoints[5].Set(-50, 0, -50);

	//0---1---2
	//|   |   |
	//5---4---3

	lMesh->BeginPolygon(); 
	lMesh->AddPolygon(0); // Control point index.
	lMesh->AddPolygon(1); // Control point index.
	lMesh->AddPolygon(4); // Control point index.
	lMesh->AddPolygon(5); // Control point index.
	lMesh->EndPolygon();

	lMesh->BeginPolygon();
	lMesh->AddPolygon(1); // Control point index.
	lMesh->AddPolygon(2); // Control point index.
	lMesh->AddPolygon(3); // Control point index.
	lMesh->AddPolygon(4); // Control point index.
	lMesh->EndPolygon();

	// We want to have one normal for each PolygonVertex
	// so we set the mapping mode to eByPolygonVertex.
	FbxGeometryElementNormal* lGeometryElementNormal = lMesh->CreateElementNormal();
	lGeometryElementNormal->SetMappingMode(FbxGeometryElement::eByPolygonVertex);

	// Set the normal values for every control point.
	lGeometryElementNormal->SetReferenceMode(FbxGeometryElement::eDirect);
	FbxVector4 lNormalYUp(0, 1, 0);
	FbxLayerElementArrayTemplate<FbxVector4>& lNormals = lGeometryElementNormal->GetDirectArray();
	for (int i = 0; i < lNbPolygonVertices; ++i)
	{
		lNormals.Add(lNormalYUp);
	}


	// Add Vertex Color
	FbxGeometryElementVertexColor* lLEVC = lMesh->CreateElementVertexColor();
	lLEVC->SetMappingMode(FbxLayerElement::eByPolygonVertex);
	lLEVC->SetReferenceMode(FbxLayerElement::eIndexToDirect);
	FbxLayerElementArrayTemplate<FbxColor>& lColors = lLEVC->GetDirectArray();
	FbxLayerElementArrayTemplate<int>& lColorIndices = lLEVC->GetIndexArray();
	FbxColor lRed(1, 0, 0);
	for (int i = 0; i < lNbPolygonVertices; ++i)
	{
		lColors.Add(lRed);
		lColorIndices.Add(i);
	}
	
	// create a FbxNode
	FbxNode* lNode = FbxNode::Create(pScene, pName);

	// set the node attribute
	lNode->SetNodeAttribute(lMesh);

	// Build the node tree.
	FbxNode* lRootNode = pScene->GetRootNode();
	lRootNode->AddChild(lNode);
	lNode->SetShadingMode(FbxNode::eTextureShading);
	
	// return the FbxNode
	return lMesh;
}

FbxShape* CreateGridShape(FbxScene* pScene, FbxMesh* pBaseGeometry, const char* pName, bool pLegacy, bool pAbsolute)
{
	FbxShape* lShape = FbxShape::Create(pScene, pName);

	//Set the shape with the passed in flags.
	lShape->SetLegacyStyle(pLegacy);
	lShape->SetAbsoluteMode(pAbsolute);

	//Copy control points position from base shape
	int lNbControlPoints = pBaseGeometry->GetControlPointsCount();
	lShape->InitControlPoints(lNbControlPoints);
	for (int i = 0; i < lNbControlPoints; ++i)
	{
		lShape->SetControlPointAt(pBaseGeometry->GetControlPointAt(i), i);
	}

	//Modify a control point
	lShape->SetControlPointAt(FbxVector4(-75,0,0), 0);

	//Modify some normals
	FbxGeometryElementNormal* lGeometryElementNormal = lShape->CreateElementNormal();
	lGeometryElementNormal->SetMappingMode(FbxGeometryElement::eByPolygonVertex);
	// Set the normal values for only modified control points
	lGeometryElementNormal->SetReferenceMode(FbxGeometryElement::eIndexToDirect);
	FbxLayerElementArrayTemplate<FbxVector4>& lNormals = lGeometryElementNormal->GetDirectArray();
	lNormals.Add(FbxVector4(0.5, 1, 0.5));
	lNormals.Add(FbxVector4(0, 0, 1));

	FbxLayerElementArrayTemplate<int>& lNormalIndices = lGeometryElementNormal->GetIndexArray();
	lNormalIndices.Add(0);
	lNormalIndices.Add(-1);
	lNormalIndices.Add(-1);
	lNormalIndices.Add(1);
	lNormalIndices.Add(-1);
	lNormalIndices.Add(-1);
	lNormalIndices.Add(-1);
	lNormalIndices.Add(-1);

	//Modify some vertex color
	FbxGeometryElementVertexColor* lLEVC = lShape->CreateElementVertexColor();
	lLEVC->SetMappingMode(FbxLayerElement::eByPolygonVertex);
	lLEVC->SetReferenceMode(FbxLayerElement::eIndexToDirect);
	FbxLayerElementArrayTemplate<FbxColor>& lColors = lLEVC->GetDirectArray();
	FbxColor lGreen(0, 1, 0);
	lColors.Add(lGreen);

	FbxLayerElementArrayTemplate<int>& lColorIndicies = lLEVC->GetIndexArray();
	lColorIndicies.Add(-1);
	lColorIndicies.Add(0);
	lColorIndicies.Add(0);
	lColorIndicies.Add(-1);
	lColorIndicies.Add(-1);
	lColorIndicies.Add(0);
	lColorIndicies.Add(-1);
	lColorIndicies.Add(-1);
	
	
	return lShape;
}

FbxBlendShapeChannel* SetupBlendShapeDeformer(FbxManager* pManager, FbxMesh* pBaseGeometry)
{
	//Create the BlendShape deformer, binds it to the geometry and set a blend shape channel.
	FbxBlendShapeChannel* lBlendShapeChannel = FbxBlendShapeChannel::Create(pManager, "SampleShapeChannel");
	FbxBlendShape* lBlendShape = FbxBlendShape::Create(pManager, "SampleBlendShape");
	pBaseGeometry->AddDeformer(lBlendShape);
	lBlendShape->AddBlendShapeChannel(lBlendShapeChannel);
	return lBlendShapeChannel;
}
