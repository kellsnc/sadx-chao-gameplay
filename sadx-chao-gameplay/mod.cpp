#include "stdafx.h"
#include "mod.h"

ChaoLeash SelectedChao[MaxPlayers]{};

static bool DrawHUDNext = false;

bool ChaoPowerups = false;
bool ChaoAssist = true;
bool ChaoLuck = true;
bool ChaoHUD = true;

struct DelayedChaoWK {
	int id;
	int player;
	int timer;
};

CollisionData ChaoLevelCol[] = {
	{ 0x91, 1, 0x70, 0, 0x782008, { 0.0, 2.0, 0.0 }, { 4.0, 4.0, 0.0 }, 0, { 0 } },
	{ 0x81, 0, 0x7, 0, 0x2400, { 0.0, 2.0, 0.0 }, { 1.5, 2.5, 0.69999999 }, 0, { 0 } },
	{ 0x95, 0, 0x77, 0xE2, 0x400, { 0.0, 0.5, 0.0 }, { 0.5f, 0.0, 0.0 }, 0, { 0 } },
	{ 0, 0, 0x77, 0, 0, { 0.0, 1.5, 0.0 }, { 3.0, 0.0, 0.0 }, 0, { 0 } }
};

void ResetSelectedChao() {
	foreach(i, SelectedChao) {
		SelectedChao[i].mode = ChaoLeashMode_Disabled;
	}
}

void LoadChaoFiles() {
	LoadChaoTexlist("AL_DX_PARTS_TEX", (NJS_TEXLIST*)0x33A1340, 0);
	LoadChaoTexlist("AL_BODY", ChaoTexLists, 0);
	LoadChaoTexlist("AL_jewel", &ChaoTexLists[4], 0);
	LoadChaoTexlist("AL_ICON", &ChaoTexLists[3], 0);
	LoadChaoTexlist("AL_EYE", &ChaoTexLists[2], 0);
	LoadChaoTexlist("AL_MOUTH", &ChaoTexLists[5], 0);
	LoadChaoTexlist("AL_TEX_COMMON", &ChaoTexLists[1], 1u);
	LoadChaoTexlist("AL_DX_OBJ_CMN", &AL_DX_OBJ_CMN_TEXLIST, 1u);
	LoadChaoTexlist("AL_DX_ETC", &AL_DX_ETC_TEXLIST, 1u);
	LoadChaoTexlist("AL_OBJECT", &AL_OBJECT_TEXLIST, 1u);
	LoadChaoTexlist("AL_TOY", &AL_TOY_TEXLIST, 1u);

	al_confirmload_load();
	LoadChaoPVPs();

	ChaoManager_Load();
}

void SetChaoPowerups(int id, ChaoData* chaodata) {
	if (ChaoPowerups == true) {
		CharObj2* co2 = CharObj2Ptrs[id];

		if (co2) {
			co2->PhysicsData.GroundAccel += static_cast<float>(min(99, chaodata->data.RunLevel)) / 90.0f;
			co2->PhysicsData.JumpSpeed += static_cast<float>(min(99, chaodata->data.FlyLevel)) / 99.0f;
			co2->PhysicsData.HSpeedCap += static_cast<float>(min(99, chaodata->data.StaminaLevel)) / 30.0f;
			co2->PhysicsData.MaxAccel += static_cast<float>(min(99, chaodata->data.PowerLevel)) / 60.0f;
		}
	}
}

ChaoData* GetChaoData(int id) {
	return (ChaoData*)(GetChaoSaveAddress() + 2072 + (2048 * id));
}

int GetHeldChao(int player) {
	EntityData1* data = EntityData1Ptrs[player];
	CharObj2* co2 = CharObj2Ptrs[player];

	if (co2 && co2->ObjectHeld && co2->ObjectHeld->Data1) {
		ChaoData1* chaodata = (ChaoData1*)co2->ObjectHeld->Data1;

		for (uint8_t i = 0; i < 24; ++i) {
			if (chaodata->ChaoDataBase_ptr == &GetChaoData(i)->data && chaodata->ChaoDataBase_ptr->Type != ChaoType_Egg) {
				return i;
			}
		}
	}

	return -1;
}

