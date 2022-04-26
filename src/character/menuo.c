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
	MenuO_ArcMain_Xmasp0,
	MenuO_ArcMain_Xmasp1,
	MenuO_ArcMain_Xmasp2,
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
	{MenuO_ArcMain_Dad, {  0,   1,  68, 121}, { 26 + 30, 115 + 68}}, //0 idle 1
	{MenuO_ArcMain_Dad, { 69,   0,  68, 120}, { 27 + 30, 115 + 68}}, //1 idle 2
	{MenuO_ArcMain_Dad, {138,   1,  68, 121}, { 27 + 30, 115 + 68}}, //2 idle 3
	{MenuO_ArcMain_Dad, {  0, 123,  66, 122}, { 26 + 30, 116 + 68}}, //3 idle 4
	{MenuO_ArcMain_Dad, { 71, 122,  66, 123}, { 26 + 30, 117 + 68}}, //4 idle 5
	{MenuO_ArcMain_Dad, {138, 122,  67, 123}, { 26 + 30, 117 + 68}}, //5 idle 6

	{MenuO_ArcMain_Spook, {  0,   0,  58,  84}, { 14 + 45, 83 + 65}}, //6 idle 1
	{MenuO_ArcMain_Spook, { 60,   0,  58,  83}, { 14 + 45, 82 + 65}}, //7 idle 2
	{MenuO_ArcMain_Spook, {120,   0,  54,  75}, { 11 + 45, 74 + 65}}, //8 idle 3
	{MenuO_ArcMain_Spook, {176,   0,  54,  76}, { 12 + 45, 75 + 65}}, //9 idle 4
	{MenuO_ArcMain_Spook, {  0,  87,  76,  82}, { 22 + 45, 80 + 65}}, //10 idle 5
	{MenuO_ArcMain_Spook, { 78,  86,  73,  80}, { 20 + 45, 79 + 65}}, //11 idle 6

	{MenuO_ArcMain_Pico, {  0,   0,  67,  72}, { 63, 66 + 72}}, //12 idle 1
	{MenuO_ArcMain_Pico, { 67,   0,  68,  73}, { 63, 67 + 72}}, //13 idle 2
	{MenuO_ArcMain_Pico, {135,   0,  69,  73}, { 62, 67 + 72}}, //14 idle 3
	{MenuO_ArcMain_Pico, {  0,  72,  68,  73}, { 61, 67 + 72}}, //15 idle 4
	{MenuO_ArcMain_Pico, { 68,  73,  68,  74}, { 61, 67 + 72}}, //16 idle 5
	{MenuO_ArcMain_Pico, {136,  73,  68,  74}, { 61, 67 + 72}}, //17 idle 6

	{MenuO_ArcMain_Mom, {  0,   0,  58, 125}, { 21, 116}}, //18 idle 1
	{MenuO_ArcMain_Mom, { 58,   0,  58, 123}, { 20, 115}}, //19 idle 2
	{MenuO_ArcMain_Mom, {116,   0,  58, 124}, { 22, 115}}, //20 idle 3
	{MenuO_ArcMain_Mom, {174,   0,  58, 125}, { 21, 116}}, //21 idle 4
	{MenuO_ArcMain_Mom, {  0, 126,  58, 125}, { 21, 116}}, //22 idle 5
	{MenuO_ArcMain_Mom, { 58, 125,  58, 126}, { 22, 117}}, //23 idle 6
	{MenuO_ArcMain_Mom, {116, 125,  58, 125}, { 22, 117}}, //24 idle 7

	{MenuO_ArcMain_Xmasp0, {  0,    0, 140, 125}, { 14 + 45, 83 + 65}}, //25 idle 1
	{MenuO_ArcMain_Xmasp0, {  0,  125, 140, 123}, { 14 + 45, 82 + 65}}, //26 idle 2
	{MenuO_ArcMain_Xmasp1, {  0,    0, 140, 124}, { 11 + 45, 74 + 65}}, //27 idle 3
	{MenuO_ArcMain_Xmasp1, {  0,  125, 140, 125}, { 12 + 45, 75 + 65}}, //28 idle 4
	{MenuO_ArcMain_Xmasp2, {  0,    0, 140, 125}, { 22 + 45, 80 + 65}}, //29 idle 5
	{MenuO_ArcMain_Xmasp2, {  0,  125, 148, 126}, { 20 + 45, 79 + 65}}, //30 idle 6
};

static const Animation char_menuo_anim[CharAnim_Max] = {
	{1, (const u8[]){ 0, 0, 1,  1,  2, 2, 3, 3,  4, 4, 5, 5, 5, 5, ASCR_CHGANI, CharAnim_Idle}}, //CharAnim_Idle
	{2, (const u8[]){ 6,  7, 7,  8, 8, 9, 9, 10, 11, 11, 8, 8, 9, 9, ASCR_CHGANI, CharAnim_Left}},         //CharAnim_Left
	{2, (const u8[]){12, 13, 14, 15, 16,  17, 17, ASCR_CHGANI, CharAnim_LeftAlt}},   //CharAnim_LeftAlt
	{2, (const u8[]){18, 19, 20, 21, 22, 23, 24, 24, ASCR_CHGANI, CharAnim_Down}},         //CharAnim_Down
	{2, (const u8[]){25, 26, 27, 28, 29, 30, 30, ASCR_CHGANI, CharAnim_DownAlt}},   //CharAnim_DownAlt
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
		"dad.tim", //MenuO_ArcMain_Dad
		"spook.tim", //MenuO_ArcMain_Spook
		"pico.tim", //MenuO_ArcMain_Pico
		"mom.tim", //MenuO_ArcMain_Mom
		"xmasp0.tim", //MenuO_ArcMain_Xmasp0
		"xmasp1.tim", //MenuO_ArcMain_Xmasp1
		"xmasp2.tim", //MenuO_ArcMain_Xmasp2
		NULL
	};
	IO_Data *arc_ptr = this->arc_ptr;
	for (; *pathp != NULL; pathp++)
		*arc_ptr++ = Archive_Find((IO_Data)char_menuo_arc_main, *pathp);
	
	//Initialize render state
	this->tex_id = this->frame = 0xFF;
	
	return (Character*)this;
}
