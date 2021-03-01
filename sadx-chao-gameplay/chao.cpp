#include "stdafx.h"
#include "mod.h"

enum CustomChaoActs {
	ChaoAct_FollowPlayer,
	ChaoAct_IdlePlayer,
	ChaoAct_Attack
};

float Chao_GetFlightSpeed(ChaoData1* data1) {
	return max(0.8f, min(data1->ChaoDataBase_ptr->FlyLevel, 99) / 50);
}

float Chao_GetAttackRange(ChaoData1* data1) {
	return max(200.0f, min(500.0f, data1->ChaoDataBase_ptr->StaminaLevel * 7));
}

NJS_VECTOR GetPlayerPoint(EntityData1* player) {
	NJS_VECTOR dir = { -6, 7, 5 };
	
	if (player->CharID == Characters_Big) {
		dir = { -10, 14, 7 };
	}
	else if (player->CharID == Characters_Gamma) {
		dir = { -8, 16, 11 };
	}
	
	return GetPointToFollow(&player->Position, &dir, &player->Rotation);
}

void FollowPlayer(ChaoData1* data1, EntityData1* player) {
	NJS_VECTOR dest = GetPlayerPoint(player);

	float dist = GetDistance(&data1->entity.Position, &dest);

	if (dist > 5.0f) {
		LookAt(&data1->entity.Position, &dest, &data1->entity.Rotation.x, &data1->entity.Rotation.y);
		MoveForward(&data1->entity, Chao_GetFlightSpeed(data1) * (dist / 8));
		data1->entity.Rotation.x = 0;
	}
	else if (CharObj2Ptrs[player->CharIndex]->Speed.x + CharObj2Ptrs[player->CharIndex]->Speed.y < 0.2f) {
		data1->entity.NextAction = ChaoAct_IdlePlayer;
	}
}

void IdlePlayer(ChaoData1* data1, EntityData1* player) {
	NJS_VECTOR dest = GetPlayerPoint(player);

	float dist = GetDistance(&data1->entity.Position, &dest);

	if (dist < 11.0f && CharObj2Ptrs[player->CharIndex]->Speed.x + CharObj2Ptrs[player->CharIndex]->Speed.y < 0.2f) {
		data1->entity.Rotation.y += 0x200;
		data1->entity.Rotation.x = 0;
		MoveForward(&data1->entity, 0.1f);
	}
	else {
		data1->entity.NextAction = ChaoAct_FollowPlayer;
	}
}

HomingAttackTarget GetClosestAttack(NJS_VECTOR* pos) {
	HomingAttackTarget target = { 0, 1000000.0f };

	for (int i = 0; i < HomingAttackTarget_Sonic_Index; ++i) {
		HomingAttackTarget* target_ = &HomingAttackTarget_Sonic[i];

		float dist = GetDistance(pos, &target_->entity->Position);

		if (dist < target.distance && target_->entity && 
			target_->entity->CollisionInfo->List == 3 &&
			target_->entity->CollisionInfo->Object->Data2 != nullptr) {
			target.distance = dist;
			target.entity = target_->entity;
		}
	}

	return target;
}

void CheckForAttack(ChaoData1* data1) {
	if (ChaoAssist == true) {
		if (rand() % (1000 / max(1, min(data1->ChaoDataBase_ptr->PowerLevel, 99))) == 0) {
			HomingAttackTarget target = GetClosestAttack(&data1->entity.Position);

			if (target.entity && target.distance < Chao_GetAttackRange(data1)) {
				data1->entity.NextAction = ChaoAct_Attack;
			}
		}
	}
}

void ChaoAttack(ObjectMaster* obj, ChaoData1* data1) {
	HomingAttackTarget target = GetClosestAttack(&data1->entity.Position);

	if (data1->entity.Status & StatusChao_Attacked) {
		if (rand() % (500 / max(1, min(data1->ChaoDataBase_ptr->StaminaLevel, 99))) == 0) {
			data1->entity.Status &= ~(StatusChao_Attacked);
		}
		else {
			data1->entity.NextAction = ChaoAct_FollowPlayer;
			data1->entity.Status &= ~(StatusChao_Attacked);
			return;
		}
	}

	if (target.entity && target.distance < Chao_GetAttackRange(data1)) {
		float dist = GetDistance(&data1->entity.Position, &target.entity->Position) + 30.0f;
		LookAt(&data1->entity.Position, &target.entity->Position, &data1->entity.Rotation.x, &data1->entity.Rotation.y);
		MoveForward(&data1->entity, Chao_GetFlightSpeed(data1) * (dist / 10));
		data1->entity.Rotation.x = 0;

		if (dist < 50.0f) {
			data1->entity.CollisionInfo->CollisionArray[2].field_3 = 0xE2;
		}
		else {
			data1->entity.CollisionInfo->CollisionArray[2].field_3 = 0;
		}
	}
	else {
		data1->entity.NextAction = ChaoAct_FollowPlayer;
		data1->entity.Status &= ~(StatusChao_Attacked);
	}
}

