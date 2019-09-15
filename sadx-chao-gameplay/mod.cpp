#include "stdafx.h"
#include <vector>

ObjectMaster *ChaoObject;
ObjectMaster *CurrentChao;

uint8_t SelectedChao = 0;
bool isloaded = false;

std::vector<NJS_PLANE> waterlist = {};

int GetCurrentChaoStage_r() {
	if (ChaoObject) return 5;
	else return CurrentChaoStage;
}

inline ChaoData* GetChaoData(uint8_t id) {
	return (ChaoData *)(GetChaoSaveAddress() + 2072 + (2048 * id));
}

void SelectChao() {
	CharObj2 * co2 = GetCharObj2(0);
	if (!co2) return;

	if (co2->ObjectHeld != nullptr) {
		if (SelectedChao == 0) {
			ObjectMaster * chao = co2->ObjectHeld;
			ChaoData1 * chaodata = (ChaoData1 *)chao->Data1;

			if (chaodata->entity.CollisionInfo->CollisionArray->origin.y == 2) {
				for (uint8_t i = 0; i < 24; ++i) {
					ChaoData * tempdata = GetChaoData(i);
					if (chaodata->ChaoDataBase_ptr
						&& tempdata->data.Lifespan == chaodata->ChaoDataBase_ptr->Lifespan
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

void GetWaterCollisions() {
	waterlist.clear();
	for (int i = 0; i < CurrentLandTable->COLCount; ++i) {
		if (CurrentLandTable->Col[i].Flags & ColFlags_Water) {

			float x_min = CurrentLandTable->Col[i].Center.x - CurrentLandTable->Col[i].Radius;
			float z_min = CurrentLandTable->Col[i].Center.z - CurrentLandTable->Col[i].Radius;
			float x_max = CurrentLandTable->Col[i].Center.x + CurrentLandTable->Col[i].Radius;
			float z_max = CurrentLandTable->Col[i].Center.z + CurrentLandTable->Col[i].Radius;
			float y = CurrentLandTable->Col[i].Center.y;

			NJS_PLANE temp = { x_min, y, z_min, x_max, 0, z_max };
			waterlist.push_back(temp);
		}
	}
}

void IsChaoInWater(ObjectMaster * a1) {
	ChaoData1 * chaodata1 = (ChaoData1*)a1->Data1;
	ChaoData2 * chaodata2 = (ChaoData2*)a1->Data2;

	float height = -10000000;
	if (waterlist.size() > 0) {
		NJS_VECTOR pos = chaodata1->entity.Position;
		NJS_PLANE wpos;
		for (int i = 0; i < waterlist.size(); ++i) {
			wpos = waterlist[i];
			if (pos.y < wpos.py + 2 && pos.y > wpos.py - 170) {
				if (pos.x > wpos.px && pos.x < wpos.vx) {
					if (pos.z > wpos.pz && pos.z < wpos.vz) {
						height = wpos.py;
					}
				}
			}
		}
	}

	WriteData((float*)0x73C24C, height);
}

void ChaoObj_Delete(ObjectMaster * a1) {
	DeleteObjectMaster(ChaoManager);
	ChaoManager = nullptr;

	DeleteObjectMaster(CurrentChao);
	CurrentChao = nullptr;
	ChaoObject = nullptr;

	//Release the chao textures
	FreeChaoTexlists();

	//reset default water height
	float height = 0;
	WriteData((float*)0x73C24C, height);
}

void ChaoObj_Main(ObjectMaster * a1) {
	uint8_t Action = a1->Data1->Action;
	
	if (Action == 0) {
		//We wait a bit before loading chao stuff
		if (!CurrentLandTable) return;

		//Load the chao textures
		LoadChaoTexlist("AL_DX_PARTS_TEX", (NJS_TEXLIST*)0x33A1340, 0);
		LoadChaoTexlist("AL_BODY", ChaoTexLists, 0);
		LoadChaoTexlist("AL_jewel", &ChaoTexLists[4], 0);
		LoadChaoTexlist("AL_ICON", &ChaoTexLists[3], 0);
		LoadChaoTexlist("AL_EYE", &ChaoTexLists[2], 0);
		LoadChaoTexlist("AL_MOUTH", &ChaoTexLists[5], 0);
		LoadChaoTexlist("AL_TEX_COMMON", &ChaoTexLists[1], 1u);

		//PVPs only need to be loaded once
		if (!isloaded) {
			al_confirmload_load();
			LoadChaoPVPs();
			isloaded = true;
		}

		ChaoManager_Load(); //Load chao behaviour
		GetWaterCollisions(); //Hacky solution to make chao swim

		ActCopy = CurrentAct;

		a1->DeleteSub = ChaoObj_Delete; //When you quit a level
		a1->Data1->Action = 1; //Wait a frame before loading a chao
	}
	else if (Action == 1) {
		//We get the chao data in the savefile
		ChaoData* chaodata = GetChaoData(SelectedChao - 1);

		//Start position is behind the player
		NJS_VECTOR v = EntityData1Ptrs[0]->Position;
		v.x -= 20;

		//Load the chao
		CurrentChao = CreateChao(chaodata, 0, CurrentChao, &v, 0);
		if (EntityData1Ptrs[0]->Action != 12) SetHeldObject(0, CurrentChao);

		a1->Data1->Action = 2;
	}
	else {
		//If the act has changed, check water collisions again
		if (ActCopy != CurrentAct) {
			GetWaterCollisions();
			ActCopy = CurrentAct;

			//Fix a bug at Emerald Coast act swap by reloading the chao
			if (CurrentLevel == LevelIDs_EmeraldCoast && CurrentAct == 1) {
				a1->DeleteSub(a1);
				a1->Data1->Action = 0;
				return;
			}
		}

		IsChaoInWater(CurrentChao);
	}
}

extern "C"
{
	__declspec(dllexport) void __cdecl Init(const char *path)
	{
		const IniFile *config = new IniFile(std::string(path) + "\\config.ini");
		delete config;

		//Trick the game into thinking we're in a specific chao garden
		//Needed to change the water height
		WriteJump((void*)0x715140, GetCurrentChaoStage_r);
	}

	__declspec(dllexport) void __cdecl OnFrame()
	{
		if ((CurrentLevel >= LevelIDs_StationSquare && CurrentLevel <= LevelIDs_Past) || IsLevelChaoGarden()) SelectChao();

		if ((GameState == 4 || GameState == 2) && SelectedChao && !ChaoObject && !IsLevelChaoGarden())
			ChaoObject = LoadObject((LoadObj)(LoadObj_Data1), 1, ChaoObj_Main);
	}

	__declspec(dllexport) ModInfo SADXModInfo = { ModLoaderVer };
}