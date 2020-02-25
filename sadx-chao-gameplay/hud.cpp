#include "stdafx.h"
#include "mod.h"

NJS_TEXNAME CHAOHUD_TEXNAME[6];
NJS_TEXLIST CHAOHUD_TEXLIST = { arrayptrandlength(CHAOHUD_TEXNAME) };

void CalculateScreenPos(NJS_QUAD_TEXTURE* quad, float p1, float p2, float s1, float s2) {
	quad->x1 = HorizontalStretch * p1;
	quad->x2 = quad->x1 + HorizontalStretch * s1;
	quad->y1 = VerticalStretch * p2;
	quad->y2 = quad->y1 + VerticalStretch * s2;
}

void ChaoHud_DrawSprite(char player, char chao, float alpha) {
	NJS_QUAD_TEXTURE Sprite = { 0, 0, 0, 0, 0, 0, 1, 1 };

	CalculateScreenPos(&Sprite, 11, 140 + (player * 45), 40, 40);

	if (!MissedFrames) {
		njSetTexture(&CHAOHUD_TEXLIST);
		SetDefaultAlphaBlend();
		Direct3D_SetZFunc(7u);
		Direct3D_EnableHudAlpha(1);
		NJS_COLOR color = { 0xFFFFFFFF };
		color.argb.a = alpha;
		SetHudColorAndTextureNum(0, color);
		DrawRectPoints((NJS_POINT2*)&Sprite, 1);
		
		CalculateScreenPos(&Sprite, 21, 150 + (player * 45), 20, 20);
		SetHudColorAndTextureNum(2 + player, color);
		DrawRectPoints((NJS_POINT2*)&Sprite, 1);
		
		if (chao == 0) {
			CalculateScreenPos(&Sprite, 11, 140 + (player * 45), 40, 40);
			SetHudColorAndTextureNum(1, color);
			DrawRectPoints((NJS_POINT2*)&Sprite, 1);
		}
	}
}

void ChaoHud_Delete(ObjectMaster* obj) {
	njReleaseTexture(&CHAOHUD_TEXLIST);
}

void ChaoHud_Main(ObjectMaster* obj) {
	EntityData1* data = obj->Data1;

	if (!data->Action) {
		LoadPVM("chaohud", &CHAOHUD_TEXLIST);
		obj->DeleteSub = ChaoHud_Delete;
		data->Action = 1;
		data->CharID = 0xFF;
	}

	if (++data->InvulnerableTime > 60) {
		data->CharID -= 2;
		if (data->CharID == 1) {
			DeleteObject_(obj);
			return;
		}
	}

	for (char p = 0; p < PLAYERCOUNT; ++p) {
		ChaoHud_DrawSprite(p, ChaoMaster.ChaoHandles[p].SelectedChao, data->CharID);
	}
}