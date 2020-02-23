#include "stdafx.h"
#include "mod.h"

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

//Flight function
inline float Chao_GetFlightSpeed(ChaoDataBase* chaodatabase) {
	return min(chaodatabase->FlyLevel, 99) / 2;
}

//Chao can make players stronger
void Chao_PlayerUp(char player, ChaoDataBase* chaodatabase) {
	PhysicsData* physics = &CharObj2Ptrs[player]->PhysicsData;
	char charid = GetCharacterID(player);

	physics->HSpeedCap = PhysicsArray[charid].HSpeedCap + ( min(99, chaodatabase->RunLevel) / 30);
	physics->GroundAccel = PhysicsArray[charid].GroundAccel + ( min(99, chaodatabase->RunLevel) / 60);
	physics->MaxAccel = PhysicsArray[charid].MaxAccel + (min(99, chaodatabase->RunLevel) / 40);
	
	physics->JumpSpeed = PhysicsArray[charid].JumpSpeed + (min(99, chaodatabase->FlyLevel) * 0.003);
}

//Custom Chao Actions
void ChaoObj_Delete(ObjectMaster* obj) {
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

void ChaoObj_Main(ObjectMaster* obj) {
	uint8_t Action = obj->Data1->Action;
	EntityData1* data = obj->Data1;
	ChaoLeash* Leash = &ChaoMaster.ChaoHandles[obj->Data1->CharIndex];

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

		obj->DeleteSub = ChaoObj_Delete; //When you quit a level
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
		if (Leash->Carried == true) {
			SetHeldObject(data->CharIndex, Leash->Chao);
			data->Action = ChaoAction_Free;
		}
		else {
			Leash->Chao->Data1->CharIndex = 1;
			data->Action = ChaoAction_Flight;
			data->CharID = 0;
		}

		ChaoData1* chaodata1 = (ChaoData1*)Leash->Chao->Data1;
		if (ChaoPowerups) Chao_PlayerUp(data->CharIndex, chaodata1->ChaoDataBase_ptr);
	}
	else {
		ChaoData1* chaodata1 = (ChaoData1*)Leash->Chao->Data1;

		if (EntityData1Ptrs[data->CharIndex] == nullptr || chaodata1 == nullptr || &chaodata1->entity == nullptr) {
			DeleteObject_(obj);
		}

		chaodata1->ChaoDataBase_ptr->Lifespan = 1;
		chaodata1->ChaoDataBase_ptr->Lifespan2 = 1;

		if (IsPlayerHoldingObject(data->CharIndex)) {
			Leash->Carried = true;
		}
		else {
			Leash->Carried = false;
		}

		//flight mode
		if (Action == ChaoAction_Flight) {
			if (data->NextAction == 0) {
				if (PressedButtons[data->CharIndex] & Buttons_C) {
					Leash->Chao->Data1->CharIndex = 0;
					data->InvulnerableTime = 0;
					data->NextAction = 1;
					return;
				}

				if (ChaoAssist && chaodata1->ChaoDataBase_ptr->PowerLevel > 5 && chaodata1->ChaoDataBase_ptr->Energy > 1000 && Chao_CheckEnemy(chaodata1)) {
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

						if (ChaoLuck) Chao_CheckLuck(chaodata1);

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
						KillEnemiesInSphere(&enemy->Data1->Position, 5);
						
						if (++data->InvulnerableTime > 120 || enemy->MainSub == BoaBoa_Main) {
							data->LoopData = nullptr;
							data->InvulnerableTime = 0;
							UpdateSetDataAndDelete(enemy);
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
		else if (Leash->Carried == false) {
			EntityData1* data1 = EntityData1Ptrs[data->CharIndex];

			if (PressedButtons[data->CharIndex] & Buttons_C) {
				Controllers[data->CharIndex].PressedButtons = Buttons_Y;

				Leash->Chao->Data1->CharIndex = 1;
				data->Action = ChaoAction_Flight;
				data->NextAction = 0;
			}
		}
	}
}

//Skip gravity calculations in flight mode
void Chao_Gravity_r(ObjectMaster* obj);
Trampoline Chao_Gravity_t(0x73FEF0, 0x73FEF8, Chao_Gravity_r);
void Chao_Gravity_r(ObjectMaster* obj) {
	if (CurrentLevel >= LevelIDs_SSGarden || obj->Data1->CharIndex != 1) {
		ObjectFunc(original, Chao_Gravity_t.Target());
		original(obj);
	}
}

//Skip chao actions in flight mode
void Chao_Movements_r(ObjectMaster* obj);
Trampoline Chao_Movements_t(0x71EFB0, 0x71EFB9, Chao_Movements_r);
void Chao_Movements_r(ObjectMaster* obj) {
	if (CurrentLevel >= LevelIDs_SSGarden || obj->Data1->CharIndex != 1) {
		ObjectFunc(original, Chao_Movements_t.Target());
		original(obj);
	}
}