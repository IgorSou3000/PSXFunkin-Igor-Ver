/*
  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "boot/character.h"
#include "boot/mem.h"
#include "boot/archive.h"
#include "boot/stage.h"
#include "boot/main.h"

//MenuO character assets
static u8 char_menuo_arc_main[] = {
	#include "iso/menuo/main.arc.h"
};

//MenuO character structure
enum
{
	MenuO_ArcMain_Dad,
	MenuO_ArcMain_Spook,
	MenuO_ArcMain_Pico,
	MenuO_ArcMain_Mom,
	MenuO_ArcMain_Xmasp,
	MenuO_ArcMain_Senpai,
	
	MenuO_Arc_Max,
};

typedef struct
{
	//Character base structure
	Character character;
	
	//Render data and state
	IO_Data arc_main;
	IO_Data arc_ptr[MenuO_Arc_Max];
	
	Gfx_Tex tex;
	u8 frame, tex_id;
} Char_MenuO;

//MenuO character definitions
static const CharFrame char_menuo_frame[] = {
	{MenuO_ArcMain_Dad, {  0,   1,  53,  91}, { 38, 90+96}}, //0 idle 1
	{MenuO_ArcMain_Dad, { 53,   1,  51,  91}, { 37, 90+96}}, //1 idle 2
	{MenuO_ArcMain_Dad, {104,   1,  51,  91}, { 37, 90+96}}, //2 idle 3
	{MenuO_ArcMain_Dad, {155,   1,  50,  92}, { 37, 91+96}}, //3 idle 4
	{MenuO_ArcMain_Dad, {  1,  92,  51,  93}, { 37, 92+96}}, //4 idle 5
	{MenuO_ArcMain_Dad, { 52,  92,  50,  93}, { 36, 92+96}}, //5 idle 6

	{MenuO_ArcMain_Spook, {  0,   0,  44,  64}, { 44, 63+96}}, //6 idle 1
	{MenuO_ArcMain_Spook, { 44,   0,  45,  63}, { 45, 62+96}}, //7 idle 2
	{MenuO_ArcMain_Spook, { 89,   0,  42,  56}, { 42, 56+96}}, //8 idle 3
	{MenuO_ArcMain_Spook, {131,   0,  42,  58}, { 42, 57+96}}, //9 idle 4
	{MenuO_ArcMain_Spook, {173,   0,  58,  62}, { 52, 60+96}}, //10 idle 5
	{MenuO_ArcMain_Spook, {  0,  64,  56,  61}, { 50, 61+96}}, //11 idle 6

	{MenuO_ArcMain_Pico, {  0,   1,  52,  55}, { 49, 51+96}}, //12 idle 1
	{MenuO_ArcMain_Pico, { 52,   1,  52,  55}, { 48, 51+96}}, //13 idle 2
	{MenuO_ArcMain_Pico, {104,   1,  53,  55}, { 48, 51+96}}, //14 idle 3
	{MenuO_ArcMain_Pico, {157,   1,  53,  56}, { 47, 51+96}}, //15 idle 4
	{MenuO_ArcMain_Pico, {  0,  56,  53,  56}, { 47, 52+96}}, //16 idle 5
	{MenuO_ArcMain_Pico, { 53,  56,  53,  56}, { 47, 52+96}}, //17 idle 6

	{MenuO_ArcMain_Mom, {  0,   1,  45,  94}, { 33, 88+96}}, //18 idle 1
	{MenuO_ArcMain_Mom, { 44,   1,  45,  94}, { 33, 87+96}}, //19 idle 2
	{MenuO_ArcMain_Mom, { 88,   1,  45,  94}, { 35, 88+96}}, //20 idle 3
	{MenuO_ArcMain_Mom, {133,   1,  44,  94}, { 34, 88+96}}, //21 idle 4
	{MenuO_ArcMain_Mom, {178,   0,  44,  96}, { 33, 89+96}}, //22 idle 5

	{MenuO_ArcMain_Xmasp, {  0,    0, 105,  93}, { 90, 87+96}}, //23 idle 1
	{MenuO_ArcMain_Xmasp, {105,    0, 106,  92}, { 91, 85+96}}, //24 idle 2
	{MenuO_ArcMain_Xmasp, {  0,  102, 106,  92}, { 90, 86+96}}, //25 idle 3
	{MenuO_ArcMain_Xmasp, {106,  102, 105,  94}, { 88, 87+96}}, //26 idle 4
};

static const Animation char_menuo_anim[CharAnim_Max] = {
	{1, (const u8[]){ 0, 0, 1,  1,  2, 2, 3, 3,  4, 4, 5, 5, 5, 5, ASCR_CHGANI, CharAnim_Idle}}, //Dad
	{2, (const u8[]){ 6,  7, 7,  8, 8, 9, 9, 10, 11, 11, 8, 8, 9, 9, ASCR_CHGANI, CharAnim_Left}},         //Spook
	{2, (const u8[]){12, 13, 14, 15, 16,  17, 17, 17, 17, 17, 17, ASCR_CHGANI, CharAnim_LeftAlt}},   //Pico
	{2, (const u8[]){18, 19, 20, 21, 22, 22, 22, 22, 22, 22, ASCR_CHGANI, CharAnim_Down}},         //Mom
	{2, (const u8[]){23, 24, 25, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, ASCR_CHGANI, CharAnim_DownAlt}},   //Xmasp
	{2, (const u8[]){ 8,  9, ASCR_BACK, 1}},         //Senpai
	{0, (const u8[]){ASCR_CHGANI, CharAnim_Idle}},   //Tankman
	{2, (const u8[]){10, 11, ASCR_BACK, 1}},         //CharAnim_Right
	{0, (const u8[]){ASCR_CHGANI, CharAnim_Idle}},   //CharAnim_RightAlt
};

//MenuO character functions
static void Char_MenuO_SetFrame(void *user, u8 frame)
{
	Char_MenuO *this = (Char_MenuO*)user;
	
	//Check if this is a new frame
	if (frame != this->frame)
	{
		//Check if new art shall be loaded
		const CharFrame *cframe = &char_menuo_frame[this->frame = frame];
		if (cframe->tex != this->tex_id)
			Gfx_LoadTex(&this->tex, this->arc_ptr[this->tex_id = cframe->tex], 0);
	}
}

static void Char_MenuO_Tick(Character *character)
{
	Char_MenuO *this = (Char_MenuO*)character;
	
	//Perform idle dance
	if ((character->pad_held & (INPUT_LEFT | INPUT_DOWN | INPUT_UP | INPUT_RIGHT)) == 0)
		Character_PerformIdle(character);
	
	//Animate and draw
	Animatable_Animate(&character->animatable, (void*)this, Char_MenuO_SetFrame);
	Character_Draw(character, &this->tex, &char_menuo_frame[this->frame]);
}

static void Char_MenuO_SetAnim(Character *character, u8 anim)
{
	//Set animation
	Animatable_SetAnim(&character->animatable, anim);
	Character_CheckStartSing(character);
}

static void Char_MenuO_Free(Character *character)
{
	(void)character;
}

static Character *Char_MenuO_New(fixed_t x, fixed_t y)
{
	//Allocate menuo object
	Char_MenuO *this = Mem_Alloc(sizeof(Char_MenuO));
	if (this == NULL)
	{
		sprintf(error_msg, "[Char_MenuO_New] Failed to allocate menuo object");
		ErrorLock();
		return NULL;
	}
	
	//Initialize character
	this->character.tick = Char_MenuO_Tick;
	this->character.set_anim = Char_MenuO_SetAnim;
	this->character.free = Char_MenuO_Free;
	
	Animatable_Init(&this->character.animatable, char_menuo_anim);
	Character_Init((Character*)this, x, y);
	
	//Set character information
	this->character.spec = 0;
	
	this->character.health_b = 0xFFbd70e2;
	this->character.health_i = 1;
	
	this->character.focus_x = FIXED_DEC(65,1);
	this->character.focus_y = FIXED_DEC(-115,1);
	this->character.focus_zoom = FIXED_DEC(1,1);
	
	//Load art
	const char **pathp = (const char *[]){
		"dad.tim", //MenuO_ArcMain_Dad
		"spook.tim", //MenuO_ArcMain_Spook
		"pico.tim", //MenuO_ArcMain_Pico
		"mom.tim", //MenuO_ArcMain_Mom
		"xmasp.tim", //MenuO_ArcMain_Xmasp
		NULL
	};
	IO_Data *arc_ptr = this->arc_ptr;
	for (; *pathp != NULL; pathp++)
		*arc_ptr++ = Archive_Find((IO_Data)char_menuo_arc_main, *pathp);
	
	//Initialize render state
	this->tex_id = this->frame = 0xFF;
	
	return (Character*)this;
}
