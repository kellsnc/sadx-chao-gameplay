#pragma once

#define TARGET_DYNAMIC(name) ((decltype(name##_r)*)name##_t->Target())
#define TARGET_STATIC(name) ((decltype(name##_r)*)name##_t.Target())
#define foreach(item, items) for (int item = 0; item < LengthOfArray(items); ++item)

static constexpr int MaxPlayers = 4;

enum StatusChao : __int16 {
	StatusChao_Held = 0x1000,

	// Custom ones:
	StatusChao_FlyPlayer = 0x2000,
	StatusChao_Attacked = 0x4000
};

enum ChaoState : __int16 {
	ChaoState_Unk1 = 0x1,
	ChaoState_Water = 0x4,
};

enum ChaoLeashModes {
	ChaoLeashMode_Disabled,
	ChaoLeashMode_Free,
	ChaoLeashMode_Held,
	ChaoLeashMode_Fly
};

struct ChaoLeash {
	ChaoLeashModes mode;
	unsigned int id;
};

struct ChaoData2_ {
	char gap0[4];
	NJS_VECTOR field_4;
	NJS_VECTOR field_10;
	char gap1C[16];
	int field_2C;
	char gap30[4];
	float field_34;
	char gap38[4];
	float field_3C;
	__int16 field_40;
	__int16 field_42;
	char gap44[4];
	float field_48;
	char gap_4C[20];
	NJS_VECTOR OtherPosition;
	char gap_6C[64];
	float field_AC;
	char field_B0[4];
	float field_B4;
	float field_B8;
	float field_BC;
	float field_C0;
	float field_C4;
	float field_C8;
	float field_CC;
	float field_D0;
	float idk1;
	float idk2;
	float WaterHeight;
	char gap_d4[309];
	char field_26B;
};

extern ChaoLeash SelectedChao[MaxPlayers];
extern bool ChaoPowerups;
extern bool ChaoAssist;
extern bool ChaoLuck;

bool IsChaoInWater(ChaoData1* chaodata1, ChaoData2_* chaodata2);
void ChaoHud_Main(ObjectMaster* obj);

FunctionPointer(int, Chao_Animation, (ObjectMaster* a1, int a2), 0x734F00);
FunctionPointer(bool, Chao_FinishedAnimation, (ObjectMaster* a1), 0x735040);
FunctionPointer(EntityData1*, SpawnAnimal, (int unknown, float x, float y, float z), 0x4BE610);
ObjectFunc(UpdateSetDataAndDelete, 0x46C150);
FunctionPointer(void, GetActiveCollisions, (float x, float y, float z, float radius), 0x43ACD0);
FunctionPointer(int, GetGroundYPosition_CheckIntersection, (Mysterious64Bytes* a1, NJS_OBJECT* a2), 0x452B30);
FunctionPointer(void, RunChaoBehaviour, (ObjectMaster* obj, void* func), 0x71EF10);

ObjectFunc(Chao_RunMovements, 0x71EFB0);
ObjectFunc(Chao_PlayAnimation, 0x734EE0);
ObjectFunc(Chao_RunEmotionBall, 0x736140);
ObjectFunc(Chao_RunActions, 0x737610);
ObjectFunc(Chao_MoveEmotionBall, 0x741F20);
ObjectFunc(Chao_RunPhysics, 0x73FD10);
ObjectFunc(Chao_RunGravity, 0x73FEF0);

float GetDistance(NJS_VECTOR* orig, NJS_VECTOR* dest);
NJS_VECTOR GetPointToFollow(NJS_VECTOR* pos, NJS_VECTOR* dir, Rotation3* rot);
void LookAt(NJS_VECTOR* from, NJS_VECTOR* to, Angle* outx, Angle* outy);
void MoveForward(EntityData1* entity, float speed);

//void __usercall PutPlayerBehind(NJS_VECTOR* pos@<edi>, EntityData1* data@<esi>, float dist)
static const void* const PutPlayerBehindPtr = (void*)0x47DD50;
static inline void PutPlayerBehind(NJS_VECTOR* pos, EntityData1* data, float dist)
{
	__asm
	{
		push[dist]
		mov esi, [data]
		mov edi, [pos]
		call PutPlayerBehindPtr
		add esp, 4
	}
}