#include "stdafx.h"
#include "mod.h"

//Get the water collision directly above the chao
void IsChaoInWater(ObjectMaster* obj) {
	ChaoData1* chaodata1 = (ChaoData1*)obj->Data1;

	WriteData<5>((void*)0x49F201, 0x90);
	WriteData<5>((void*)0x49F1C0, 0x90);

	struct_a3 dyncolinfo;
	float height = -10000000;
	RunEntityIntersections(&chaodata1->entity, &dyncolinfo);

	if (dyncolinfo.ColFlagsA & (0x400002 | ColFlags_Water)) {
		if (dyncolinfo.DistanceMin > -1000000) height = dyncolinfo.DistanceMin + 1;
	}
	else {
		chaodata1->entity.Position.y += 10;
		RunEntityIntersections(&chaodata1->entity, &dyncolinfo);
		chaodata1->entity.Position.y -= 10;
	}

	WriteCall((void*)0x49F201, SpawnRipples);
	WriteCall((void*)0x49F1C0, SpawnSplashParticles);

	WriteData((float*)0x73C24C, height);
}

//Different water height for each chao in levels to swim in any water collision.
//Don't check if held or following player.
void Chao_Main_r(ObjectMaster* obj);
Trampoline Chao_Main_t((int)Chao_Main, (int)Chao_Main + 0x6, Chao_Main_r);
void Chao_Main_r(ObjectMaster* obj) {
	if (CurrentLevel < LevelIDs_SSGarden && (obj->Data1->Status & 0x1000) == 0 && obj->Data1->CharIndex != 1) {
		IsChaoInWater(obj);
	}

	ObjectFunc(original, Chao_Main_t.Target());
	original(obj);
}