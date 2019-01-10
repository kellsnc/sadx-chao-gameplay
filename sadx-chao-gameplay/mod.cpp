#include "stdafx.h"
#include <SADXModLoader.h>
#include "IniFile.hpp"

extern "C"
{
	__declspec(dllexport) void __cdecl Init(const char *path)
	{
		const IniFile *config = new IniFile(std::string(path) + "\\config.ini");
		delete config;
	}

	__declspec(dllexport) void __cdecl OnFrame()
	{
		
	}
	
	__declspec(dllexport) ModInfo SADXModInfo = { ModLoaderVer };
}