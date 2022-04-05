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
static u8 char_bfweeb_arc_main[] = {
	#include "iso/bf/weeb.arc.h"
};

//Boyfriend Weeb player types
enum
{
	BFWeeb_ArcMain_Weeb0,
	BFWeeb_ArcMain_Weeb1,
	BFWeeb_ArcMain_Weeb2,
	BFWeeb_ArcMain_Weeb3,
	BFWeeb_ArcMain_Weeb4,
	BFWeeb_ArcMain_Weeb5,
	
	BFWeeb_ArcMain_Max,
};

typedef struct
{
	//Character base structure
	Character character;
	
	//Render data and state
	IO_Data arc_ptr[BFWeeb_ArcMain_Max];
	
	Gfx_Tex tex;
	u8 frame, tex_id;
	
	u8 skull_scale;
} Char_BFWeeb;

//Boyfriend Weeb player definitions
static const CharFrame char_bfweeb_frame[] = {
	{BFWeeb_ArcMain_Weeb0, {  0,   0,  60,  55}, { 35,  52}}, //0 idle 1
	{BFWeeb_ArcMain_Weeb0, { 61,   0,  61,  56}, { 35,  53}}, //1 idle 2
	{BFWeeb_ArcMain_Weeb0, {123,   0,  59,  58}, { 33,  55}}, //2 idle 3
	{BFWeeb_ArcMain_Weeb0, {183,   0,  58,  59}, { 31,  56}}, //3 idle 4
	{BFWeeb_ArcMain_Weeb0, {  0,  56,  58,  58}, { 32,  55}}, //4 idle 5
	
	{BFWeeb_ArcMain_Weeb0, { 59,  57,  54,  57}, { 34,  53}}, //5 left 1
	{BFWeeb_ArcMain_Weeb0, {114,  59,  55,  57}, { 33,  53}}, //6 left 2
	
	{BFWeeb_ArcMain_Weeb0, {170,  60,  55,  52}, { 31,  48}}, //7 down 1
	{BFWeeb_ArcMain_Weeb0, {  0, 115,  54,  53}, { 31,  49}}, //8 down 2
	
	{BFWeeb_ArcMain_Weeb0, { 55, 116,  57,  64}, { 26,  60}}, //9 up 1
	{BFWeeb_ArcMain_Weeb0, {113, 117,  58,  63}, { 27,  59}}, //10 up 2
	
	{BFWeeb_ArcMain_Weeb0, {172, 113,  57,  56}, { 22,  52}}, //11 right 1
	{BFWeeb_ArcMain_Weeb0, {  0, 169,  55,  56}, { 22,  52}}, //12 right 2
	
	{BFWeeb_ArcMain_Weeb0, { 56, 181,  54,  57}, { 33,  53}}, //13 left miss 1
	{BFWeeb_ArcMain_Weeb0, {113, 181,  55,  57}, { 33,  53}}, //14 left miss 2
	
	{BFWeeb_ArcMain_Weeb0, {169, 170,  53,  56}, { 31,  52}}, //15 down miss 1
	{BFWeeb_ArcMain_Weeb1, {  0,   0,  54,  53}, { 31,  49}}, //16 down miss 2
	
	{BFWeeb_ArcMain_Weeb1, { 55,   0,  60,  61}, { 28,  57}}, //17 up miss 1
	{BFWeeb_ArcMain_Weeb1, {116,   0,  58,  63}, { 27,  59}}, //18 up miss 2
	
	{BFWeeb_ArcMain_Weeb1, {175,   0,  54,  56}, { 22,  52}}, //19 right miss 1
	{BFWeeb_ArcMain_Weeb1, {  0,  54,  55,  56}, { 22,  52}}, //20 right miss 2

	{BFWeeb_ArcMain_Weeb1, { 56,  62,  64,  60}, { 56 - 25,  60 - 3}}, //21 death weeb 1 1
	{BFWeeb_ArcMain_Weeb1, {120,  64,  73,  60}, { 57 - 25,  60 - 3}}, //22 death weeb 1 2
	{BFWeeb_ArcMain_Weeb1, {  0, 120,  68,  58}, { 57 - 25,  58 - 3}}, //23 death weeb 1 3
	{BFWeeb_ArcMain_Weeb1, { 69, 123,  73,  58}, { 61 - 25,  58 - 3}}, //24 death weeb 1 4
	{BFWeeb_ArcMain_Weeb1, {144, 125,  71,  58}, { 58 - 25,  58 - 3}}, //25 death weeb 1 5
	{BFWeeb_ArcMain_Weeb1, {  0, 179,  58,  58}, { 53 - 25,  58 - 3}}, //26 death weeb 1 6

	{BFWeeb_ArcMain_Weeb2, {  4,   1,  58,  58}, { 53 - 25,  58 - 3}}, //27 death weeb 2 1
	{BFWeeb_ArcMain_Weeb2, { 64,   1,  58,  58}, { 53 - 25,  58 - 3}}, //28 death weeb 2 2
	{BFWeeb_ArcMain_Weeb2, {126,   1,  58,  58}, { 53 - 25,  58 - 3}}, //29 death weeb 2 3
	{BFWeeb_ArcMain_Weeb2, {188,   0,  61,  64}, { 56 - 25,  58 - 3}}, //30 death weeb 2 4
	{BFWeeb_ArcMain_Weeb2, {  3,  65,  59,  60}, { 54 - 25,  58 - 3}}, //31 death weeb 2 5
	{BFWeeb_ArcMain_Weeb2, { 63,  65,  58,  63}, { 53 - 25,  58 - 3}}, //32 death weeb 2 6
	{BFWeeb_ArcMain_Weeb2, {122,  65,  58,  62}, { 53 - 25,  58 - 3}}, //33 death weeb 2 7
	{BFWeeb_ArcMain_Weeb2, {186,  65,  58,  62}, { 53 - 25,  58 - 3}}, //34 death weeb 2 8
	{BFWeeb_ArcMain_Weeb2, {  4, 129,  58,  62}, { 53 - 25,  58 - 3}}, //35 death weeb 2 9
	{BFWeeb_ArcMain_Weeb2, { 63, 129,  58,  62}, { 53 - 25,  58 - 3}}, //36 death weeb 2 10
	{BFWeeb_ArcMain_Weeb2, {122, 129,  58,  62}, { 53 - 25,  58 - 3}}, //37 death weeb 2 11
	{BFWeeb_ArcMain_Weeb2, {184, 129,  58,  62}, { 53 - 25,  58 - 3}}, //38 death weeb 2 12
	{BFWeeb_ArcMain_Weeb2, {  4, 193,  58,  62}, { 53 - 25,  58 - 3}}, //39 death weeb 2 13
	{BFWeeb_ArcMain_Weeb2, { 63, 193,  58,  62}, { 53 - 25,  58 - 3}}, //40 death weeb 2 14
	{BFWeeb_ArcMain_Weeb2, {122, 193,  58,  62}, { 53 - 25,  58 - 3}}, //41 death weeb 2 15
	{BFWeeb_ArcMain_Weeb2, {181, 193,  58,  62}, { 53 - 25,  58 - 3}}, //42 death weeb 2 16

	{BFWeeb_ArcMain_Weeb3, {  4,   1,  58,  62}, { 53 - 25,  58 - 3}}, //43 death weeb 2 1
	{BFWeeb_ArcMain_Weeb3, { 64,   1,  58,  62}, { 53 - 25,  58 - 3}}, //44 death weeb 2 2
	{BFWeeb_ArcMain_Weeb3, {126,   1,  58,  62}, { 53 - 25,  58 - 3}}, //45 death weeb 2 3
	{BFWeeb_ArcMain_Weeb3, {191,   1,  58,  62}, { 53 - 25,  58 - 3}}, //46 death weeb 2 4
	{BFWeeb_ArcMain_Weeb3, {  4,  65,  58,  62}, { 53 - 25,  58 - 3}}, //47 death weeb 2 5
	{BFWeeb_ArcMain_Weeb3, { 63,  65,  58,  62}, { 53 - 25,  58 - 3}}, //48 death weeb 2 6
	{BFWeeb_ArcMain_Weeb3, {122,  65,  58,  62}, { 53 - 25,  58 - 3}}, //49 death weeb 2 7
	{BFWeeb_ArcMain_Weeb3, {186,  65,  58,  62}, { 53 - 25,  58 - 3}}, //50 death weeb 2 8
	{BFWeeb_ArcMain_Weeb3, {  4, 129,  58,  62}, { 53 - 25,  58 - 3}}, //51 death weeb 2 9
	{BFWeeb_ArcMain_Weeb3, { 63, 129,  58,  62}, { 53 - 25,  58 - 3}}, //52 death weeb 2 10
	{BFWeeb_ArcMain_Weeb3, {122, 129,  58,  62}, { 53 - 25,  58 - 3}}, //53 death weeb 2 11
	{BFWeeb_ArcMain_Weeb3, {184, 129,  58,  62}, { 53 - 25,  58 - 3}}, //54 death weeb 2 12
	{BFWeeb_ArcMain_Weeb3, {  4, 193,  58,  62}, { 53 - 25,  58 - 3}}, //55 death weeb 2 13
	{BFWeeb_ArcMain_Weeb3, { 63, 193,  58,  62}, { 53 - 25,  58 - 3}}, //56 death weeb 2 14
	{BFWeeb_ArcMain_Weeb3, {122, 193,  58,  62}, { 53 - 25,  58 - 3}}, //57 death weeb 2 15
	{BFWeeb_ArcMain_Weeb3, {181, 193,  58,  62}, { 53 - 25,  58 - 3}}, //58 death weeb 2 16

	{BFWeeb_ArcMain_Weeb4, {  5,   2,  58,  62}, { 53 - 25,  58 - 3}}, //59 death weeb 2 1
	{BFWeeb_ArcMain_Weeb4, { 65,   2,  58,  62}, { 53 - 25,  58 - 3}}, //60 death weeb 2 2
	{BFWeeb_ArcMain_Weeb4, {127,   2,  58,  62}, { 53 - 25,  58 - 3}}, //61 death weeb 2 3
	{BFWeeb_ArcMain_Weeb4, {192,   2,  58,  62}, { 53 - 25,  58 - 3}}, //62 death weeb 2 4
	{BFWeeb_ArcMain_Weeb4, {  5,  66,  58,  62}, { 53 - 25,  58 - 3}}, //63 death weeb 2 5
	{BFWeeb_ArcMain_Weeb4, { 64,  66,  58,  62}, { 53 - 25,  58 - 3}}, //64 death weeb 2 6
	{BFWeeb_ArcMain_Weeb4, {123,  66,  58,  62}, { 53 - 25,  58 - 3}}, //65 death weeb 2 7

	{BFWeeb_ArcMain_Weeb5, {  2,   6,  58,  62}, { 53 - 25,  58 - 3}}, //66 death weeb 2 1
	{BFWeeb_ArcMain_Weeb5, { 65,   6,  58,  62}, { 53 - 25,  58 - 3}}, //67 death weeb 2 2
	{BFWeeb_ArcMain_Weeb5, {131,   6,  58,  62}, { 53 - 25,  58 - 3}}, //68 death weeb 2 3
	{BFWeeb_ArcMain_Weeb5, {197,   6,  58,  62}, { 53 - 25,  58 - 3}}, //69 death weeb 2 4
	{BFWeeb_ArcMain_Weeb5, {  2,  73,  58,  62}, { 53 - 25,  58 - 3}}, //70 death weeb 2 5
	{BFWeeb_ArcMain_Weeb5, { 63,  73,  58,  62}, { 53 - 25,  58 - 3}}, //71 death weeb 2 6
	{BFWeeb_ArcMain_Weeb5, {125,  73,  58,  62}, { 53 - 25,  58 - 3}}, //72 death weeb 2 7
	{BFWeeb_ArcMain_Weeb5, {195,  73,  58,  62}, { 53 - 25,  58 - 3}}, //73 death weeb 2 8
};

