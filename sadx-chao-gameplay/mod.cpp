#include "stdafx.h"
#include "mod.h"

ChaoHandle	ChaoMaster;
NJS_VECTOR	bombpos;
float		bombsize;

//Chao selection functions
ChaoData* GetChaoData(uint8_t id) {
	return (ChaoData *)(GetChaoSaveAddress() + 2072 + (2048 * id));
}

char GetChaoByPointer(ObjectMaster* chao) {
	ChaoData1* chaodata = (ChaoData1*)chao->Data1;

	if (!chaodata) return 0;

	for (uint8_t i = 0; i < 24; ++i) {
		ChaoData* tempdata = GetChaoData(i);
		if (chaodata->ChaoDataBase_ptr
			&& tempdata->data.Lifespan == chaodata->ChaoDataBase_ptr->Lifespan
			&& tempdata->data.DNA.FavoriteFruit1 == chaodata->ChaoDataBase_ptr->DNA.FavoriteFruit1
			&& tempdata->data.Energy == chaodata->ChaoDataBase_ptr->Energy) {
			return i + 1;
		}
	}

	return 0;
}

void SelectChao(char player) {
	CharObj2 * co2 = GetCharObj2(player);

	if (!co2) return;

	if (co2->ObjectHeld != nullptr) {
		if (ChaoMaster.ChaoHandles[player].SelectedChao == NULL) {
			ChaoMaster.ChaoHandles[player].SelectedChao = GetChaoByPointer(co2->ObjectHeld);
			ChaoMaster.ChaoHandles[player].Carried = true;
		}
	}
	else if (ChaoMaster.ChaoHandles[player].Chao && 
		ChaoMaster.ChaoHandles[player].Chao->Data1->Action == ChaoAction_Flight) {
		ChaoMaster.ChaoHandles[player].SelectedChao = GetChaoByPointer(ChaoMaster.ChaoHandles[player].Chao);
		ChaoMaster.ChaoHandles[player].Carried = true;
	}
	else {
		ChaoMaster.ChaoHandles[player].SelectedChao = NULL;
	}
}

//Add Chao to the enemies damage detection
bool OhNoImDead2(EntityData1* a1, ObjectData2* a2);
Trampoline OhNoImDead2_t(0x004CE030, 0x004CE036, OhNoImDead2);
bool OhNoImDead2(EntityData1* a1, ObjectData2* a2) {
	if (a1->CollisionInfo->CollidingObject) {
		if (a1->CollisionInfo->CollidingObject->Object->MainSub == Chao_Main
			&& a1->CollisionInfo->CollidingObject->Object->Data1->Action == ChaoAction_Attack) return 1;
	}

	if (bombsize && GetDistance(&bombpos, &a1->Position) < bombsize) {
		bombsize = 0;
		return 1;
	}
	
	FunctionPointer(bool, original, (EntityData1 * a1, ObjectData2 * a2), OhNoImDead2_t.Target());
	return original(a1, a2);
}

void KillEnemiesInSphere(NJS_VECTOR* pos, float radius) {
	bombpos = *pos;
	bombsize = radius;
}

void LoadNextChaoStage_r();
Trampoline LoadNextChaoStage_t((int)LoadNextChaoStage, (int)LoadNextChaoStage + 0x5, LoadNextChaoStage_r);
void LoadNextChaoStage_r() {
	for (char p = 0; p < 8; ++p) {
		ChaoMaster.ChaoHandles[p].SelectedChao = 0;
	}

	ChaoMaster.ChaoLoaded = false;

	VoidFunc(original, LoadNextChaoStage_t.Target());
	original();
}

int GetCurrentChaoStage_r() {
	if (CurrentLevel < LevelIDs_SSGarden) return 5;
	else return CurrentChaoStage;
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
		//Check the held chao as the player leave the garden or field
		//Use those as the selected chao
		if (GameState == 9) {
			if (CurrentLevel >= LevelIDs_StationSquare) {
				for (char p = 0; p < 8; ++p) {
					SelectChao(p);
				}
			}
		}

		if (GameState != 4 && GameState != 15 && GameState != 16) {
			ChaoMaster.ChaoLoaded = false;
		}
		
		//Load Chao at the beginning of levels or fields
		if ((GameState == 4 || GameState == 2) && !IsLevelChaoGarden() && ChaoMaster.ChaoLoaded == false) {
			for (char p = 0; p < 8; ++p) {

				if (p == 0) ChaoMaster.ChaoHandles[p].SelectedChao = 1;
				if (p == 1) ChaoMaster.ChaoHandles[p].SelectedChao = 2;

				if (ChaoMaster.ChaoHandles[p].Handle) {
					continue;
				}

				if (ChaoMaster.ChaoHandles[p].SelectedChao) {
					if (!EntityData1Ptrs[p]) {
						ChaoMaster.ChaoHandles[p].SelectedChao = 0;
						continue;
					}

					ChaoMaster.ChaoHandles[p].Handle = LoadObject((LoadObj)(LoadObj_Data1), 1, ChaoObj_Main);
					ChaoMaster.ChaoHandles[p].Handle->Data1->CharIndex = p;
				}
			}

			ChaoMaster.ChaoLoaded = true;
		}
	}

	__declspec(dllexport) ModInfo SADXModInfo = { ModLoaderVer };
}