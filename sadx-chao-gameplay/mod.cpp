#include "stdafx.h"
#include "mod.h"

ChaoHandle	ChaoMaster;
NJS_VECTOR	bombpos;
float		bombsize;

int GetCurrentChaoStage_r() {
	if (CurrentLevel < LevelIDs_SSGarden) return 5;
	else return CurrentChaoStage;
}

//Chao selection functions
inline ChaoData* GetChaoData(uint8_t id) {
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
			ChaoMaster.PreviousLevel = CurrentLevel;
		}
	}
	else if (ChaoMaster.ChaoHandles[player].Chao && 
		ChaoMaster.ChaoHandles[player].Chao->Data1->Action == ChaoAction_Flight) {
		ChaoMaster.ChaoHandles[player].SelectedChao = GetChaoByPointer(ChaoMaster.ChaoHandles[player].Chao);
		ChaoMaster.PreviousLevel = CurrentLevel;
	}
	else {
		ChaoMaster.ChaoHandles[player].SelectedChao = NULL;
	}
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
void ChaoObj_Delete(ObjectMaster * obj) {
	DeleteObjectMaster(ChaoManager);
	ChaoManager = nullptr;

	DeleteObjectMaster(ChaoMaster.ChaoHandles[obj->Data1->CharIndex].Chao);
	ChaoMaster.ChaoHandles[obj->Data1->CharIndex].Chao = nullptr;
	ChaoMaster.ChaoHandles[obj->Data1->CharIndex].Handle = nullptr;

	//Release the chao textures
	FreeChaoTexlists();

	//reset default water height
	float height = 0;
	WriteData((float*)0x73C24C, height);
}

void ChaoObj_Main(ObjectMaster * a1) {
	uint8_t Action = a1->Data1->Action;
	EntityData1* data = a1->Data1;
	ChaoLeash* Leash = &ChaoMaster.ChaoHandles[a1->Data1->CharIndex];

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
		if (!ChaoMaster.AreChaoPVPLoaded) {
			al_confirmload_load();
			LoadChaoPVPs();
			ChaoMaster.AreChaoPVPLoaded = true;
		}
		
		ChaoManager_Load(); //Load chao behaviour
		GetWaterCollisions(); //Hacky solution to make chao swim

		ActCopy = CurrentAct;

		a1->DeleteSub = ChaoObj_Delete; //When you quit a level
		data->Action = ChaoAction_LoadChao; //Wait a frame before loading a chao
	}
	else if (Action == ChaoAction_LoadChao) {
		//We get the chao data in the savefile
		ChaoData* chaodata = GetChaoData(Leash->SelectedChao - 1);

		//Start position is behind the player
		NJS_VECTOR v = EntityData1Ptrs[0]->Position;
		v.x -= 20;

		//Load the chao
		Leash->Chao = CreateChao(chaodata, 0, Leash->Chao, &v, 0);
		if (EntityData1Ptrs[0]->Action != 12 && CurrentLevel >= LevelIDs_StationSquare && CurrentLevel <= LevelIDs_Past) {
			SetHeldObject(0, Leash->Chao);
			data->Action = ChaoAction_Free;
		}
		else {
			Leash->Chao->Data1->CharIndex = 1;
			data->Action = ChaoAction_Flight;
			data->CharID = 0;
		}
	}
	else {
		ChaoData1* chaodata1 = (ChaoData1*)Leash->Chao->Data1;

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
					Leash->Chao->Data1->CharIndex = 0;
					data->InvulnerableTime = 0;
					data->NextAction = 1;
					return;
				}

				if (chaodata1->ChaoDataBase_ptr->PowerLevel > 5 && chaodata1->ChaoDataBase_ptr->Energy > 1000 && Chao_CheckEnemy(chaodata1)) {
					data->Action = ChaoAction_Attack;
					data->NextAction = 0;
					return;
				}

				EntityData1* data1 = EntityData1Ptrs[data->CharIndex];

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
					Chao_Animation(Leash->Chao, 289);
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
				Chao_Animation(Leash->Chao, 289);
			}
		}
		else {
			for (uint8_t player = 0; player < 8; ++player) {
				EntityData1* data1 = EntityData1Ptrs[player];
				if (!data1) continue;

				if (PressedButtons[player] & Buttons_D &&
					GetDistance(&data1->Position, &chaodata1->entity.Position) < 50) {
					Leash->Chao->Data1->CharIndex = 1;
					data->Action = ChaoAction_Flight;
					data->NextAction = 0;
					data->CharID = player;
				}
			}
		}
	}
}

//Skip gravity calculations if following player
void Chao_Gravity_r(ObjectMaster* obj);
Trampoline Chao_Gravity_t(0x73FEF0, 0x73FEF8, Chao_Gravity_r);
void Chao_Gravity_r(ObjectMaster* obj) {
	if (CurrentLevel >= LevelIDs_SSGarden || obj->Data1->CharIndex != 1) {
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
		if (GameState == 15) {
			if (CurrentLevel > LevelIDs_StationSquare) {
				for (char p = 0; p < 8; ++p) {
					SelectChao(p);
				}
			}
		}
		
		if ((GameState == 4 || GameState == 2) && !IsLevelChaoGarden() && ChaoMaster.PreviousLevel != CurrentLevel) {
			for (char p = 0; p < 8; ++p) {

#ifndef DEBUG
				if (p == 0) ChaoMaster.ChaoHandles[p].SelectedChao = 1;
#endif

				if (ChaoMaster.ChaoHandles[p].SelectedChao) {
					ChaoMaster.ChaoHandles[p].Handle = LoadObject((LoadObj)(LoadObj_Data1), 1, ChaoObj_Main);
					ChaoMaster.ChaoHandles[p].Handle->Data1->CharIndex = p;
				}
			}

			ChaoMaster.PreviousLevel = CurrentLevel;
		}

		
	}

	__declspec(dllexport) ModInfo SADXModInfo = { ModLoaderVer };
}