void __cdecl CreateChao_Delayed(ObjectMaster* obj) {
	DelayedChaoWK* wk = (DelayedChaoWK*)obj->UnknownB_ptr;

	if (wk->timer > 1) {
		EntityData1* player = EntityData1Ptrs[wk->player];
		NJS_VECTOR pos = { 0, 0, 0 };

		if (player && SelectedChao[wk->player].mode == ChaoLeashMode_Free) {
			PutPlayerBehind(&pos, player, 5.0f);
		}
		
		ChaoData* chaodata = GetChaoData(wk->id);
		ObjectMaster* chao = CreateChao(chaodata, 0, 0, &pos, 0);
		chao->Data1->CharIndex = wk->player;
		SetChaoPowerups(wk->player, chaodata);

		if (SelectedChao[wk->player].mode == ChaoLeashMode_Held) {
			if (player && !(player->Status & Status_HoldObject)) {
				SetHeldObject(wk->player, chao);
			}
		}
		else if (SelectedChao[wk->player].mode == ChaoLeashMode_Fly) {
			chao->Data1->Status |= StatusChao_FlyPlayer;
		}

		DeleteObject_(obj);
	}
	else {
		wk->timer++;
	}
}

void LoadLevelChao(int id, int player) {
	DelayedChaoWK* wk = (DelayedChaoWK*)LoadObject(LoadObj_UnknownB, 0, CreateChao_Delayed)->UnknownB_ptr;
	wk->id = id;
	wk->player = player;
}

void __cdecl LoadLevel_r(ObjectMaster* obj);
Trampoline LoadLevel_t((int)LoadLevel, (int)LoadLevel + 0x7, LoadLevel_r);
void __cdecl LoadLevel_r(ObjectMaster* obj) {
	TARGET_STATIC(LoadLevel)(obj);

	// Load all chao
	if (IsLevelChaoGarden() == false) {
		foreach(selection, SelectedChao) {
			if (SelectedChao[selection].mode != ChaoLeashMode_Disabled) {
				LoadLevelChao(SelectedChao[selection].id, selection);
			}
		}
	}
}

void __cdecl InitLevel_r(ObjectMaster* obj);
Trampoline InitLevel_t(0x415210, 0x415216, InitLevel_r);
void __cdecl InitLevel_r(ObjectMaster* obj)
{
	TARGET_STATIC(InitLevel)(obj);

	// Load chao stuff
	if (IsLevelChaoGarden() == false)
	{
		foreach(selection, SelectedChao)
		{
			if (SelectedChao[selection].mode != ChaoLeashMode_Disabled)
			{
				LoadChaoFiles();

				if (ChaoHUD && DrawHUDNext == true)
				{
					LoadObject(LoadObj_Data1, 6, ChaoHud_Main);
					DrawHUDNext = false;
				}

				break;
			}
		}
	}
}

void SelectChao()
{
	for (int i = 0; i < MaxPlayers; ++i)
	{
		int id = GetHeldChao(i);

		if (id >= 0)
		{
			SelectedChao[i].id = id;
			SelectedChao[i].mode = ChaoLeashMode_Held;
		}
	}
}

void __cdecl Chao_LeaveGarden_r(int level, int act);
Trampoline Chao_LeaveGarden_t(0x715730, 0x715735, Chao_LeaveGarden_r);
void __cdecl Chao_LeaveGarden_r(int level, int act)
{
	ResetSelectedChao();
	SelectChao();
	DrawHUDNext = true;

	TARGET_STATIC(Chao_LeaveGarden)(level, act);
}

void __cdecl ChaoCollision_Init(ObjectMaster* obj, CollisionData* collisionArray, int count, unsigned __int8 list)
{
	if (IsLevelChaoGarden() == false)
	{
		Collision_Init(obj, arrayptrandlength(ChaoLevelCol), list);
	}
	else
	{
		Collision_Init(obj, collisionArray, count, list);
	}
}

bool IsLevelChaoGarden_r()
{
	foreach(i, SelectedChao)
	{
		if (SelectedChao[i].id != ChaoLeashMode_Disabled)
		{
			return true;
		}
	}

	if (CurrentLevel >= LevelIDs_SSGarden)
	{
		return true;
	}

	return false;
}

extern "C" {
	__declspec(dllexport) void __cdecl Init(const char *path)
	{
		const IniFile *config = new IniFile(std::string(path) + "\\config.ini");
		ChaoPowerups = config->getBool("Functionalities", "EnablePowerups", false);
		ChaoAssist = config->getBool("Functionalities", "EnableChaoAssist", true);
		ChaoLuck = config->getBool("Functionalities", "EnableChaoLuck", true);
		ChaoHUD = config->getBool("Functionalities", "EnableHUD", true);
		delete config;

		WriteCall((void*)0x720781, ChaoCollision_Init);

		// Whistle in levels
		WriteCall((void*)0x442570, IsLevelChaoGarden_r);
		WriteCall((void*)0x442680, IsLevelChaoGarden_r);
		
#ifndef NDEBdUG
		SelectedChao[0].id = 0;
		SelectedChao[0].mode = ChaoLeashMode_Fly;
		SelectedChao[1].id = 12;
		SelectedChao[1].mode = ChaoLeashMode_Held;
#endif
	}

	__declspec(dllexport) ModInfo SADXModInfo = { ModLoaderVer };
}