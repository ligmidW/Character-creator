// Fill out your copyright notice in the Description page of Project Settings.


#include "FbxFileLoad.h"

#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
//#include "FbxReaderFFile.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/SWindow.h"
#include "FbxSdkReader.h"
#include "DesktopPlatformModule.h"
#include "IDesktopPlatform.h"
FbxFileLoad::FbxFileLoad()
{
	IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
	if (DesktopPlatform)
	{
		TArray<FString> OutFiles;
		void* ParentWindowHandle = nullptr;
		//get UE main window
		if (FSlateApplication::IsInitialized())
		{
			TSharedPtr<SWindow> MainWindow = FSlateApplication::Get().GetActiveTopLevelWindow();
			if (MainWindow.IsValid() && MainWindow->GetNativeWindow().IsValid())
			{
				ParentWindowHandle = MainWindow->GetNativeWindow()->GetOSWindowHandle();
			}
		}

		DesktopPlatform->OpenFileDialog(
			ParentWindowHandle,
			TEXT("Choose FBX file"),
			TEXT(""),
			TEXT(""),
			TEXT("FBX files (*.fbx)|*.fbx"),
			EFileDialogFlags::None,
			OutFiles
		);

		if (OutFiles.Num() > 0)
		{
			FString FilePath = OutFiles[0];
			UFbxSdkReader::ReadFbxFile(FilePath);
		}
	}
}

FbxFileLoad::~FbxFileLoad()
{
}