void LevelChao_Fly(ObjectMaster* obj, ChaoData1* data1, ChaoData2* data2, EntityData1* player) {
	switch (data1->entity.NextAction) {
	case ChaoAct_FollowPlayer:
		FollowPlayer(data1, player);
		CheckForAttack(data1);
		data1->entity.CollisionInfo->CollisionArray[2].field_3 = 0;
		break;
	case ChaoAct_IdlePlayer:
		IdlePlayer(data1, player);
		CheckForAttack(data1);
		data1->entity.CollisionInfo->CollisionArray[2].field_3 = 0;
		break;
	case ChaoAct_Attack:
		ChaoAttack(obj, data1);
		break;
	}

	// Flying animation
	if (FrameCounterUnpaused % 30 == 0) {
		Chao_Animation(obj, 289);
	}

	Chao_PlayAnimation(obj);
	Chao_RunEmotionBall(obj);
	Chao_MoveEmotionBall(obj);
	Chao_RunPhysics(obj);

	// Detach if Y is pressed
	if (PressedButtons[data1->entity.CharIndex] & Buttons_Y) {
		data1->entity.Status &= ~StatusChao_FlyPlayer;
		SelectedChao[data1->entity.CharIndex].mode = ChaoLeashMode_Free;
	}
}

void LevelChao_Normal(ObjectMaster* obj, ChaoData1* data1, ChaoData2* data2) {
	Chao_RunMovements(obj);
	Chao_PlayAnimation(obj);
	Chao_RunEmotionBall(obj);
	Chao_RunActions(obj);
	Chao_MoveEmotionBall(obj);
	
	// Fly if Y is pressed
	if (!(data1->entity.Status & StatusChao_Held)) {
		Chao_RunGravity(obj);
		SelectedChao[data1->entity.CharIndex].mode = ChaoLeashMode_Free;

		if (PressedButtons[data1->entity.CharIndex] & Buttons_Y) {
			data1->entity.Status |= StatusChao_FlyPlayer;
			SelectedChao[data1->entity.CharIndex].mode = ChaoLeashMode_Fly;
		}
	}
	else {
		SelectedChao[data1->entity.CharIndex].mode = ChaoLeashMode_Held;
	}

	Chao_RunPhysics(obj);
}

void __cdecl Chao_Main_r(ObjectMaster* obj);
Trampoline Chao_Main_t((int)Chao_Main, (int)Chao_Main + 0x6, Chao_Main_r);
void __cdecl Chao_Main_r(ObjectMaster* obj) {
	ChaoData1* data1 = (ChaoData1*)obj->Data1;
	ChaoData2* data2 = (ChaoData2*)obj->Data2;

	if (IsLevelChaoGarden() == false) {
		EntityData1* player = EntityData1Ptrs[data1->entity.CharIndex];

		// If the player cannot be found, act as a normal Chao
		if (player == nullptr) {
			TARGET_STATIC(Chao_Main)(obj);
			return;
		}

		// Run custom actions
		if (!(data1->entity.Status & StatusChao_FlyPlayer)) {
			LevelChao_Normal(obj, data1, data2);
		}
		else {
			LevelChao_Fly(obj, data1, data2, player);
		}

		if (!(data1->entity.Status & StatusChao_Held)) {
			Chao_AddToCollisionList(obj);
		}

		obj->DisplaySub(obj);
		RunObjectChildren(obj);
	}
	else {
		TARGET_STATIC(Chao_Main)(obj);
	}
}

void Chao_CheckLuck(ChaoData1* chaodata) {
	if (chaodata->ChaoDataBase_ptr->LuckyGrade > 0) {
		if (rand() % (101 - (min(chaodata->ChaoDataBase_ptr->LuckLevel, 99))) == 0) {
			SpawnAnimal(2, chaodata->entity.Position.x, chaodata->entity.Position.y, chaodata->entity.Position.z);

			int grade = 2;
			while (1) {
				if (chaodata->ChaoDataBase_ptr->LuckyGrade >= grade) {
					if (rand() % (grade * 2) == 0) {
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

bool OhNoImDead2_r(EntityData1* a1, ObjectData2* a2);
Trampoline OhNoImDead2_t(0x004CE030, 0x004CE036, OhNoImDead2_r);
bool OhNoImDead2_r(EntityData1* a1, ObjectData2* a2) {
	if (ChaoLuck == true) {
		if (a1->CollisionInfo->CollidingObject) {
			if (a1->CollisionInfo->CollidingObject->Object->MainSub == Chao_Main) {
				ChaoData1* data = (ChaoData1*)a1->CollisionInfo->CollidingObject->Object->Data1;
				data->entity.Status |= StatusChao_Attacked;
				Chao_CheckLuck(data);
				return true;
			}
		}
	}
	
	TARGET_STATIC(OhNoImDead2)(a1, a2);
}