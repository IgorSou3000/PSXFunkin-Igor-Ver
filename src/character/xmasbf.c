/*
  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "boot/character.h"
#include "boot/mem.h"
#include "boot/archive.h"
#include "boot/stage.h"
#include "boot/random.h"
#include "boot/main.h"

//Boyfriend player assets
static u8 char_xmasbf_arc_main[] = {
	#include "iso/bf/xmas.arc.h"
};

//Boyfriend skull fragments
static SkullFragment char_xmasbf_skull[15] = {
	{ 1 * 8, -87 * 8, -13, -13},
	{ 9 * 8, -88 * 8,   5, -22},
	{18 * 8, -87 * 8,   9, -22},
	{26 * 8, -85 * 8,  13, -13},
	
	{-3 * 8, -82 * 8, -13, -11},
	{ 8 * 8, -85 * 8,  -9, -15},
	{20 * 8, -82 * 8,   9, -15},
	{30 * 8, -79 * 8,  13, -11},
	
	{-1 * 8, -74 * 8, -13, -5},
	{ 8 * 8, -77 * 8,  -9, -9},
	{19 * 8, -75 * 8,   9, -9},
	{26 * 8, -74 * 8,  13, -5},
	
	{ 5 * 8, -73 * 8, -5, -3},
	{14 * 8, -76 * 8,  9, -6},
	{26 * 8, -67 * 8, 15, -3},
};

//Boyfriend player types
enum
{
	XmasBF_ArcMain_XmasBF0,
	XmasBF_ArcMain_XmasBF1,
	XmasBF_ArcMain_XmasBF2,
	XmasBF_ArcMain_XmasBF3,
	XmasBF_ArcMain_XmasBF4,
	XmasBF_ArcMain_XmasBF5,
	XmasBF_ArcMain_Dead0, //BREAK
	XmasBF_ArcDead_Dead1, //Mic Drop
	XmasBF_ArcDead_Dead2, //Twitch
	XmasBF_ArcDead_Retry, //Retry prompt
	
	XmasBF_ArcMain_Max,
};

#define XmasBF_Arc_Max XmasBF_ArcMain_Max

typedef struct
{
	//Character base structure
	Character character;
	
	//Render data and state
	IO_Data arc_main;
	IO_Data arc_ptr[XmasBF_Arc_Max];
	
	Gfx_Tex tex, tex_retry;
	u8 frame, tex_id;
	
	u8 retry_bump;
	
	SkullFragment skull[COUNT_OF(char_xmasbf_skull)];
	u8 skull_scale;
} Char_XmasBF;

//Boyfriend player definitions
static const CharFrame char_xmasbf_frame[] = {
	{XmasBF_ArcMain_XmasBF0, {  0,   0, 102, 100}, { 53,  91}}, //0 idle 1
	{XmasBF_ArcMain_XmasBF0, {103,   0, 102, 100}, { 53,  91}}, //1 idle 2
	{XmasBF_ArcMain_XmasBF0, {  0, 100, 102, 102}, { 53,  93}}, //2 idle 3
	{XmasBF_ArcMain_XmasBF0, {103, 100, 102, 105}, { 53,  97}}, //3 idle 4
	{XmasBF_ArcMain_XmasBF1, {  0,   0, 103, 105}, { 52,  97}}, //4 idle 5
	
	{XmasBF_ArcMain_XmasBF1, {104,   0,  96, 102}, { 56,  95}}, //5 left 1
	{XmasBF_ArcMain_XmasBF1, {  0, 106,  94, 104}, { 54,  95}}, //6 left 2
	
	{XmasBF_ArcMain_XmasBF1, { 95, 103,  97,  94}, { 52,  82}}, //7 down 1
	{XmasBF_ArcMain_XmasBF2, {  0,   0,  94,  92}, { 52,  83}}, //8 down 2
	
	{XmasBF_ArcMain_XmasBF2, { 95,   0,  93, 112}, { 41, 102}}, //9 up 1
	{XmasBF_ArcMain_XmasBF2, {  0,  92,  94, 112}, { 42, 103}}, //10 up 2
	
	{XmasBF_ArcMain_XmasBF2, { 95, 113, 102, 102}, { 41,  95}}, //11 right 1
	{XmasBF_ArcMain_XmasBF3, {  0,   0, 102, 102}, { 41,  95}}, //12 right 2
	
	{XmasBF_ArcMain_XmasBF4, {  0,   0,  93, 108}, { 52, 101}}, //20 left miss 1
	{XmasBF_ArcMain_XmasBF4, { 94,   0,  93, 108}, { 52, 101}}, //21 left miss 2
	
	{XmasBF_ArcMain_XmasBF4, {  0, 110,  96, 100}, { 50,  90}}, //22 down miss 1
	{XmasBF_ArcMain_XmasBF4, { 97, 110,  97,  99}, { 50,  89}}, //23 down miss 2
	
	{XmasBF_ArcMain_XmasBF5, {  0,   0,  90, 107}, { 44,  99}}, //24 up miss 1
	{XmasBF_ArcMain_XmasBF5, { 91,   0,  89, 108}, { 44, 100}}, //25 up miss 2
	
	{XmasBF_ArcMain_XmasBF5, {  0, 108, 100, 109}, { 42, 101}}, //26 right miss 1
	{XmasBF_ArcMain_XmasBF5, {101, 110, 101, 109}, { 43, 101}}, //27 right miss 2

	{XmasBF_ArcMain_Dead0, {  0,   0, 128, 128}, { 53,  98}}, //23 dead0 0
	{XmasBF_ArcMain_Dead0, {128,   0, 128, 128}, { 53,  98}}, //24 dead0 1
	{XmasBF_ArcMain_Dead0, {  0, 128, 128, 128}, { 53,  98}}, //25 dead0 2
	{XmasBF_ArcMain_Dead0, {128, 128, 128, 128}, { 53,  98}}, //26 dead0 3
	
	{XmasBF_ArcDead_Dead1, {  0,   0, 128, 128}, { 53,  98}}, //27 dead1 0
	{XmasBF_ArcDead_Dead1, {128,   0, 128, 128}, { 53,  98}}, //28 dead1 1
	{XmasBF_ArcDead_Dead1, {  0, 128, 128, 128}, { 53,  98}}, //29 dead1 2
	{XmasBF_ArcDead_Dead1, {128, 128, 128, 128}, { 53,  98}}, //30 dead1 3
	
	{XmasBF_ArcDead_Dead2, {  0,   0, 128, 128}, { 53,  98}}, //31 dead2 body twitch 0
	{XmasBF_ArcDead_Dead2, {128,   0, 128, 128}, { 53,  98}}, //32 dead2 body twitch 1
	{XmasBF_ArcDead_Dead2, {  0, 128, 128, 128}, { 53,  98}}, //33 dead2 balls twitch 0
	{XmasBF_ArcDead_Dead2, {128, 128, 128, 128}, { 53,  98}}, //34 dead2 balls twitch 1
};

static const Animation char_xmasbf_anim[PlayerAnim_Max] = {
	{2, (const u8[]){ 0,  1,  2,  3,  4, ASCR_BACK, 1}}, //CharAnim_Idle
	{2, (const u8[]){ 5,  6, ASCR_BACK, 1}},             //CharAnim_Left
	{0, (const u8[]){ASCR_CHGANI, CharAnim_Idle}},       //CharAnim_LeftAlt
	{2, (const u8[]){ 7,  8, ASCR_BACK, 1}},             //CharAnim_Down
	{0, (const u8[]){ASCR_CHGANI, CharAnim_Idle}},       //CharAnim_DownAlt
	{2, (const u8[]){ 9, 10, ASCR_BACK, 1}},             //CharAnim_Up
	{0, (const u8[]){ASCR_CHGANI, CharAnim_Idle}},       //CharAnim_UpAlt
	{2, (const u8[]){11, 12, ASCR_BACK, 1}},             //CharAnim_Right
	{0, (const u8[]){ASCR_CHGANI, CharAnim_Idle}},       //CharAnim_RightAlt
	
	{1, (const u8[]){ 5, 13, 13, 14, ASCR_BACK, 1}},     //PlayerAnim_LeftMiss
	{1, (const u8[]){ 7, 15, 15, 16, ASCR_BACK, 1}},     //PlayerAnim_DownMiss
	{1, (const u8[]){ 9, 17, 17, 18, ASCR_BACK, 1}},     //PlayerAnim_UpMiss
	{1, (const u8[]){11, 19, 19, 20, ASCR_BACK, 1}},     //PlayerAnim_RightMiss
	
	{2, (const u8[]){13, 14, 15, ASCR_BACK, 1}},         //PlayerAnim_Peace
	{2, (const u8[]){16, 17, 18, 19, ASCR_REPEAT}},      //PlayerAnim_Sweat
	
	{3, (const u8[]){21, 22, 23, 24, 24, 24, 24, 24, 24, 24, ASCR_CHGANI, PlayerAnim_Dead1}}, //PlayerAnim_Dead0
	{3, (const u8[]){24, ASCR_REPEAT}},                                                       //PlayerAnim_Dead1
	{4, (const u8[]){25, 26, 27, 28, 28, 28, 28, 28, 28, 28, ASCR_CHGANI, PlayerAnim_Dead3}}, //PlayerAnim_Dead2
	{3, (const u8[]){28, ASCR_REPEAT}},                                                       //PlayerAnim_Dead3
	{3, (const u8[]){29, 30, 28, 28, 28, 28, 28, ASCR_CHGANI, PlayerAnim_Dead3}},             //PlayerAnim_Dead4
	{3, (const u8[]){31, 32, 28, 28, 28, 28, 28, ASCR_CHGANI, PlayerAnim_Dead3}},             //PlayerAnim_Dead5
	
	{10, (const u8[]){28, 28, 28, ASCR_CHGANI, PlayerAnim_Dead7}}, //PlayerAnim_Dead4
	{ 3, (const u8[]){31, 32, 28, ASCR_REPEAT}},  //PlayerAnim_Dead5
};

//Boyfriend player functions
static void Char_XmasBF_SetFrame(void *user, u8 frame)
{
	Char_XmasBF *this = (Char_XmasBF*)user;
	
	//Check if this is a new frame
	if (frame != this->frame)
	{
		//Check if new art shall be loaded
		const CharFrame *cframe = &char_xmasbf_frame[this->frame = frame];
		if (cframe->tex != this->tex_id)
			Gfx_LoadTex(&this->tex, this->arc_ptr[this->tex_id = cframe->tex], 0);
	}
}

static void Char_XmasBF_Tick(Character *character)
{
	Char_XmasBF *this = (Char_XmasBF*)character;
	
	//Handle animation updates
	if ((character->pad_held & (INPUT_LEFT | INPUT_DOWN | INPUT_UP | INPUT_RIGHT)) == 0 ||
	    (character->animatable.anim != CharAnim_Left &&
	     character->animatable.anim != CharAnim_LeftAlt &&
	     character->animatable.anim != CharAnim_Down &&
	     character->animatable.anim != CharAnim_DownAlt &&
	     character->animatable.anim != CharAnim_Up &&
	     character->animatable.anim != CharAnim_UpAlt &&
	     character->animatable.anim != CharAnim_Right &&
	     character->animatable.anim != CharAnim_RightAlt))
		Character_CheckEndSing(character);
	
	if (stage.flag & STAGE_FLAG_JUST_STEP)
	{
		//Perform idle dance
		if (Animatable_Ended(&character->animatable) &&
			(character->animatable.anim != CharAnim_Left &&
		     character->animatable.anim != CharAnim_LeftAlt &&
		     character->animatable.anim != PlayerAnim_LeftMiss &&
		     character->animatable.anim != CharAnim_Down &&
		     character->animatable.anim != CharAnim_DownAlt &&
		     character->animatable.anim != PlayerAnim_DownMiss &&
		     character->animatable.anim != CharAnim_Up &&
		     character->animatable.anim != CharAnim_UpAlt &&
		     character->animatable.anim != PlayerAnim_UpMiss &&
		     character->animatable.anim != CharAnim_Right &&
		     character->animatable.anim != CharAnim_RightAlt &&
		     character->animatable.anim != PlayerAnim_RightMiss) &&
			(stage.song_step & 0x7) == 0)
			character->set_anim(character, CharAnim_Idle);
		
		//Stage specific animations
		if (stage.note_scroll >= 0)
		{
			switch (stage.stage_id)
			{
				case StageId_1_4: //Tutorial peace
					if (stage.song_step > 64 && stage.song_step < 192 && (stage.song_step & 0x3F) == 60)
						character->set_anim(character, PlayerAnim_Peace);
					break;
				case StageId_1_1: //Bopeebo peace
					if ((stage.song_step & 0x1F) == 28)
						character->set_anim(character, PlayerAnim_Peace);
					break;
				default:
					break;
			}
		}
	}
	
	//Retry screen
	if (character->animatable.anim >= PlayerAnim_Dead3)
	{
		//Tick skull fragments
		if (this->skull_scale)
		{
			SkullFragment *frag = this->skull;
			for (size_t i = 0; i < COUNT_OF_MEMBER(Char_XmasBF, skull); i++, frag++)
			{
				//Draw fragment
				RECT frag_src = {
					(i & 1) ? 112 : 96,
					(i >> 1) << 4,
					16,
					16
				};
				fixed_t skull_dim = (FIXED_DEC(16,1) * this->skull_scale) >> 6;
				fixed_t skull_rad = skull_dim >> 1;
				RECT_FIXED frag_dst = {
					character->x + (((fixed_t)frag->x << FIXED_SHIFT) >> 3) - skull_rad - stage.camera.x,
					character->y + (((fixed_t)frag->y << FIXED_SHIFT) >> 3) - skull_rad - stage.camera.y,
					skull_dim,
					skull_dim,
				};
				Stage_DrawTex(&this->tex_retry, &frag_src, &frag_dst, FIXED_MUL(stage.camera.zoom, stage.bump));
				
				//Move fragment
				frag->x += frag->xsp;
				frag->y += ++frag->ysp;
			}
			
			//Decrease scale
			this->skull_scale--;
		}
		
		//Draw input options
		u8 input_scale = 16 - this->skull_scale;
		if (input_scale > 16)
			input_scale = 0;
		
		RECT button_src = {
			 0, 96,
			16, 16
		};
		RECT_FIXED button_dst = {
			character->x - FIXED_DEC(32,1) - stage.camera.x,
			character->y - FIXED_DEC(88,1) - stage.camera.y,
			(FIXED_DEC(16,1) * input_scale) >> 4,
			FIXED_DEC(16,1),
		};
		
		//Cross - Retry
		Stage_DrawTex(&this->tex_retry, &button_src, &button_dst, FIXED_MUL(stage.camera.zoom, stage.bump));
		
		//Circle - Blueball
		button_src.x = 16;
		button_dst.y += FIXED_DEC(56,1);
		Stage_DrawTex(&this->tex_retry, &button_src, &button_dst, FIXED_MUL(stage.camera.zoom, stage.bump));
		
		//Draw 'RETRY'
		u8 retry_frame;
		
		if (character->animatable.anim == PlayerAnim_Dead6)
		{
			//Selected retry
			retry_frame = 2 - (this->retry_bump >> 3);
			if (retry_frame >= 3)
				retry_frame = 0;
			if (this->retry_bump & 2)
				retry_frame += 3;
			
			if (++this->retry_bump == 0xFF)
				this->retry_bump = 0xFD;
		}
		else
		{
			//Idle
			retry_frame = 1 +  (this->retry_bump >> 2);
			if (retry_frame >= 3)
				retry_frame = 0;
			
			if (++this->retry_bump >= 55)
				this->retry_bump = 0;
		}
		
		RECT retry_src = {
			(retry_frame & 1) ? 48 : 0,
			(retry_frame >> 1) << 5,
			48,
			32
		};
		RECT_FIXED retry_dst = {
			character->x -  FIXED_DEC(7,1) - stage.camera.x,
			character->y - FIXED_DEC(92,1) - stage.camera.y,
			FIXED_DEC(48,1),
			FIXED_DEC(32,1),
		};
		Stage_DrawTex(&this->tex_retry, &retry_src, &retry_dst, FIXED_MUL(stage.camera.zoom, stage.bump));
	}
	
	//Animate and draw character
	Animatable_Animate(&character->animatable, (void*)this, Char_XmasBF_SetFrame);
	Character_Draw(character, &this->tex, &char_xmasbf_frame[this->frame]);
}

static void Char_XmasBF_SetAnim(Character *character, u8 anim)
{
	Char_XmasBF *this = (Char_XmasBF*)character;
	
	//Perform animation checks
	switch (anim)
	{
		case PlayerAnim_Dead0:
			character->focus_x = FIXED_DEC(0,1);
			character->focus_y = FIXED_DEC(-40,1);
			character->focus_zoom = FIXED_DEC(125,100);
			break;
		case PlayerAnim_Dead2:
			//Load retry art
			Gfx_LoadTex(&this->tex_retry, this->arc_ptr[XmasBF_ArcDead_Retry], 0);
			break;
	}
	
	//Set animation
	Animatable_SetAnim(&character->animatable, anim);
	Character_CheckStartSing(character);
}

static void Char_XmasBF_Free(Character *character)
{
	(void)character;
}

static Character *Char_XmasBF_New(fixed_t x, fixed_t y)
{
	//Allocate boyfriend object
	Char_XmasBF *this = Mem_Alloc(sizeof(Char_XmasBF));
	if (this == NULL)
	{
		sprintf(error_msg, "[Char_XmasBF_New] Failed to allocate boyfriend object");
		ErrorLock();
		return NULL;
	}
	
	//Initialize character
	this->character.tick = Char_XmasBF_Tick;
	this->character.set_anim = Char_XmasBF_SetAnim;
	this->character.free = Char_XmasBF_Free;
	
	Animatable_Init(&this->character.animatable, char_xmasbf_anim);
	Character_Init((Character*)this, x, y);
	
	//Set character information
	this->character.spec = CHAR_SPEC_MISSANIM;
	
	this->character.health_b = 0xFF2ba9c9;
	this->character.health_i = 0;
	
	this->character.focus_x = FIXED_DEC(-50,1);
	this->character.focus_y = FIXED_DEC(-65,1);
	this->character.focus_zoom = FIXED_DEC(1,1);
	
	//Load art	
	const char **pathp = (const char *[]){
		"xmasbf0.tim",   //XmasBF_ArcMain_XmasBF0
		"xmasbf1.tim",   //XmasBF_ArcMain_XmasBF1
		"xmasbf2.tim",   //XmasBF_ArcMain_XmasBF2
		"xmasbf3.tim",   //XmasBF_ArcMain_XmasBF3
		"xmasbf4.tim",   //XmasBF_ArcMain_XmasBF4
		"xmasbf5.tim",   //XmasBF_ArcMain_XmasBF5
		"dead0.tim", //XmasBF_ArcMain_Dead0
		"dead1.tim", //XmasBF_ArcMain_Dead0
		"dead2.tim", //XmasBF_ArcMain_Dead0
		"retry.tim", //XmasBF_ArcMain_Dead0
		NULL
	};
	IO_Data *arc_ptr = this->arc_ptr;
	for (; *pathp != NULL; pathp++)
		*arc_ptr++ = Archive_Find((IO_Data)char_xmasbf_arc_main, *pathp);
	
	//Initialize render state
	this->tex_id = this->frame = 0xFF;
	
	//Initialize player state
	this->retry_bump = 0;
	
	//Copy skull fragments
	memcpy(this->skull, char_xmasbf_skull, sizeof(char_xmasbf_skull));
	this->skull_scale = 64;
	
	SkullFragment *frag = this->skull;
	for (size_t i = 0; i < COUNT_OF_MEMBER(Char_XmasBF, skull); i++, frag++)
	{
		//Randomize trajectory
		frag->xsp += RandomRange(-4, 4);
		frag->ysp += RandomRange(-2, 2);
	}
	
	return (Character*)this;
}
