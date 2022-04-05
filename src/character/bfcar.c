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

//Car Boyfriend player assets
static u8 char_bfcar_arc_main[] = {
	#include "iso/bf/car.arc.h"
};

//Car Boyfriend skull fragments
static SkullFragment char_bfcar_skull[15] = {
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

//Car Boyfriend player types
enum
{
	BFCar_ArcMain_BFCar0,
	BFCar_ArcMain_BFCar1,
	BFCar_ArcMain_BFCar2,
	BFCar_ArcMain_BFCar3,
	BFCar_ArcMain_BFCar4,
	BFCar_ArcMain_BFCar5,
	BFCar_ArcMain_BFCar6,
	BFCar_ArcMain_BFCar7,
	BF_ArcMain_BF5,
	BF_ArcMain_BF6,
	BF_ArcMain_Dead0, //BREAK
	BF_ArcDead_Dead1,
	BF_ArcDead_Dead2,
	BF_ArcDead_Retry, //Retry prompt
	
	BFCar_ArcMain_Max,
};

typedef struct
{
	//Character base structure
	Character character;
	
	//Render data and state
	IO_Data arc_ptr[BFCar_ArcMain_Max];
	
	Gfx_Tex tex, tex_retry;
	u8 frame, tex_id;
	u8 retry_bump;
	
	SkullFragment skull[COUNT_OF(char_bfcar_skull)];
	u8 skull_scale;
} Char_BFCar;

//Car Boyfriend player definitions
static const CharFrame char_bfcar_frame[] = {
	{BFCar_ArcMain_BFCar0, {  0,   1, 104, 100}, { 53,  89}}, //0 idle 1
	{BFCar_ArcMain_BFCar0, {104,   1, 104, 100}, { 53,  89}}, //1 idle 2
	{BFCar_ArcMain_BFCar0, {  0, 103, 104, 100}, { 53,  89}}, //2 idle 3
	{BFCar_ArcMain_BFCar0, {104, 105, 104, 101}, { 53,  90}}, //3 idle 4
	{BFCar_ArcMain_BFCar1, {  2,   0, 102, 101}, { 53,  90}}, //4 idle 5
	{BFCar_ArcMain_BFCar1, {105,   0, 103, 104}, { 53,  94}}, //5 idle 6
	{BFCar_ArcMain_BFCar1, {  1, 102, 103, 104}, { 53,  94}}, //6 idle 7
	{BFCar_ArcMain_BFCar1, {104, 104, 104, 104}, { 53,  94}}, //7 idle 8
	{BFCar_ArcMain_BFCar2, {  4,   1, 103, 105}, { 53,  94}}, //8 idle 9
	{BFCar_ArcMain_BFCar2, {108,   1, 103, 105}, { 53,  94}}, //9 idle 10
	{BFCar_ArcMain_BFCar2, {  0, 114, 104, 105}, { 53,  94}}, //10 idle 11

	{BFCar_ArcMain_BFCar2, {112, 116,  97, 102}, { 53,  90}}, //11 left 1
	{BFCar_ArcMain_BFCar3, {  2,   3,  97, 102}, { 53,  90}}, //12 left 2
	{BFCar_ArcMain_BFCar3, {110,   3,  94, 102}, { 50,  90}}, //13 left 3
	{BFCar_ArcMain_BFCar3, {  1, 116,  95, 102}, { 50,  90}}, //14 left 4
	{BFCar_ArcMain_BFCar3, {114, 116,  95, 102}, { 50,  90}}, //15 left 5

	{BFCar_ArcMain_BFCar4, {  6,  16,  94,  90}, { 50,  80}}, //16 down 1
	{BFCar_ArcMain_BFCar4, {109,  16,  96,  90}, { 50,  80}}, //17 down 2
	{BFCar_ArcMain_BFCar4, {  3, 128,  95,  91}, { 50,  80}}, //18 down 3
	{BFCar_ArcMain_BFCar4, {115, 128,  95,  91}, { 50,  80}}, //19 down 4
	{BFCar_ArcMain_BFCar5, {  6,  15,  94,  91}, { 50,  80}}, //20 down 5

	{BFCar_ArcMain_BFCar5, {118,   1,  94, 112}, { 43, 100}}, //21 up 1
	{BFCar_ArcMain_BFCar5, { 13, 107,  94, 112}, { 43, 100}}, //22 up 2
	{BFCar_ArcMain_BFCar6, { 10,   0,  95, 111}, { 43, 100}}, //23 up 3
	{BFCar_ArcMain_BFCar6, {117,   0,  95, 111}, { 43, 100}}, //24 up 4

	{BFCar_ArcMain_BFCar6, {  1, 116, 104, 103}, { 40,  92}}, //25 right 1
	{BFCar_ArcMain_BFCar6, {108, 116, 103, 103}, { 40,  92}}, //26 right 2
	{BFCar_ArcMain_BFCar7, {  0,   7, 104, 103}, { 40,  92}}, //27 right 3
	{BFCar_ArcMain_BFCar7, {109,  10, 103, 103}, { 40,  92}}, //28 right 4
	{BFCar_ArcMain_BFCar7, {  1, 116, 103, 103}, { 40,  92}}, //29 right 5

	{BF_ArcMain_BF5, {  0,   0,  93, 108}, { 52, 101}}, //30 left miss 1
	{BF_ArcMain_BF5, { 94,   0,  93, 108}, { 52, 101}}, //31 left miss 2
	
	{BF_ArcMain_BF5, {  0, 109,  95,  98}, { 50,  90}}, //32 down miss 1
	{BF_ArcMain_BF5, { 96, 109,  95,  97}, { 50,  89}}, //33 down miss 2
	
	{BF_ArcMain_BF6, {  0,   0,  90, 107}, { 44,  99}}, //34 up miss 1
	{BF_ArcMain_BF6, { 91,   0,  89, 108}, { 44, 100}}, //35 up miss 2
	
	{BF_ArcMain_BF6, {  0, 108,  99, 108}, { 42, 101}}, //36 right miss 1
	{BF_ArcMain_BF6, {100, 109, 101, 108}, { 43, 101}}, //37 right miss 2

	{BF_ArcMain_Dead0, {  0,   0, 128, 128}, { 53,  98}}, //23 dead0 0
	{BF_ArcMain_Dead0, {128,   0, 127, 128}, { 53,  98}}, //24 dead0 1
	{BF_ArcMain_Dead0, {  0, 128, 128, 127}, { 53,  98}}, //25 dead0 2
	{BF_ArcMain_Dead0, {128, 128, 127, 127}, { 53,  98}}, //26 dead0 3
	
	{BF_ArcDead_Dead1, {  0,   0, 128, 128}, { 53,  98}}, //27 dead1 0
	{BF_ArcDead_Dead1, {128,   0, 127, 128}, { 53,  98}}, //28 dead1 1
	{BF_ArcDead_Dead1, {  0, 128, 128, 127}, { 53,  98}}, //29 dead1 2
	{BF_ArcDead_Dead1, {128, 128, 127, 127}, { 53,  98}}, //30 dead1 3
	
	{BF_ArcDead_Dead2, {  0,   0, 128, 128}, { 53,  98}}, //31 dead2 body twitch 0
	{BF_ArcDead_Dead2, {128,   0, 127, 128}, { 53,  98}}, //32 dead2 body twitch 1
	{BF_ArcDead_Dead2, {  0, 128, 128, 127}, { 53,  98}}, //33 dead2 balls twitch 0
	{BF_ArcDead_Dead2, {128, 128, 127, 127}, { 53,  98}}, //34 dead2 balls twitch 1
};

static const Animation char_bfcar_anim[PlayerAnim_Max] = {
	{1, (const u8[]){ 0,  1,  2,  3,  4,  5,  5,  6,  6,  7,  8,  9, 10, 10, 10, ASCR_BACK, 1}}, //CharAnim_Idle
	{1, (const u8[]){11, 12, 13, 14, 15, ASCR_BACK, 2}},             //CharAnim_Left
	{0, (const u8[]){ASCR_CHGANI, CharAnim_Idle}},       //CharAnim_LeftAlt
	{1, (const u8[]){16, 17, 18, 19, 20, ASCR_BACK, 2}},            //CharAnim_Down
	{0, (const u8[]){ASCR_CHGANI, CharAnim_Idle}},       //CharAnim_DownAlt
	{1, (const u8[]){21, 22, 23, 24, ASCR_BACK, 2}},             //CharAnim_Up
	{0, (const u8[]){ASCR_CHGANI, CharAnim_Idle}},       //CharAnim_UpAlt
	{1, (const u8[]){25, 26, 27, 28, 29, ASCR_BACK, 2}},            //CharAnim_Right
	{0, (const u8[]){ASCR_CHGANI, CharAnim_Idle}},       //CharAnim_RightAlt
	
	{1, (const u8[]){11, 30, 30, 31, ASCR_BACK, 1}},     //PlayerAnim_LeftMiss
	{1, (const u8[]){16, 32, 32, 33, ASCR_BACK, 1}},     //PlayerAnim_DownMiss
	{1, (const u8[]){21, 34, 34, 35, ASCR_BACK, 1}},     //PlayerAnim_UpMiss
	{1, (const u8[]){25, 36, 36, 37, ASCR_BACK, 1}},     //PlayerAnim_RightMiss
	
	{0, (const u8[]){ASCR_CHGANI, CharAnim_Idle}},         //PlayerAnim_Peace
	{0, (const u8[]){ASCR_CHGANI, CharAnim_Idle}},       //PlayerAnim_Sweat

	{3, (const u8[]){28+10, 29+10, 30+10, 31+10, 31+10, 31+10, 31+10, 31+10, 31+10, 31+10, ASCR_CHGANI, PlayerAnim_Dead1}}, //PlayerAnim_Dead0
	{3, (const u8[]){31+10, ASCR_REPEAT}},                                                       //PlayerAnim_Dead1
	{4, (const u8[]){32+10, 33+10, 34+10, 35+10, 35+10, 35+10, 35+10, 35+10, 35+10, 35+10, ASCR_CHGANI, PlayerAnim_Dead3}}, //PlayerAnim_Dead2
	{3, (const u8[]){35+10, ASCR_REPEAT}},                                                       //PlayerAnim_Dead3
	{3, (const u8[]){36+10, 37+10, 35+10, 35+10, 35+10, 35+10, 35+10, ASCR_CHGANI, PlayerAnim_Dead3}},             //PlayerAnim_Dead4
	{3, (const u8[]){38+10, 39+10, 35+10, 35+10, 35+10, 35+10, 35+10, ASCR_CHGANI, PlayerAnim_Dead3}},             //PlayerAnim_Dead5
	
	{10, (const u8[]){35+10, 35+10, 35+10, ASCR_CHGANI, PlayerAnim_Dead7}}, //PlayerAnim_Dead4
	{ 3, (const u8[]){38+10, 39+10, 35+10, ASCR_REPEAT}},  //PlayerAnim_Dead5
};

//Car Boyfriend player functions
static void Char_BFCar_SetFrame(void *user, u8 frame)
{
	Char_BFCar *this = (Char_BFCar*)user;
	
	//Check if this is a new frame
	if (frame != this->frame)
	{
		//Check if new art shall be loaded
		const CharFrame *cframe = &char_bfcar_frame[this->frame = frame];
		if (cframe->tex != this->tex_id)
			Gfx_LoadTex(&this->tex, this->arc_ptr[this->tex_id = cframe->tex], 0);
	}
}

static void Char_BFCar_Tick(Character *character)
{
	Char_BFCar *this = (Char_BFCar*)character;
	
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
	
	//Perform idle dance
	if (stage.flag & STAGE_FLAG_JUST_STEP)
	{
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
	}

	//Retry screen
	if (character->animatable.anim >= PlayerAnim_Dead3)
	{
		//Tick skull fragments
		if (this->skull_scale)
		{
			SkullFragment *frag = this->skull;
			for (size_t i = 0; i < COUNT_OF_MEMBER(Char_BFCar, skull); i++, frag++)
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
	Animatable_Animate(&character->animatable, (void*)this, Char_BFCar_SetFrame);
	Character_Draw(character, &this->tex, &char_bfcar_frame[this->frame]);
}

static void Char_BFCar_SetAnim(Character *character, u8 anim)
{
	Char_BFCar *this = (Char_BFCar*)character;

	//Perform animation checks
	switch (anim)
	{
		case PlayerAnim_Dead0:
			//Begin adjust focus
			character->focus_x = FIXED_DEC(0,1);
			character->focus_y = FIXED_DEC(-40,1);
			character->focus_zoom = FIXED_DEC(125,100);
			break;
		case PlayerAnim_Dead2:		
			//Load retry art
			Gfx_LoadTex(&this->tex_retry, this->arc_ptr[BF_ArcDead_Retry], 0);
			break;
	}

	//Set animation
	Animatable_SetAnim(&character->animatable, anim);
	Character_CheckStartSing(character);
}

static void Char_BFCar_Free(Character *character)
{
	(void)character;
}

static Character *Char_BFCar_New(fixed_t x, fixed_t y)
{
	//Allocate boyfriend object
	Char_BFCar *this = Mem_Alloc(sizeof(Char_BFCar));
	if (this == NULL)
	{
		sprintf(error_msg, "[Char_BFCar_New] Failed to allocate boyfriend object");
		ErrorLock();
		return NULL;
	}
	
	//Initialize character
	this->character.tick = Char_BFCar_Tick;
	this->character.set_anim = Char_BFCar_SetAnim;
	this->character.free = Char_BFCar_Free;
	
	Animatable_Init(&this->character.animatable, char_bfcar_anim);
	Character_Init((Character*)this, x, y);
	
	//Set character information
	this->character.spec = CHAR_SPEC_MISSANIM;
	
	this->character.health_b = 0xFF2ba9c9;
	this->character.health_i = 0;

	this->character.flip = true;
	
	this->character.focus_x = FIXED_DEC(-50,1);
	this->character.focus_y = (stage.stage_id == StageId_1_4) ? FIXED_DEC(-85,1) : FIXED_DEC(-65,1);
	this->character.focus_zoom = FIXED_DEC(1,1);
	
	//Load art
	const char **pathp = (const char *[]){
		"bfcar0.tim",   //BFCar_ArcMain_BFCar0
		"bfcar1.tim",   //BFCar_ArcMain_BFCar1
		"bfcar2.tim",   //BFCar_ArcMain_BFCar2
		"bfcar3.tim",   //BFCar_ArcMain_BFCar3
		"bfcar4.tim",   //BFCar_ArcMain_BFCar4
		"bfcar5.tim",   //BFCar_ArcMain_BFCar5
		"bfcar6.tim",   //BFCar_ArcMain_BFCar6
		"bfcar7.tim",   //BFCar_ArcMain_BFCar7
		"bf5.tim",   //BF_ArcMain_BF5
		"bf6.tim",   //BF_ArcMain_BF6
		"dead0.tim", //BF_ArcMain_Dead0
		"dead1.tim", //BF_ArcMain_Dead1
		"dead2.tim", //BF_ArcMain_Dead2
		"retry.tim", //BF_ArcMain_Retry
		NULL
	};
	IO_Data *arc_ptr = this->arc_ptr;
	for (; *pathp != NULL; pathp++)
		*arc_ptr++ = Archive_Find((IO_Data)char_bfcar_arc_main, *pathp);
	
	//Initialize render state
	this->tex_id = this->frame = 0xFF;

	//Initialize player state
	this->retry_bump = 0;
	
	//Copy skull fragments
	memcpy(this->skull, char_bfcar_skull, sizeof(char_bfcar_skull));
	this->skull_scale = 64;
	
	SkullFragment *frag = this->skull;
	for (size_t i = 0; i < COUNT_OF_MEMBER(Char_BFCar, skull); i++, frag++)
	{
		//Randomize trajectory
		frag->xsp += RandomRange(-4, 4);
		frag->ysp += RandomRange(-2, 2);
	}
	
	return (Character*)this;
}
