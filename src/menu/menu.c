/*
  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "menu.h"

#include "boot/mem.h"
#include "boot/main.h"
#include "boot/timer.h"
#include "boot/io.h"
#include "boot/gfx.h"
#include "boot/audio.h"
#include "boot/pad.h"
#include "boot/archive.h"
#include "boot/mutil.h"
#include "boot/network.h"

#include "boot/font.h"
#include "boot/trans.h"
#include "boot/loadscr.h"

#include "boot/stage.h"
#include "boot/save.h"
#include "boot/movie.h"
#include "stdlib.h"


//Characters
//Menu BF
#include "character/menup.c"
//Menu Opponent
#include "character/menuo.c"
//Girlfriend
#include "character/gf.c"

u32 Menu_Sounds[3];

static fixed_t Char_GF_GetParallax(Char_GF *this)
{
	(void)this;
	return FIXED_UNIT;
}

//Menu messages
static const char *funny_messages[][2] = {
	{"PSX PORT BY CUCKYDEV", "YOU KNOW IT"},
	{"PORTED BY CUCKYDEV", "WHAT YOU GONNA DO"},
	{"FUNKIN", "FOREVER"},
	{"WHAT THE HELL", "RITZ PSX"},
	{"LIKE PARAPPA", "BUT COOLER"},
	{"THE JAPI", "EL JAPI"},
	{"PICO FUNNY", "PICO FUNNY"},
	{"OPENGL BACKEND", "BY CLOWNACY"},
	{"CUCKYFNF", "SETTING STANDARDS"},
	{"lool", "inverted colours"},
	{"NEVER LOOK AT", "THE ISSUE TRACKER"},
	{"ZERO POINT ZERO TWO TWO EIGHT", "ONE FIVE NINE ONE ZERO FIVE"},
	{"DOPE ASS GAME", "PLAYSTATION MAGAZINE"},
	{"NEWGROUNDS", "FOREVER"},
	{"NO FPU", "NO PROBLEM"},
	{"OK OKAY", "WATCH THIS"},
	{"ITS MORE MALICIOUS", "THAN ANYTHING"},
	{"USE A CONTROLLER", "LOL"},
	{"SNIPING THE KICKSTARTER", "HAHA"},
	{"SHITS UNOFFICIAL", "NOT A PROBLEM"},
	{"SYSCLK", "RANDOM SEED"},
	{"THEY DIDNT HIT THE GOAL", "STOP"},
	{"FCEFUWEFUETWHCFUEZDSLVNSP", "PQRYQWENQWKBVZLZSLDNSVPBM"},
	{"THE FLOORS ARE", "THREE DIMENSIONAL"},
	{"PSXFUNKIN BY CUCKYDEV", "SUCK IT DOWN"},
	{"PLAYING ON EPSXE HUH", "YOURE THE PROBLEM"},
	{"NEXT IN LINE", "ATARI"},
	{"HAXEFLIXEL", "COME ON"},
	{"HAHAHA", "I DONT CARE"},
	{"GET ME TO STOP", "TRY"},
	{"FNF MUKBANG GIF", "THATS UNRULY"},
	{"OPEN SOURCE", "FOREVER"},
	{"ITS A PORT", "ITS WORSE"},
	{"WOW GATO", "WOW GATO"},
	{"BALLS FISH", "BALLS FISH"},
	{"TURINGMANIA", "IS BETTER"},
	{"NUTRIA EN UN", "SUETER DE LANA"},
	{"CHECK OUT THIS", "COOL ASS HOT DOG"},
};

#ifdef PSXF_NETWORK

//Menu string type
#define MENUSTR_CHARS 0x20
typedef char MenuStr[MENUSTR_CHARS + 1];

#endif

struct 
  {
    char *p_title;
    char *p_message;
    int  *p_data;
  } envMessage;

//Menu state
static struct
{
	//Menu state
	u8 page, next_page;
	boolean page_swap;
	u8 select, next_select;
	u8 switchop;
	boolean swap;
	
	fixed_t scroll;
	fixed_t trans_time;
	
	//Page specific state
	union
	{
		struct
		{
			u8 funny_message;
		} opening;
		struct
		{
			fixed_t logo_bump;
			fixed_t fade, fadespd;
		} title;
		struct
		{
			fixed_t fade, fadespd;
		} story;
		struct
		{
			fixed_t back_r, back_g, back_b;
		} freeplay;
		struct
		{
			fixed_t back_r, back_g, back_b;
		} credit;
	#ifdef PSXF_NETWORK
		struct
		{
			boolean type;
			MenuStr port;
			MenuStr pass;
		} net_host;
		struct
		{
			boolean type;
			MenuStr ip;
			MenuStr port;
			MenuStr pass;
		} net_join;
		struct
		{
			boolean swap;
		} net_op;
	#endif
	} page_state;
	
	union
	{
		struct
		{
			u8 id, diff;
			boolean story;
		} stage;
	} page_param;
	
	//Menu assets
	Gfx_Tex tex_back, tex_ng, tex_story, tex_title, tex_extra, tex_credit0;
	FontData font_bold, font_arial;
	
	Character *gf; //Title Girlfriend
	Character *bf; //Menu BF
	Character *opponent; //Menu Opponents
} menu;

#ifdef PSXF_NETWORK

//Menu string functions
static void MenuStr_Process(MenuStr *this, s32 x, s32 y, const char *fmt, boolean select, boolean type)
{
	//Append typed input
	if (select && type)
	{
		if (pad_type[0] != '\0')
			strncat(*this, pad_type, MENUSTR_CHARS - strlen(*this));
		if (pad_backspace)
		{
			size_t i = strlen(*this);
			if (i != 0)
				(*this)[i - 1] = '\0';
		}
	}
	
	//Get text to draw
	char buf[0x100];
	sprintf(buf, fmt, *this);
	if (select && type && (animf_count & 2))
		strcat(buf, "_");
	
	//Draw text
	menu.font_arial.draw_col(&menu.font_arial, buf, x, y, FontAlign_Left, 0x80, 0x80, select ? 0x00 : 0x80);
	menu.font_arial.draw_col(&menu.font_arial, buf, x+1, y+1, FontAlign_Left, 0x00, 0x00, 0x00);
}

#endif


//Internal menu functions
char menu_text_buffer[0x100];

static const char *Menu_LowerIf(const char *text, boolean lower)
{
	//Copy text
	char *dstp = menu_text_buffer;
	if (lower)
	{
		for (const char *srcp = text; *srcp != '\0'; srcp++)
		{
			if (*srcp >= 'A' && *srcp <= 'Z')
				*dstp++ = *srcp | 0x20;
			else
				*dstp++ = *srcp;
		}
	}
	else
	{
		for (const char *srcp = text; *srcp != '\0'; srcp++)
		{
			if (*srcp >= 'a' && *srcp <= 'z')
				*dstp++ = *srcp & ~0x20;
			else
				*dstp++ = *srcp;
		}
	}
	
	//Terminate text
	*dstp++ = '\0';
	return menu_text_buffer;
}

//Draw Credit image functions
static void DrawCredit(u8 i, s16 x, s16 y, boolean select)
{
	while (x > 128)
	{
	x -= 192;
	y += 64;
	}
	//Get src and dst
	RECT src = {
		(i % 4) * 64,
	    (i / 4) * 64,
	    64,
		64
	};
	RECT dst = {
		x,
		y,
		64,
		64
	};	
	//Draw credit icon
	if (select == false)
	Gfx_DrawTex(&menu.tex_credit0, &src, &dst);
	else
	Gfx_BlendTex(&menu.tex_credit0, &src, &dst, 1);
    }

static void Menu_DrawBack(boolean flash, s32 scroll, u8 r0, u8 g0, u8 b0, u8 r1, u8 g1, u8 b1)
{
	RECT back_src = {0, 0, 255, 255};
	RECT back_dst = {0, -scroll - SCREEN_WIDEADD2, SCREEN_WIDTH, SCREEN_WIDTH * 4 / 5};
	
	if (flash || (animf_count & 4) == 0)
		Gfx_DrawTexCol(&menu.tex_back, &back_src, &back_dst, r0, g0, b0);
	else
		Gfx_DrawTexCol(&menu.tex_back, &back_src, &back_dst, r1, g1, b1);
}

static void Menu_DifficultySelector(s32 x, s32 y)
{
	//Change difficulty
	if (menu.next_page == menu.page && Trans_Idle())
	{
		if (pad_state.press & PAD_LEFT)
		{
			if (menu.page_param.stage.diff > StageDiff_Easy)
				menu.page_param.stage.diff--;
			else
				menu.page_param.stage.diff = StageDiff_Hard;
		}
		if (pad_state.press & PAD_RIGHT)
		{
			if (menu.page_param.stage.diff < StageDiff_Hard)
				menu.page_param.stage.diff++;
			else
				menu.page_param.stage.diff = StageDiff_Easy;
		}
	}
	
	//Draw difficulty arrows
	static const RECT arrow_src[2][2] = {
		{{224, 64, 16, 32}, {224, 96, 16, 32}}, //left
		{{240, 64, 16, 32}, {240, 96, 16, 32}}, //right
	};
	
	Gfx_BlitTex(&menu.tex_story, &arrow_src[0][(pad_state.held & PAD_LEFT) != 0], x - 40 - 16, y - 16);
	Gfx_BlitTex(&menu.tex_story, &arrow_src[1][(pad_state.held & PAD_RIGHT) != 0], x + 40, y - 16);
	
	//Draw difficulty
	static const RECT diff_srcs[] = {
		{  0, 96, 64, 18},
		{ 64, 96, 80, 18},
		{144, 96, 64, 18},
	};
	
	const RECT *diff_src = &diff_srcs[menu.page_param.stage.diff];
	Gfx_BlitTex(&menu.tex_story, diff_src, x - (diff_src->w >> 1), y - 9 + ((pad_state.press & (PAD_LEFT | PAD_RIGHT)) != 0));
}

static void Menu_DrawWeek(const char *week, s32 x, s32 y)
{
	//Draw label
	if (week == NULL)
	{
		//Tutorial
		RECT label_src = {0, 0, 112, 32};
		Gfx_BlitTex(&menu.tex_story, &label_src, x, y);
	}
	else
	{
		//Week
		RECT label_src = {0, 32, 80, 32};
		Gfx_BlitTex(&menu.tex_story, &label_src, x, y);
		
		//Number
		x += 80;
		for (; *week != '\0'; week++)
		{
			//Draw number
			u8 i = *week - '0';
			
			RECT num_src = {128 + ((i & 3) << 5), ((i >> 2) << 5), 32, 32};
			Gfx_BlitTex(&menu.tex_story, &num_src, x, y);
			x += 32;
		}
	}
}

//Menu functions
void Menu_Load2(MenuPage page)
{
	//Load menu assets
	IO_Data overlay_data;
	
	Gfx_LoadTex(&menu.tex_back,  overlay_data = Overlay_DataRead(), 0); Mem_Free(overlay_data); //back.tim
	Gfx_LoadTex(&menu.tex_ng,    overlay_data = Overlay_DataRead(), 0); Mem_Free(overlay_data); //ng.tim
	Gfx_LoadTex(&menu.tex_story, overlay_data = Overlay_DataRead(), 0); Mem_Free(overlay_data); //story.tim
	Gfx_LoadTex(&menu.tex_title, overlay_data = Overlay_DataRead(), 0); Mem_Free(overlay_data); //title.tim
	Gfx_LoadTex(&menu.tex_extra, overlay_data = Overlay_DataRead(), 0); Mem_Free(overlay_data); //extra.tim
	Gfx_LoadTex(&menu.tex_credit0, overlay_data = Overlay_DataRead(), 0); Mem_Free(overlay_data); //credit0.tim
	
	FontData_Bold(&menu.font_bold, overlay_data = Overlay_DataRead()); Mem_Free(overlay_data); //bold.tim
	FontData_Arial(&menu.font_arial, overlay_data = Overlay_DataRead()); Mem_Free(overlay_data); //arial.tim
	
	//Initialize Girlfriend, Menu BF, Menu Opponents and stage
	menu.gf = Char_GF_New(FIXED_DEC(62,1), FIXED_DEC(-12,1));
	menu.bf = Char_BF_New(FIXED_DEC(0,1), FIXED_DEC( 17,1));
	menu.opponent = Char_MenuO_New(FIXED_DEC(-90,1), FIXED_DEC(84,1));
	stage.camera.x = stage.camera.y = FIXED_DEC(0,1);
	stage.camera.bzoom = FIXED_UNIT;
	stage.gf_speed = 4;
	
	//Initialize menu state
	menu.select = menu.next_select = 0;
	
	switch (menu.page = menu.next_page = page)
	{
		case MenuPage_Opening:
			//Get funny message to use
			//Do this here so timing is less reliant on VSync
			#ifdef PSXF_PC
				menu.page_state.opening.funny_message = time(NULL) % COUNT_OF(funny_messages);
			#else
				menu.page_state.opening.funny_message = ((*((volatile u32*)0xBF801120)) >> 3) % COUNT_OF(funny_messages); //sysclk seeding
			#endif
			break;
		default:
			break;
	}
	menu.page_swap = true;
	
	menu.trans_time = 0;
	Trans_Clear();
	
	stage.song_step = 0;

	//Set background colour
	Gfx_SetClear(0, 0, 0);
    
	// to load
	CdlFILE file;
    IO_FindFile(&file, "\\SOUND\\SCROLL.VAG;1");
    u32 *data = IO_ReadFile(&file);
    Menu_Sounds[0] = Audio_LoadVAGData(data, file.size);

	IO_FindFile(&file, "\\SOUND\\CONFIRM.VAG;1");
    data = IO_ReadFile(&file);
    Menu_Sounds[1] = Audio_LoadVAGData(data, file.size);

	IO_FindFile(&file, "\\SOUND\\CANCEL.VAG;1");
    data = IO_ReadFile(&file);
    Menu_Sounds[2] = Audio_LoadVAGData(data, file.size);
    
	for (int i = 0; i < 3; i++)
	printf("address = %08x\n", Menu_Sounds[i]);

	free(data);

	//Play menu music
	Audio_LoadMus("\\MENU\\MENU.MUS;1");
	Audio_PlayMus(true);
	Audio_SetVolume(0, 0x3FFF, 0x0000);
	Audio_SetVolume(1, 0x0000, 0x3FFF);
}

void Menu_Unload(void)
{
	//Free title Girlfriend
	Character_Free(menu.gf);
}

void Menu_ToStage(StageId id, StageDiff diff, boolean story)
{
	menu.next_page = MenuPage_Stage;
	menu.page_param.stage.id = id;
	menu.page_param.stage.story = story;
	menu.page_param.stage.diff = diff;
	Trans_Start();
}

void Menu_Tick(void)
{
	//Clear per-frame flags
	stage.flag &= ~STAGE_FLAG_JUST_STEP;
	
	//Get song position
	u16 next_step = Audio_GetTime() / FIXED_DEC(15, 102);
	if (next_step != stage.song_step)
	{
		if (next_step >= stage.song_step)
			stage.flag |= STAGE_FLAG_JUST_STEP;
		stage.song_step = next_step;
	}
	
	//Handle transition out
	if (Trans_Tick())
	{
		//Change to set next page
		menu.page_swap = true;
		menu.page = menu.next_page;
		menu.select = menu.next_select;
	}
	
	//Tick menu page
	MenuPage exec_page;
	switch (exec_page = menu.page)
	{
		case MenuPage_Opening:
		{
			u16 beat = stage.song_step >> 2;
			
			//Start title screen if opening ended
			if (beat >= 16)
			{
				menu.page = menu.next_page = MenuPage_Title;
				menu.page_swap = true;
				//Fallthrough
			}
			else
			{
				//Start title screen if start pressed
				if (pad_state.held & PAD_START)
					menu.page = menu.next_page = MenuPage_Title;
				
				//Draw different text depending on beat
				RECT src_ng = {0, 0, 128, 128};
				const char **funny_message = funny_messages[menu.page_state.opening.funny_message];
				
				switch (beat)
				{
					case 3:
						menu.font_bold.draw(&menu.font_bold, "PRESENT", SCREEN_WIDTH2, SCREEN_HEIGHT2 + 32, FontAlign_Center);
				//Fallthrough
					case 2:
					case 1:
						menu.font_bold.draw(&menu.font_bold, "NINJAMUFFIN",   SCREEN_WIDTH2, SCREEN_HEIGHT2 - 32, FontAlign_Center);
						menu.font_bold.draw(&menu.font_bold, "PHANTOMARCADE", SCREEN_WIDTH2, SCREEN_HEIGHT2 - 16, FontAlign_Center);
						menu.font_bold.draw(&menu.font_bold, "KAWAISPRITE",   SCREEN_WIDTH2, SCREEN_HEIGHT2,      FontAlign_Center);
						menu.font_bold.draw(&menu.font_bold, "EVILSKER",      SCREEN_WIDTH2, SCREEN_HEIGHT2 + 16, FontAlign_Center);
						break;
					
					case 7:
						menu.font_bold.draw(&menu.font_bold, "NEWGROUNDS",    SCREEN_WIDTH2, SCREEN_HEIGHT2 - 32, FontAlign_Center);
						Gfx_BlitTex(&menu.tex_ng, &src_ng, (SCREEN_WIDTH - 128) >> 1, SCREEN_HEIGHT2 - 16);
				//Fallthrough
					case 6:
					case 5:
						menu.font_bold.draw(&menu.font_bold, "IN ASSOCIATION", SCREEN_WIDTH2, SCREEN_HEIGHT2 - 64, FontAlign_Center);
						menu.font_bold.draw(&menu.font_bold, "WITH",           SCREEN_WIDTH2, SCREEN_HEIGHT2 - 48, FontAlign_Center);
						break;
					
					case 11:
						menu.font_bold.draw(&menu.font_bold, funny_message[1], SCREEN_WIDTH2, SCREEN_HEIGHT2, FontAlign_Center);
				//Fallthrough
					case 10:
					case 9:
						menu.font_bold.draw(&menu.font_bold, funny_message[0], SCREEN_WIDTH2, SCREEN_HEIGHT2 - 16, FontAlign_Center);
						break;
					
					case 15:
						menu.font_bold.draw(&menu.font_bold, "FUNKIN", SCREEN_WIDTH2, SCREEN_HEIGHT2 + 8, FontAlign_Center);
				//Fallthrough
					case 14:
						menu.font_bold.draw(&menu.font_bold, "NIGHT", SCREEN_WIDTH2, SCREEN_HEIGHT2 - 8, FontAlign_Center);
				//Fallthrough
					case 13:
						menu.font_bold.draw(&menu.font_bold, "FRIDAY", SCREEN_WIDTH2, SCREEN_HEIGHT2 - 24, FontAlign_Center);
						break;
				}
				break;
			}
		}
	//Fallthrough
		case MenuPage_Title:
		{
			//Initialize page
			if (menu.page_swap)
			{
				menu.page_state.title.logo_bump = (FIXED_DEC(7,1) / 24) - 1;
				menu.page_state.title.fade = FIXED_DEC(255,1);
				menu.page_state.title.fadespd = FIXED_DEC(90,1);
			}
			
			//Draw white fade
			if (menu.page_state.title.fade > 0)
			{
				static const RECT flash = {0, 0, SCREEN_WIDTH, SCREEN_HEIGHT};
				u8 flash_col = menu.page_state.title.fade >> FIXED_SHIFT;
				Gfx_BlendRect(&flash, flash_col, flash_col, flash_col, 1);
				menu.page_state.title.fade -= FIXED_MUL(menu.page_state.title.fadespd, timer_dt);
			}
			
			//Go to main menu when start is pressed
			if (menu.trans_time > 0 && (menu.trans_time -= timer_dt) <= 0)
				Trans_Start();
			
			if ((pad_state.press & PAD_START) && menu.next_page == menu.page && Trans_Idle())
			{
				//play confirm sound
				Audio_PlaySound(Menu_Sounds[1]);
				menu.trans_time = FIXED_UNIT;
				menu.page_state.title.fade = FIXED_DEC(255,1);
				menu.page_state.title.fadespd = FIXED_DEC(300,1);
				menu.next_page = MenuPage_Main;
				menu.next_select = 0;
			}
			
			//Draw Friday Night Funkin' logo
			if ((stage.flag & STAGE_FLAG_JUST_STEP) && (stage.song_step & 0x3) == 0 && menu.page_state.title.logo_bump == 0)
				menu.page_state.title.logo_bump = (FIXED_DEC(7,1) / 24) - 1;
			
			static const fixed_t logo_scales[] = {
				FIXED_DEC(1,1),
				FIXED_DEC(101,100),
				FIXED_DEC(102,100),
				FIXED_DEC(103,100),
				FIXED_DEC(105,100),
				FIXED_DEC(110,100),
				FIXED_DEC(97,100),
			};
			fixed_t logo_scale = logo_scales[(menu.page_state.title.logo_bump * 24) >> FIXED_SHIFT];
			u32 x_rad = (logo_scale * (176 >> 1)) >> FIXED_SHIFT;
			u32 y_rad = (logo_scale * (112 >> 1)) >> FIXED_SHIFT;
			
			RECT logo_src = {0, 0, 176, 112};
			RECT logo_dst = {
				100 - x_rad + (SCREEN_WIDEADD2 >> 1),
				68 - y_rad,
				x_rad << 1,
				y_rad << 1
			};
			Gfx_DrawTex(&menu.tex_title, &logo_src, &logo_dst);
			
			if (menu.page_state.title.logo_bump > 0)
				if ((menu.page_state.title.logo_bump -= timer_dt) < 0)
					menu.page_state.title.logo_bump = 0;
			
			//Draw "Press Start to Begin"
			if (menu.next_page == menu.page)
			{
				//Blinking blue
				s16 press_lerp = (MUtil_Cos(animf_count << 3) + 0x100) >> 1;
				u8 press_r = 51 >> 1;
				u8 press_g = (58  + ((press_lerp * (255 - 58))  >> 8)) >> 1;
				u8 press_b = (206 + ((press_lerp * (255 - 206)) >> 8)) >> 1;
				
				RECT press_src = {0, 112, 256, 32};
				Gfx_BlitTexCol(&menu.tex_title, &press_src, (SCREEN_WIDTH - 256) / 2, SCREEN_HEIGHT - 48, press_r, press_g, press_b);
			}
			else
			{
				//Flash white
				RECT press_src = {0, (animf_count & 1) ? 144 : 112, 256, 32};
				Gfx_BlitTex(&menu.tex_title, &press_src, (SCREEN_WIDTH - 256) / 2, SCREEN_HEIGHT - 48);
			}
			
			//Draw Girlfriend
			menu.gf->tick(menu.gf);
			break;
		}
		case MenuPage_Main:
		{
			static const char *menu_options[] = {
				"STORY MODE",
				"FREEPLAY",
				"CREDITS",
				"OPTIONS",
				#ifdef PSXF_NETWORK
					"JOIN SERVER",
					"HOST SERVER",
				#endif
			};
			
			//Initialize page
			if (menu.page_swap)
				menu.scroll = menu.select *
				#ifndef PSXF_NETWORK
					FIXED_DEC(8,1);
				#else
					FIXED_DEC(12,1);
				#endif
			
			//Draw version identification
			menu.font_bold.draw(&menu.font_bold,
				"PSXFUNKIN BY CUCKYDEV",
				16,
				SCREEN_HEIGHT - 32,
				FontAlign_Left
			);
			
			//Handle option and selection
			if (menu.trans_time > 0 && (menu.trans_time -= timer_dt) <= 0)
				Trans_Start();
			
			if (menu.next_page == menu.page && Trans_Idle())
			{
				//Change option
				if (pad_state.press & PAD_UP)
				{
					//play scroll sound
                    Audio_PlaySound(Menu_Sounds[0]);
					if (menu.select > 0)
						menu.select--;
					else
						menu.select = COUNT_OF(menu_options) - 1;
				}
				if (pad_state.press & PAD_DOWN)
				{
					//play scroll sound
                    Audio_PlaySound(Menu_Sounds[0]);
					if (menu.select < COUNT_OF(menu_options) - 1)
						menu.select++;
					else
						menu.select = 0;
				}
				
				//Select option if cross is pressed
				if (pad_state.press & (PAD_START | PAD_CROSS))
				{
					//play confirm sound
					Audio_PlaySound(Menu_Sounds[1]);
					switch (menu.select)
					{
						case 0: //Story Mode
							menu.next_page = MenuPage_Story;
							break;
						case 1: //Freeplay
							menu.next_page = MenuPage_Freeplay;
							break;
						case 2: //Credits
							menu.next_page = MenuPage_Credits;
							break;
						case 3: //Options
							menu.next_page = MenuPage_Options;
							break;
					#ifdef PSXF_NETWORK
						case 4: //Join Server
							menu.next_page = Network_Inited() ? MenuPage_NetJoin : MenuPage_NetInitFail;
							break;
						case 5: //Host Server
							menu.next_page = Network_Inited() ? MenuPage_NetHost : MenuPage_NetInitFail;
							break;
					#endif
					}
					menu.next_select = 0;
					menu.trans_time = FIXED_UNIT;
				}

				if (pad_state.press & PAD_SELECT)
			{
				menu.next_page = MenuPage_Movie;
				menu.trans_time = FIXED_UNIT;
				menu.page_state.title.fade = FIXED_DEC(255,1);
				menu.page_state.title.fadespd = FIXED_DEC(300,1);
			}
				
				//Return to title screen if circle is pressed
				if (pad_state.press & PAD_CIRCLE)
				{
					//play cancel sound
					Audio_PlaySound(Menu_Sounds[2]);
					menu.next_page = MenuPage_Title;
					Trans_Start();
				}
			}
			
			//Draw options
			s32 next_scroll = menu.select *
			#ifndef PSXF_NETWORK
				FIXED_DEC(8,1);
			#else
				FIXED_DEC(12,1);
			#endif
			menu.scroll += (next_scroll - menu.scroll) >> 2;
			
			if (menu.next_page == menu.page || menu.next_page == MenuPage_Title)
			{
				//Draw all options
				for (u8 i = 0; i < COUNT_OF(menu_options); i++)
				{
					menu.font_bold.draw(&menu.font_bold,
						Menu_LowerIf(menu_options[i], menu.select != i),
						SCREEN_WIDTH2,
						SCREEN_HEIGHT2 + (i << 5) - 48 - (menu.scroll >> FIXED_SHIFT),
						FontAlign_Center
					);
				}
			}
			else if (animf_count & 2)
			{
				//Draw selected option
				menu.font_bold.draw(&menu.font_bold,
					menu_options[menu.select],
					SCREEN_WIDTH2,
					SCREEN_HEIGHT2 + (menu.select << 5) - 48 - (menu.scroll >> FIXED_SHIFT),
					FontAlign_Center
				);
			}
			
			//Draw background
			Menu_DrawBack(
				menu.next_page == menu.page || menu.next_page == MenuPage_Title,
			#ifndef PSXF_NETWORK
				menu.scroll >> (FIXED_SHIFT + 1),
			#else
				menu.scroll >> (FIXED_SHIFT + 3),
			#endif
				253 >> 1, 231 >> 1, 113 >> 1,
				253 >> 1, 113 >> 1, 155 >> 1
			);
			break;
		}
		case MenuPage_Story:
		{         
		//opponent stuff
		if (pad_state.press & (PAD_DOWN | PAD_UP))
			{
			 //shit code that fixes an opponent bug
			 s16 check = 0;
			 if (pad_state.press & PAD_UP)
			 check = -1;

			 if (pad_state.press & PAD_DOWN)
			 check = 1;

			switch (menu.select + check)
			 {
				case 0: //Dad
				case 1:
				menu.opponent->set_anim(menu.opponent, CharAnim_Idle);
					break;
				case 2: //Spook
				menu.opponent->set_anim(menu.opponent, CharAnim_Left);
					break;
				case 3: //Pico
				menu.opponent->set_anim(menu.opponent, CharAnim_LeftAlt);
					break;
				case 4: //Mom
				menu.opponent->set_anim(menu.opponent, CharAnim_Down);
					break;
				case 5: //Christimas Parents
			    menu.opponent->set_anim(menu.opponent, CharAnim_DownAlt);
					break;
			}
	}

			static const struct
			{
				const char *week;
				StageId stage;
				const char *name;
				const char *tracks[3];
			} menu_options[] = {
				{NULL, StageId_1_4, "TUTORIAL", {"TUTORIAL", NULL, NULL}},
				{"1", StageId_1_1, "DADDY DEAREST", {"BOPEEBO", "FRESH", "DADBATTLE"}},
				{"2", StageId_2_1, "SPOOKY MONTH", {"SPOOKEEZ", "SOUTH", "MONSTER"}},
				{"3", StageId_3_1, "PICO", {"PICO", "PHILLY NICE", "BLAMMED"}},
				{"4", StageId_4_1, "MOMMY MUST MURDER", {"SATIN PANTIES", "HIGH", "MILF"}},
				{"5", StageId_5_1, "RED SNOW", {"COCOA", "EGGNOG", "WINTER HORRORLAND"}},
				{"6", StageId_6_1, "HATING SIMULATOR", {"SENPAI", "ROSES", "THORNS"}},
				{"7", StageId_7_1, "TANKMAN", {"UGH", "GUNS", "STRESS"}},
			};
            
			//draw "tracks"
			RECT tracks_src = {87, 65, 50, 10};
	        RECT tracks_dst = {20, SCREEN_HEIGHT - (4 * 24) + 15, 55, 15};
	
		   Gfx_DrawTex(&menu.tex_story, &tracks_src, &tracks_dst);
			
			//Initialize page
			if (menu.page_swap)
			{
				menu.scroll = 0;
				menu.page_param.stage.diff = StageDiff_Normal;
				menu.page_state.title.fade = FIXED_DEC(0,1);
				menu.page_state.title.fadespd = FIXED_DEC(0,1);
			}
			
			//Draw white fade
			if (menu.page_state.title.fade > 0)
			{
				static const RECT flash = {0, 0, SCREEN_WIDTH, SCREEN_HEIGHT};
				u8 flash_col = menu.page_state.title.fade >> FIXED_SHIFT;
				Gfx_BlendRect(&flash, flash_col, flash_col, flash_col, 1);
				menu.page_state.title.fade -= FIXED_MUL(menu.page_state.title.fadespd, timer_dt);
			}
			
			//Draw difficulty selector
			Menu_DifficultySelector(SCREEN_WIDTH - 55, 160);
			
			//Handle option and selection
			if (menu.trans_time > 0 && (menu.trans_time -= timer_dt) <= 0)
				Trans_Start();
			
			if (menu.next_page == menu.page && Trans_Idle())
			{
				//Change option
				if (pad_state.press & PAD_UP)
				{
					//play scroll sound
                    Audio_PlaySound(Menu_Sounds[0]);
					if (menu.select > 0)
						menu.select--;
					else
						menu.select = COUNT_OF(menu_options) - 1;
				}
				if (pad_state.press & PAD_DOWN)
				{
					//play scroll sound
                    Audio_PlaySound(Menu_Sounds[0]);
					if (menu.select < COUNT_OF(menu_options) - 1)
						menu.select++;
					else
						menu.select = 0;
				}
				
				//Select option if cross is pressed
				if (pad_state.press & (PAD_START | PAD_CROSS))
				{
					//play confirm sound
					Audio_PlaySound(Menu_Sounds[1]);
					menu.bf->set_anim(menu.bf, CharAnim_Left); //Make peace when we press start
					menu.next_page = MenuPage_Stage;
					menu.page_param.stage.id = menu_options[menu.select].stage;
					menu.page_param.stage.story = true;
					menu.trans_time = FIXED_UNIT;
				}
				
				//Return to main menu if circle is pressed
				if (pad_state.press & PAD_CIRCLE)
				{
					//play cancel sound
					Audio_PlaySound(Menu_Sounds[2]);
					menu.next_page = MenuPage_Main;
					menu.next_select = 0; //Story Mode
					Trans_Start();
				}
			}
			
			//Draw week name and tracks
			menu.font_arial.draw_col(&menu.font_arial,
				menu_options[menu.select].name,
				SCREEN_WIDTH - 8,
				6,
				FontAlign_Right,
				86,
				80,
				83
			);
			
			const char * const *trackp = menu_options[menu.select].tracks;
			for (size_t i = 0; i < COUNT_OF(menu_options[menu.select].tracks); i++, trackp++)
			{
				if (*trackp != NULL)
					menu.font_arial.draw_col(&menu.font_arial,
						*trackp,
						50,
						SCREEN_HEIGHT - (4 * 24) + (i * 12) + 35,
						FontAlign_Center,
						209 >> 1,
						87 >> 1,
						119 >> 1
					);
			}

			//Draw Menu BF
			menu.bf->tick(menu.bf);

			//Draw Menu Opponent
			menu.opponent->tick(menu.opponent);
			
			//Draw upper strip
			RECT name_bar = {0, 16, SCREEN_WIDTH, 128};
			Gfx_DrawRect(&name_bar, 249, 207, 81);
			
			//Draw options
			s32 next_scroll = menu.select * FIXED_DEC(48,1);
			menu.scroll += (next_scroll - menu.scroll) >> 3;
			
			if (menu.next_page == menu.page || menu.next_page == MenuPage_Main)
			{
				//Draw all options
				for (u8 i = 0; i < COUNT_OF(menu_options); i++)
				{
					s32 y = 64 + (i * 48) - (menu.scroll >> FIXED_SHIFT);
					if (y <= 16)
						continue;
					if (y >= SCREEN_HEIGHT)
						break;
					Menu_DrawWeek(menu_options[i].week, 95, y + 80);
				}
			}
			else if (animf_count & 2)
			{
				//Draw selected option
				Menu_DrawWeek(menu_options[menu.select].week, 95, 64 + (menu.select * 48) + 80 - (menu.scroll >> FIXED_SHIFT));
			}
			
			break;
		}
		case MenuPage_Freeplay:
		{
			static const struct
			{
				StageId stage;
				u32 col;
				const char *text;
			} menu_options[] = {
				{StageId_4_4, 0xFFFC96D7, "TEST"},
				{StageId_1_4, 0xFF9271FD, "TUTORIAL"},
				{StageId_1_1, 0xFF9271FD, "BOPEEBO"},
				{StageId_1_2, 0xFF9271FD, "FRESH"},
				{StageId_1_3, 0xFF9271FD, "DADBATTLE"},
				{StageId_2_1, 0xFF223344, "SPOOKEEZ"},
				{StageId_2_2, 0xFF223344, "SOUTH"},
				{StageId_2_3, 0xFF223344, "MONSTER"},
				{StageId_3_1, 0xFF941653, "PICO"},
				{StageId_3_2, 0xFF941653, "PHILLY NICE"},
				{StageId_3_3, 0xFF941653, "BLAMMED"},
				{StageId_4_1, 0xFFFC96D7, "SATIN PANTIES"},
				{StageId_4_2, 0xFFFC96D7, "HIGH"},
				{StageId_4_3, 0xFFFC96D7, "MILF"},
				{StageId_5_1, 0xFFA0D1FF, "COCOA"},
				{StageId_5_2, 0xFFA0D1FF, "EGGNOG"},
				{StageId_5_3, 0xFFA0D1FF, "WINTER HORRORLAND"},
				{StageId_6_1, 0xFFFF78BF, "SENPAI"},
				{StageId_6_2, 0xFFFF78BF, "ROSES"},
				{StageId_6_3, 0xFFFF78BF, "THORNS"},
				{StageId_7_1, 0xFFF6B604, "UGH"},
				{StageId_7_2, 0xFFF6B604, "GUNS"},
				{StageId_7_3, 0xFFF6B604, "STRESS"},
			};
			
			//Initialize page
			if (menu.page_swap)
			{
				menu.scroll = COUNT_OF(menu_options) * FIXED_DEC(24 + SCREEN_HEIGHT2,1);
				menu.page_param.stage.diff = StageDiff_Normal;
				menu.page_state.freeplay.back_r = FIXED_DEC(255,1);
				menu.page_state.freeplay.back_g = FIXED_DEC(255,1);
				menu.page_state.freeplay.back_b = FIXED_DEC(255,1);
			}
			
			//Draw page label
			menu.font_bold.draw(&menu.font_bold,
				"FREEPLAY",
				16,
				SCREEN_HEIGHT - 32,
				FontAlign_Left
			);
			
			//Draw difficulty selector
			Menu_DifficultySelector(SCREEN_WIDTH - 100, SCREEN_HEIGHT2 - 48);
			
			//Handle option and selection
			if (menu.next_page == menu.page && Trans_Idle())
			{
				//Change option
				if (pad_state.press & PAD_UP)
				{
					//play scroll sound
                    Audio_PlaySound(Menu_Sounds[0]);
					if (menu.select > 0)
						menu.select--;
					else
						menu.select = COUNT_OF(menu_options) - 1;
				}
				if (pad_state.press & PAD_DOWN)
				{
					//play scroll sound
                    Audio_PlaySound(Menu_Sounds[0]);
					if (menu.select < COUNT_OF(menu_options) - 1)
						menu.select++;
					else
						menu.select = 0;
				}
				
				//Select option if cross is pressed
				if (pad_state.press & (PAD_START | PAD_CROSS))
				{
					//play confirm sound
					Audio_PlaySound(Menu_Sounds[1]);
					menu.next_page = MenuPage_Stage;
					menu.page_param.stage.id = menu_options[menu.select].stage;
					menu.page_param.stage.story = false;
					Trans_Start();
				}
				
				//Return to main menu if circle is pressed
				if (pad_state.press & PAD_CIRCLE)
				{
					//play cancel sound
					Audio_PlaySound(Menu_Sounds[2]);
					menu.next_page = MenuPage_Main;
					menu.next_select = 1; //Freeplay
					Trans_Start();
				}
			}
			
			//Draw options
			s32 next_scroll = menu.select * FIXED_DEC(24,1);
			menu.scroll += (next_scroll - menu.scroll) >> 4;
			
			for (u8 i = 0; i < COUNT_OF(menu_options); i++)
			{
				//Get position on screen
				s32 y = (i * 24) - 8 - (menu.scroll >> FIXED_SHIFT);
				if (y <= -SCREEN_HEIGHT2 - 8)
					continue;
				if (y >= SCREEN_HEIGHT2 + 8)
					break;
				
				//Draw text
				menu.font_bold.draw(&menu.font_bold,
					Menu_LowerIf(menu_options[i].text, menu.select != i),
					48 + (y >> 2),
					SCREEN_HEIGHT2 + y - 8,
					FontAlign_Left
				);
			}
			
			//Draw background
			fixed_t tgt_r = (fixed_t)((menu_options[menu.select].col >> 16) & 0xFF) << FIXED_SHIFT;
			fixed_t tgt_g = (fixed_t)((menu_options[menu.select].col >>  8) & 0xFF) << FIXED_SHIFT;
			fixed_t tgt_b = (fixed_t)((menu_options[menu.select].col >>  0) & 0xFF) << FIXED_SHIFT;
			
			menu.page_state.freeplay.back_r += (tgt_r - menu.page_state.freeplay.back_r) >> 4;
			menu.page_state.freeplay.back_g += (tgt_g - menu.page_state.freeplay.back_g) >> 4;
			menu.page_state.freeplay.back_b += (tgt_b - menu.page_state.freeplay.back_b) >> 4;
			
			Menu_DrawBack(
				true,
				8,
				menu.page_state.freeplay.back_r >> (FIXED_SHIFT + 1),
				menu.page_state.freeplay.back_g >> (FIXED_SHIFT + 1),
				menu.page_state.freeplay.back_b >> (FIXED_SHIFT + 1),
				0, 0, 0
			);
			break;
		}
		case MenuPage_Credits:
		{
			static const struct
			{
				const char *text;
				const char *text2;
				s16 icon;
			} menu_options[] = {
				{"IGORSOU",  "MAKE MOSTLY\nOF THE PORT",0},
				{"UNSTOPABLE", "HELPED WITH\nOFFSETS",1},
				{"LORD SCOUT", "HELPED WITH\nOFFSETS",2},
				{"JOHN PAUL",  "PLAYTESTER\nAND FRIEND",7},
				{"LUCKY","MISS AND\nACCURATE CODE",4},
				{"CUCKYDEV","MAKE THE\nPSXFUNKIN",6},
				{"PSXFUNKIN DISCORD","DISCORD", 3},
				{"ZERIBEN","FRIEND",5},
			};
			
			//Initialize page
			if (menu.page_swap)
			{
				menu.scroll = COUNT_OF(menu_options) * FIXED_DEC(24 + SCREEN_HEIGHT2,1);
				menu.page_param.stage.diff = StageDiff_Normal;
				menu.page_state.credit.back_r = FIXED_DEC(255,1);
				menu.page_state.credit.back_g = FIXED_DEC(255,1);
				menu.page_state.credit.back_b = FIXED_DEC(255,1);
			}
			
			//Handle option and selection
			if (menu.next_page == menu.page && Trans_Idle())
			{
				
				//Change option
				if (pad_state.press & PAD_LEFT)
				{
					//play scroll sound
                    Audio_PlaySound(Menu_Sounds[0]);
			        if (menu.select > 0)
						menu.select--;
					else
						menu.select = COUNT_OF(menu_options) - 1;
				}
				if (pad_state.press & PAD_RIGHT)
				{
					//play scroll sound
                    Audio_PlaySound(Menu_Sounds[0]);
				    if (menu.select < COUNT_OF(menu_options) - 1)
						menu.select++;
					else
						menu.select = 0;
				}
				if (pad_state.press & PAD_DOWN)
				{
					//play scroll sound
                    Audio_PlaySound(Menu_Sounds[0]);
				    if (menu.select < COUNT_OF(menu_options) - 3)
						menu.select += 3;
				}
				if (pad_state.press & PAD_UP)
				{
					//play scroll sound
                    Audio_PlaySound(Menu_Sounds[0]);
			        if (menu.select > 2)
						menu.select -= 3;
				}
				
				//Return to main menu if circle is pressed
				if (pad_state.press & PAD_CIRCLE)
				{
					//play cancel sound
					Audio_PlaySound(Menu_Sounds[2]);
					menu.next_page = MenuPage_Main;
					menu.next_select = 2; //Credits
					Trans_Start();
				}
			}

			   //Get l1 and r1 src and dst
				RECT l1_src = {14, 8, 30, 19};

				RECT r1_src = {15,32, 30, 18};
	
				Gfx_BlitTex(&menu.tex_extra, &l1_src, 0, 10);
				Gfx_BlitTex(&menu.tex_extra, &r1_src, 290, 10);
                  
				    menu.font_bold.draw(&menu.font_bold,
					"FORK MADE BY",
					160,
					10,
					FontAlign_Center
					);
					 menu.font_bold.draw(&menu.font_bold,
					"NAME",
					260,
					33,
					FontAlign_Center
					);
					menu.font_bold.draw(&menu.font_bold,
					"INFO",
					260,
					123,
					FontAlign_Center
					);

			      		//Draw credits information
			       		 menu.font_arial.draw(&menu.font_arial,
						Menu_LowerIf(menu_options[menu.select].text2, false),
						200,
						150,
						FontAlign_Left
						);

			       			//Draw text
							menu.font_arial.draw(&menu.font_arial,
							Menu_LowerIf(menu_options[menu.select].text, false),
							260,
							70,
							FontAlign_Center
							);

							RECT rectangle_src = {195, 30, 140, 20};
							Gfx_BlendRect(&rectangle_src, 6, 6, 6, 0);

							RECT rectangle2_src = {195, 120, 140, 20};
							Gfx_BlendRect(&rectangle2_src, 6, 6, 6, 0);

							//draw big square
							RECT square_src = {195, 30, 140, 220};
							Gfx_BlendRect(&square_src,111,111,111, 0);
			
			//Draw options		
			for (u8 i = 0; i < COUNT_OF(menu_options); i++)
			{
				//Draw credits image
				if (menu_options[i].icon != -1)
				DrawCredit(menu_options[i].icon, (i*64), 30, menu.select != i);
			}
			
			//Draw background	
			Menu_DrawBack(
				true,
				0,
			    65 >> 1,
				67 >> 1,
				65 >> 1,
				0, 0, 0
			);
			break;
		}
		case MenuPage_Options:
		{
			static const char *gamemode_strs[] = {"Normal", "Swap", "Two Player"};
			static const struct
			{
				enum
				{
					OptType_Boolean,
					OptType_Enum,
				} type;
				const char *text;
				void *value;
				union
				{
					struct
					{
						int a;
					} spec_boolean;
					struct
					{
						s32 max;
						const char **strs;
					} spec_enum;
				} spec;
			} menu_mainoptions[] = {
				//general options
				{OptType_Enum,    "Gamemode", &stage.mode, {.spec_enum = {COUNT_OF(gamemode_strs), gamemode_strs}}},
				{OptType_Boolean, "Ghost Tap", &stage.ghost, {.spec_boolean = {0}}},
				{OptType_Boolean, "BotPlay", &stage.botplay, {.spec_boolean = {0}}},
				//Note options
				{OptType_Boolean, "Downscroll", &stage.downscroll, {.spec_boolean = {0}}},
				{OptType_Boolean, "Middlescroll", &stage.middlescroll, {.spec_boolean = {0}}},
			};

			static const struct
			{
				const char *text;
			} menu_options[] = {
			{"General"},
			{"Notes"},
			{"Misc"},
			};
			
			//Initialize page
			if (menu.page_swap)
				menu.scroll = COUNT_OF(menu_mainoptions) * FIXED_DEC(24 + SCREEN_HEIGHT2,1);
			
			//Handle option and selection
			if (menu.next_page == menu.page && Trans_Idle())
			{
				//switch options
				if (pad_state.press & PAD_R1)
				{
				 if (menu.switchop < COUNT_OF(menu_options) - 1)
				 menu.switchop++;

				 else
				 menu.switchop = 0;
				}
				if (pad_state.press & PAD_L1)
				{		 
					if (menu.switchop > 0)
						menu.switchop--;
					else
					menu.switchop = COUNT_OF(menu_options) - 1;
				}
				//Change option
				if (pad_state.press & PAD_UP)
				{	
					//play scroll sound
                    Audio_PlaySound(Menu_Sounds[0]);	 
					if (menu.select > 0)
						menu.select--;
				}
				if (pad_state.press & PAD_DOWN)
				{
					//play scroll sound
                    Audio_PlaySound(Menu_Sounds[0]);
					if (menu.select < COUNT_OF(menu_mainoptions) - 1)
						menu.select++;
				}
			}
				
				//Handle option changing
				switch (menu_mainoptions[menu.select].type)
				{
					case OptType_Boolean:
						if (pad_state.press & (PAD_CROSS | PAD_LEFT | PAD_RIGHT))
							*((boolean*)menu_mainoptions[menu.select].value) ^= 1;
						break;
					case OptType_Enum:
						if (pad_state.press & PAD_LEFT)
							if (--*((s32*)menu_mainoptions[menu.select].value) < 0)
								*((s32*)menu_mainoptions[menu.select].value) = menu_mainoptions[menu.select].spec.spec_enum.max - 1;
						if (pad_state.press & PAD_RIGHT)
							if (++*((s32*)menu_mainoptions[menu.select].value) >= menu_mainoptions[menu.select].spec.spec_enum.max)
								*((s32*)menu_mainoptions[menu.select].value) = 0;
						break;
				}
				
				//Return to main menu if circle is pressed
				if (pad_state.press & PAD_CIRCLE)
				{
					//play cancel sound
					Audio_PlaySound(Menu_Sounds[2]);
					menu.next_page = MenuPage_Main;
					menu.next_select = 3; //Options
					Trans_Start();
				}

				//Get l1 and r1 src and dst
				RECT l1_src = {14, 8, 30, 19};

				RECT r1_src = {15,32, 30, 18};
	
				Gfx_BlitTex(&menu.tex_extra, &l1_src, 0, 10);
				Gfx_BlitTex(&menu.tex_extra, &r1_src, 290, 10);
			
			//Draw options
			s32 next_scroll = menu.select * FIXED_DEC(24,1);
			menu.scroll += (next_scroll - menu.scroll) >> 4;

			for (u8 i = 0; i < COUNT_OF(menu_options); i++)
			{
				//Get position on screen
				s32 x = (i * 90);
				
				//Draw text
				menu.font_arial.draw(&menu.font_arial,
				    menu_options[i].text,
					48 + x,
				    20,
					FontAlign_Left
				);
				//draw square for general,notes and misc, and make a different color when u select
				RECT square_src = {26 + x, 15, 90, 18};
				if (menu.switchop != i)
				Gfx_BlendRect(&square_src, 85, 82, 75, 0);
				else
				Gfx_BlendRect(&square_src, 136, 131, 120, 0);
			}
			
			for (u8 i = 0; i < COUNT_OF(menu_mainoptions); i++)
			{
				//Get position on screen
				s32 y = (i * 15) - 8;
				
				//Draw text
				char text[0x80];
				switch (menu_mainoptions[i].type)
				{
					case OptType_Boolean:
						sprintf(text, "%s %s", menu_mainoptions[i].text, *((boolean*)menu_mainoptions[i].value) ? "ON" : "OFF");
						break;
					case OptType_Enum:
						sprintf(text, "%s %s", menu_mainoptions[i].text, menu_mainoptions[i].spec.spec_enum.strs[*((s32*)menu_mainoptions[i].value)]);
						break;
				}
				//draw a font with different color when u are with a option select
				menu.font_arial.draw_col(&menu.font_arial,
					text,
					90,
				    60 + y - 8,
					FontAlign_Center,
					(menu.select != i) ? 111 : 0x80,
					(menu.select != i) ? 111 : 0x80,
					(menu.select != i) ? 111 : 0x80
				);
			}
			
			//draw big square
			RECT square_src = {26, 33, 270, 180};
			Gfx_BlendRect(&square_src,111,111,111, 0);

			//Draw background
			Menu_DrawBack(
				true,
				8,
				253 >> 1, 113 >> 1, 155 >> 1,
				0, 0, 0
			);
			break;
		}
		case MenuPage_Stage:
		{
			//Unload menu state and load stage
			Menu_Unload();
			Stage_LoadScr(menu.page_param.stage.id, menu.page_param.stage.diff, menu.page_param.stage.story);
			return;
		}
		case MenuPage_Movie:
		{
			//Unload
			Menu_Unload();
			//Play movie
			LoadScr_Start();
			gameloop = GameLoop_Movie;
			LoadScr_End();
			break;
		}
		default:
			break;
	}
	
	//Clear page swap flag
	menu.page_swap = menu.page != exec_page;
}
