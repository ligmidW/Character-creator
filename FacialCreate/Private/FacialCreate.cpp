// Copyright Epic Games, Inc. All Rights Reserved.

#include "FacialCreate.h"
#include "Misc/MessageDialog.h"
#include "ToolMenus.h"
#include "FbxFileLoad.h"


#include "ContentBrowserModule.h"
#include "LevelEditor.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "IContentBrowserSingleton.h"

#define LOCTEXT_NAMESPACE "FFacialCreateModule"
static const FName FacialCreateTabName("FacialCreate");
void FFacialCreateModule::StartupModule()
{
	RegisterContentBrowserMenuExtender();
}

void FFacialCreateModule::ShutdownModule()
{
	UnregisterContentBrowserMenuExtender();
}

void FFacialCreateModule::RegisterContentBrowserMenuExtender()
{
	UToolMenus* ToolMenus = UToolMenus::Get();


	if (!ToolMenus)
	{
		UE_LOG(LogTemp, Warning, TEXT("Invalid ToolMenus or not in Game folder"));
		return;
	}

	UToolMenu* Menu = ToolMenus->ExtendMenu("ContentBrowser.AddNewContextMenu");

	FToolMenuSection& Section = Menu->FindOrAddSection("BasicOperations");

	Section.AddSubMenu(
		"ImportFacialRig",
		TAttribute<FText>(LOCTEXT("ImportFacialRig_Label", "Import Facial Rig")),
		TAttribute<FText>(LOCTEXT("ImportFacialRig_Tooltip", "Import facial rig data")),
		FNewToolMenuDelegate::CreateLambda([](UToolMenu* SubMenu)
			{
				FToolMenuSection& SubMenuSection = SubMenu->AddSection("FacialRigImportSection",
					LOCTEXT("FacialRigImportSection", "Facial Rig Import Options"));

				SubMenuSection.AddMenuEntry(
					"ImportFromFBX",
					LOCTEXT("ImportFromFBX_Label", "Import from FBX"),
					LOCTEXT("ImportFromFBX_Tooltip", "Import facial rig from FBX file"),
					FSlateIcon(),
					FUIAction(FExecuteAction::CreateLambda([]()
						{
							FbxFileLoad::FbxFileLoad();
							
						}))
				);

				SubMenuSection.AddMenuEntry(
					"ImportFromJSON",
					LOCTEXT("ImportFromJSON_Label", "Import from DNA"),
					LOCTEXT("ImportFromJSON_Tooltip", "Import facial rig from JSON file"),
					FSlateIcon(),
					FUIAction(FExecuteAction::CreateLambda([]()
						{
						}))
				);

			}),
		false,
		FSlateIcon()
	);
}
void FFacialCreateModule::UnregisterContentBrowserMenuExtender()
{
	UToolMenus* ToolMenus = UToolMenus::Get();
	if (!ToolMenus)
	{
		return;
	}

	ToolMenus->RemoveSection("ContentBrowser.AddNewContextMenu", "BasicOperations");
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FFacialCreateModule, FacialCreate)