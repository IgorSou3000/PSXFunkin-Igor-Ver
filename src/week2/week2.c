/*
  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "week2.h"

#include "boot/stage.h"
#include "boot/archive.h"
#include "boot/main.h"
#include "boot/mem.h"
#include "boot/audio.h"

#include "stdlib.h"

//thunder stuff
fixed_t week2_thunder;
fixed_t week2_thunderspd = FIXED_DEC(290,1);

//thunder sound
u32 Week2_Sounds[2];

//Charts
static u8 week2_cht_spookeez_easy[] = {
	#include "iso/chart/spookeez-easy.json.cht.h"
};
static u8 week2_cht_spookeez_normal[] = {
	#include "iso/chart/spookeez.json.cht.h"
};
static u8 week2_cht_spookeez_hard[] = {
	#include "iso/chart/spookeez-hard.json.cht.h"
};

static u8 week2_cht_south_easy[] = {
	#include "iso/chart/south-easy.json.cht.h"
};
static u8 week2_cht_south_normal[] = {
	#include "iso/chart/south.json.cht.h"
};
static u8 week2_cht_south_hard[] = {
	#include "iso/chart/south-hard.json.cht.h"
};

static u8 week2_cht_monster_easy[] = {
	#include "iso/chart/monster-easy.json.cht.h"
};
static u8 week2_cht_monster_normal[] = {
	#include "iso/chart/monster.json.cht.h"
};
static u8 week2_cht_monster_hard[] = {
	#include "iso/chart/monster-hard.json.cht.h"
};

static IO_Data week2_cht[][3] = {
	{
		(IO_Data)week2_cht_spookeez_easy,
		(IO_Data)week2_cht_spookeez_normal,
		(IO_Data)week2_cht_spookeez_hard,
	},
	{
		(IO_Data)week2_cht_south_easy,
		(IO_Data)week2_cht_south_normal,
		(IO_Data)week2_cht_south_hard,
	},
	{
		(IO_Data)week2_cht_monster_easy,
		(IO_Data)week2_cht_monster_normal,
		(IO_Data)week2_cht_monster_hard,
	},
};

//Characters
//Boyfriend
#include "character/bf.c"

//Spooky Kids
#include "character/spook.c"

//Monster
#include "character/monster.c"

//Girlfriend
#include "character/gf.c"

static fixed_t Char_GF_GetParallax(Char_GF *this)
{
	(void)this;
	return FIXED_UNIT;
}

//Week 2 textures
static Gfx_Tex week2_tex_back0; //Background
static Gfx_Tex week2_tex_back1; //Window
static Gfx_Tex week2_tex_back2; //Lightning window


//Lightning Window Animation
boolean week2_lightanim;
s16 week2_anims;

//Week 2 background functions
static void Week2_Load(void)
{
	//Load assets
	IO_Data overlay_data;
	
	Gfx_LoadTex(&stage.tex_huds, overlay_data = Overlay_DataRead(), 0); Mem_Free(overlay_data); //huds.tim
	Gfx_LoadTex(&stage.tex_hud1, overlay_data = Overlay_DataRead(), 0); Mem_Free(overlay_data); //hud1.tim
	
	Gfx_LoadTex(&week2_tex_back0, overlay_data = Overlay_DataRead(), 0); Mem_Free(overlay_data); //back0.tim
	Gfx_LoadTex(&week2_tex_back1, overlay_data = Overlay_DataRead(), 0); Mem_Free(overlay_data); //back1.tim
	Gfx_LoadTex(&week2_tex_back2, overlay_data = Overlay_DataRead(), 0); Mem_Free(overlay_data); //back2.ti
	
	//Load characters
	stage.player = Char_BF_New(FIXED_DEC(56,1), FIXED_DEC(85,1));

	if (stage.stage_id == StageId_2_3) //monster
	stage.opponent = Char_Monster_New(FIXED_DEC(-90,1), FIXED_DEC(85,1));
	else
	stage.opponent = Char_Spook_New(FIXED_DEC(-90,1), FIXED_DEC(85,1));

	stage.gf = Char_GF_New(FIXED_DEC(0,1), FIXED_DEC(-15,1));

	//load thunder sound
	CdlFILE file;
    IO_FindFile(&file, "\\SOUND\\THUNDER1.VAG;1");
    u32 *data = IO_ReadFile(&file);
    Week2_Sounds[0] = Audio_LoadVAGData(data, file.size);

	IO_FindFile(&file, "\\SOUND\\THUNDER2.VAG;1");
    data = IO_ReadFile(&file);
    Week2_Sounds[1] = Audio_LoadVAGData(data, file.size);
    
	for (int i = 0; i < 2; i++)
	printf("address = %08x\n", Week2_Sounds[i]);

	Mem_Free(data);

	Gfx_SetClear(47, 51, 90);
}

static void Week2_Tick()
{
	if (stage.player->animatable.anim == PlayerAnim_Sweat)
	stage.player->ignoreanim = true;
	else 
	stage.player->ignoreanim = false;

	//Stage specific events
	if (stage.flag & STAGE_FLAG_JUST_STEP)
	{
		switch (stage.stage_id)
		{
			case StageId_2_1:
			case StageId_2_2:
			case StageId_2_3:
            //make sure the thunder and the stuffs just be on when opponent hit da notes
				//BF sweat
				if ((stage.song_step > 35 && RandomRange(0,180) == 90) || (stage.stage_id == StageId_2_3 && stage.song_step == -17))
				{
					week2_lightanim = true;
					stage.player->set_anim(stage.player, PlayerAnim_Sweat);
					week2_thunder = FIXED_DEC(255,1);
					Audio_PlaySound(Week2_Sounds[RandomRange(0,1)]);
				}
				break;
			default:
				break;
			}
		}
	//windows anim
	if (week2_lightanim == true)
		week2_anims = (stage.song_step >> 1) % 0x3;
    
	//stop windows anim
	if (week2_anims == 2)
	week2_lightanim = false;
}

static void Week2_DrawFG()
{
    //Draw white week2_thunder
    if (week2_thunder > 0)
	{
	static const RECT flash = {0, 0, SCREEN_WIDTH, SCREEN_HEIGHT};
	u8 flash_col = week2_thunder >> FIXED_SHIFT;
	Gfx_BlendRect(&flash, flash_col, flash_col, flash_col, 1);
	week2_thunder -= FIXED_MUL(week2_thunderspd, timer_dt);
	}
}
static void Week2_DrawBG()
{
	fixed_t fx, fy;
	fx = stage.camera.x;
	fy = stage.camera.y;
    
	FntPrint("lightwin %d", week2_anims);
	
	if (week2_lightanim == true)
	{
	//Draw Lightning window 
	RECT lightning_src = {0,0, 220, 127};
	RECT_FIXED lightning_dst = {
		FIXED_DEC(-172,1) - fx,
		FIXED_DEC(-128,1) - fy,
		FIXED_DEC(216,1),
		FIXED_DEC(120,1)
	};

	Stage_DrawTex(&week2_tex_back2, &lightning_src, &lightning_dst, stage.camera.bzoom);
}

	//Draw window
	RECT window_src = {0, 0, 228, 128};
	RECT_FIXED window_dst = {
		FIXED_DEC(-170,1) - fx,
		FIXED_DEC(-125,1) - fy,
		FIXED_DEC(216,1),
		FIXED_DEC(120,1)
	};
	
	Stage_DrawTex(&week2_tex_back1, &window_src, &window_dst, stage.camera.bzoom);
	
	//Draw window light
	RECT windowl_src = {0, 128, 255, 127};
	RECT_FIXED windowl_dst = {
		FIXED_DEC(-130,1) - fx,
		FIXED_DEC(44,1) - fy,
		FIXED_DEC(350,1),
		FIXED_DEC(146,1)
	};
	
	Stage_DrawTex(&week2_tex_back1, &windowl_src, &windowl_dst, stage.camera.bzoom);
	
	//Draw background
	RECT back_src = {0, 0, 255, 255};
	RECT_FIXED back_dst = {
		FIXED_DEC(-185,1) - fx,
		FIXED_DEC(-125,1) - fy,
		FIXED_DEC(353,1),
		FIXED_DEC(267,1)
	};
	
	Stage_DrawTex(&week2_tex_back0, &back_src, &back_dst, stage.camera.bzoom);
}

static IO_Data Week2_GetChart(void)
{
	return week2_cht[stage.stage_id - StageId_2_1][stage.stage_diff];
}

static boolean Week2_LoadScreen(void)
{
	return stage.stage_id == StageId_2_2; //Going to Monster
}

static boolean Week2_NextStage(void)
{
	switch (stage.stage_id)
	{
		case StageId_2_1: //Spookeez
			stage.stage_id = StageId_2_2;
			return true;
		case StageId_2_2: //South
			stage.stage_id = StageId_2_3;
			Character_Free(stage.opponent);
			stage.opponent = Char_Monster_New(FIXED_DEC(-90,1), FIXED_DEC(85,1));
			return true;
		case StageId_2_3: //Monster
			return false;
		default:
			return false;
	}
}

void Week2_SetPtr(void)
{
	//Set pointers
	stageoverlay_load = Week2_Load;
	stageoverlay_tick = Week2_Tick;
	stageoverlay_notemoviment = NULL;
	stageoverlay_drawbg = Week2_DrawBG;
	stageoverlay_drawmd = NULL;
	stageoverlay_drawfg = Week2_DrawFG;
	stageoverlay_dialog = NULL;
	stageoverlay_free = NULL;
	stageoverlay_getchart = Week2_GetChart;
	stageoverlay_loadscreen = Week2_LoadScreen;
	stageoverlay_nextstage = Week2_NextStage;
}
