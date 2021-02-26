#include "stdafx.h"
#include "mod.h"

float GetDistance(NJS_VECTOR* orig, NJS_VECTOR* dest) {
	return sqrtf(powf(dest->x - orig->x, 2) + powf(dest->y - orig->y, 2) + powf(dest->z - orig->z, 2));
}

NJS_VECTOR GetPointToFollow(NJS_VECTOR* pos, NJS_VECTOR* dir, Rotation3* rot) {
	NJS_VECTOR point;
	njPushMatrix(_nj_unit_matrix_);
	njTranslateV(0, pos);
	njRotateY(0, -rot->y);
	njCalcPoint(0, dir, &point);
	njPopMatrix(1u);
	return point;
}

void MoveForward(EntityData1* entity, float speed) {
	njPushMatrix(_nj_unit_matrix_);
	njTranslateEx(&entity->Position);
	njRotateY(0, entity->Rotation.y);
	njRotateX(0, entity->Rotation.x);
	njTranslate(0, 0, 0, speed);
	njGetTranslation(0, &entity->Position);
	njPopMatrix(1u);
}

bool IsPlayerHoldingChao(char player, ChaoData1* chao) {
	EntityData1* data = EntityData1Ptrs[player];
	CharObj2* co2 = CharObj2Ptrs[player];

	if (co2->ObjectHeld && co2->ObjectHeld->Data1) {
		ChaoData1* chaodata = (ChaoData1*)co2->ObjectHeld->Data1;
		if (chaodata->ChaoDataBase_ptr == chao->ChaoDataBase_ptr) {
			return true;
		}
	}

	return false;
}

void njLookAt(NJS_VECTOR* from, NJS_VECTOR* to, Angle* outx, Angle* outy) {
	NJS_VECTOR unit = *to;
	
	njSubVector(&unit, from);

	if (outy) {
		*outy = static_cast<Angle>(atan2f(unit.x, unit.z) * 65536.0f * 0.1591549762031479f);
	}

	if (outx) {
		if (from->y == to->y) {
			*outx = 0;
		}
		else {
			Float len = 1.0f / squareroot(unit.z * unit.z + unit.x * unit.x + unit.y * unit.y);

			*outx = static_cast<Angle>((acos(len * 3.3499999f) * 65536.0f * 0.1591549762031479f)
				- (acos(-(len * unit.y)) * 65536.0f * 0.1591549762031479f));
		}
	}
}

int GetChaoByPointer(ObjectMaster* chao) {
	ChaoData1* chaodata = (ChaoData1*)chao->Data1;

	if (!chaodata) return 0;

	for (uint8_t i = 0; i < 24; ++i) {
		if (chaodata->ChaoDataBase_ptr == &GetChaoData(i)->data && chaodata->ChaoDataBase_ptr->Type != ChaoType_Egg) {
			return i + 1;
		}
	}

	return 0;
}