/*
  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

//menu bf sprites and offsets by bilious

#include "boot/character.h"
#include "boot/mem.h"
#include "boot/archive.h"
#include "boot/stage.h"
#include "boot/random.h"
#include "boot/main.h"

//Menu BF player assets
static u8 char_menubf_arc_main[] = {
	#include "iso/menup/main.arc.h"
};

//Menu BF player types
enum
{
	BFMenu_ArcMain_BF,
	
	BF_ArcMain_Max,
};

typedef struct
{
	//Character base structure
	Character character;
	
	//Render data and state
	IO_Data arc_ptr[BF_ArcMain_Max];
	
	Gfx_Tex tex, tex_retry;
	u8 frame, tex_id;
	u8 skull_scale;
} Char_BF;

//Menu BF player definitions
static const CharFrame char_menubf_frame[] = {
	{BFMenu_ArcMain_BF, {  0,   0,  83,  80}, { 53,  92}}, //idle0 0
	{BFMenu_ArcMain_BF, { 83,   0,  83,  79}, { 53,  91}}, //idle1 1
	{BFMenu_ArcMain_BF, {166,   0,  82,  81}, { 52,  93}}, //idle2 2
	{BFMenu_ArcMain_BF, {  0,  81,  84,  84}, { 52,  96}}, //idle3 3
	{BFMenu_ArcMain_BF, { 84,  81,  83,  84}, { 52,  96}}, //idle4 4

	{BFMenu_ArcMain_BF, {167,  81,  80,  84}, { 52,  96}}, //peace0 5
	{BFMenu_ArcMain_BF, {  0, 165,  84,  84}, { 53,  95}}, //peace1 6
	{BFMenu_ArcMain_BF, { 84, 165,  85,  84}, { 53,  95}}, //peace2 7
};

static const Animation char_menubf_anim[PlayerAnim_Max] = {
	{2, (const u8[]){ 0, 1, 2, 3, 4, ASCR_BACK, 1}}, //CharAnim_Idle
	{2, (const u8[]){ 5, 6, 6, 7, 7, 7,  ASCR_BACK, 1}},         //CharAnim_Left
	{0, (const u8[]){ASCR_CHGANI, CharAnim_Idle}},   //CharAnim_LeftAlt
	{0, (const u8[]){ASCR_CHGANI, CharAnim_Idle}},         //CharAnim_Down
	{0, (const u8[]){ASCR_CHGANI, CharAnim_Idle}},   //CharAnim_DownAlt
	{0, (const u8[]){ASCR_CHGANI, CharAnim_Idle}},         //CharAnim_Up
	{0, (const u8[]){ASCR_CHGANI, CharAnim_Idle}},   //CharAnim_UpAlt
	{0, (const u8[]){ASCR_CHGANI, CharAnim_Idle}},         //CharAnim_Right
	{0, (const u8[]){ASCR_CHGANI, CharAnim_Idle}},   //CharAnim_RightAlt
};

//Menu BF player functions
static void Char_BF_SetFrame(void *user, u8 frame)
{
	Char_BF *this = (Char_BF*)user;
	
	//Check if this is a new frame
	if (frame != this->frame)
	{
		//Check if new art shall be loaded
		const CharFrame *cframe = &char_menubf_frame[this->frame = frame];
		if (cframe->tex != this->tex_id)
			Gfx_LoadTex(&this->tex, this->arc_ptr[this->tex_id = cframe->tex], 0);
	}
}

static void Char_BF_Tick(Character *character)
{
	Char_BF *this = (Char_BF*)character;
	
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
	
	//Animate and draw character
	Animatable_Animate(&character->animatable, (void*)this, Char_BF_SetFrame);
	Character_Draw(character, &this->tex, &char_menubf_frame[this->frame]);
}

static void Char_BF_SetAnim(Character *character, u8 anim)
{
	//Set animation
	Animatable_SetAnim(&character->animatable, anim);
	Character_CheckStartSing(character);
}

static void Char_BF_Free(Character *character)
{
	(void)character;
}

static Character *Char_BF_New(fixed_t x, fixed_t y)
{
	//Allocate boyfriend object
	Char_BF *this = Mem_Alloc(sizeof(Char_BF));
	if (this == NULL)
	{
		sprintf(error_msg, "[Char_BF_New] Failed to allocate boyfriend object");
		ErrorLock();
		return NULL;
	}
	
	//Initialize character
	this->character.tick = Char_BF_Tick;
	this->character.set_anim = Char_BF_SetAnim;
	this->character.free = Char_BF_Free;
	
	Animatable_Init(&this->character.animatable, char_menubf_anim);
	Character_Init((Character*)this, x, y);
	
	//Set character information
	this->character.spec = CHAR_SPEC_MISSANIM;
	
	this->character.health_b = 0xFF7cd6f5;
	this->character.health_i = 0;
	
	this->character.focus_x = FIXED_DEC(-50,1);
	this->character.focus_y = (stage.stage_id == StageId_1_4) ? FIXED_DEC(-85,1) : FIXED_DEC(-65,1);
	this->character.focus_zoom = FIXED_DEC(1,1);
	
	//Load art
	const char **pathp = (const char *[]){
		"bf.tim",   //BF_ArcMain_BF0
		NULL
	};
	IO_Data *arc_ptr = this->arc_ptr;
	for (; *pathp != NULL; pathp++)
		*arc_ptr++ = Archive_Find((IO_Data)char_menubf_arc_main, *pathp);
	
	//Initialize render state
	this->tex_id = this->frame = 0xFF;
	
	return (Character*)this;
}
