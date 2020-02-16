#include "stdafx.h"
#include <vector>

ObjectMaster* ChaoObject;
ObjectMaster* CurrentChao;
CollisionInfo* ChaoCol;

uint8_t SelectedChao = 0;
bool isloaded = false;

std::vector<NJS_PLANE> waterlist = {};

NJS_VECTOR bombpos;
float bombsize;

FunctionPointer(int, Chao_Animation, (ObjectMaster *a1, int a2), 0x734F00);
FunctionPointer(bool, Chao_FinishedAnimation, (ObjectMaster *a1), 0x735040);
FunctionPointer(EntityData1*, SpawnAnimal, (int unknown, float x, float y, float z), 0x4BE610);

enum ChaoActions {
	ChaoAction_Init,
	ChaoAction_LoadChao,
	ChaoAction_Free,
	ChaoAction_Flight,
	ChaoAction_Attack
};

//Common functions
float GetDistance(NJS_VECTOR* orig, NJS_VECTOR* dest) {
	return sqrtf(powf(dest->x - orig->x, 2) + powf(dest->y - orig->y, 2) + powf(dest->z - orig->z, 2));
}

NJS_VECTOR GetPointToFollow(NJS_VECTOR* pos, Rotation3* rot) {
	NJS_VECTOR point;

	NJS_VECTOR dir = { -10, 10, 5 };
	njPushMatrix(_nj_unit_matrix_);
	njTranslateV(0, pos);
	njRotateY(0, -rot->y);
	njCalcPoint(0, &dir, &point);
	njPopMatrix(1u);
	return point;
}

Rotation3 fPositionToRotation(NJS_VECTOR* orig, NJS_VECTOR* point) {
	NJS_VECTOR dist;
	Rotation3 result;

	dist.x = orig->x - point->x;
	dist.y = orig->y - point->y;
	dist.z = orig->z - point->z;

	result.x = atan2(dist.y, dist.z) * 65536.0 * -0.1591549762031479;
	result.y = atan2(dist.x, dist.z) * 65536.0 * 0.1591549762031479;

	result.y = -result.y - 0x4000;
	return result;
}

NJS_VECTOR GetPathPosition(NJS_VECTOR* orig, NJS_VECTOR* dest, float state) {
	NJS_VECTOR result;
	result.x = (dest->x - orig->x) * state + orig->x;
	result.y = (dest->y - orig->y) * state + orig->y;
	result.z = (dest->z - orig->z) * state + orig->z;

	return result;
}

bool IsPointInsideSphere(NJS_VECTOR* center, NJS_VECTOR* pos, float radius) {
	return (powf(pos->x - center->x, 2) + pow(pos->y - center->y, 2) + pow(pos->z - center->z, 2)) <= pow(radius, 2);
}

int GetCurrentChaoStage_r() {
	if (ChaoObject) return 5;
	else return CurrentChaoStage;
}

//Chao selection functions
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

			if (!chaodata) return;

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
		if (!ChaoObject || ChaoObject->Data1->Action != ChaoAction_Flight) {
			if (GameState == 15) SelectedChao = 0;
		}
	}
}

//Water height calculation
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

void IsChaoInWater(ObjectMaster* a1) {
	ChaoData1* chaodata1 = (ChaoData1*)a1->Data1;
	ChaoData2* chaodata2 = (ChaoData2*)a1->Data2;

	float height = -10000000;
	if (waterlist.size() > 0) {
		NJS_VECTOR pos = chaodata1->entity.Position;
		NJS_PLANE wpos;
		for (int i = 0; i < waterlist.size(); ++i) {
			wpos = waterlist[i];
			if (pos.y < wpos.py + 2 && pos.y > wpos.py - 170) {
				if (pos.x > wpos.px&& pos.x < wpos.vx) {
					if (pos.z > wpos.pz&& pos.z < wpos.vz) {
						height = wpos.py;
					}
				}
			}
		}
	}

	WriteData((float*)0x73C24C, height);
}

//Enemy attack functions
ObjectMaster* Chao_GetClosestEnemy(NJS_VECTOR* pos, float stamina) {
	ObjectMaster* current = ObjectListThing[3];
	while (1) {
		if (current->MainSub == Kiki_Main || current->MainSub == RhinoTank_Main || current->MainSub == Sweep_Main
			|| current->MainSub == SpinnerA_Main || current->MainSub == SpinnerB_Main || current->MainSub == SpinnerC_Main
			|| current->MainSub == UnidusA_Main || current->MainSub == UnidusB_Main || current->MainSub == UnidusC_Main
			|| current->MainSub == Leon_Main || current->MainSub == BoaBoa_Main || current->MainSub == ESman) {
			
			float dist = GetDistance(pos, &current->Data1->Position);
			if (GetDistance(pos, &current->Data1->Position) < 200 + stamina) return current;
			else {
				if (current->Next) {
					current = current->Next;
					continue;
				}
				else break;
			}
		}
		else {
			if (current->Next) current = current->Next;
			else break;
		}
	}
	return nullptr;
}

