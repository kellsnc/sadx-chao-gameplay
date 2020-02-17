#include "stdafx.h"
#include "mod.h"

std::vector<NJS_PLANE> waterlist = {};

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

//Different water height for each chao
void Chao_Main_r(ObjectMaster* obj);
Trampoline Chao_Main_t((int)Chao_Main, (int)Chao_Main + 0x6, Chao_Main_r);
void Chao_Main_r(ObjectMaster* obj) {
	if (CurrentLevel < LevelIDs_SSGarden) {
		IsChaoInWater(obj);
	}

	ObjectFunc(original, Chao_Main_t.Target());
	original(obj);
}