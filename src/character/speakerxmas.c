/*
  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "boot/stage.h"
#include "boot/archive.h"
#include "boot/main.h"
#include "boot/mem.h"
#include "boot/timer.h"

//Christimas Speaker structure
typedef struct
{
	//Speaker state
	Gfx_Tex tex;
	fixed_t bump;
} Speaker;

//Christimas Speaker assets
static const u8 speaker_tim[] = {
	#include "iso/gf/speaker.tim.h"
};

//Christimas Light assets
static const u8 week5_arc_light[] = {
	#include "iso/gf/light.arc.h"
};

static const CharFrame light_frame[] = {
	{0, {  0,   0, 200, 114}, { 71,  18}}, //0 light 1
	{0, {  0, 115, 200, 114}, { 71,  17}}, //1 light 2
	{1, {  0,   0, 200, 114}, { 71,  18}}, //2 light 3
	{1, {  0, 115, 200, 114}, { 71,  17}}, //3 light 4
};

static const Animation light_anim[] = {
	{2, (const u8[]){0, 1, 2, 3, ASCR_BACK, 1}}, //light
};

//Light state
static IO_Data week5_arc_light_ptr[2];
static Gfx_Tex week5_tex_light;
static u8 week5_light_frame, week5_light_tex_id;

//Light functions
static void Week5_Light_SetFrame(void *user, u8 frame)
{
	(void)user;
	
	//Check if this is a new frame
	if (frame != week5_light_frame)
	{
		//Check if new art shall be loaded
		const CharFrame *cframe = &light_frame[week5_light_frame = frame];
		if (cframe->tex != week5_light_tex_id)
			Gfx_LoadTex(&week5_tex_light, week5_arc_light_ptr[week5_light_tex_id = cframe->tex], 0);
	}
}

static void Week5_Light_Draw(fixed_t x, fixed_t y)
{
	//Draw Light
	const CharFrame *cframe = &light_frame[week5_light_frame];
	
	fixed_t ox = x - ((fixed_t)cframe->off[0] << FIXED_SHIFT);
	fixed_t oy = y - ((fixed_t)cframe->off[1] << FIXED_SHIFT);
	
	RECT src = {cframe->src[0], cframe->src[1], cframe->src[2], cframe->src[3]};
	RECT_FIXED dst = {ox, oy, src.w << FIXED_SHIFT, src.h << FIXED_SHIFT};
	Stage_DrawTex(&week5_tex_light, &src, &dst, stage.camera.bzoom);
}

static Animatable week5_light_animatable;

//Christimas Speaker functions
static void Speaker_Init(Speaker *this)
{
	//Initialize speaker state
	this->bump = 0;
	
	//Load speaker graphics
	Gfx_LoadTex(&this->tex, (IO_Data)speaker_tim, 0);

	//Load light textures
	week5_arc_light_ptr[0] = Archive_Find((IO_Data)week5_arc_light, "light0.tim");
	week5_arc_light_ptr[1] = Archive_Find((IO_Data)week5_arc_light, "light1.tim");
	
	//Initialize light state
	Animatable_Init(&week5_light_animatable, light_anim);
	Animatable_SetAnim(&week5_light_animatable, 0);
	week5_light_frame = week5_light_tex_id = 0xFF; //Force art load
}

static void Speaker_Bump(Speaker *this)
{
	//Set bump
	this->bump = FIXED_DEC(4,1) / 24 - 1;
}

static void Speaker_Tick(Speaker *this, fixed_t x, fixed_t y, fixed_t parallax)
{

	//Animate and draw lights
	if (stage.flag & STAGE_FLAG_JUST_STEP)
	{
		switch (stage.song_step & 3)
		{
			case 0:
				Animatable_SetAnim(&week5_light_animatable, 0);
				break;
		}
	}

	    Animatable_Animate(&week5_light_animatable, NULL, Week5_Light_SetFrame);
		Week5_Light_Draw(x - FIXED_DEC(34,1) -FIXED_MUL(stage.camera.x, parallax), y + FIXED_DEC(7,1) - FIXED_MUL(stage.camera.y, parallax));
	//Get frame to use according to bump
	u8 frame;
	if (this->bump > 0)
	{
		frame = (this->bump * 24) >> FIXED_SHIFT;
		this->bump -= timer_dt;
	}
	else
	{
		frame = 0;
	}
	
	//Draw Christimas speakers
	static const struct SpeakerPiece
	{
		u8 rect[4];
		u8 ox, oy;
	} speaker_draw[4][2] = {
		{ //bump 0
			{{ 97,  88, 158, 88},   0,  0},
			{{  0, 176,  18, 56}, 158, 32},
		},
		{ //bump 1
			{{176,   0,  79, 88},   0,  0},
			{{  0,  88,  97, 88},  79,  0},
		},
		{ //bump 2
			{{  0,   0,  88, 88},   0,  0},
			{{ 88,   0,  88, 88},  88,  0},
		},
		{ //bump 3
			{{  0,   0,  88, 88},   0,  0},
			{{ 88,   0,  88, 88},  88,  0},
		}
	};
	
	const struct SpeakerPiece *piece = speaker_draw[frame];
	for (int i = 0; i < 2; i++, piece++)
	{
		//Draw piece
		RECT piece_src = {piece->rect[0], piece->rect[1], piece->rect[2], piece->rect[3]};
		RECT_FIXED piece_dst = {
			x - FIXED_DEC(88,1) + ((fixed_t)piece->ox << FIXED_SHIFT) - FIXED_MUL(stage.camera.x, parallax),
			y + ((fixed_t)piece->oy << FIXED_SHIFT) - FIXED_MUL(stage.camera.y, parallax),
			(fixed_t)piece->rect[2] << FIXED_SHIFT,
			(fixed_t)piece->rect[3] << FIXED_SHIFT,
		};
		
		Stage_DrawTex(&this->tex, &piece_src, &piece_dst, stage.camera.bzoom);
	}
}
