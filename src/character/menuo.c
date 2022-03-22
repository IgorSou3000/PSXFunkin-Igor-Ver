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
	{MenuO_ArcMain_Dad, {  2,   1,  68, 121}, { 42, 183+3}}, //0 idle 1
	{MenuO_ArcMain_Dad, { 70,   2,  68, 120}, { 43, 183+2}}, //1 idle 2
	{MenuO_ArcMain_Dad, {138,   1,  68, 121}, { 43, 183+3}}, //2 idle 3
	{MenuO_ArcMain_Dad, {  2, 123,  67, 122}, { 42, 183+4}}, //3 idle 4
	{MenuO_ArcMain_Dad, { 74, 123,  66, 123}, { 42, 183+4}}, //4 idle 5
	{MenuO_ArcMain_Dad, {141, 123,  67, 123}, { 42, 183+4}}, //5 idle 6
};

static const Animation char_menuo_anim[CharAnim_Max] = {
	{2, (const u8[]){ 0,  1,  2,  3,  4,  5, ASCR_BACK, 1}}, //CharAnim_Idle
	{2, (const u8[]){ 4,  5, ASCR_BACK, 1}},         //CharAnim_Left
	{0, (const u8[]){ASCR_CHGANI, CharAnim_Idle}},   //CharAnim_LeftAlt
	{2, (const u8[]){ 6,  7, ASCR_BACK, 1}},         //CharAnim_Down
	{0, (const u8[]){ASCR_CHGANI, CharAnim_Idle}},   //CharAnim_DownAlt
	{2, (const u8[]){ 8,  9, ASCR_BACK, 1}},         //CharAnim_Up
	{0, (const u8[]){ASCR_CHGANI, CharAnim_Idle}},   //CharAnim_UpAlt
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
		"dad.tim", //MenuO_ArcMain_Idle0
		NULL
	};
	IO_Data *arc_ptr = this->arc_ptr;
	for (; *pathp != NULL; pathp++)
		*arc_ptr++ = Archive_Find((IO_Data)char_menuo_arc_main, *pathp);
	
	//Initialize render state
	this->tex_id = this->frame = 0xFF;
	
	return (Character*)this;
}