static const Animation char_bfweeb_anim[PlayerAnim_Max] = {
	{2, (const u8[]){ 0,  1,  2,  3, 4, ASCR_BACK, 1}}, //CharAnim_Idle
	{2, (const u8[]){ 5,  6, ASCR_BACK, 1}},            //CharAnim_Left
	{0, (const u8[]){ASCR_CHGANI, CharAnim_Idle}},      //CharAnim_LeftAlt
	{2, (const u8[]){ 7,  8, ASCR_BACK, 1}},            //CharAnim_Down
	{0, (const u8[]){ASCR_CHGANI, CharAnim_Idle}},      //CharAnim_DownAlt
	{2, (const u8[]){ 9, 10, ASCR_BACK, 1}},            //CharAnim_Up
	{0, (const u8[]){ASCR_CHGANI, CharAnim_Idle}},      //CharAnim_UpAlt
	{2, (const u8[]){11, 12, ASCR_BACK, 1}},            //CharAnim_Right
	{0, (const u8[]){ASCR_CHGANI, CharAnim_Idle}},      //CharAnim_RightAlt
	
	{2, (const u8[]){13, 14, ASCR_BACK, 1}},            //CharAnim_Left
	{2, (const u8[]){15, 16, ASCR_BACK, 1}},            //CharAnim_Down
	{2, (const u8[]){17, 18, ASCR_BACK, 1}},            //CharAnim_Up
	{2, (const u8[]){19, 20, ASCR_BACK, 1}},            //CharAnim_Right
	
	{2, (const u8[]){20, 21, 22, ASCR_BACK, 1}},        //PlayerAnim_Peace
	{2, (const u8[]){ASCR_CHGANI, CharAnim_Idle}},      //PlayerAnim_Sweat
	
	{5, (const u8[]){21, 21, 22, 22, 23, 23, 24, 25, 26, ASCR_CHGANI, PlayerAnim_Dead1}}, //CharAnim_Idle
	{1, (const u8[]){26, 26, ASCR_BACK, 1}}, //CharAnim_Idle
	{2, (const u8[]){27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58,  ASCR_CHGANI, PlayerAnim_Dead3}}, //CharAnim_Idle
	{2, (const u8[]){59, 60, 61, 62, 63, 64, 65, ASCR_BACK, 1}}, //CharAnim_Idle
	{2, (const u8[]){59, 60, 61, 62, 63, 64, 65, ASCR_CHGANI, PlayerAnim_Dead3}},             //PlayerAnim_Dead4
	{2, (const u8[]){59, 60, 61, 62, 63, 64, 65, ASCR_CHGANI, PlayerAnim_Dead3}}, 

	{2, (const u8[]){66, 67, 68, 69, 70, 71, 71, 71, 71, 72, 72, 72, 72, 73, 73, 73, 58, 58, 58, 58, 58, 58, ASCR_CHGANI, PlayerAnim_Dead7}},
	{3, (const u8[]){58, 58, 58, 58, 58, 58, 58, 58, 58, 58, 58, 58, ASCR_BACK, 1}},
};

