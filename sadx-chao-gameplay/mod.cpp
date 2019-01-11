#include "stdafx.h"

ObjectMaster *ChaoObject;
ObjectMaster *CurrentChao;

uint8_t SelectedChao = 0;
bool isloaded = false;

#pragma region Own Chao Object
void ChaoObj_Main(ObjectMaster * a1) {
	if (a1->Data1->Action == 0) {
		ChaoData * chaodata = (ChaoData *)(GetChaoSaveAddress() + 2072 + (2048 * (SelectedChao - 1))); //get chao data

		NJS_VECTOR v = EntityData1Ptrs[0]->Position;
		v.x -= 20;

		CurrentChao = CreateChao(chaodata, 0, CurrentChao, &v, 0);
		if (EntityData1Ptrs[0]->Action != 12) SetHeldObject(0, CurrentChao);

		a1->Data1->Action = 1;
	}
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

	a1->MainSub = ChaoObj_Main; //everyframe except when game paused
	a1->DeleteSub = ChaoObj_Delete; //when you quit a level
}
#pragma endregion

void SelectChao() {
	CharObj2 * co2 = GetCharObj2(0);
	if (!co2) return;

	if (co2->ObjectHeld != nullptr) {
		if (SelectedChao == 0) {
			ObjectMaster * chao = co2->ObjectHeld;
			ChaoData1 * chaodata = (ChaoData1 *)chao->Data1;

			if (chaodata->entity.CollisionInfo->CollisionArray->origin.y == 2) {
				for (uint8_t i = 0; i < 24; ++i) {
					ChaoData * tempdata = (ChaoData *)(GetChaoSaveAddress() + 2072 + (2048 * i));
					if (tempdata->data.Lifespan == chaodata->ChaoDataBase_ptr->Lifespan
						&& tempdata->data.DNA.FavoriteFruit1 == chaodata->ChaoDataBase_ptr->DNA.FavoriteFruit1
						&& tempdata->data.Energy == chaodata->ChaoDataBase_ptr->Energy) {
						SelectedChao = i + 1;
					}
				}
			}
		}
	}
	else {
		if (GameState == 15) SelectedChao = 0;
	}
}

int GetCurrentChaoStage_r() {
	if (ChaoObject) return 4;
	else return CurrentChaoStage;
}

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
		if ((CurrentLevel > 25 && CurrentLevel < 35) || IsLevelChaoGarden()) SelectChao();

		if ((GameState == 4 || GameState == 2) && SelectedChao && !ChaoObject && !IsLevelChaoGarden())
			ChaoObject = LoadObject((LoadObj)(LoadObj_Data1), 1, ChaoObj_Init);
	}

	__declspec(dllexport) ModInfo SADXModInfo = { ModLoaderVer };
}