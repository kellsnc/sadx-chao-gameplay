#include "stdafx.h"
#include "mod.h"

float GetDistance(NJS_VECTOR* orig, NJS_VECTOR* dest) {
	return sqrtf(powf(dest->x - orig->x, 2) + powf(dest->y - orig->y, 2) + powf(dest->z - orig->z, 2));
}

NJS_VECTOR GetPointToFollow(NJS_VECTOR* pos, Rotation3* rot) {
	NJS_VECTOR point;

	NJS_VECTOR dir = { -10, 10, 5 };
	njPushMatrix(_nj_unit_matrix_);
	njTranslateV(0, pos);
	njRotateY(0, -rot->y);
	njCalcPoint(0, &dir, &point);
	njPopMatrix(1u);
	return point;
}

Rotation3 fPositionToRotation(NJS_VECTOR* orig, NJS_VECTOR* point) {
	NJS_VECTOR dist;
	Rotation3 result;

	dist.x = orig->x - point->x;
	dist.y = orig->y - point->y;
	dist.z = orig->z - point->z;

	result.x = atan2(dist.y, dist.z) * 65536.0 * -0.1591549762031479;
	result.y = atan2(dist.x, dist.z) * 65536.0 * 0.1591549762031479;

	result.y = -result.y - 0x4000;
	return result;
}

NJS_VECTOR GetPathPosition(NJS_VECTOR* orig, NJS_VECTOR* dest, float state) {
	NJS_VECTOR result;
	result.x = (dest->x - orig->x) * state + orig->x;
	result.y = (dest->y - orig->y) * state + orig->y;
	result.z = (dest->z - orig->z) * state + orig->z;

	return result;
}

bool IsPointInsideSphere(NJS_VECTOR* center, NJS_VECTOR* pos, float radius) {
	return (powf(pos->x - center->x, 2) + pow(pos->y - center->y, 2) + pow(pos->z - center->z, 2)) <= pow(radius, 2);
}