//swap left and right for test song
static const Animation char_bfweebreverse_anim[PlayerAnim_Max] = {
	{2, (const u8[]){ 0,  1,  2,  3, 4, ASCR_BACK, 1}}, //CharAnim_Idle
	{2, (const u8[]){11, 12, ASCR_BACK, 1}},            //CharAnim_Left
	{0, (const u8[]){ASCR_CHGANI, CharAnim_Idle}},      //CharAnim_LeftAlt
	{2, (const u8[]){ 7,  8, ASCR_BACK, 1}},            //CharAnim_Down
	{0, (const u8[]){ASCR_CHGANI, CharAnim_Idle}},      //CharAnim_DownAlt
	{2, (const u8[]){ 9, 10, ASCR_BACK, 1}},            //CharAnim_Up
	{0, (const u8[]){ASCR_CHGANI, CharAnim_Idle}},      //CharAnim_UpAlt
	{2, (const u8[]){ 5,  6, ASCR_BACK, 1}},            //CharAnim_Right
	{0, (const u8[]){ASCR_CHGANI, CharAnim_Idle}},      //CharAnim_RightAlt
	
	{2, (const u8[]){19, 20, ASCR_BACK, 1}},            //CharAnim_Left
	{2, (const u8[]){15, 16, ASCR_BACK, 1}},            //CharAnim_Down
	{2, (const u8[]){17, 18, ASCR_BACK, 1}},            //CharAnim_Up
	{2, (const u8[]){13, 14, ASCR_BACK, 1}},            //CharAnim_Right
	
	{2, (const u8[]){20, 21, 22, ASCR_BACK, 1}},        //PlayerAnim_Peace
	{2, (const u8[]){ASCR_CHGANI, CharAnim_Idle}},      //PlayerAnim_Sweat
	
	{5, (const u8[]){21, 21, 22, 22, 23, 23, 24, 25, 26, ASCR_CHGANI, PlayerAnim_Dead1}}, //CharAnim_Idle
	{1, (const u8[]){26, 26, ASCR_BACK, 1}}, //CharAnim_Idle
	{2, (const u8[]){27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58,  ASCR_CHGANI, PlayerAnim_Dead3}}, //CharAnim_Idle
	{2, (const u8[]){59, 60, 61, 62, 63, 64, 65, ASCR_BACK, 1}}, //CharAnim_Idle
	{2, (const u8[]){59, 60, 61, 62, 63, 64, 65, ASCR_CHGANI, PlayerAnim_Dead3}},             //PlayerAnim_Dead4
	{2, (const u8[]){59, 60, 61, 62, 63, 64, 65, ASCR_CHGANI, PlayerAnim_Dead3}}, 

	{2, (const u8[]){66, 67, 68, 69, 70, 71, 71, 71, 71, 72, 72, 72, 72, 73, 73, 73, 58, 58, 58, 58, 58, 58, ASCR_CHGANI, PlayerAnim_Dead7}},
	{3, (const u8[]){58, 58, 58, 58, 58, 58, 58, 58, 58, 58, 58, 58, ASCR_BACK, 1}},
};

