#pragma once

typedef struct {
	char SelectedChao;
	bool Carried;
	ObjectMaster* Handle;
	ObjectMaster* Chao;
} ChaoLeash;

typedef struct {
	ChaoLeash ChaoHandles[8];
	bool ChaoLoaded;
	bool JustOutOfGarden;
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
FunctionPointer(int, WhistleFunction, (EntityData1* a1, EntityData2* a2, CharObj2* a3, int flag), 0x442570);
FunctionPointer(int, BigWhistle, (int a3, int a4, int a5, int a6), 0x423BE0);

extern ChaoHandle ChaoMaster;
ChaoData* GetChaoData(uint8_t id);
void KillEnemiesInSphere(NJS_VECTOR* pos, float radius);

void ChaoObj_Main(ObjectMaster* obj);

float GetDistance(NJS_VECTOR* orig, NJS_VECTOR* dest);
NJS_VECTOR GetPointToFollow(NJS_VECTOR* pos, Rotation3* rot);
Rotation3 fPositionToRotation(NJS_VECTOR* orig, NJS_VECTOR* point);
NJS_VECTOR GetPathPosition(NJS_VECTOR* orig, NJS_VECTOR* dest, float state);
bool IsPointInsideSphere(NJS_VECTOR* center, NJS_VECTOR* pos, float radius);
bool IsPlayerHoldingObject(char player);

void GetWaterCollisions();
void IsChaoInWater(ObjectMaster* obj);

void ChaoHud_Main(ObjectMaster* obj);

#define PLAYERCOUNT 4

//from https://github.com/nihilus/hexrays_tools/blob/master/code/defs.h
#define _BYTE  uint8_t
#define _WORD  uint16_t
#define BYTEn(x, n)   (*((_BYTE*)&(x)+n))
#define BYTE1(x)   BYTEn(x,  1)
#define HIWORD(x)   (*((_WORD*)&(x)+1))