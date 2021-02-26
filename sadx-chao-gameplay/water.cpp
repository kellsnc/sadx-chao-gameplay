#include "stdafx.h"
#include "mod.h"

bool IsChaoInWater(ChaoData1* chaodata1, ChaoData2_* chaodata2) {
	Mysterious64Bytes colthing;
	colthing.Position = { chaodata1->entity.Position.x, chaodata1->entity.Position.y - 1.0f, chaodata1->entity.Position.z };

	// Get all collisions within a radius
	GetActiveCollisions(colthing.Position.x, colthing.Position.y, colthing.Position.z, 200.0f);
	float max = 10000000.0f;
	int top_flags = 0;

	// For all found collisions
	if (DynamicCOLCount_B > 0) {
		for (int i = 0; i < DynamicCOLCount_B; ++i) {
			DynamicCOL* col = &DynamicCOLArray_LandTable[i];

			// Check if the collision is above, then only save if the lowest ever found
			if (GetGroundYPosition_CheckIntersection(&colthing, col->Model)) {
				if (colthing.struct_v3_b.SomeFlag == 1
					&& colthing.Position.y < colthing.struct_v3_b.Distance
					&& colthing.struct_v3_b.Distance < max) {
					max = colthing.struct_v3_b.Distance;
					top_flags = col->Flags;
				}
			}
		}
	}

	// If the collision directly above is water
	if (top_flags & (0x400002 | ColFlags_Water)) {
		chaodata2->WaterHeight = max;
		return true;
	}

	return false;
}

signed int __cdecl Chao_DetectWater_r(ObjectMaster* obj);
Trampoline Chao_DetectWater_t(0x73C200, 0x73C207, Chao_DetectWater_r);
signed int __cdecl Chao_DetectWater_r(ObjectMaster* obj) {
	ChaoData1* data1 = (ChaoData1*)obj->Data1;
	ChaoData2_* data2 = (ChaoData2_*)obj->Data2;

	__int16 state = data1->unknown_d[0].field_0;
	bool result = false;

	if (data1->entity.Status & StatusChao_Held) {
		state &= ~ChaoState_Unk1;
	}
	else {
		if (IsChaoInWater(data1, data2)) {
			if (!(state & ChaoState_Unk1)) {
				state |= ChaoState_Unk1;
				RunChaoBehaviour(obj, (void*)0x73C110);
			}

			if (data2->field_4.y < 0.0f) {
				data2->field_4.y = data2->field_4.y * 0.1f;
			}

			state |= ChaoState_Water;
			result = true;
		}
		else {
			state &= ~(ChaoState_Water | ChaoState_Unk1);
		}
	}

	data1->unknown_d[0].field_0 = state;
	return result;
}