static void BFWeeb_ReverseDraw(Character *this, Gfx_Tex *tex, const CharFrame *cframe)
{
	//Draw character
	fixed_t x = this->x - FIXED_MUL(stage.camera.x, FIXED_UNIT) - FIXED_DEC(-cframe->off[0]*2,1);
	fixed_t y = this->y - FIXED_MUL(stage.camera.y, FIXED_UNIT) - FIXED_DEC(cframe->off[1]*2,1);
	
	RECT src = {cframe->src[0], cframe->src[1], cframe->src[2], cframe->src[3]};
	RECT_FIXED dst = {x, y, (-src.w*2+12) << FIXED_SHIFT, (src.h*2-12) << FIXED_SHIFT};
	Stage_DrawTex(tex, &src, &dst, stage.camera.bzoom);
}

//Boyfriend Weeb player functions
static void Char_BFWeeb_SetFrame(void *user, u8 frame)
{
	Char_BFWeeb *this = (Char_BFWeeb*)user;
	
	//Check if this is a new frame
	if (frame != this->frame)
	{
		//Check if new art shall be loaded
		const CharFrame *cframe = &char_bfweeb_frame[this->frame = frame];
		if (cframe->tex != this->tex_id)
			Gfx_LoadTex(&this->tex, this->arc_ptr[this->tex_id = cframe->tex], 0);
	}
}

