/*
  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "week7.h"

#include "boot/stage.h"
#include "boot/archive.h"
#include "boot/mem.h"
#include "boot/mutil.h"
#include "boot/timer.h"

//Charts
static u8 week7_cht_ugh_easy[] = {
	#include "iso/chart/ugh-easy.json.cht.h"
};
static u8 week7_cht_ugh_normal[] = {
	#include "iso/chart/ugh.json.cht.h"
};
static u8 week7_cht_ugh_hard[] = {
	#include "iso/chart/ugh-hard.json.cht.h"
};

static u8 week7_cht_guns_easy[] = {
	#include "iso/chart/guns-easy.json.cht.h"
};
static u8 week7_cht_guns_normal[] = {
	#include "iso/chart/guns.json.cht.h"
};
static u8 week7_cht_guns_hard[] = {
	#include "iso/chart/guns-hard.json.cht.h"
};

static u8 week7_cht_stress_easy[] = {
	#include "iso/chart/stress-easy.json.cht.h"
};
static u8 week7_cht_stress_normal[] = {
	#include "iso/chart/stress.json.cht.h"
};
static u8 week7_cht_stress_hard[] = {
	#include "iso/chart/stress-hard.json.cht.h"
};

static IO_Data week7_cht[][3] = {
	{
		(IO_Data)week7_cht_ugh_easy,
		(IO_Data)week7_cht_ugh_normal,
		(IO_Data)week7_cht_ugh_hard,
	},
	{
		(IO_Data)week7_cht_guns_easy,
		(IO_Data)week7_cht_guns_normal,
		(IO_Data)week7_cht_guns_hard,
	},
	{
		(IO_Data)week7_cht_stress_easy,
		(IO_Data)week7_cht_stress_normal,
		(IO_Data)week7_cht_stress_hard,
	}
};

//Characters
//Boyfriend
#include "character/bf.c"

//Senpai
#include "character/tank.c"

//Girlfriend
#include "character/gf.c"

static fixed_t Char_GF_GetParallax(Char_GF *this)
{
	(void)this;
	return FIXED_UNIT;
}

//Week7 Textures
static Gfx_Tex week7_tex_back0; //Background
static Gfx_Tex week7_tex_back1; //Trees
static Gfx_Tex week7_tex_back2; //Freaks
static Gfx_Tex week7_tex_back3; //Thorns background

static fixed_t week7_tank_x;
static fixed_t week7_tank_timer;

//Week 7 background functions
#define TANK_START_X FIXED_DEC(-400,1)
#define TANK_END_X   FIXED_DEC(400,1)
#define TANK_TIME_A FIXED_DEC(35,1)
#define TANK_TIME_B FIXED_DEC(55,1)

//Week7 background functions
static void Week7_Load(void)
{
	//Load assets
	IO_Data overlay_data;
	
	Gfx_LoadTex(&stage.tex_hud0, overlay_data = Overlay_DataRead(), 0); Mem_Free(overlay_data); //hud0.tim
	Gfx_LoadTex(&stage.tex_huds, overlay_data = Overlay_DataRead(), 0); Mem_Free(overlay_data); //huds.tim
	Gfx_LoadTex(&stage.tex_hud1, overlay_data = Overlay_DataRead(), 0); Mem_Free(overlay_data); //hud1.tim
	
	Gfx_LoadTex(&week7_tex_back0, overlay_data = Overlay_DataRead(), 0); Mem_Free(overlay_data); //back0.tim
	Gfx_LoadTex(&week7_tex_back1, overlay_data = Overlay_DataRead(), 0); Mem_Free(overlay_data); //back1.tim
	Gfx_LoadTex(&week7_tex_back2, overlay_data = Overlay_DataRead(), 0); Mem_Free(overlay_data); //back2.tim
	Gfx_LoadTex(&week7_tex_back3, overlay_data = Overlay_DataRead(), 0); Mem_Free(overlay_data); //back3.tim

	//Use sky coloured background
	Gfx_SetClear(245, 202, 81);
	
	//Load characters
	stage.player = Char_BF_New(FIXED_DEC(105,1),  FIXED_DEC(100,1));
	stage.opponent = Char_Tank_New(FIXED_DEC(-120,1),  FIXED_DEC(100,1));
	stage.gf = Char_GF_New(FIXED_DEC(0,1),  FIXED_DEC(-15,1));

	//Initialize tank state
    week7_tank_x = TANK_END_X;
	week7_tank_timer = RandomRange(TANK_TIME_A, TANK_TIME_B);
}

static void Week7_DrawBG(void)
{
	fixed_t fx, fy;
	
	//Draw foreground
	fx = stage.camera.x;
	fy = stage.camera.y;
	
	RECT fg_src = {0, 0, 255, 128};
	RECT_FIXED fg_dst = {
		FIXED_DEC(-340,1) - fx,
		FIXED_DEC(18,1) - fy,
		FIXED_DEC(340,1),
		FIXED_DEC(170,1)
	};
	
	Stage_DrawTex(&week7_tex_back0, &fg_src, &fg_dst, stage.camera.bzoom);
	fg_dst.x += fg_dst.w;
	fg_src.y += 127;
	Stage_DrawTex(&week7_tex_back0, &fg_src, &fg_dst, stage.camera.bzoom);
	
	//Move tank
	week7_tank_timer -= timer_dt;
	if (week7_tank_timer <= 0)
	{
		week7_tank_timer = RandomRange(TANK_TIME_A, TANK_TIME_B);
		week7_tank_x = TANK_START_X;
	}
	
	if (week7_tank_x < TANK_END_X)
		week7_tank_x += timer_dt * 30;
	
	//Get tank position
	fx = stage.camera.x * 2 / 3;
	fy = stage.camera.y * 2 / 3;
	
	u8 tank_angle = week7_tank_x * 0x34 / TANK_END_X;
	
	s16 tank_sin = MUtil_Sin(tank_angle);
	s16 tank_cos = MUtil_Cos(tank_angle);
	
	fixed_t tank_y = FIXED_DEC(175,1) - ((tank_cos * FIXED_DEC(180,1)) >> 8);
	
	//Get tank rotated points
	POINT tank_p0 = {-45, -45};
	MUtil_RotatePoint(&tank_p0, tank_sin, tank_cos);
	
	POINT tank_p1 = { 45, -45};
	MUtil_RotatePoint(&tank_p1, tank_sin, tank_cos);
	
	POINT tank_p2 = {-45,  45};
	MUtil_RotatePoint(&tank_p2, tank_sin, tank_cos);
	
	POINT tank_p3 = { 45,  45};
	MUtil_RotatePoint(&tank_p3, tank_sin, tank_cos);
	
	//Draw tank
	RECT tank_src = {129, 1, 126, 126};
	if (animf_count & 2)
		tank_src.y += 127;
	
	POINT_FIXED tank_d0 = {
		week7_tank_x + ((fixed_t)tank_p0.x << FIXED_SHIFT) - fx,
		      tank_y + ((fixed_t)tank_p0.y << FIXED_SHIFT) - fy
	};
	POINT_FIXED tank_d1 = {
		week7_tank_x + ((fixed_t)tank_p1.x << FIXED_SHIFT) - fx,
		      tank_y + ((fixed_t)tank_p1.y << FIXED_SHIFT) - fy
	};
	POINT_FIXED tank_d2 = {
		week7_tank_x + ((fixed_t)tank_p2.x << FIXED_SHIFT) - fx,
		      tank_y + ((fixed_t)tank_p2.y << FIXED_SHIFT) - fy
	};
	POINT_FIXED tank_d3 = {
		week7_tank_x + ((fixed_t)tank_p3.x << FIXED_SHIFT) - fx,
		      tank_y + ((fixed_t)tank_p3.y << FIXED_SHIFT) - fy
	};
	
	Stage_DrawTexArb(&week7_tex_back2, &tank_src, &tank_d0, &tank_d1, &tank_d2, &tank_d3, stage.camera.bzoom);
	
	//Draw sniper
	fx = stage.camera.x >> 1;
	fy = stage.camera.y >> 1;
	
	RECT snipe_src = {0, 0, 128, 255};
	RECT_FIXED snipe_dst = {
		FIXED_DEC(-190,1) - fx,
		FIXED_DEC(-90,1) - fy,
		FIXED_DEC(90,1),
		FIXED_DEC(180,1),
	};
	
	fixed_t snipe_bop;
	if ((stage.song_step & 0x3) == 0)
		snipe_bop = FIXED_UNIT - ((stage.note_scroll / 24) & FIXED_LAND);
	else
		snipe_bop = 0;
	
	snipe_dst.x -= snipe_bop << 1;
	snipe_dst.y += snipe_bop << 2;
	snipe_dst.w += snipe_bop << 2;
	snipe_dst.h -= snipe_bop << 2;
	
	Stage_DrawTex(&week7_tex_back2, &snipe_src, &snipe_dst, stage.camera.bzoom);
	
	//Draw ruins
	fx = stage.camera.x >> 2;
	fy = stage.camera.y >> 2;
	
	RECT ruinsf_src = {0, 0, 255, 72};
	RECT_FIXED ruinsf_dst = {
		FIXED_DEC(-240,1) - fx,
		FIXED_DEC(-70,1) - fy,
		FIXED_DEC(480,1),
		FIXED_DEC(135,1)
	};
	
	Stage_DrawTex(&week7_tex_back1, &ruinsf_src, &ruinsf_dst, stage.camera.bzoom);
	
	fx = stage.camera.x / 6;
	fy = stage.camera.y / 6;
	
	RECT ruinsb_src = {0, 72, 2558, 44};
	RECT_FIXED ruinsb_dst = {
		FIXED_DEC(-240,1) - fx,
		FIXED_DEC(-60,1) - fy,
		FIXED_DEC(480,1),
		FIXED_DEC(135,1)
	};
	
	Stage_DrawTex(&week7_tex_back1, &ruinsb_src, &ruinsb_dst, stage.camera.bzoom);
	
	//Draw clouds
	fx = stage.camera.x / 7;
	fy = stage.camera.y / 7;
	
	RECT cloud_src = {0, 116, 255, 53};
	RECT_FIXED cloud_dst = {
		FIXED_DEC(-260,1) - fx,
		FIXED_DEC(-130,1) - fy,
		FIXED_DEC(260,1),
		FIXED_DEC(260,1) * 53 / 256
	};
	
	Stage_DrawTex(&week7_tex_back1, &cloud_src, &cloud_dst, stage.camera.bzoom);
	cloud_dst.x += cloud_dst.w;
	cloud_dst.h = cloud_dst.w * 83 / 256;
	cloud_src.y = 172;
	cloud_src.h = 83;
	Stage_DrawTex(&week7_tex_back1, &cloud_src, &cloud_dst, stage.camera.bzoom);
	
	//Draw mountains
	fx = stage.camera.x >> 3;
	fy = stage.camera.y >> 3;
	
	RECT mountain_src = {0, 0, 255, 128};
	RECT_FIXED mountain_dst = {
		FIXED_DEC(-260,1) - fx,
		FIXED_DEC(-110,1) - fy,
		FIXED_DEC(260,1),
		FIXED_DEC(130,1)
	};
	
	Stage_DrawTex(&week7_tex_back3, &mountain_src, &mountain_dst, stage.camera.bzoom);
	mountain_dst.x += mountain_dst.w;
	mountain_src.y += 127;
	mountain_dst.h = FIXED_DEC(131,1);
	
	Stage_DrawTex(&week7_tex_back3, &mountain_src, &mountain_dst, stage.camera.bzoom);
}

static IO_Data Week7_GetChart(void)
{
	return week7_cht[stage.stage_id - StageId_7_1][stage.stage_diff];
}

static boolean Week7_NextStage(void)
{
	switch (stage.stage_id)
	{
		case StageId_7_1: //Senpai
			stage.stage_id = StageId_7_2;
			return true;
		case StageId_7_2: //Roses
			stage.stage_id = StageId_7_3;
			return true;
		case StageId_7_3: //Thorns
			return false;
		default:
			return false;
	}
}

void Week7_SetPtr(void)
{
	//Set pointers
	stageoverlay_load = Week7_Load;
	stageoverlay_tick = NULL;
	stageoverlay_notemoviment = NULL;
	stageoverlay_drawbg =  Week7_DrawBG;
	stageoverlay_drawmd = NULL;
	stageoverlay_drawfg = NULL;
	stageoverlay_free = NULL;
	stageoverlay_getchart = Week7_GetChart;
	stageoverlay_loadscreen = NULL;
	stageoverlay_nextstage = Week7_NextStage;
}
