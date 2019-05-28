#include "stdafx.h"
#include <vector>

ObjectMaster *ChaoObject;
ObjectMaster *CurrentChao;

uint8_t SelectedChao = 0;
bool isloaded = false;

std::vector<NJS_PLANE> waterlist = {};
bool checkwater = false;

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

int GetCurrentChaoStage_r() {
	if (ChaoObject) return 5;
	else return CurrentChaoStage;
}

void __cdecl InitLandTableObject_r(NJS_OBJECT *a1)
{
	checkwater = false;

	NJS_OBJECT *v1 = a1;
	do
	{
		Uint32 v2 = v1->evalflags;
		Uint32 v6 = v1->evalflags;
		if (!(v1->evalflags & 0x8))
		{
			NJS_MODEL_SADX *v3 = v1->basicdxmodel;
			if (v3)
			{
				int v4 = v3->nbMeshset;
				NJS_MESHSET_SADX *v5 = v3->meshsets;
				if (v3->nbMeshset)
				{
					do
					{
						InitLandTableMeshSet(v3, v5);
						++v5;
						--v4;
					} while (v4);
					v2 = v6;
				}
			}
		}
		if (!(v2 & 0x10))
		{
			InitLandTableObject(v1->child);
		}
		v1 = v1->sibling;
	} while (v1);
}

void Chao_SwimInWater(ObjectMaster * a1) {
	ChaoData1 * chaodata1 = (ChaoData1*)a1->Data1;
	ChaoData2 * chaodata2 = (ChaoData2*)a1->Data2;

	if (!checkwater) {
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
		checkwater = true;
	}
	else {
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
}

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

	Chao_SwimInWater(CurrentChao); //allow chaos to swim in any water colision
}

void ChaoObj_Delete(ObjectMaster * a1) {
	DeleteObjectMaster(ChaoManager);
	ChaoManager = nullptr;
	DeleteObjectMaster(CurrentChao);
	CurrentChao = nullptr;
	ChaoObject = nullptr;
	FreeChaoTexlists();
	float height = 0;
	WriteData((float*)0x73C24C, height);
	checkwater = false;
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

extern "C"
{
	__declspec(dllexport) void __cdecl Init(const char *path)
	{
		const IniFile *config = new IniFile(std::string(path) + "\\config.ini");
		delete config;

		WriteJump((void*)0x715140, GetCurrentChaoStage_r);
		WriteJump((void*)0x787140, InitLandTableObject_r);
	}

	__declspec(dllexport) void __cdecl OnFrame()
	{
		if ((CurrentLevel >= LevelIDs_StationSquare && CurrentLevel <= LevelIDs_Past) || IsLevelChaoGarden()) SelectChao();

		if ((GameState == 4 || GameState == 2) && SelectedChao && !ChaoObject && !IsLevelChaoGarden())
			ChaoObject = LoadObject((LoadObj)(LoadObj_Data1), 1, ChaoObj_Init);
	}

	__declspec(dllexport) ModInfo SADXModInfo = { ModLoaderVer };
}