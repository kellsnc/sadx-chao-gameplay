#pragma once

typedef struct {
	char SelectedChao;
	bool Carried;
	ObjectMaster* Handle;
	ObjectMaster* Chao;
} ChaoLeash;

typedef struct {
	ChaoLeash ChaoHandles[8];
	int PreviousLevel;
	bool AreChaoPVPLoaded;
} ChaoHandle;

enum ChaoActions {
	ChaoAction_Init,
	ChaoAction_LoadChao,
	ChaoAction_Free,
	ChaoAction_Flight,
	ChaoAction_Attack
};

FunctionPointer(int, Chao_Animation, (ObjectMaster* a1, int a2), 0x734F00);
FunctionPointer(bool, Chao_FinishedAnimation, (ObjectMaster* a1), 0x735040);
FunctionPointer(EntityData1*, SpawnAnimal, (int unknown, float x, float y, float z), 0x4BE610);

float GetDistance(NJS_VECTOR* orig, NJS_VECTOR* dest);
NJS_VECTOR GetPointToFollow(NJS_VECTOR* pos, Rotation3* rot);
Rotation3 fPositionToRotation(NJS_VECTOR* orig, NJS_VECTOR* point);
NJS_VECTOR GetPathPosition(NJS_VECTOR* orig, NJS_VECTOR* dest, float state);
bool IsPointInsideSphere(NJS_VECTOR* center, NJS_VECTOR* pos, float radius);

void GetWaterCollisions();
void IsChaoInWater(ObjectMaster* a1);