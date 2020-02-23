#include "stdafx.h"
#include "mod.h"

ChaoHandle	ChaoMaster;
NJS_VECTOR	bombpos;
float		bombsize;

bool ChaoPowerups = false;
bool ChaoAssist = true;
bool ChaoLuck = true;
bool ChaoHUD = true;

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

	if (ChaoMaster.ChaoHandles[player].Chao &&
		ChaoMaster.ChaoHandles[player].Handle->Data1->Action == ChaoAction_Flight) {
		ChaoMaster.ChaoHandles[player].SelectedChao = GetChaoByPointer(ChaoMaster.ChaoHandles[player].Chao);
		ChaoMaster.ChaoHandles[player].Carried = false;
	}
	else if (co2->ObjectHeld != nullptr) {
		if (ChaoMaster.ChaoHandles[player].SelectedChao == NULL) {
			ChaoMaster.ChaoHandles[player].SelectedChao = GetChaoByPointer(co2->ObjectHeld);
			ChaoMaster.ChaoHandles[player].Carried = true;
			if (CurrentLevel >= LevelIDs_SSGarden) ChaoMaster.LoadHUD = true;
		}
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

bool IsLevelChaoGarden_r() {
	for (char p = 0; p < PLAYERCOUNT; ++p) {
		if (ChaoMaster.ChaoHandles[p].Chao) return true;
	}

	return false;
}

bool IsLevelChaoGarden_orig() {
	if (CurrentLevel >= LevelIDs_SSGarden) return true;

	return false;
}

extern "C"
{
	__declspec(dllexport) void __cdecl Init(const char *path)
	{
		const IniFile *config = new IniFile(std::string(path) + "\\config.ini");
		ChaoPowerups = config->getBool("Functionalities", "EnablePowerups", false);
		ChaoAssist = config->getBool("Functionalities", "EnableChaoAssist", true);
		ChaoLuck = config->getBool("Functionalities", "EnableChaoLuck", true);
		ChaoHUD = config->getBool("Functionalities", "EnableHUD", true);
		delete config;
		
		//Trick the game into thinking we're in a specific chao garden
		//Needed to change the water height
		WriteJump(GetCurrentChaoStage, GetCurrentChaoStage_r);
		WriteJump(IsLevelChaoGarden, IsLevelChaoGarden_r);
		WriteCall((void*)0x40FDC0, IsLevelChaoGarden_orig);
	}

	__declspec(dllexport) void __cdecl OnFrame()
	{
		//Check the held chao as the player leave the garden or field
		//Use those as the selected chao
		if (GameState == 9) {
			if ((CurrentLevel >= LevelIDs_StationSquare && CurrentLevel <= LevelIDs_Past) || CurrentLevel >= LevelIDs_SSGarden) {
				for (char p = 0; p < 8; ++p) {
					SelectChao(p);
				}
			}

			return;
		}

		//Tell the system we can reload Chao
		if (GameState != 4 && GameState != 15 && GameState != 16) {
			ChaoMaster.ChaoLoaded = false;

			if (GameMode == GameModes_Menu) {
				for (char p = 0; p < PLAYERCOUNT; ++p) {
					ChaoMaster.ChaoHandles[p].SelectedChao = 0;
				}
			}

			return;
		}

		//Load Chao at the beginning of levels or fields
		if ((GameState == 4 || GameState == 2) && CurrentLevel < LevelIDs_SSGarden && ChaoMaster.ChaoLoaded == false) {
			for (char p = 0; p < PLAYERCOUNT; ++p) {

#ifndef NDEBUG
				if (p == 0 && ChaoMaster.ChaoHandles[0].SelectedChao == 0) ChaoMaster.ChaoHandles[p].SelectedChao = 1;
				if (p == 1 && ChaoMaster.ChaoHandles[1].SelectedChao == 0) ChaoMaster.ChaoHandles[p].SelectedChao = 2;
#endif

				if (ChaoMaster.ChaoHandles[p].Handle) {
					continue;
				}

				if (ChaoMaster.ChaoHandles[p].SelectedChao) {
					if (!EntityData1Ptrs[p]) {
						ChaoMaster.ChaoHandles[p].SelectedChao = 0;
						ChaoMaster.LoadHUD = true;
						continue;
					}

					ChaoMaster.ChaoHandles[p].Handle = LoadObject((LoadObj)(LoadObj_Data1), 1, ChaoObj_Main);
					ChaoMaster.ChaoHandles[p].Handle->Data1->CharIndex = p;
					ChaoMaster.ChaoLoaded = true;
				}
			}

			if (ChaoHUD && ChaoMaster.ChaoLoaded == true && ChaoMaster.LoadHUD == true) {
				LoadObject(LoadObj_Data1, 6, ChaoHud_Main);
			}

			ChaoMaster.ChaoLoaded = true;
			ChaoMaster.LoadHUD = false;
		}
	}

	__declspec(dllexport) ModInfo SADXModInfo = { ModLoaderVer };
}