bool Chao_CheckEnemy(ChaoData1* chaodata) {
	if (Chao_GetClosestEnemy(&chaodata->entity.Position, chaodata->ChaoDataBase_ptr->StaminaLevel)) {
		if (rand() % (1000 / max(1, min(chaodata->ChaoDataBase_ptr->PowerLevel, 99))) == 0) {
			return true;
		}
	}

	return false;
}

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

//Extra animals if the Chao is lucky
void Chao_CheckLuck(ChaoData1* chaodata) {
	if (chaodata->ChaoDataBase_ptr->LuckyGrade > 0) {
		if (rand() % (101 - (min(chaodata->ChaoDataBase_ptr->LuckLevel, 99))) == 0) {
			SpawnAnimal(2, chaodata->entity.Position.x, chaodata->entity.Position.y, chaodata->entity.Position.z);

			int grade = 2;
			while (1) {
				if (chaodata->ChaoDataBase_ptr->LuckyGrade >= grade) {
					if (rand() % grade == 0) {
						SpawnAnimal(2, chaodata->entity.Position.x, chaodata->entity.Position.y, chaodata->entity.Position.z);
						grade += 1;
						continue;
					}
					else {
						break;
					}
				}
				else {
					break;
				}
			}
		}
	}
}

// Flight functions
inline float Chao_GetFlightSpeed(ChaoDataBase* chaodatabase) {
	return min(chaodatabase->FlyLevel, 99) / 2;
}

