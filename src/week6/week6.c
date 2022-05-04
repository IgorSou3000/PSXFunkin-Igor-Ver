/*
  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "week6.h"

#include "boot/stage.h"
#include "boot/archive.h"
#include "boot/mem.h"
#include "boot/mutil.h"
#include "boot/timer.h"

#include "boot/font.h"
#include "boot/audio.h"
//OG dialog code by bilious
//changes and improvements by igorsou3000(me LOL)

//made this a struct for avoid repeat this so much
typedef struct
{
const char * text; //The text that is displayed
u8 pport; //player's portrait
}Dialogs;

//this variable will receive the size of the dialogs and transfer it to week6_alldia
u8 week6_setsize;

//week6 sounds
u32 Week6_Sounds[1];

//Charts
static u8 week6_cht_senpai_easy[] = {
	#include "iso/chart/senpai-easy.json.cht.h"
};
static u8 week6_cht_senpai_normal[] = {
	#include "iso/chart/senpai.json.cht.h"
};
static u8 week6_cht_senpai_hard[] = {
	#include "iso/chart/senpai-hard.json.cht.h"
};

static u8 week6_cht_roses_easy[] = {
	#include "iso/chart/roses-easy.json.cht.h"
};
static u8 week6_cht_roses_normal[] = {
	#include "iso/chart/roses.json.cht.h"
};
static u8 week6_cht_roses_hard[] = {
	#include "iso/chart/roses-hard.json.cht.h"
};

static u8 week6_cht_thorns_easy[] = {
	#include "iso/chart/thorns-easy.json.cht.h"
};
static u8 week6_cht_thorns_normal[] = {
	#include "iso/chart/thorns.json.cht.h"
};
static u8 week6_cht_thorns_hard[] = {
	#include "iso/chart/thorns-hard.json.cht.h"
};

static IO_Data week6_cht[][3] = {
	{
		(IO_Data)week6_cht_senpai_easy,
		(IO_Data)week6_cht_senpai_normal,
		(IO_Data)week6_cht_senpai_hard,
	},
	{
		(IO_Data)week6_cht_roses_easy,
		(IO_Data)week6_cht_roses_normal,
		(IO_Data)week6_cht_roses_hard,
	},
	{
		(IO_Data)week6_cht_thorns_easy,
		(IO_Data)week6_cht_thorns_normal,
		(IO_Data)week6_cht_thorns_hard,
	}
};

//Freaks assets
static const CharFrame freaks_frame[] = {
	{0, {  0,   0,  83,  79}, { 17,  79}}, //normal 0
	{0, { 84,   0,  83,  76}, { 18,  76}}, //normal 1
	{0, {168,   0,  84,  82}, { 17,  82}}, //normal 2
	{0, {  0,  80,  83,  81}, { 17,  81}}, //normal 3
	
	{0, { 84,  77,  83,  79}, { 17,  79}}, //disuaded 0
	{0, {168,  83,  83,  76}, { 18,  76}}, //disuaded 1
	{0, {  0, 162,  84,  82}, { 17,  82}}, //disuaded 2
	{0, { 85, 157,  83,  81}, { 17,  81}}, //disuaded 3
};

static const Animation freaks_anim[] = {
	{2, (const u8[]){1, 0, 0, 3, ASCR_BACK, 1}},
	{2, (const u8[]){2, 3, 3, 0, ASCR_BACK, 1}},
};

//Characters
//Boyfriend
#include "character/bfweeb.c"

//Senpai
#include "character/senpai.c"
#include "character/senpaim.c"
#include "character/spirit.c"

//Girlfriend
#include "character/gfweeb.c"

//Week 6 Textures
static Gfx_Tex week6_tex_back0; //Background
static Gfx_Tex week6_tex_back1; //Trees
static Gfx_Tex week6_tex_back2; //Freaks
static Gfx_Tex week6_tex_back3; //Thorns background
static FontData week6_font_arial; //Arial font weeb
static u8 week6_select;

//Freaks state
static Animatable week6_freaks_animatable;
static u8 week6_freaks_frame;

//week 6 dialogs
static void Week6_Dialog(void)
{
	//get src and dst of da heads
    //bf head
	RECT bf_src = {108, 154, 58, 45};
	RECT_FIXED bf_dst = {
	FIXED_DEC(40,1),
	FIXED_DEC(-20,1),
	FIXED_DEC(78,1),
	FIXED_DEC(65,1)
	};

	//senpai head
	RECT senpai_src = {56, 142, 52, 57};
	RECT_FIXED senpai_dst = {
	FIXED_DEC(-100,1),
	FIXED_DEC(-32,1),
	FIXED_DEC(72,1),
	FIXED_DEC(77,1)
	};
    
	//senpai mad head
	RECT senpaim_src = { 3, 141, 54, 58};
	RECT_FIXED senpaim_dst = {
	FIXED_DEC(-90,1),
	FIXED_DEC(-33,1),
	FIXED_DEC(74,1),
	FIXED_DEC(78,1)
	};

	//spirit head
	RECT spirit_src = {207, 138, 48, 106};
    RECT_FIXED spirit_dst = {
	FIXED_DEC(-100,1),
	FIXED_DEC(-82,1),
	FIXED_DEC(68,1),
	FIXED_DEC(126,1)
	};

//get src and dst for box
	RECT box_src = {3, 199, 205, 56};
	RECT_FIXED box_dst = {
		FIXED_DEC(-140,1),
		FIXED_DEC(39,1),
		FIXED_DEC(290,1),
		FIXED_DEC(70,1)
	};
	RECT blackbox = {20,170,270, 50};

//senpai dialog
static Dialogs week6_dialog1[] = {
	{"Ah, a new fair maiden has come\nin search of true love!",1},
	{"A serenade between gentlemen\nshall decide where her beautiful\nheart shall reside.",1},
	{"Beep bo bop",0},
};

//roses dialog
static Dialogs week6_dialog2[] = {
	{"Not bad for an ugly worm.",2},
	{"But this time I'll rip your nuts off\nright after your girlfriend\nfinishes gargling mine.",2},
	{"Bop beep be be skdoo bep",0},
};

//Thorns dialog
static Dialogs week6_dialog3[] = {
	{"Direct contact with real humans,\nafter being trapped in here for\nso long...",3},
	{"and HER of all people.",3},
	{"I'll make her father pay for what\nhe's done to me and all the\nothers....",3},
	{"I'll beat you and make you take\nmy place.",3},
	{"You don't mind your bodies\nbeing borrowed right? It's only\nfair...",3},
};

//this variable will receive dialog information
Dialogs week6_alldia[week6_setsize];

//pog code that receives information from dialogs
switch(stage.stage_id)
{
case StageId_6_1:
week6_setsize = COUNT_OF(week6_dialog1);
week6_alldia[week6_select] = week6_dialog1[week6_select];
break;
case StageId_6_2:
week6_setsize = COUNT_OF(week6_dialog2);
week6_alldia[week6_select] = week6_dialog2[week6_select];
break;
case StageId_6_3:
week6_setsize = COUNT_OF(week6_dialog3);
week6_alldia[week6_select] = week6_dialog3[week6_select];
break;
default:
break;
}

//draw heads
switch (week6_alldia[week6_select].pport)
{
case 0:
//bf
Stage_DrawTex(&stage.tex_hud1, &bf_src, &bf_dst, stage.bump);
break;

case 1:
//senpai
Stage_DrawTex(&stage.tex_hud1, &senpai_src, &senpai_dst, stage.bump);
break;

case 2:
//senpai mad
Stage_DrawTex(&stage.tex_hud1, &senpaim_src, &senpaim_dst, stage.bump);
break;

case 3:
//spirit
Stage_DrawTex(&stage.tex_hud1, &spirit_src, &spirit_dst, stage.bump);
break;
}

	if (pad_state.press & PAD_CROSS)
	{
		//if dialog not over,start next phrase ,else,finish dialog
			Audio_PlaySound(Week6_Sounds[0]);
			if (week6_select < COUNT_OF(week6_alldia) - 1)
			week6_select++;

			else
			stage.dialog = true;
	}

		//Draw normal dialog
		if (stage.stage_id != StageId_6_3)
		{
			week6_font_arial.draw_col(&week6_font_arial,
	    	week6_alldia[week6_select].text,
			60,
	    	180,
			FontAlign_Left,
			57 >> 1,
			25 >> 1,
			23 >> 1
			);
		}

			//thorns dialogs
			else
			{
			week6_font_arial.draw(&week6_font_arial,
	    	week6_alldia[week6_select].text,
			60,
	    	180,
			FontAlign_Left
			);
		}

	//draw normal box
	if (stage.stage_id != StageId_6_3)
	Stage_DrawTex(&stage.tex_hud1, &box_src, &box_dst, stage.bump);
    
	//draw thorns box
	else
	Gfx_DrawRect(&blackbox,0,0,0);
    
	//transparent white
     RECT screen = {0, 0, SCREEN_WIDTH, SCREEN_HEIGHT};
	 Gfx_BlendRect(&screen, 255, 255, 255, 0);
}

//Freaks functions
void Week6_Freaks_SetFrame(void *user, u8 frame)
{
	(void)user;
	week6_freaks_frame = frame;
}

void Week6_Freaks_Draw(fixed_t x, fixed_t y, boolean flip)
{
	//Draw character
	static const u8 frame_map[4][2][2] = {
		{{0, 3}, {4, 7}},
		{{1, 2}, {5, 6}},
		{{2, 1}, {6, 5}},
		{{3, 0}, {7, 4}},
	};
	
	const CharFrame *cframe = &freaks_frame[frame_map[week6_freaks_frame][stage.stage_id == StageId_6_2][flip]];
	
	fixed_t ox = x - ((fixed_t)cframe->off[0] << FIXED_SHIFT);
	fixed_t oy = y - ((fixed_t)cframe->off[1] << FIXED_SHIFT);
	
	RECT src = {cframe->src[0], cframe->src[1], cframe->src[2], cframe->src[3]};
	RECT_FIXED dst = {ox, oy, src.w << FIXED_SHIFT, src.h << FIXED_SHIFT};
	Stage_DrawTex(&week6_tex_back2, &src, &dst, stage.camera.bzoom);
}

//Week 6 background functions
static void Week6_Load(void)
{
	//Load assets
	IO_Data overlay_data;
	
	Gfx_LoadTex(&stage.tex_huds, overlay_data = Overlay_DataRead(), 0); Mem_Free(overlay_data); //huds.tim
	Gfx_LoadTex(&stage.tex_hud1, overlay_data = Overlay_DataRead(), 0); Mem_Free(overlay_data); //hud1.tim
	
	Gfx_LoadTex(&week6_tex_back0, overlay_data = Overlay_DataRead(), 0); Mem_Free(overlay_data); //back0.tim
	Gfx_LoadTex(&week6_tex_back1, overlay_data = Overlay_DataRead(), 0); Mem_Free(overlay_data); //back1.tim
	Gfx_LoadTex(&week6_tex_back2, overlay_data = Overlay_DataRead(), 0); Mem_Free(overlay_data); //back2.tim
	Gfx_LoadTex(&week6_tex_back3, overlay_data = Overlay_DataRead(), 0); Mem_Free(overlay_data); //back3.tim
	
	//Load characters
	stage.player = Char_BFWeeb_New(FIXED_DEC(52,1), FIXED_DEC(50,1));
	switch (stage.stage_id)
	{
		case StageId_6_1: //Senpai
			stage.opponent = Char_Senpai_New(FIXED_DEC(-60,1), FIXED_DEC(50,1));
			break;
		case StageId_6_2: //Roses
			stage.opponent = Char_SenpaiM_New(FIXED_DEC(-60,1), FIXED_DEC(50,1));
			break;
		case StageId_6_3: //Thorns
			stage.opponent = Char_Spirit_New(FIXED_DEC(-60,1), FIXED_DEC(50,1));
			break;
		default:
		    stage.opponent = Char_Senpai_New(FIXED_DEC(-60,1), FIXED_DEC(50,1));
			break;
	}
	stage.gf = Char_GFWeeb_New(FIXED_DEC(0,1), FIXED_DEC(45,1));

	FontData_Arial(&week6_font_arial, overlay_data = Overlay_DataRead()); Mem_Free(overlay_data); //arialw.tim
	
	//Initialize freaks state
	Animatable_Init(&week6_freaks_animatable, freaks_anim);
	Animatable_SetAnim(&week6_freaks_animatable, 0);

	// Begin Read Sound effects
	CdlFILE file;
    IO_FindFile(&file, "\\SOUND\\CLICK.VAG;1");
    u32 *data = IO_ReadFile(&file);
    Week6_Sounds[0] = Audio_LoadVAGData(data, file.size);

	for (int i = 0; i < 1; i++)
	printf("address = %08x\n", Week6_Sounds[i]);

	Mem_Free(data);
}

static fixed_t week6_back_paraly[] = {
	FIXED_DEC(15,100),
	FIXED_DEC(15,100),
	FIXED_DEC(15,100),
	FIXED_DEC(15,100),
	FIXED_DEC(7,10),
	FIXED_DEC(13,10),
};

static fixed_t week6_back_warpx[] = {
	FIXED_DEC(5,1),
	FIXED_DEC(5,1),
	FIXED_DEC(5,1),
	FIXED_DEC(4,1),
	FIXED_DEC(3,1),
	FIXED_DEC(3,1),
};

static fixed_t week6_back_warpy[] = {
	FIXED_DEC(25,10),
	FIXED_DEC(20,10),
	FIXED_DEC(15,10),
	FIXED_DEC(10,10),
	FIXED_DEC(0,10),
	FIXED_DEC(0,10),
};

static s32 Week6_GetX(int x, int y)
{
	return ((fixed_t)x << (FIXED_SHIFT + 5)) + FIXED_DEC(-128,1) - FIXED_MUL(stage.camera.x, week6_back_paraly[y]) + ((MUtil_Cos((animf_count << 2) + ((x + y) << 5)) * week6_back_warpx[y]) >> 8);
}

static s32 Week6_GetY(int x, int y)
{
	return ((fixed_t)y << (FIXED_SHIFT + 5)) + FIXED_DEC(-86,1) - FIXED_MUL(stage.camera.y, week6_back_paraly[y]) + ((MUtil_Sin((animf_count << 2) + ((x + y) << 5)) * week6_back_warpy[y]) >> 8);
}

static void Week6_DrawBG3(void)
{
	//Get quad points
	POINT_FIXED back_dst[6][9];
	for (int y = 0; y < 6; y++)
	{
		for (int x = 0; x < 9; x++)
		{
			back_dst[y][x].x = Week6_GetX(x, y);
			back_dst[y][x].y = Week6_GetY(x, y);
		}
	}
	
	//Draw 32x32 quads of the background
	for (int y = 0; y < 5; y++)
	{
		RECT back_src = {0, y * 32, 32, 32};
		for (int x = 0; x < 8; x++)
		{
			//Draw quad and increment source rect
			Stage_DrawTexArb(&week6_tex_back3, &back_src, &back_dst[y][x], &back_dst[y][x + 1], &back_dst[y + 1][x], &back_dst[y + 1][x + 1], stage.camera.bzoom);
			if ((back_src.x += 32) >= 0xE0)
				back_src.w--;
		}
	}
}
static void Week6_DrawBG(void)
{
	fixed_t fx, fy;
	
	//Animate and draw freaks
	fx = (stage.camera.x << 2) / 5;
	fy = (stage.camera.y << 2) / 5;
	
	if (stage.flag & STAGE_FLAG_JUST_STEP)
	{
		switch (stage.song_step & 7)
		{
			case 0:
				Animatable_SetAnim(&week6_freaks_animatable, 0);
				break;
			case 4:
				Animatable_SetAnim(&week6_freaks_animatable, 1);
				break;
		}
	}
	Animatable_Animate(&week6_freaks_animatable, NULL, Week6_Freaks_SetFrame);
	
	Week6_Freaks_Draw(FIXED_DEC(-110,1) - fx, FIXED_DEC(44,1) - fy, false);
	Week6_Freaks_Draw( FIXED_DEC(-20,1) - fx, FIXED_DEC(44,1) - fy, true);
	Week6_Freaks_Draw(  FIXED_DEC(70,1) - fx, FIXED_DEC(44,1) - fy, false);
	
	//Draw foreground trees
	fx = stage.camera.x >> 1;
	fy = stage.camera.y >> 1;
	
	static const struct Week6_FGTree
	{
		RECT src;
		fixed_t x, y;
		fixed_t off[4];
	} fg_tree[] = {
		{{  0, 0, 100, 82}, FIXED_DEC(-116,1), FIXED_DEC(-88,1), {FIXED_DEC(32,10), FIXED_DEC(34,10), FIXED_DEC(28,10), FIXED_DEC(30,10)}},
		{{101, 0,  99, 80},    FIXED_DEC(0,1), FIXED_DEC(-84,1), {FIXED_DEC(31,10), FIXED_DEC(30,10), FIXED_DEC(29,10), FIXED_DEC(32,10)}},
	};
	
	const struct Week6_FGTree *fg_tree_p = fg_tree;
	for (size_t i = 0; i < COUNT_OF(fg_tree); i++, fg_tree_p++)
	{
		//Get distorted points
		POINT_FIXED treep[4] = {
			{fg_tree_p->x,                                 fg_tree_p->y},
			{fg_tree_p->x + FIXED_DEC(fg_tree_p->src.w,1), fg_tree_p->y},
			{fg_tree_p->x,                                 fg_tree_p->y + FIXED_DEC(fg_tree_p->src.h,1)},
			{fg_tree_p->x + FIXED_DEC(fg_tree_p->src.w,1), fg_tree_p->y + FIXED_DEC(fg_tree_p->src.h,1)},
		};
		for (int j = 0; j < 4; j++)
		{
			treep[j].x += ((MUtil_Cos(FIXED_MUL(animf_count, fg_tree_p->off[j])) * FIXED_DEC(3,1)) >> 8) - fx;
			treep[j].y += ((MUtil_Sin(FIXED_MUL(animf_count, fg_tree_p->off[j])) * FIXED_DEC(3,1)) >> 8) - fy;
		}
		
		Stage_DrawTexArb(&week6_tex_back1, &fg_tree_p->src, &treep[0], &treep[1], &treep[2], &treep[3], stage.camera.bzoom);
	}
	
	//Draw background trees
	RECT bg_tree_l_src = {0, 83, 62, 45};
	RECT_FIXED bg_tree_l_dst = {
		FIXED_DEC(-106,1) - fx,
		FIXED_DEC(-26,1) - fy,
		FIXED_DEC(62,1),
		FIXED_DEC(45,1)
	};
	
	RECT bg_tree_r_src = {63, 83, 61, 50};
	RECT_FIXED bg_tree_r_dst = {
		FIXED_DEC(44,1) - fx,
		FIXED_DEC(-26,1) - fy,
		FIXED_DEC(61,1),
		FIXED_DEC(50,1)
	};
	
	Stage_DrawTex(&week6_tex_back1, &bg_tree_l_src, &bg_tree_l_dst, stage.camera.bzoom);
	Stage_DrawTex(&week6_tex_back1, &bg_tree_r_src, &bg_tree_r_dst, stage.camera.bzoom);
	
	//Draw school
	fx = stage.camera.x >> 3;
	fy = stage.camera.y >> 3;
	
	RECT school_src = {0, 0, 255, 75};
	RECT_FIXED school_dst = {
		FIXED_DEC(-128,1) - fx,
		FIXED_DEC(-56,1) - fy,
		FIXED_DEC(255,1),
		FIXED_DEC(75,1)
	};
	
	Stage_DrawTex(&week6_tex_back0, &school_src, &school_dst, stage.camera.bzoom);
	
	//Draw street
	//fx = stage.camera.x >> 3;
	//fy = stage.camera.y >> 3;
	
	POINT_FIXED street_d0 = {
		FIXED_DEC(-128,1) - fx,
		FIXED_DEC(19,1) - FIXED_DEC(1,1) - fy,
	};
	POINT_FIXED street_d1 = {
		FIXED_DEC(-128,1) + FIXED_DEC(255,1) - fx,
		FIXED_DEC(19,1) - FIXED_DEC(1,1) - fy,
	};
	
	fx = stage.camera.x * 3 / 2;
	fy = stage.camera.y * 3 / 2;
	
	POINT_FIXED street_d2 = {
		FIXED_DEC(-128,1) - fx,
		FIXED_DEC(19,1) + FIXED_DEC(58,1) - fy,
	};
	POINT_FIXED street_d3 = {
		FIXED_DEC(-128,1) + FIXED_DEC(255,1) - fx,
		FIXED_DEC(19,1) + FIXED_DEC(58,1) - fy,
	};
	
	RECT street_src = {0, 75, 255, 54};
	
	Stage_DrawTexArb(&week6_tex_back0, &street_src, &street_d0, &street_d1, &street_d2, &street_d3, stage.camera.bzoom);
	
	//Draw sky
	fx = stage.camera.x >> 4;
	fy = stage.camera.y >> 4;
	
	RECT sky_src = {0, 130, 255, 125};
	RECT_FIXED sky_dst = {
		FIXED_DEC(-128,1) - fx,
		FIXED_DEC(-72,1) - fy,
		FIXED_DEC(255,1),
		FIXED_DEC(125,1)
	};
	
	Stage_DrawTex(&week6_tex_back0, &sky_src, &sky_dst, stage.camera.bzoom);
}

static IO_Data Week6_GetChart(void)
{
	return week6_cht[stage.stage_id - StageId_6_1][stage.stage_diff];
}

static boolean Week6_LoadScreen(void)
{
	return stage.stage_id == StageId_6_2; //Going to Thorns
}

static boolean Week6_NextStage(void)
{
	switch (stage.stage_id)
	{
		case StageId_6_1: //Senpai
			stage.stage_id = StageId_6_2;
			Character_Free(stage.opponent);
			stage.opponent = Char_SenpaiM_New(FIXED_DEC(-60,1), FIXED_DEC(50,1));
			week6_select = 0;
			return true;
		case StageId_6_2: //Roses
			stage.stage_id = StageId_6_3;
			stageoverlay_drawbg = Week6_DrawBG3;
			Character_Free(stage.opponent);
			stage.opponent = Char_Spirit_New(FIXED_DEC(-60,1), FIXED_DEC(50,1));
			week6_select = 0;
			return true;
		case StageId_6_3: //Thorns
		week6_select = 0;
			return false;
		default:
			return false;
	}
}

void Week6_SetPtr(void)
{
	//Set pointers
	stageoverlay_load = Week6_Load;
	stageoverlay_tick = NULL;
	stageoverlay_notemoviment = NULL;
	stageoverlay_drawbg = (stage.stage_id == StageId_6_3) ? Week6_DrawBG3 : Week6_DrawBG;
	stageoverlay_drawmd = NULL;
	stageoverlay_drawfg = NULL;
	stageoverlay_dialog = Week6_Dialog;
	stageoverlay_free = NULL;
	stageoverlay_getchart = Week6_GetChart;
	stageoverlay_loadscreen = Week6_LoadScreen;
	stageoverlay_nextstage = Week6_NextStage;
}
