// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class FToolBarBuilder;
class FMenuBuilder;

class FFacialCreateModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	
private:
	void RegisterContentBrowserMenuExtender();

	void UnregisterContentBrowserMenuExtender();


private:
	TSharedPtr<class FUICommandList> PluginCommands;
};
