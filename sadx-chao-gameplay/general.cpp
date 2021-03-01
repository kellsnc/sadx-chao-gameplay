#include "stdafx.h"
#include "mod.h"

float GetDistance(NJS_VECTOR* orig, NJS_VECTOR* dest) {
	return sqrtf(powf(dest->x - orig->x, 2) + powf(dest->y - orig->y, 2) + powf(dest->z - orig->z, 2));
}

NJS_VECTOR GetPointToFollow(NJS_VECTOR* pos, NJS_VECTOR* dir, Rotation3* rot) {
	NJS_VECTOR point;
	njPushMatrix(_nj_unit_matrix_);
	njTranslateV(0, pos);
	njRotateZ(0, rot->z);
	njRotateX(0, rot->x);
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

void LookAt(NJS_VECTOR* from, NJS_VECTOR* to, Angle* outx, Angle* outy) {
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