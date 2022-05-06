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

//MenuGF assets
static u8 char_menugf_arc_main[] = {
	#include "iso/menugf/main.arc.h"
};

//MenuGF character structure
enum
{
	MenuGF_ArcMain_MenuGF0,
	MenuGF_ArcMain_MenuGF1,
	
	MenuGF_Arc_Max,
};

typedef struct
{
	//Character base structure
	Character character;
	
	//Render data and state
	IO_Data arc_ptr[MenuGF_Arc_Max];
	
	Gfx_Tex tex;
	u8 frame, tex_id;
	
	//Pico test
	u16 *pico_p;
} Char_MenuGF;

//MenuGF character definitions
static const CharFrame char_menugf_frame[] = {
	{MenuGF_ArcMain_MenuGF0, {  1,   0,  86,  78}, { 37,  72}}, //0 bop left 1
	{MenuGF_ArcMain_MenuGF0, { 87,   0,  86,  78}, { 37,  72}}, //1 bop left 2
	{MenuGF_ArcMain_MenuGF0, {  1,  82,  86,  78}, { 37,  72}}, //2 bop left 3
	{MenuGF_ArcMain_MenuGF0, { 87,  82,  86,  78}, { 37,  72}}, //3 bop left 4
	{MenuGF_ArcMain_MenuGF0, {  0, 164,  86,  79}, { 37,  73}}, //4 bop left 5
	{MenuGF_ArcMain_MenuGF0, { 86, 164,  86,  79}, { 37,  73}}, //5 bop left 6
	
	{MenuGF_ArcMain_MenuGF1, {  3,   1,  85,  77}, { 36,  71}}, //6 bop right 1
	{MenuGF_ArcMain_MenuGF1, { 88,   1,  86,  77}, { 37,  71}}, //7 bop right 2
	{MenuGF_ArcMain_MenuGF1, {  2,  80,  86,  77}, { 37,  71}}, //8 bop right 3
	{MenuGF_ArcMain_MenuGF1, { 88,  80,  86,  77}, { 37,  71}}, //9 bop right 4
	{MenuGF_ArcMain_MenuGF1, {  2, 159,  86,  79}, { 37,  73}}, //10 bop right 5
	{MenuGF_ArcMain_MenuGF1, { 88, 160,  86,  78}, { 37,  72}}, //11 bop right 6
	
};

static const Animation char_menugf_anim[CharAnim_Max] = {
	{0, (const u8[]){ASCR_CHGANI, CharAnim_LeftAlt}},                        //CharAnim_Idle
	{0, (const u8[]){ASCR_CHGANI, CharAnim_LeftAlt}},                                   //CharAnim_Left
	{1, (const u8[]){ 0,  0,  1,  1,  2,  2,  3,  3,  4,  4,  5, ASCR_BACK, 1}}, //CharAnim_LeftAlt
	{0, (const u8[]){ASCR_CHGANI, CharAnim_LeftAlt}},                                    //CharAnim_Down
	{0, (const u8[]){ASCR_CHGANI, CharAnim_LeftAlt}},                          //CharAnim_DownAlt
	{0, (const u8[]){ASCR_CHGANI, CharAnim_LeftAlt}},                                     //CharAnim_Up
	{0, (const u8[]){ASCR_CHGANI, CharAnim_LeftAlt}},                                     //CharAnim_UpAlt
	{0, (const u8[]){ASCR_CHGANI, CharAnim_LeftAlt}},                                     //CharAnim_Right
	{1, (const u8[]){6,  6,  7,  7,  8,  8,  9,  9, 10, 10, 11,  ASCR_BACK, 1}}, //CharAnim_RightAlt
};

//MenuGF character functions
static void Char_MenuGF_SetFrame(void *user, u8 frame)
{
	Char_MenuGF *this = (Char_MenuGF*)user;
	
	//Check if this is a new frame
	if (frame != this->frame)
	{
		//Check if new art shall be loaded
		const CharFrame *cframe = &char_menugf_frame[this->frame = frame];
		if (cframe->tex != this->tex_id)
			Gfx_LoadTex(&this->tex, this->arc_ptr[this->tex_id = cframe->tex], 0);
	}
}

static fixed_t Char_MenuGF_GetParallax(Char_MenuGF *this);

static void Char_MenuGF_Tick(Character *character)
{
	Char_MenuGF *this = (Char_MenuGF*)character;
	
		//Dance to the beat
		if (stage.note_scroll >= character->sing_end)
		{
			if ((stage.flag & STAGE_FLAG_JUST_STEP) && (stage.song_step % stage.gf_speed) == 0)
			{
				//Switch animation
				if (character->animatable.anim == CharAnim_LeftAlt)
					character->set_anim(character, CharAnim_RightAlt);
				else
					character->set_anim(character, CharAnim_LeftAlt);
				
			}
		}
	
	//Animate and draw
	fixed_t parallax = Char_MenuGF_GetParallax(this);
	Animatable_Animate(&character->animatable, (void*)this, Char_MenuGF_SetFrame);
	Character_DrawParallax(character, &this->tex, &char_menugf_frame[this->frame], parallax);
}

static void Char_MenuGF_SetAnim(Character *character, u8 anim)
{
	//Set animation
	Animatable_SetAnim(&character->animatable, anim);
}

static void Char_MenuGF_Free(Character *character)
{
	(void)character;
}

static Character *Char_MenuGF_New(fixed_t x, fixed_t y)
{
	//Allocate menugf object
	Char_MenuGF *this = Mem_Alloc(sizeof(Char_MenuGF));
	if (this == NULL)
	{
		sprintf(error_msg, "[Char_MenuGF_New] Failed to allocate menugf object");
		ErrorLock();
		return NULL;
	}
	
	//Initialize character
	this->character.tick = Char_MenuGF_Tick;
	this->character.set_anim = Char_MenuGF_SetAnim;
	this->character.free = Char_MenuGF_Free;
	
	Animatable_Init(&this->character.animatable, char_menugf_anim);
	Character_Init((Character*)this, x, y);
	
	//Set character information
	this->character.spec = 0;
	
	this->character.health_i = 2;
	this->character.health_b = 0xFFA5014B;
	
	this->character.focus_x = FIXED_DEC(2,1);
	this->character.focus_y = FIXED_DEC(-40,1);
	this->character.focus_zoom = FIXED_DEC(2,1);
	
	//Load art
	const char **pathp = (const char *[]){
		"gf0.tim", //MenuGF_ArcMain_MenuGF0
		"gf1.tim", //MenuGF_ArcMain_MenuGF1
		NULL
	};
	IO_Data *arc_ptr = this->arc_ptr;
	for (; *pathp != NULL; pathp++)
		*arc_ptr++ = Archive_Find((IO_Data)char_menugf_arc_main, *pathp);
	
	//Initialize render state
	this->tex_id = this->frame = 0xFF;
	
	return (Character*)this;
}
