/*
  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "week5.h"

#include "boot/stage.h"
#include "boot/archive.h"
#include "boot/main.h"
#include "boot/mem.h"

//Charts
static u8 week5_cht_cocoa_easy[] = {
	#include "iso/chart/cocoa-easy.json.cht.h"
};
static u8 week5_cht_cocoa_normal[] = {
	#include "iso/chart/cocoa.json.cht.h"
};
static u8 week5_cht_cocoa_hard[] = {
	#include "iso/chart/cocoa-hard.json.cht.h"
};

static u8 week5_cht_eggnog_easy[] = {
	#include "iso/chart/eggnog-easy.json.cht.h"
};
static u8 week5_cht_eggnog_normal[] = {
	#include "iso/chart/eggnog.json.cht.h"
};
static u8 week5_cht_eggnog_hard[] = {
	#include "iso/chart/eggnog-hard.json.cht.h"
};

static u8 week5_cht_winter_horrorland_easy[] = {
	#include "iso/chart/winter-horrorland-easy.json.cht.h"
};
static u8 week5_cht_winter_horrorland_normal[] = {
	#include "iso/chart/winter-horrorland.json.cht.h"
};
static u8 week5_cht_winter_horrorland_hard[] = {
	#include "iso/chart/winter-horrorland-hard.json.cht.h"
};

static IO_Data week5_cht[][3] = {
	{
		(IO_Data)week5_cht_cocoa_easy,
		(IO_Data)week5_cht_cocoa_normal,
		(IO_Data)week5_cht_cocoa_hard,
	},
	{
		(IO_Data)week5_cht_eggnog_easy,
		(IO_Data)week5_cht_eggnog_normal,
		(IO_Data)week5_cht_eggnog_hard,
	},
	{
		(IO_Data)week5_cht_winter_horrorland_easy,
		(IO_Data)week5_cht_winter_horrorland_normal,
		(IO_Data)week5_cht_winter_horrorland_hard,
	},
};

//Characters
//Boyfriend
#include "character/xmasbf.c"

//Christimas Parents
#include "character/xmasp.c"

//Christimas Monster
#include "character/monsterx.c"

//Girlfriend
#include "character/xmasgf.c"

static fixed_t Char_XmasGF_GetParallax(Char_XmasGF *this)
{
	(void)this;
	return FIXED_UNIT;
}

//Week 5 textures
static Gfx_Tex week5_tex_back0; //Back wall
static Gfx_Tex week5_tex_back1; //Second floor
static Gfx_Tex week5_tex_back2; //Lower bop
//static Gfx_Tex week5_tex_back3; //Santa
static Gfx_Tex week5_tex_back4; //Upper bop
static Gfx_Tex week5_tex_back5; //Tree
static Gfx_Tex week5_tex_back0a2; //Evil Back wall
static Gfx_Tex week5_tex_back1a2; //Evil tree

//Week 5 background functions
static void Week5_Load(void)
{
	//Load assets
	IO_Data overlay_data;
	
	Gfx_LoadTex(&stage.tex_huds, overlay_data = Overlay_DataRead(), 0); Mem_Free(overlay_data); //huds.tim
	Gfx_LoadTex(&stage.tex_hud1, overlay_data = Overlay_DataRead(), 0); Mem_Free(overlay_data); //hud1.tim
	
	Gfx_LoadTex(&week5_tex_back0, overlay_data = Overlay_DataRead(), 0); Mem_Free(overlay_data); //back0.tim
	Gfx_LoadTex(&week5_tex_back1, overlay_data = Overlay_DataRead(), 0); Mem_Free(overlay_data); //back1.tim
	Gfx_LoadTex(&week5_tex_back2, overlay_data = Overlay_DataRead(), 0); Mem_Free(overlay_data); //back2.tim
	//Gfx_LoadTex(&week5_tex_back3, overlay_data = Overlay_DataRead(), 0); Mem_Free(overlay_data); //back3.tim
	Gfx_LoadTex(&week5_tex_back4, overlay_data = Overlay_DataRead(), 0); Mem_Free(overlay_data); //back4.tim
	Gfx_LoadTex(&week5_tex_back5, overlay_data = Overlay_DataRead(), 0); Mem_Free(overlay_data); //back5.tim

	Gfx_LoadTex(&week5_tex_back0a2, overlay_data = Overlay_DataRead(), 0); Mem_Free(overlay_data); //back0a2.tim
	Gfx_LoadTex(&week5_tex_back1a2, overlay_data = Overlay_DataRead(), 0); Mem_Free(overlay_data); //back1a2.tim
	
	//Load characters
	stage.player = Char_XmasBF_New(FIXED_DEC(90,1), FIXED_DEC(85,1));
    
	if (stage.stage_id == StageId_5_3)
	stage.opponent = Char_Monsterx_New(FIXED_DEC(-125,1), FIXED_DEC(70,1));
	else
    stage.opponent = Char_XmasP_New(FIXED_DEC(-190,1), FIXED_DEC(85,1));
	stage.gf = Char_XmasGF_New(FIXED_DEC(0,1), FIXED_DEC(-15,1));
}

static void Week5_DrawBG()
{
	
	fixed_t fx, fy;
	
	fixed_t beat_bop;
	if ((stage.song_step & 0x3) == 0)
		beat_bop = FIXED_UNIT - ((stage.note_scroll / 24) & FIXED_LAND);
	else
		beat_bop = 0;
	
	//Draw Santa
	
	//Draw snow
	fx = stage.camera.x;
	fy = stage.camera.y;
	
	RECT snow_src = {120, 155, 135, 100};
	RECT_FIXED snow_dst = {
		FIXED_DEC(-350,1) - fx,
		FIXED_DEC(44,1) - fy,
		FIXED_DEC(570,1),
		FIXED_DEC(27,1)
	};
	
	Stage_DrawTex(&week5_tex_back2, &snow_src, &snow_dst, stage.camera.bzoom);
	snow_src.y = 255; snow_src.h = 0;
	snow_dst.y += snow_dst.h - FIXED_UNIT;
	snow_dst.h *= 3;
	Stage_DrawTex(&week5_tex_back2, &snow_src, &snow_dst, stage.camera.bzoom);
	
	//Draw boppers
	static const struct Back_Week5_LowerBop
	{
		RECT src;
		RECT_FIXED dst;
	} lbop_piece[] = {
		{{0, 0, 80, 102}, {FIXED_DEC(-315,1), FIXED_DEC(-30,1), FIXED_DEC(80,1), FIXED_DEC(102,1)}},
		{{175, 3, 74, 150}, {FIXED_DEC(-120,1), FIXED_DEC(-80,1), FIXED_DEC(74,1), FIXED_DEC(151,1)}},
		{{81, 0, 70, 128}, {FIXED_DEC(30,1), FIXED_DEC(-70,1), FIXED_DEC(70,1), FIXED_DEC(128,1)}},
		{{151, 0, 23, 132}, {FIXED_DEC(100,1), FIXED_DEC(-70,1), FIXED_DEC(23,1), FIXED_DEC(132,1)}},
		{{0, 109, 41, 139}, {FIXED_DEC(123,1), FIXED_DEC(-69,1), FIXED_DEC(41,1), FIXED_DEC(139,1)}},
		{{41, 126, 69, 129}, {FIXED_DEC(164,1), FIXED_DEC(-52,1), FIXED_DEC(69,1), FIXED_DEC(130,1)}},
	};
	
	const struct Back_Week5_LowerBop *lbop_p = lbop_piece;
	for (size_t i = 0; i < COUNT_OF(lbop_piece); i++, lbop_p++)
	{
		RECT_FIXED lbop_dst = {
			lbop_p->dst.x - fx - (beat_bop << 1),
			lbop_p->dst.y - fy + (beat_bop << 3),
			lbop_p->dst.w + (beat_bop << 2),
			lbop_p->dst.h - (beat_bop << 3),
		};
		Stage_DrawTex(&week5_tex_back2, &lbop_p->src, &lbop_dst, stage.camera.bzoom);
	}
	
	//Draw tree
	fx = stage.camera.x * 2 / 5;
	fy = stage.camera.y * 2 / 5;
	
	RECT tree_src = {0, 0, 174, 210};
	RECT_FIXED tree_dst = {
		FIXED_DEC(-86,1) - fx,
		FIXED_DEC(-150,1) - fy,
		FIXED_DEC(174,1),
		FIXED_DEC(210,1)
	};
	
	Stage_DrawTex(&week5_tex_back5, &tree_src, &tree_dst, stage.camera.bzoom);
	
	//Draw second floor
	fx = stage.camera.x >> 2;
	fy = stage.camera.y >> 2;
	
	static const struct Back_Week5_FloorPiece
	{
		RECT src;
		fixed_t scale;
	} floor_piece[] = {
		{{  0, 0, 161, 255}, FIXED_DEC(14,10)},
		{{161, 0,   9, 255}, FIXED_DEC(7,1)},
		{{171, 0,  85, 255}, FIXED_DEC(14,10)},
	};
	
	RECT_FIXED floor_dst = {
		FIXED_DEC(-220,1) - fx,
		FIXED_DEC(-115,1) - fy,
		0,
		FIXED_DEC(180,1)
	};
	
	const struct Back_Week5_FloorPiece *floor_p = floor_piece;
	for (size_t i = 0; i < COUNT_OF(floor_piece); i++, floor_p++)
	{
		floor_dst.w = floor_p->src.w ? (floor_p->src.w * floor_p->scale) : floor_p->scale;
		Stage_DrawTex(&week5_tex_back1, &floor_p->src, &floor_dst, stage.camera.bzoom);
		floor_dst.x += floor_dst.w;
	}
	
	//Draw boppers
	static const struct Back_Week5_UpperBop
	{
		RECT src;
		RECT_FIXED dst;
	} ubop_piece[] = {
		{{0, 0, 255, 76}, {FIXED_DEC(-200,1), FIXED_DEC(-132,1), FIXED_DEC(256,1)*6/7, FIXED_DEC(76,1)*6/7}},
		{{0, 76, 255, 76}, {FIXED_DEC(50,1), FIXED_DEC(-132,1), FIXED_DEC(256,1)*6/7, FIXED_DEC(76,1)*6/7}}
	};
	
	const struct Back_Week5_UpperBop *ubop_p = ubop_piece;
	for (size_t i = 0; i < COUNT_OF(ubop_piece); i++, ubop_p++)
	{
		RECT_FIXED ubop_dst = {
			ubop_p->dst.x - fx,
			ubop_p->dst.y - fy + (beat_bop << 2),
			ubop_p->dst.w,
			ubop_p->dst.h - (beat_bop << 2),
		};
		Stage_DrawTex(&week5_tex_back4, &ubop_p->src, &ubop_dst, stage.camera.bzoom);
	}
	
	//Draw back wall
	fx = stage.camera.x >> 3;
	fy = stage.camera.y >> 3;
	
	static const struct Back_Week5_WallPiece
	{
		RECT src;
		fixed_t scale;
	} wall_piece[] = {
		{{  0, 0, 113, 255}, FIXED_DEC(1,1)},
		{{113, 0,   6, 255}, FIXED_DEC(17,1)},
		{{119, 0, 137, 255}, FIXED_DEC(1,1)},
	};
	
	RECT_FIXED wall_dst = {
		FIXED_DEC(-180,1) - fx,
		FIXED_DEC(-130,1) - fy,
		0,
		FIXED_DEC(190,1)
	};
	
	RECT wall_src = {0, 255, 0, 0};
	RECT_FIXED wall_fill;
	wall_fill.x = wall_dst.x;
	wall_fill.y = wall_dst.y + wall_dst.h - FIXED_UNIT;
	wall_fill.w = FIXED_DEC(300,1);
	wall_fill.h = FIXED_DEC(100,1);
	Stage_DrawTex(&week5_tex_back0, &wall_src, &wall_fill, stage.camera.bzoom);
	
	const struct Back_Week5_WallPiece *wall_p = wall_piece;
	for (size_t i = 0; i < COUNT_OF(wall_piece); i++, wall_p++)
	{
		wall_dst.w = wall_p->src.w ? (wall_p->src.w * wall_p->scale) : wall_p->scale;
		Stage_DrawTex(&week5_tex_back0, &wall_p->src, &wall_dst, stage.camera.bzoom);
		wall_dst.x += wall_dst.w;
	}
}


//evil!!!
static void Week5_DrawBGEvil()
{
    fixed_t fx, fy;

	//Draw evil snow
	fx = stage.camera.x;
	fy = stage.camera.y;
	
	RECT snow_src = {120, 155, 135, 100};
	RECT_FIXED snow_dst = {
		FIXED_DEC(-350,1) - fx,
		FIXED_DEC(44,1) - fy,
		FIXED_DEC(570,1),
		FIXED_DEC(27,1)
	};

	Stage_DrawTexCol(&week5_tex_back2, &snow_src, &snow_dst, stage.camera.bzoom, 210 >> 1, 115 >> 1, 177 >> 1);
	snow_src.y = 255; snow_src.h = 0;
	snow_dst.y += snow_dst.h - FIXED_UNIT;
	snow_dst.h *= 3;
	Stage_DrawTexCol(&week5_tex_back2, &snow_src, &snow_dst, stage.camera.bzoom, 210 >> 1, 115 >> 1, 177 >> 1);

    //Draw evil tree
	fx = stage.camera.x * 2 / 5;
	fy = stage.camera.y * 2 / 5;
	
	RECT tree_src = {0, 0, 144, 255};
	RECT_FIXED tree_dst = {
		FIXED_DEC(-86,1) - fx,
		FIXED_DEC(-220,1) - fy,
		FIXED_DEC(204,1),
		FIXED_DEC(315,1)
	};
	
	Stage_DrawTex(&week5_tex_back1a2, &tree_src, &tree_dst, stage.camera.bzoom);

    //Draw evil back wall
	fx = stage.camera.x >> 3;
	fy = stage.camera.y >> 3;
	
	static const struct Back_Week5_WallPiece
	{
		RECT src;
		fixed_t scale;
	} wall_piece[] = {
		{{  0, 0, 113, 255}, FIXED_DEC(1,1)},
		{{113, 0,   6, 255}, FIXED_DEC(17,1)},
		{{119, 0, 137, 255}, FIXED_DEC(1,1)},
	};
	
	RECT_FIXED wall_dst = {
		FIXED_DEC(-180,1) - fx,
		FIXED_DEC(-130,1) - fy,
		0,
		FIXED_DEC(190,1)
	};

	RECT wall_src = {23, 158, 0, 0};
	RECT_FIXED wall_fill;
	wall_fill.x = wall_dst.x;
	wall_fill.y = wall_dst.y + wall_dst.h - FIXED_UNIT;
	wall_fill.w = FIXED_DEC(300,1);
	wall_fill.h = FIXED_DEC(100,1);
	Stage_DrawTex(&week5_tex_back0a2, &wall_src, &wall_fill, stage.camera.bzoom);
	
	const struct Back_Week5_WallPiece *wall_p = wall_piece;
	for (size_t i = 0; i < COUNT_OF(wall_piece); i++, wall_p++)
	{
		wall_dst.w = wall_p->src.w ? (wall_p->src.w * wall_p->scale) : wall_p->scale;
		Stage_DrawTex(&week5_tex_back0a2, &wall_p->src, &wall_dst, stage.camera.bzoom);
		wall_dst.x += wall_dst.w;
	}
}

static IO_Data Week5_GetChart(void)
{
	return week5_cht[stage.stage_id - StageId_5_1][stage.stage_diff];
}

static boolean Week5_LoadScreen(void)
{
	return stage.stage_id == StageId_5_2; //Going to Monster
}

static boolean Week5_NextStage(void)
{
	switch (stage.stage_id)
	{
		case StageId_5_1: //Cocoa
			stage.stage_id = StageId_5_2;
			return true;
		case StageId_5_2: //Eggnog
			stage.stage_id = StageId_5_3;
			stageoverlay_drawbg = Week5_DrawBGEvil; //switch to evil!!!!
			Character_Free(stage.opponent);
			stage.opponent = Char_Monsterx_New(FIXED_DEC(-125,1), FIXED_DEC(70,1));
			return true;
		case StageId_5_3: //Winter-horroland
			return false;
		default:
			return false;
	}
}

void Week5_SetPtr(void)
{
	//Set pointers
	stageoverlay_load = Week5_Load;
	stageoverlay_tick = NULL;
	stageoverlay_notemoviment = NULL;
	stageoverlay_drawbg = (stage.stage_id == StageId_5_3) ? Week5_DrawBGEvil : Week5_DrawBG;
	stageoverlay_drawmd = NULL;
	stageoverlay_drawfg = NULL;
	stageoverlay_dialog = NULL;
	stageoverlay_free = NULL;
	stageoverlay_getchart = Week5_GetChart;
	stageoverlay_loadscreen = Week5_LoadScreen;
	stageoverlay_nextstage = Week5_NextStage;
}
