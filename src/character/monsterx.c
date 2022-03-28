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

//Monster PSX Code by Lord Scout
//sprites by IgorSou3000

//Monsterx character assets
static u8 char_monsterx_arc_main[] = {
	#include "iso/monsterx/main.arc.h"
};

//Monsterx character structure
enum
{
	Monsterx_ArcMain_Idle0,
	Monsterx_ArcMain_Idle1,
	Monsterx_ArcMain_Idle2,
	Monsterx_ArcMain_Left,
	Monsterx_ArcMain_Down,
	Monsterx_ArcMain_Up,
	Monsterx_ArcMain_Right,
	
	Monsterx_Arc_Max,
};

typedef struct
{
	//Character base structure
	Character character;
	
	//Render data and state
	IO_Data arc_main;
	IO_Data arc_ptr[Monsterx_Arc_Max];
	
	Gfx_Tex tex;
	u8 frame, tex_id;
} Char_Monsterx;

//Monsterx character definitions
static const CharFrame char_monsterx_frame[] = {
	{Monsterx_ArcMain_Idle0, {  0,   0, 104, 150+40}, { 58, 139}}, //0 idle 1
	{Monsterx_ArcMain_Idle0, {106,   0, 105, 151+40}, { 59, 140}}, //1 idle 2
	{Monsterx_ArcMain_Idle1, {  0,   0, 110, 152+40}, { 62, 142}}, //2 idle 3
	{Monsterx_ArcMain_Idle1, {112,   0, 110, 157+40}, { 62, 147}}, //3 idle 4
	{Monsterx_ArcMain_Idle2, {  0,   0, 120, 164+40}, { 71, 154}}, //4 idle 3
	{Monsterx_ArcMain_Idle2, {123,   0, 121, 167+40}, { 72, 157}}, //5 idle 4
	
	{Monsterx_ArcMain_Left, {  0,   0,  94, 172+40}, { 59, 163-5}}, //6 left 1
	{Monsterx_ArcMain_Left, { 95,   0,  95, 170+40}, { 60, 160-5}}, //7 left 2
	
	{Monsterx_ArcMain_Down, {  0,   0, 106, 143+30}, { 56, 123+5}}, //8 down 1
	{Monsterx_ArcMain_Down, {107,   0, 108, 147+30}, { 58, 128+5}}, //9 down 2
	
	{Monsterx_ArcMain_Up, {  1,   0, 101, 189+40}, { 63, 181}}, //10 up 1
	{Monsterx_ArcMain_Up, {103,   0, 104, 180+40}, { 66, 171}}, //11 up 2
	
	{Monsterx_ArcMain_Right, {  0,   0, 115, 169+40}, { 59, 158}}, //12 right 1
	{Monsterx_ArcMain_Right, {116,   0, 117, 167+40}, { 61, 158}}, //13 right 2
};

static const Animation char_monsterx_anim[CharAnim_Max] = {
	{2, (const u8[]){ 0,  1,  2,  3,  4,  5,  ASCR_BACK, 1}}, //CharAnim_Idle
	{2, (const u8[]){12, 13, ASCR_BACK, 1}},         //CharAnim_Left
	{0, (const u8[]){ASCR_CHGANI, CharAnim_Idle}},   //CharAnim_LeftAlt
	{2, (const u8[]){ 8,  9, ASCR_BACK, 1}},         //CharAnim_Down
	{0, (const u8[]){ASCR_CHGANI, CharAnim_Idle}},   //CharAnim_DownAlt
	{2, (const u8[]){ 10,  11, ASCR_BACK, 1}},         //CharAnim_Up
	{0, (const u8[]){ASCR_CHGANI, CharAnim_Idle}},   //CharAnim_UpAlt
	{2, (const u8[]){ 6,  7, ASCR_BACK, 1}},         //CharAnim_Right
	{0, (const u8[]){ASCR_CHGANI, CharAnim_Idle}},   //CharAnim_RightAlt
};

//Monsterx character functions
void Char_Monsterx_SetFrame(void *user, u8 frame)
{
	Char_Monsterx *this = (Char_Monsterx*)user;
	
	//Check if this is a new frame
	if (frame != this->frame)
	{
		//Check if new art shall be loaded
		const CharFrame *cframe = &char_monsterx_frame[this->frame = frame];
		if (cframe->tex != this->tex_id)
			Gfx_LoadTex(&this->tex, this->arc_ptr[this->tex_id = cframe->tex], 0);
	}
}

void Char_Monsterx_Tick(Character *character)
{
	Char_Monsterx *this = (Char_Monsterx*)character;
	
	//Perform idle dance
	if ((character->pad_held & (INPUT_LEFT | INPUT_DOWN | INPUT_UP | INPUT_RIGHT)) == 0)
		Character_PerformIdle(character);
	
	//Animate and draw
	Animatable_Animate(&character->animatable, (void*)this, Char_Monsterx_SetFrame);
	Character_Draw(character, &this->tex, &char_monsterx_frame[this->frame]);
}

void Char_Monsterx_SetAnim(Character *character, u8 anim)
{
	//Set animation
	Animatable_SetAnim(&character->animatable, anim);
	Character_CheckStartSing(character);
}

void Char_Monsterx_Free(Character *character)
{
	Char_Monsterx *this = (Char_Monsterx*)character;
	
	//Free art
	Mem_Free(this->arc_main);
}

Character *Char_Monsterx_New(fixed_t x, fixed_t y)
{
	//Allocate monsterx object
	Char_Monsterx *this = Mem_Alloc(sizeof(Char_Monsterx));
	if (this == NULL)
	{
		sprintf(error_msg, "[Char_Monsterx_New] Failed to allocate monsterx object");
		ErrorLock();
		return NULL;
	}
	
	//Initialize character
	this->character.tick = Char_Monsterx_Tick;
	this->character.set_anim = Char_Monsterx_SetAnim;
	this->character.free = Char_Monsterx_Free;
	
	Animatable_Init(&this->character.animatable, char_monsterx_anim);
	Character_Init((Character*)this, x, y);
	
	//Set character information
	this->character.spec = 0;
	
	this->character.health_b = 0xFFe9f36b;
	this->character.health_i = 2;
	
	this->character.focus_x = FIXED_DEC(65,1);
	this->character.focus_y = FIXED_DEC(-90,1);
	this->character.focus_zoom = FIXED_DEC(1,1);
	
	//Load art
	const char **pathp = (const char *[]){
		"idle0.tim", //Monsterx_ArcMain_Idle0
		"idle1.tim", //Monsterx_ArcMain_Idle1
		"idle2.tim",
		"left.tim",  //Monsterx_ArcMain_Left
		"down.tim",  //Monsterx_ArcMain_Down
		"up.tim",    //Monsterx_ArcMain_Up
		"right.tim", //Monsterx_ArcMain_Right
		NULL
	};
	IO_Data *arc_ptr = this->arc_ptr;
	for (; *pathp != NULL; pathp++)
		*arc_ptr++ = Archive_Find((IO_Data)char_monsterx_arc_main, *pathp);
	
	//Initialize render state
	this->tex_id = this->frame = 0xFF;
	
	return (Character*)this;
}
