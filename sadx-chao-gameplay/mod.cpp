#include "stdafx.h"

ObjectMaster *ChaoObject;
ObjectMaster *CurrentChao; // esi

uint8_t SelectedChao;
bool isloaded = false;

int GetCurrentChaoStage_r() {
	if (ChaoObject)
		return 4;
	else 
		return CurrentChaoStage;
}

#pragma region Own Chao Object
void ChaoObj_Display(ObjectMaster * a1) {

}

void ChaoObj_Main(ObjectMaster * a1) {
	if (a1->Data1->Action == 0) {
		ChaoData * chaodata = (ChaoData *)(GetChaoSaveAddress() + 2072 + (2048 * (SelectedChao - 1))); //get chao data
		CreateChao(chaodata, 0, CurrentChao, &EntityData1Ptrs[0]->Position, 0);
		a1->Data1->Action = 1;
	}

	ChaoObj_Display(a1);
}

void ChaoObj_Delete(ObjectMaster * a1) {
	DeleteObjectMaster(ChaoManager);
	ChaoManager = nullptr;
	DeleteObjectMaster(CurrentChao);
	CurrentChao = nullptr;
	ChaoObject = nullptr;
	FreeChaoTexlists();
}

void ChaoObj_Init(ObjectMaster * a1) {
	LoadChaoTexlist("AL_DX_PARTS_TEX", (NJS_TEXLIST*)0x33A1340, 0);
	LoadChaoTexlist("AL_BODY", ChaoTexLists, 0);
	LoadChaoTexlist("AL_jewel", &ChaoTexLists[4], 0);
	LoadChaoTexlist("AL_ICON", &ChaoTexLists[3], 0);
	LoadChaoTexlist("AL_EYE", &ChaoTexLists[2], 0);
	LoadChaoTexlist("AL_MOUTH", &ChaoTexLists[5], 0);
	LoadChaoTexlist("AL_TEX_COMMON", &ChaoTexLists[1], 1u);

	if (!isloaded) {
		al_confirmload_load();
		LoadChaoPVPs();
		isloaded = true;
	}

	ChaoManager_Load(); //load chao behaviour

	a1->DisplaySub = ChaoObj_Display;
	a1->MainSub = ChaoObj_Main; //everyframe except when game paused
	a1->DeleteSub = ChaoObj_Delete; //when you quit a level
}
#pragma endregion

extern "C"
{
	__declspec(dllexport) void __cdecl Init(const char *path)
	{
		const IniFile *config = new IniFile(std::string(path) + "\\config.ini");
		delete config;

		WriteJump((void*)0x715140, GetCurrentChaoStage_r);
	}

	__declspec(dllexport) void __cdecl OnFrame()
	{
		SelectedChao = 1;

		if (GameState == 4 && SelectedChao && !ChaoObject && !IsLevelChaoGarden()) 
			ChaoObject = LoadObject((LoadObj)(LoadObj_Data1), 1, ChaoObj_Init);
	}
	
	__declspec(dllexport) ModInfo SADXModInfo = { ModLoaderVer };
}