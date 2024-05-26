// Copyright 2020-2021 Fly Dream Dev. All Rights Reserved.

#include "WorldDirectorNPC.h"

#define LOCTEXT_NAMESPACE "FWorldDirectorNPCModule"

void FWorldDirectorNPCModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
}

void FWorldDirectorNPCModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FWorldDirectorNPCModule, WorldDirectorNPC)