static void Char_BFWeeb_Tick(Character *character)
{
	Char_BFWeeb *this = (Char_BFWeeb*)character;
	
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
	}
	
	//Animate and draw character
	Animatable_Animate(&character->animatable, (void*)this, Char_BFWeeb_SetFrame);
	if (stage.stage_id == StageId_4_4)
	BFWeeb_ReverseDraw(character, &this->tex, &char_bfweeb_frame[this->frame]);
	else
	Character_Draw(character, &this->tex, &char_bfweeb_frame[this->frame]);
}

static void Char_BFWeeb_SetAnim(Character *character, u8 anim)
{
	
	//Perform animation checks
	switch (anim)
	{
		case PlayerAnim_Dead0:
			//Begin adjust focus
			character->focus_x = FIXED_DEC(0,1);
			break;
	}
	
	//Set animation
	Animatable_SetAnim(&character->animatable, anim);
	Character_CheckStartSing(character);
}

static void Char_BFWeeb_Free(Character *character)
{
	(void)character;
}

static Character *Char_BFWeeb_New(fixed_t x, fixed_t y)
{
	//Allocate boyfriend object
	Char_BFWeeb *this = Mem_Alloc(sizeof(Char_BFWeeb));
	if (this == NULL)
	{
		sprintf(error_msg, "[Char_BFWeeb_New] Failed to allocate boyfriend weeb object");
		ErrorLock();
		return NULL;
	}
	
	//Initialize character
	this->character.tick = Char_BFWeeb_Tick;
	this->character.set_anim = Char_BFWeeb_SetAnim;
	this->character.free = Char_BFWeeb_Free;
	
	if (stage.stage_id == StageId_4_4)
	Animatable_Init(&this->character.animatable, char_bfweebreverse_anim);
	else
	Animatable_Init(&this->character.animatable, char_bfweeb_anim);

	Character_Init((Character*)this, x, y);
	//Set character information
	this->character.spec = CHAR_SPEC_MISSANIM;
	
	this->character.health_b = 0xFF7cd6f5;
	this->character.health_i = 0;
	
	//use different x,y and zoom for test song
	this->character.focus_x = (stage.stage_id == StageId_4_4) ? FIXED_DEC(70,1) : FIXED_DEC(-34,1);
	this->character.focus_y = (stage.stage_id == StageId_4_4) ? FIXED_DEC(-75,1) : FIXED_DEC(-40,1);
	this->character.focus_zoom = (stage.stage_id == StageId_4_4) ? FIXED_DEC(1,1) : FIXED_DEC(2,1);
	
	//Load art
	const char **pathp = (const char *[]){
		"weeb0.tim",   //BF_ArcMain_BF0
		"weeb1.tim",   //BF_ArcMain_BF1
		"weeb2.tim",   //BF_ArcMain_BF0
		"weeb3.tim",   //BF_ArcMain_BF1
		"weeb4.tim",   //BF_ArcMain_BF0
		"weeb5.tim",   //BF_ArcMain_BF0
		NULL
	};
	IO_Data *arc_ptr = this->arc_ptr;
	for (; *pathp != NULL; pathp++)
		*arc_ptr++ = Archive_Find((IO_Data)char_bfweeb_arc_main, *pathp);
	
	//Initialize render state
	this->tex_id = this->frame = 0xFF;
	
	return (Character*)this;
}