//Custom Chao Actions
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
	EntityData1* data = a1->Data1;

	if (Action == ChaoAction_Init) {
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
		data->Action = ChaoAction_LoadChao; //Wait a frame before loading a chao
	}
	else if (Action == ChaoAction_LoadChao) {
		//We get the chao data in the savefile
		ChaoData* chaodata = GetChaoData(SelectedChao - 1);

		//Start position is behind the player
		NJS_VECTOR v = EntityData1Ptrs[0]->Position;
		v.x -= 20;

		//Load the chao
		CurrentChao = CreateChao(chaodata, 0, CurrentChao, &v, 0);
		if (EntityData1Ptrs[0]->Action != 12 && CurrentLevel >= LevelIDs_StationSquare && CurrentLevel <= LevelIDs_Past) {
			SetHeldObject(0, CurrentChao);
			data->Action = ChaoAction_Free;
		}
		else {
			CurrentChao->Data1->CharIndex = 1;
			data->Action = ChaoAction_Flight;
			data->CharID = 0;
		}
	}
	else {
		ChaoData1* chaodata1 = (ChaoData1*)CurrentChao->Data1;

		//If the act has changed, check water collisions again
		if (ActCopy != CurrentAct) {
			GetWaterCollisions();
			ActCopy = CurrentAct;

			//Fix a bug at Emerald Coast act swap by reloading the chao
			if (CurrentLevel == LevelIDs_EmeraldCoast && CurrentAct == 1) {
				a1->DeleteSub(a1);
				data->Action = ChaoAction_Init;
				return;
			}
		}

		//flight mode
		if (Action == ChaoAction_Flight) {
			if (data->NextAction == 0) {
				if (PressedButtons[data->CharID] & Buttons_D) {
					CurrentChao->Data1->CharIndex = 0;
					data->InvulnerableTime = 0;
					data->NextAction = 1;
					return;
				}

				if (chaodata1->ChaoDataBase_ptr->PowerLevel > 5 && chaodata1->ChaoDataBase_ptr->Energy > 1000 && Chao_CheckEnemy(chaodata1)) {
					data->Action = ChaoAction_Attack;
					data->NextAction = 0;
					return;
				}

				EntityData1* data1 = EntityData1Ptrs[data->CharID];

				data->Position = GetPointToFollow(&data1->Position, &data1->Rotation);
				float dist = GetDistance(&data->Position, &chaodata1->entity.Position);

				//adjust flight speed with the fly level of the chao
				if (dist > 1000) chaodata1->entity.Position = data->Position;
				dist += Chao_GetFlightSpeed(chaodata1->ChaoDataBase_ptr);

				if (dist < 5) {
					chaodata1->entity.Position = GetPathPosition(&chaodata1->entity.Position, &data->Position, dist / (100 + (400 - dist)));
					chaodata1->entity.Rotation.y = -data1->Rotation.y + 0x4000;

					if (GetDistance(&data->Position, &chaodata1->entity.Position) < 1) {
						chaodata1->entity.Position = data->Position;
						chaodata1->entity.Action = 1;
					}
				}
				else if (dist < 30) {
					chaodata1->entity.Position = GetPathPosition(&chaodata1->entity.Position, &data->Position, dist / 400);
					chaodata1->entity.Rotation.y = -fPositionToRotation(&chaodata1->entity.Position, &data->Position).y + 0x4000;
				}
				else {
					chaodata1->entity.Position = GetPathPosition(&chaodata1->entity.Position, &data->Position, dist / 300);
					chaodata1->entity.Rotation.y = -fPositionToRotation(&chaodata1->entity.Position, &data->Position).y + 0x4000;
				}

				if (FrameCounterUnpaused % 30 == 0) {
					Chao_Animation(CurrentChao, 289);
				}
			}
			else if (data->NextAction == 1) {
				if (++data->InvulnerableTime > 60) {
					data->Action = ChaoAction_Free;
				}
			}
		}
		else if (Action == ChaoAction_Attack) {
			uint8_t stamina = chaodata1->ChaoDataBase_ptr->StaminaLevel;

			if (data->NextAction == 0) {
				ObjectMaster* enemy = Chao_GetClosestEnemy(&chaodata1->entity.Position, stamina);
				if (enemy) {
					data->LoopData = (Loop*)enemy;
					data->NextAction = 1;
				}
				else {
					data->NextAction = 0;
					data->Action = ChaoAction_Flight;
				}

			}
			else {
				if (data->LoopData) {
					ObjectMaster* enemy = (ObjectMaster*)data->LoopData;
					
					if (!enemy->Data1) {
						data->NextAction = 0;
						data->LoopData = nullptr;

						Chao_CheckLuck(chaodata1);

						if (chaodata1->ChaoDataBase_ptr->PowerGrade > 2) {
							if (rand() % 3 == 0) {
								stamina -= 10;
								data->Action = ChaoAction_Attack;
								return;
							}
						}

						data->Action = ChaoAction_Flight;
						return;
					}

					if (IsPointInsideSphere(&enemy->Data1->Position, &chaodata1->entity.Position, 15)) {
						bombpos = enemy->Data1->Position;
						bombsize = 5;

						if (++data->InvulnerableTime > 120) {
							data->LoopData = nullptr;
							DeleteObject_(enemy);
							return;
						}
					}

					data->Position = enemy->Data1->Position;
					float dist = GetDistance(&data->Position, &chaodata1->entity.Position);
					float speed = Chao_GetFlightSpeed(chaodata1->ChaoDataBase_ptr);

					dist += speed;
					dist = fmax(fmin(dist, 200), 80 + speed);
					
					chaodata1->entity.Position = GetPathPosition(&chaodata1->entity.Position, &data->Position, dist / 3000);
					chaodata1->entity.Rotation.y = -fPositionToRotation(&chaodata1->entity.Position, &data->Position).y + 0x4000;
				}
				else {
					data->NextAction = 0;
					data->Action = ChaoAction_Flight;
				}
			}

			if (FrameCounterUnpaused % 30 == 0) {
				Chao_Animation(CurrentChao, 289);
			}
		}
		else {
			for (uint8_t player = 0; player < 8; ++player) {
				EntityData1* data1 = EntityData1Ptrs[player];
				if (!data1) continue;

				if (PressedButtons[player] & Buttons_D &&
					GetDistance(&data1->Position, &chaodata1->entity.Position) < 50) {
					CurrentChao->Data1->CharIndex = 1;
					data->Action = ChaoAction_Flight;
					data->NextAction = 0;
					data->CharID = player;
				}
			}
		}
	}
}

//Different water height for multiple chao
void Chao_Main_r(ObjectMaster* obj);
Trampoline Chao_Main_t((int)Chao_Main, (int)Chao_Main + 0x6, Chao_Main_r);
void Chao_Main_r(ObjectMaster* obj) {
	if (CurrentLevel < 39) {
		IsChaoInWater(obj);
	}

	ObjectFunc(original, Chao_Main_t.Target());
	original(obj);
}

//Skip gravity calculations if following player
void Chao_Gravity_r(ObjectMaster* obj);
Trampoline Chao_Gravity_t(0x73FEF0, 0x73FEF8, Chao_Gravity_r);
void Chao_Gravity_r(ObjectMaster* obj) {
	if (CurrentLevel > 38 || CurrentChao->Data1->CharIndex != 1) {
		ObjectFunc(original, Chao_Gravity_t.Target());
		original(obj);
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