/*
  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "stage.h"

#include "mem.h"
#include "timer.h"
#include "audio.h"
#include "pad.h"
#include "main.h"
#include "random.h"
#include "movie.h"
#include "network.h"

#include "menu/menu.h"
#include "trans.h"
#include "loadscr.h"

#include "object/combo.h"
#include "object/splash.h"

#include "stdlib.h"

//Stage constants
//#define STAGE_NOHUD //Disable the HUD

//#define STAGE_FREECAM //Freecam

//u32 Stage_Sounds[1];

//notes initialization
void Note_Init(void)
{
	//OG code by Unstop4ble,Changes by IgorSou3000
	 //middle scroll x
		if(stage.middlescroll)
		{
		 for (int i = 0; i <= 3; i++)
		 {
		 u16 x = i*34;
		 //bf
		 stage.note_x[i] = FIXED_DEC(x - 55,1) + FIXED_DEC(SCREEN_WIDEADD,4);
		 }

		 //opponent
		 stage.note_x[4] = stage.note_x[5] = stage.note_x[6] = stage.note_x[7] = FIXED_DEC(-256,1) - FIXED_DEC(SCREEN_WIDEADD,4);
		}
	//normal note x
	else
	{
	//Note x
 	//BF
	 stage.note_x[0] = FIXED_DEC(26,1) + FIXED_DEC(SCREEN_WIDEADD,4);
	 stage.note_x[1] = FIXED_DEC(60,1) + FIXED_DEC(SCREEN_WIDEADD,4);//+34
	 stage.note_x[2] = FIXED_DEC(94,1) + FIXED_DEC(SCREEN_WIDEADD,4);
	 stage.note_x[3] = FIXED_DEC(128,1) + FIXED_DEC(SCREEN_WIDEADD,4);
	//Opponent
	 stage.note_x[4] = FIXED_DEC(-128,1) - FIXED_DEC(SCREEN_WIDEADD,4);
	 stage.note_x[5] = FIXED_DEC(-94,1) - FIXED_DEC(SCREEN_WIDEADD,4);//+34
	 stage.note_x[6] = FIXED_DEC(-60,1) - FIXED_DEC(SCREEN_WIDEADD,4);
	 stage.note_x[7] = FIXED_DEC(-26,1) - FIXED_DEC(SCREEN_WIDEADD,4);
	}
     
	 //Note y
	 //BF
	 stage.note_y[0] = FIXED_DEC(32 - SCREEN_HEIGHT2, 1);
	 stage.note_y[1] = FIXED_DEC(32 - SCREEN_HEIGHT2, 1);
	 stage.note_y[2] = FIXED_DEC(32 - SCREEN_HEIGHT2, 1);
	 stage.note_y[3] = FIXED_DEC(32 - SCREEN_HEIGHT2, 1);
	//Opponent
	 stage.note_y[4] = FIXED_DEC(32 - SCREEN_HEIGHT2, 1);
	 stage.note_y[5] = FIXED_DEC(32 - SCREEN_HEIGHT2, 1);
	 stage.note_y[6] = FIXED_DEC(32 - SCREEN_HEIGHT2, 1);
	 stage.note_y[7] = FIXED_DEC(32 - SCREEN_HEIGHT2, 1);
}


static const u16 note_key[] = {INPUT_LEFT, INPUT_DOWN, INPUT_UP, INPUT_RIGHT};
static const u8 note_anims[4][3] = {
	{CharAnim_Left,  CharAnim_LeftAlt,  PlayerAnim_LeftMiss},
	{CharAnim_Down,  CharAnim_DownAlt,  PlayerAnim_DownMiss},
	{CharAnim_Up,    CharAnim_UpAlt,    PlayerAnim_UpMiss},
	{CharAnim_Right, CharAnim_RightAlt, PlayerAnim_RightMiss},
};

//Stage state
Stage stage;

//Stage overlay state
StageOverlay_Load stageoverlay_load;
StageOverlay_Tick stageoverlay_tick;
StageOverlay_NoteMoviment stageoverlay_notemoviment;
StageOverlay_DrawBG stageoverlay_drawbg;
StageOverlay_DrawMD stageoverlay_drawmd;
StageOverlay_DrawFG stageoverlay_drawfg;
StageOverlay_Free stageoverlay_free;
StageOverlay_GetChart stageoverlay_getchart;
StageOverlay_LoadScreen stageoverlay_loadscreen;
StageOverlay_NextStage stageoverlay_nextstage;

//Stage definitions
#include "stagedef_disc1.h"
  

//Stage move bg function
void Stage_MoveTex(u32 input, s16 x, s16 y)
{
    if (pad_state.held & input && pad_state.held & PAD_LEFT)
			x--;
	if (pad_state.held & input && pad_state.held & PAD_UP)
			y--;
	if (pad_state.held & input && pad_state.held & PAD_RIGHT)
			x++;
	if (pad_state.held & input && pad_state.held & PAD_DOWN)
			y++;

	FntPrint("bg x is %d and y is %d", x, y);
}
//Stage music functions
static void Stage_StartVocal(void)
{
	Audio_SetVolume(2, 0x3FFF, 0x3FFF);
}

static void Stage_CutVocal(void)
{
	Audio_SetVolume(2, 0x0000, 0x0000);
}

//Stage camera functions
static void Stage_FocusCharacter(Character *ch, fixed_t div)
{
	//Use character focus settings to update target position and zoom
	stage.camera.tx = ch->x + ch->focus_x;
	stage.camera.ty = ch->y + ch->focus_y;
	stage.camera.tz = ch->focus_zoom;
	stage.camera.td = div;
}

static void Stage_ScrollCamera(void)
{
	#ifdef STAGE_FREECAM
		if (pad_state.held & PAD_LEFT)
			stage.camera.x -= FIXED_DEC(2,1);
		if (pad_state.held & PAD_UP)
			stage.camera.y -= FIXED_DEC(2,1);
		if (pad_state.held & PAD_RIGHT)
			stage.camera.x += FIXED_DEC(2,1);
		if (pad_state.held & PAD_DOWN)
			stage.camera.y += FIXED_DEC(2,1);
		if (pad_state.held & PAD_TRIANGLE)
			stage.camera.zoom -= FIXED_DEC(1,100);
		if (pad_state.held & PAD_CROSS)
			stage.camera.zoom += FIXED_DEC(1,100);
	#else
		//Get delta position
		fixed_t dx = stage.camera.tx - stage.camera.x;
		fixed_t dy = stage.camera.ty - stage.camera.y;
		fixed_t dz = stage.camera.tz - stage.camera.zoom;
		
		//Scroll based off current divisor
		stage.camera.x += FIXED_MUL(dx, stage.camera.td);
		stage.camera.y += FIXED_MUL(dy, stage.camera.td);
		stage.camera.zoom += FIXED_MUL(dz, stage.camera.td);
		
		//Shake in Week 4
		if (stage.stage_id >= StageId_4_1 && stage.stage_id <= StageId_4_3)
		{
			stage.camera.x += RandomRange(FIXED_DEC(-1,10),FIXED_DEC(1,10));
			stage.camera.y += RandomRange(FIXED_DEC(-25,100),FIXED_DEC(25,100));
		}
	#endif
	
	//Update other camera stuff
	stage.camera.bzoom = FIXED_MUL(stage.camera.zoom, stage.bump);
}

//Stage section functions
static void Stage_ChangeBPM(u16 bpm, u16 step)
{
	//Update last BPM
	stage.last_bpm = bpm;
	
	//Update timing base
	if (stage.step_crochet)
		stage.time_base += FIXED_DIV(((fixed_t)step - stage.step_base) << FIXED_SHIFT, stage.step_crochet);
	stage.step_base = step;
	
	//Get new crochet and times
	stage.step_crochet = ((fixed_t)bpm << FIXED_SHIFT) * 8 / 240; //15/12/24
	stage.step_time = FIXED_DIV(FIXED_DEC(12,1), stage.step_crochet);
	
	//Get new crochet based values
	stage.early_safe = stage.late_safe = stage.step_crochet / 6; //10 frames
	stage.late_sus_safe = stage.late_safe;
	stage.early_sus_safe = stage.early_safe * 2 / 5;
}

static Section *Stage_GetPrevSection(Section *section)
{
	if (section > stage.sections)
		return section - 1;
	return NULL;
}

static u16 Stage_GetSectionStart(Section *section)
{
	Section *prev = Stage_GetPrevSection(section);
	if (prev == NULL)
		return 0;
	return prev->end;
}

//Section scroll structure
typedef struct
{
	fixed_t start;   //Seconds
	fixed_t length;  //Seconds
	u16 start_step;  //Sub-steps
	u16 length_step; //Sub-steps
	
	fixed_t size; //Note height
} SectionScroll;

static void Stage_GetSectionScroll(SectionScroll *scroll, Section *section)
{
	//Get BPM
	u16 bpm = section->flag & SECTION_FLAG_BPM_MASK;
	
	//Get section step info
	scroll->start_step = Stage_GetSectionStart(section);
	scroll->length_step = section->end - scroll->start_step;
	
	//Get section time length
	scroll->length = (scroll->length_step * FIXED_DEC(15,1) / 12) * 24 / bpm;
	
	//Get note height
	scroll->size = FIXED_MUL(stage.speed, scroll->length * (12 * 150) / scroll->length_step) + FIXED_UNIT;
}

//Note hit detection
static u8 Stage_HitNote(PlayerState *this, u8 type, fixed_t offset)
{
	//Get hit type
	if (offset < 0)
		offset = -offset;
	
	u8 hit_type;
	if (offset > stage.late_safe * 9 / 11)
		hit_type = 3; //SHIT
	else if (offset > stage.late_safe * 6 / 11)
		hit_type = 2; //BAD
	else if (offset > stage.late_safe * 3 / 11)
		hit_type = 1; //GOOD
	else
		hit_type = 0; //SICK
	
	if (stage.ghost && hit_type == 3)
	{
		//Miss combo
		if (this->combo)
		{
			//Kill combo
			if (stage.gf != NULL && this->combo > 5)
				stage.gf->set_anim(stage.gf, CharAnim_DownAlt); //Cry if we lost a large combo
			this->combo = 0;
			
			//Create combo object telling of our lost combo
			Obj_Combo *combo = Obj_Combo_New(
				this->character->focus_x,
				this->character->focus_y,
				hit_type,
				0
			);
			if (combo != NULL)
				ObjectList_Add(&stage.objlist_fg, (Object*)combo);
		}
	
		return hit_type;
	}
	
	//Increment combo and score
	this->combo++;
	
	static const s32 score_inc[] = {
		35, //SICK
		20, //GOOD
		10, //BAD
		 5, //SHIT
	};
	this->score += score_inc[hit_type];
	this->refresh_score = true;
	this->min_accuracy += 4;
	this->refresh_score = true;

	if (hit_type == 3) //SHIT
	this->max_accuracy += 10;

	else if (hit_type == 2) //BAD
	this->max_accuracy += 8;

	else if (hit_type == 1) //GOOD
	this->max_accuracy += 6;

	else //SICK
	this->max_accuracy += 4;
	this->refresh_accuracy = true;
	
	//Restore vocals and health
	Stage_StartVocal();
	this->health += 230;
	
	//Create combo object telling of our combo
	Obj_Combo *combo = Obj_Combo_New(
		this->character->focus_x,
		this->character->focus_y,
		hit_type,
		this->combo >= 10 ? this->combo : 0xFFFF
	);
	if (combo != NULL)
		ObjectList_Add(&stage.objlist_fg, (Object*)combo);
	
	//Create note splashes if SICK
	if (hit_type == 0)
	{
		for (int i = 0; i < 3; i++)
		{
			//Create splash object
			Obj_Splash *splash = Obj_Splash_New(
				stage.note_x[type],
				stage.note_y[type] * (stage.downscroll ? -1 : 1),
				type & 0x3
			);
			if (splash != NULL)
				ObjectList_Add(&stage.objlist_splash, (Object*)splash);
		}
	}
	
	return hit_type;
}

static void Stage_MissNote(PlayerState *this)
{
	this->max_accuracy += 7;
	this->refresh_accuracy = true;
	this->miss += 1;
	this->refresh_miss = true;

	if (this->combo)
	{
		//Kill combo
		if (stage.gf != NULL && this->combo > 5)
			stage.gf->set_anim(stage.gf, CharAnim_DownAlt); //Cry if we lost a large combo
		this->combo = 0;
		
		//Create combo object telling of our lost combo
		Obj_Combo *combo = Obj_Combo_New(
			this->character->focus_x,
			this->character->focus_y,
			0xFF,
			0
		);
		if (combo != NULL)
			ObjectList_Add(&stage.objlist_fg, (Object*)combo);
	}
}

static void Stage_NoteCheck(PlayerState *this, u8 type)
{
	//Perform note check
	for (Note *note = stage.cur_note;; note++)
	{
		if (!(note->type & NOTE_FLAG_MINE))
		{
			//Check if note can be hit
			fixed_t note_fp = (fixed_t)note->pos << FIXED_SHIFT;
			if (note_fp - stage.early_safe > stage.note_scroll)
				break;
			if (note_fp + stage.late_safe < stage.note_scroll)
				continue;
			if ((note->type & NOTE_FLAG_HIT) || (note->type & (NOTE_FLAG_OPPONENT | 0x3)) != type || (note->type & NOTE_FLAG_SUSTAIN))
				continue;
			
			//Hit the note
			note->type |= NOTE_FLAG_HIT;
             
			if (this->character->ignoreanim != true)
		    this->character->set_anim(this->character, note_anims[type & 0x3][(note->type & NOTE_FLAG_ALT_ANIM) != 0]);

			u8 hit_type = Stage_HitNote(this, type, stage.note_scroll - note_fp);
			this->arrow_hitan[type & 0x3] = stage.step_time;
			
				(void)hit_type;
			return;
		}
		else
		{
			//Check if mine can be hit
			fixed_t note_fp = (fixed_t)note->pos << FIXED_SHIFT;
			if (note_fp - (stage.late_safe * 3 / 5) > stage.note_scroll)
				break;
			if (note_fp + (stage.late_safe * 2 / 5) < stage.note_scroll)
				continue;
			if ((note->type & NOTE_FLAG_HIT) || (note->type & (NOTE_FLAG_OPPONENT | 0x3)) != type || (note->type & NOTE_FLAG_SUSTAIN))
				continue;
			
			//Hit the mine
			note->type |= NOTE_FLAG_HIT;
			
				this->health -= 2000;
			if (this->character->spec & CHAR_SPEC_MISSANIM)
				this->character->set_anim(this->character, note_anims[type & 0x3][2]);
			else
				this->character->set_anim(this->character, note_anims[type & 0x3][0]);
			this->arrow_hitan[type & 0x3] = -1;
			return;
		}
	}
	
	//Missed a note
	this->arrow_hitan[type & 0x3] = -1;
	
	if (!stage.ghost)
	{
		if (this->character->spec & CHAR_SPEC_MISSANIM)
		{
			if (this->character->ignoreanim != true)
		       this->character->set_anim(this->character, note_anims[type & 0x3][2]);
		}
		else
		{
			if (this->character->ignoreanim != true)
			 this->character->set_anim(this->character, note_anims[type & 0x3][0]);
		}
		Stage_MissNote(this);
		
		this->health -= 400*2;
		this->score -= 1;
		this->refresh_score = true;
	}
}

static void Stage_SustainCheck(PlayerState *this, u8 type)
{
	//Perform note check
	for (Note *note = stage.cur_note;; note++)
	{
		//Check if note can be hit
		fixed_t note_fp = (fixed_t)note->pos << FIXED_SHIFT;
		if (note_fp - stage.early_sus_safe > stage.note_scroll)
			break;
		if (note_fp + stage.late_sus_safe < stage.note_scroll)
			continue;
		if ((note->type & NOTE_FLAG_HIT) || (note->type & (NOTE_FLAG_OPPONENT | 0x3)) != type || !(note->type & NOTE_FLAG_SUSTAIN))
			continue;
		
		//Hit the note
		note->type |= NOTE_FLAG_HIT;
		
		if (this->character->ignoreanim != true)
		   this->character->set_anim(this->character, note_anims[type & 0x3][(note->type & NOTE_FLAG_ALT_ANIM) != 0]);

		
		Stage_StartVocal();
		this->health += 230;
		this->arrow_hitan[type & 0x3] = stage.step_time;
	}
}

static void Stage_ProcessPlayer(PlayerState *this, Pad *pad, boolean playing)
{
	//Handle player note presses
	if (!(stage.botplay))
	{
		if (playing)
		{
			u8 i = (this->character == stage.opponent) ? NOTE_FLAG_OPPONENT : 0;
			
			this->pad_held = this->character->pad_held = pad->held;
			this->pad_press = pad->press;
			
			if (this->pad_held & INPUT_LEFT)
				Stage_SustainCheck(this, 0 | i);
			if (this->pad_held & INPUT_DOWN)
				Stage_SustainCheck(this, 1 | i);
			if (this->pad_held & INPUT_UP)
				Stage_SustainCheck(this, 2 | i);
			if (this->pad_held & INPUT_RIGHT)
				Stage_SustainCheck(this, 3 | i);
			
			if (this->pad_press & INPUT_LEFT)
				Stage_NoteCheck(this, 0 | i);
			if (this->pad_press & INPUT_DOWN)
				Stage_NoteCheck(this, 1 | i);
			if (this->pad_press & INPUT_UP)
				Stage_NoteCheck(this, 2 | i);
			if (this->pad_press & INPUT_RIGHT)
				Stage_NoteCheck(this, 3 | i);
		}
		else
		{
			this->pad_held = this->character->pad_held = 0;
			this->pad_press = 0;
		}
	}
	
		//Do perfect note checks
		if (stage.botplay)
		{
		if (playing)
		{
			u8 i = (this->character == stage.opponent) ? NOTE_FLAG_OPPONENT : 0;
			
			u8 hit[4] = {0, 0, 0, 0};
			for (Note *note = stage.cur_note;; note++)
			{
				//Check if note can be hit
				fixed_t note_fp = (fixed_t)note->pos << FIXED_SHIFT;
				if (note_fp - stage.early_safe - FIXED_DEC(12,1) > stage.note_scroll)
					break;
				if (note_fp + stage.late_safe < stage.note_scroll)
					continue;
				if ((note->type & NOTE_FLAG_MINE) || (note->type & NOTE_FLAG_OPPONENT) != i)
					continue;
				
				//Handle note hit
				if (!(note->type & NOTE_FLAG_SUSTAIN))
				{
					if (note->type & NOTE_FLAG_HIT)
						continue;
					if (stage.note_scroll >= note_fp)
						hit[note->type & 0x3] |= 1;
					else if (!(hit[note->type & 0x3] & 8))
						hit[note->type & 0x3] |= 2;
				}
				else if (!(hit[note->type & 0x3] & 2))
				{
					if (stage.note_scroll <= note_fp)
						hit[note->type & 0x3] |= 4;
					hit[note->type & 0x3] |= 8;
				}
			}
			
			//Handle input
			this->pad_held = 0;
			this->pad_press = 0;
			
			for (u8 j = 0; j < 4; j++)
			{
				if (hit[j] & 5)
				{
					this->pad_held |= note_key[j];
					Stage_SustainCheck(this, j | i);
				}
				if (hit[j] & 1)
				{
					this->pad_press |= note_key[j];
					Stage_NoteCheck(this, j | i);
				}
			}
			
			this->character->pad_held = this->pad_held;
		}
		else
		{
			this->pad_held = this->character->pad_held = 0;
			this->pad_press = 0;
		}
	}
}

//Stage drawing functions
void Stage_DrawTexCol(Gfx_Tex *tex, const RECT *src, const RECT_FIXED *dst, fixed_t zoom, u8 cr, u8 cg, u8 cb)
{
	fixed_t xz = dst->x;
	fixed_t yz = dst->y;
	fixed_t wz = dst->w;
	fixed_t hz = dst->h;
	
	#ifdef STAGE_NOHUD
		if (tex == &stage.tex_hud0 || tex == &stage.tex_hud1)
			return;
	#endif
	
	fixed_t l = (SCREEN_WIDTH2  << FIXED_SHIFT) + FIXED_MUL(xz, zoom);// + FIXED_DEC(1,2);
	fixed_t t = (SCREEN_HEIGHT2 << FIXED_SHIFT) + FIXED_MUL(yz, zoom);// + FIXED_DEC(1,2);
	fixed_t r = l + FIXED_MUL(wz, zoom);
	fixed_t b = t + FIXED_MUL(hz, zoom);
	
	l >>= FIXED_SHIFT;
	t >>= FIXED_SHIFT;
	r >>= FIXED_SHIFT;
	b >>= FIXED_SHIFT;
	
	RECT sdst = {
		l,
		t,
		r - l,
		b - t,
	};
	Gfx_DrawTexCol(tex, src, &sdst, cr, cg, cb);
}

void Stage_DrawTex(Gfx_Tex *tex, const RECT *src, const RECT_FIXED *dst, fixed_t zoom)
{
	Stage_DrawTexCol(tex, src, dst, zoom, 0x80, 0x80, 0x80);
}

void Stage_DrawTexArbCol(Gfx_Tex *tex, const RECT *src, const POINT_FIXED *p0, const POINT_FIXED *p1, const POINT_FIXED *p2, const POINT_FIXED *p3, u8 r, u8 g, u8 b, fixed_t zoom)
{
	//Don't draw if HUD and HUD is disabled
	#ifdef STAGE_NOHUD
		if (tex == &stage.tex_hud0 || tex == &stage.tex_hud1)
			return;
	#endif
	
	//Get screen-space points
	POINT s0 = {SCREEN_WIDTH2 + (FIXED_MUL(p0->x, zoom) >> FIXED_SHIFT), SCREEN_HEIGHT2 + (FIXED_MUL(p0->y, zoom) >> FIXED_SHIFT)};
	POINT s1 = {SCREEN_WIDTH2 + (FIXED_MUL(p1->x, zoom) >> FIXED_SHIFT), SCREEN_HEIGHT2 + (FIXED_MUL(p1->y, zoom) >> FIXED_SHIFT)};
	POINT s2 = {SCREEN_WIDTH2 + (FIXED_MUL(p2->x, zoom) >> FIXED_SHIFT), SCREEN_HEIGHT2 + (FIXED_MUL(p2->y, zoom) >> FIXED_SHIFT)};
	POINT s3 = {SCREEN_WIDTH2 + (FIXED_MUL(p3->x, zoom) >> FIXED_SHIFT), SCREEN_HEIGHT2 + (FIXED_MUL(p3->y, zoom) >> FIXED_SHIFT)};
	
	Gfx_DrawTexArbCol(tex, src, &s0, &s1, &s2, &s3, r, g, b);
}

void Stage_DrawTexArb(Gfx_Tex *tex, const RECT *src, const POINT_FIXED *p0, const POINT_FIXED *p1, const POINT_FIXED *p2, const POINT_FIXED *p3, fixed_t zoom)
{
	Stage_DrawTexArbCol(tex, src, p0, p1, p2, p3, 0x80, 0x80, 0x80, zoom);
}

void Stage_BlendTexArbCol(Gfx_Tex *tex, const RECT *src, const POINT_FIXED *p0, const POINT_FIXED *p1, const POINT_FIXED *p2, const POINT_FIXED *p3, fixed_t zoom, u8 r, u8 g, u8 b, u8 mode)
{
	//Don't draw if HUD and HUD is disabled
	#ifdef STAGE_NOHUD
		if (tex == &stage.tex_hud0 || tex == &stage.tex_hud1)
			return;
	#endif
	
	//Get screen-space points
	POINT s0 = {SCREEN_WIDTH2 + (FIXED_MUL(p0->x, zoom) >> FIXED_SHIFT), SCREEN_HEIGHT2 + (FIXED_MUL(p0->y, zoom) >> FIXED_SHIFT)};
	POINT s1 = {SCREEN_WIDTH2 + (FIXED_MUL(p1->x, zoom) >> FIXED_SHIFT), SCREEN_HEIGHT2 + (FIXED_MUL(p1->y, zoom) >> FIXED_SHIFT)};
	POINT s2 = {SCREEN_WIDTH2 + (FIXED_MUL(p2->x, zoom) >> FIXED_SHIFT), SCREEN_HEIGHT2 + (FIXED_MUL(p2->y, zoom) >> FIXED_SHIFT)};
	POINT s3 = {SCREEN_WIDTH2 + (FIXED_MUL(p3->x, zoom) >> FIXED_SHIFT), SCREEN_HEIGHT2 + (FIXED_MUL(p3->y, zoom) >> FIXED_SHIFT)};
	
	Gfx_BlendTexArbCol(tex, src, &s0, &s1, &s2, &s3, r, g, b, mode);
}

void Stage_BlendTexArb(Gfx_Tex *tex, const RECT *src, const POINT_FIXED *p0, const POINT_FIXED *p1, const POINT_FIXED *p2, const POINT_FIXED *p3, fixed_t zoom, u8 mode)
{
	Stage_BlendTexArbCol(tex, src, p0, p1, p2, p3, zoom, 0x80, 0x80, 0x80, mode);
}

//Stage HUD functions
static void Stage_DrawHealth(s16 health, u8 i, s8 ox)
{
	//Check if we should use 'dying' frame
	fixed_t hx;
	s8 dying;
	if (ox < 0)
		dying = (health >= 18000) * 38;
	else
		dying = (health <= 2000) * 38;
	
	//Get src and dst
	hx = (128 << FIXED_SHIFT) * (10000 - health) / 10000;
	RECT src = {
		(i % 1) * 76 + dying,
	    (i / 1) * 38,
	    38,
		38
	};
	RECT_FIXED dst = {
		hx + ox * FIXED_DEC(21,1) - FIXED_DEC(19,1),
		FIXED_DEC(SCREEN_HEIGHT2 - 38 + 4 - 18, 1),
		src.w << FIXED_SHIFT,
		src.h << FIXED_SHIFT
	};
	if (stage.downscroll)
		dst.y = FIXED_DEC(-113,1);

	//invert icon image
	if (stage.mode == StageMode_Swap)
	{
	   dst.w = -dst.w;
	   dst.x += FIXED_DEC(34,1);
	}
	
	//Draw health icon
	Stage_DrawTex(&stage.tex_hud1, &src, &dst, FIXED_MUL(stage.bump, stage.sbump));
    }

static void Stage_DrawStrum(u8 i, RECT *note_src, RECT_FIXED *note_dst)
{
	(void)note_dst;
	
	PlayerState *this = &stage.player_state[((i ^ stage.note_swap) & NOTE_FLAG_OPPONENT) != 0];
	i &= 0x3;
	
	if (this->arrow_hitan[i] > 0)
	{
		//Play hit animation
		u8 frame = ((this->arrow_hitan[i] << 1) / stage.step_time) & 1;
		note_src->x = (i + 1) << 5;
		note_src->y = 64 - (frame << 5);
		
		this->arrow_hitan[i] -= timer_dt;
		if (this->arrow_hitan[i] <= 0)
		{
			if (this->pad_held & note_key[i])
				this->arrow_hitan[i] = 1;
			else
				this->arrow_hitan[i] = 0;
		}
	}
	else if (this->arrow_hitan[i] < 0)
	{
		//Play depress animation
		note_src->x = (i + 1) << 5;
		note_src->y = 96;
		if (!(this->pad_held & note_key[i]))
			this->arrow_hitan[i] = 0;
	}
	else
	{
		note_src->x = 0;
		note_src->y = i << 5;
	}
}

static void Stage_DrawNotes(void)
{
	//Check if opponent should draw as bot
	u8 bot = (stage.mode >= StageMode_2P) ? 0 : NOTE_FLAG_OPPONENT;
	
	//Initialize scroll state
	SectionScroll scroll;
	scroll.start = stage.time_base;
	
	Section *scroll_section = stage.section_base;
	Stage_GetSectionScroll(&scroll, scroll_section);
	
	//Push scroll back until cur_note is properly contained
	while (scroll.start_step > stage.cur_note->pos)
	{
		//Look for previous section
		Section *prev_section = Stage_GetPrevSection(scroll_section);
		if (prev_section == NULL)
			break;
		
		//Push scroll back
		scroll_section = prev_section;
		Stage_GetSectionScroll(&scroll, scroll_section);
		scroll.start -= scroll.length;
	}
	
	//Draw notes
	for (Note *note = stage.cur_note; note->pos != 0xFFFF; note++)
	{
		//Update scroll
		while (note->pos >= scroll_section->end)
		{
			//Push scroll forward
			scroll.start += scroll.length;
			Stage_GetSectionScroll(&scroll, ++scroll_section);
		}
		
		//Get note information
		u8 i = ((note->type ^ stage.note_swap) & NOTE_FLAG_OPPONENT) != 0;
		PlayerState *this = &stage.player_state[i];
		
		fixed_t note_fp = (fixed_t)note->pos << FIXED_SHIFT;
		fixed_t time = (scroll.start - stage.song_time) + (scroll.length * (note->pos - scroll.start_step) / scroll.length_step);
		fixed_t y = stage.note_y[(note->type & 0x7)] + FIXED_MUL(stage.speed, time * 150);
		
		//Check if went above screen
		if (y < FIXED_DEC(-16 - SCREEN_HEIGHT2, 1))
		{
			//Wait for note to exit late time
			if (note_fp + stage.late_safe >= stage.note_scroll)
				continue;
			
			//Miss note if player's note
			if (!((note->type ^ stage.note_swap) & (bot | NOTE_FLAG_HIT | NOTE_FLAG_MINE)))
			{
					//Missed note
					Stage_CutVocal();
					Stage_MissNote(this);
		            this->health -= 475;
				}
			
			//Update current note
			stage.cur_note++;
		}
		else
		{
			//Don't draw if below screen
			RECT note_src;
			RECT_FIXED note_dst;
			if (y > (FIXED_DEC(SCREEN_HEIGHT,2) + scroll.size) || note->pos == 0xFFFF)
				break;
			
			//Draw note
			if (note->type & NOTE_FLAG_SUSTAIN)
			{
				//Check for sustain clipping
				fixed_t clip;
				y -= scroll.size;
				if (((note->type ^ stage.note_swap) & (bot | NOTE_FLAG_HIT)) || ((this->pad_held & note_key[note->type & 0x3]) && (note_fp + stage.late_sus_safe >= stage.note_scroll)))
				{
					 clip = stage.note_y[(note->type & 0x7)] - y;
					if (clip < 0)
						clip = 0;
				}
				else
				{
					clip = 0;
				}
				
				//Draw sustain
				if (note->type & NOTE_FLAG_SUSTAIN_END)
				{
					if (clip < (24 << FIXED_SHIFT))
					{
						note_src.x = 160;
						note_src.y = ((note->type & 0x3) << 5) + 4 + (clip >> FIXED_SHIFT);
						note_src.w = 32;
						note_src.h = 28 - (clip >> FIXED_SHIFT);
						
						note_dst.x = stage.note_x[note->type & 0x7] - FIXED_DEC(16,1);
						note_dst.y = y + clip;
						note_dst.w = note_src.w << FIXED_SHIFT;
						note_dst.h = (note_src.h << FIXED_SHIFT);
						
						if (stage.downscroll)
						{
							note_dst.y = -note_dst.y;
							note_dst.h = -note_dst.h;
						}
						Stage_DrawTex(&stage.tex_hud0, &note_src, &note_dst, stage.bump);
					}
				}
				else
				{
					//Get note height
					fixed_t next_time = (scroll.start - stage.song_time) + (scroll.length * (note->pos + 12 - scroll.start_step) / scroll.length_step);
					fixed_t next_y = stage.note_y[(note->type & 0x7)] + FIXED_MUL(stage.speed, next_time * 150) - scroll.size;
					fixed_t next_size = next_y - y;
					
					if (clip < next_size)
					{
						note_src.x = 160;
						note_src.y = ((note->type & 0x3) << 5);
						note_src.w = 32;
						note_src.h = 16;
						
						note_dst.x = stage.note_x[note->type & 0x7] - FIXED_DEC(16,1);
						note_dst.y = y + clip;
						note_dst.w = note_src.w << FIXED_SHIFT;
						note_dst.h = (next_y - y) - clip;
						
						if (stage.downscroll)
							note_dst.y = -note_dst.y - note_dst.h;
						Stage_DrawTex(&stage.tex_hud0, &note_src, &note_dst, stage.bump);
					}
				}
			}
			else if (note->type & NOTE_FLAG_MINE)
			{
				//Don't draw if already hit
				if (note->type & NOTE_FLAG_HIT)
					continue;
				
				//Draw note body
				note_src.x = 192 + ((note->type & 0x1) << 5);
				note_src.y = (note->type & 0x2) << 4;
				note_src.w = 32;
				note_src.h = 32;
				
				note_dst.x = stage.note_x[note->type & 0x7] - FIXED_DEC(16,1);
				note_dst.y = y - FIXED_DEC(16,1);
				note_dst.w = note_src.w << FIXED_SHIFT;
				note_dst.h = note_src.h << FIXED_SHIFT;
				
				if (stage.downscroll)
					note_dst.y = -note_dst.y - note_dst.h;
				Stage_DrawTex(&stage.tex_hud0, &note_src, &note_dst, stage.bump);
				
					//Draw note fire
					note_src.x = 192 + ((animf_count & 0x1) << 5);
					note_src.y = 64 + ((animf_count & 0x2) * 24);
					note_src.w = 32;
					note_src.h = 48;
					
					if (stage.downscroll)
					{
						note_dst.y += note_dst.h;
						note_dst.h = note_dst.h * -3 / 2;
					}
					else
					{
						note_dst.h = note_dst.h * 3 / 2;
					}
					Stage_DrawTex(&stage.tex_hud0, &note_src, &note_dst, stage.bump);
			}
			else
			{
				//Don't draw if already hit
				if (note->type & NOTE_FLAG_HIT)
					continue;
				
				//Draw note
				note_src.x = 32 + ((note->type & 0x3) << 5);
				note_src.y = 0;
				note_src.w = 32;
				note_src.h = 32;
				
				note_dst.x = stage.note_x[note->type & 0x7] - FIXED_DEC(16,1);
				note_dst.y = y - FIXED_DEC(16,1);
				note_dst.w = note_src.w << FIXED_SHIFT;
				note_dst.h = note_src.h << FIXED_SHIFT;
				
				if (stage.downscroll)
					note_dst.y = -note_dst.y - note_dst.h;

				Stage_DrawTex(&stage.tex_hud0, &note_src, &note_dst, stage.bump);
			}
		}
	}
}

//Stage loads
static void Stage_LoadChart(void)
{
	//Get chart data
	stage.chart_data = stageoverlay_getchart();
	u8 *chart_byte = (u8*)stage.chart_data;
	
	//Directly use section and notes pointers
	stage.sections = (Section*)(chart_byte + 6);
	stage.notes = (Note*)(chart_byte + ((u16*)stage.chart_data)[2]);
	
	for (Note *note = stage.notes; note->pos != 0xFFFF; note++)
		stage.num_notes++;
	
	//Count max scores
	stage.player_state[0].max_score = 0;
	stage.player_state[1].max_score = 0;
	for (Note *note = stage.notes; note->pos != 0xFFFF; note++)
	{
		if (note->type & (NOTE_FLAG_SUSTAIN | NOTE_FLAG_MINE))
			continue;
		if (note->type & NOTE_FLAG_OPPONENT)
			stage.player_state[1].max_score += 35;
		else
			stage.player_state[0].max_score += 35;
	}
	if (stage.mode >= StageMode_2P && stage.player_state[1].max_score > stage.player_state[0].max_score)
		stage.max_score = stage.player_state[1].max_score;
	else
		stage.max_score = stage.player_state[0].max_score;
	
	stage.cur_section = stage.sections;
	stage.cur_note = stage.notes;
	
	stage.speed = *((fixed_t*)stage.chart_data);
	
	stage.step_crochet = 0;
	stage.time_base = 0;
	stage.step_base = 0;
	stage.section_base = stage.cur_section;
	Stage_ChangeBPM(stage.cur_section->flag & SECTION_FLAG_BPM_MASK, 0);
}

static void Stage_LoadMusic(void)
{
	//Offset sing ends
	stage.player->sing_end -= stage.note_scroll;
	stage.opponent->sing_end -= stage.note_scroll;
	if (stage.gf != NULL)
		stage.gf->sing_end -= stage.note_scroll;
    /*
	// Begin Read Sound effects
	CdlFILE file;
    IO_FindFile(&file, "\\SOUND\\MISS.VAG;1");
    u32 *data = IO_ReadFile(&file);
    Stage_Sounds[0] = Audio_LoadVAGData(data, file.size);

	for (int i = 0; i < 1; i++)
	printf("address = %08x\n", Stage_Sounds[i]);

	free(data);
	*/

	//Begin reading mus
	Audio_LoadMus(stage.stage_def->mus_path);
	
	//Initialize music state
	stage.note_scroll = FIXED_DEC(-5 * 4 * 12,1);
	stage.song_time = FIXED_DIV(stage.note_scroll, stage.step_crochet);
	stage.interp_time = 0;
	stage.interp_ms = 0;
	stage.interp_speed = 0;
	
	//Offset sing ends again
	stage.player->sing_end += stage.note_scroll;
	stage.opponent->sing_end += stage.note_scroll;
	if (stage.gf != NULL)
		stage.gf->sing_end += stage.note_scroll;
}

static void Stage_LoadState(void)
{
	//Initialize stage state
	stage.flag = 0;
	
	stage.gf_speed = 1 << 2;
	
	stage.state = StageState_Play;
	
	if (stage.mode == StageMode_Swap)
	{
		stage.player_state[0].character = stage.opponent;
		stage.player_state[1].character = stage.player;
	}
	else
	{
		stage.player_state[0].character = stage.player;
		stage.player_state[1].character = stage.opponent;
	}
	
	for (int i = 0; i < 2; i++)
	{
		memset(stage.player_state[i].arrow_hitan, 0, sizeof(stage.player_state[i].arrow_hitan));
		
		stage.player_state[i].health = 10000;
		stage.player_state[i].combo = 0;
		
		stage.player_state[i].refresh_score = false;
		stage.player_state[i].score = 0;
		stage.player_state[i].score_text[0] = '0';
		stage.player_state[i].score_text[1] = '\0';

		stage.player_state[i].refresh_miss = false;
		stage.player_state[i].miss = 0;

		stage.player_state[i].accuracy = 0;
		stage.player_state[i].max_accuracy = 0;
		stage.player_state[i].min_accuracy = 0;

		stage.player_state[i].accuracy_text[0] = '0';
		stage.player_state[i].accuracy_text[1] = '\0';

		stage.player_state[i].miss_text[0] = '0';
		stage.player_state[i].miss_text[1] = '\0';
		
		stage.player_state[i].pad_held = stage.player_state[i].pad_press = 0;
	}
	
	ObjectList_Free(&stage.objlist_splash);
	ObjectList_Free(&stage.objlist_fg);
	ObjectList_Free(&stage.objlist_bg);
}

static void Stage_InitCamera(void)
{
	//Set camera focus
	if (stage.cur_section->flag & SECTION_FLAG_OPPFOCUS)
		Stage_FocusCharacter(stage.opponent, FIXED_UNIT);
	else
		Stage_FocusCharacter(stage.player, FIXED_UNIT);
	
	//Initialize camera to focus
	stage.camera.x = stage.camera.tx;
	stage.camera.y = stage.camera.ty;
	stage.camera.zoom = stage.camera.tz;
	
	stage.bump = FIXED_UNIT;
	stage.sbump = FIXED_UNIT;
}

//Stage functions
void Stage_Load(StageId id, StageDiff difficulty, boolean story)
{
	//Get stage definition
	stage.stage_def = &stage_defs[stage.stage_id = id];
	stage.stage_diff = difficulty;
	stage.story = story;
	
	//Load overlay
	Overlay_Load(stage.stage_def->overlay_path);
	stage.stage_def->overlay_setptr();
	
	//Load stage and chart
	stageoverlay_load();
	Stage_LoadChart();
	
	//Initialize stage state
	stage.story = story;
	Stage_LoadState();
	
	//Initialize camera
	Stage_InitCamera();
    
	//Initialize notes
	Note_Init();
	
	//Initialize stage according to mode
	stage.note_swap = (stage.mode == StageMode_Swap) ? NOTE_FLAG_OPPONENT : 0;
	
	//Load music
	stage.note_scroll = 0;
	Stage_LoadMusic();
	
	//Test offset
	stage.offset = 0;
	
	//Set game state
	gameloop = GameLoop_Stage;
}

void Stage_LoadScr(StageId id, StageDiff difficulty, boolean story)
{
	//Wrap stage load in loading screen
	LoadScr_Start();
	Stage_Load(id, difficulty, story);
	LoadScr_End();
}

void Stage_Unload(void)
{
	//Free objects
	ObjectList_Free(&stage.objlist_splash);
	ObjectList_Free(&stage.objlist_fg);
	ObjectList_Free(&stage.objlist_bg);
	
	//Free characters
	Character_Free(stage.player);
	stage.player = NULL;
	Character_Free(stage.opponent);
	stage.opponent = NULL;
	Character_Free(stage.gf);
	stage.gf = NULL;
	
	//Free stage
	if (stageoverlay_free != NULL)
		stageoverlay_free();
}

void Stage_Tick(void)
{
	SeamLoad:;
	
	//Tick transition
		//Return to menu when start is pressed
		if (pad_state.press & PAD_START)
		{
			stage.trans = (stage.state == StageState_Play) ? StageTrans_Menu : StageTrans_Reload;
			Trans_Start();
		}
	
	if (Trans_Tick())
	{
		switch (stage.trans)
		{
			case StageTrans_NextStage:
				//Load next stage
				if (stageoverlay_nextstage())
				{
					stage.stage_def = &stage_defs[stage.stage_id];
					Stage_LoadChart();
					Stage_LoadState();
					Stage_InitCamera();
					Stage_LoadMusic();
					Timer_Reset();
					break;
				}
		//Fallthrough
			case StageTrans_Menu:
				//Load appropriate menu
				Stage_Unload();
				
				LoadScr_Start();
				if (stage.stage_id <= StageId_LastVanilla)
					{
						if (stage.story)
							Menu_Load(MenuPage_Story);
						else
							Menu_Load(MenuPage_Freeplay);
					}
					else
					{
						Menu_Load(MenuPage_Credits);
					}
				LoadScr_End();
				
				gameloop = GameLoop_Menu;
				return;
			case StageTrans_Reload:
				//Reload song
				Stage_Unload();
				
				LoadScr_Start();
				Stage_Load(stage.stage_id, stage.stage_diff, stage.story);
				LoadScr_End();
				break;
		}
	}
	
	switch (stage.state)
	{
		case StageState_Play:
		{

			//Clear per-frame flags
			stage.flag &= ~(STAGE_FLAG_JUST_STEP | STAGE_FLAG_SCORE_REFRESH);
			
			//Get song position
			boolean playing;
			fixed_t next_scroll;
			
				if (stage.note_scroll < 0)
				{
					//Play countdown sequence
					stage.song_time += timer_dt;
					
					//Update song
					if (stage.song_time >= 0)
					{
						//Start song
						playing = true;
						
						Audio_PlayMus(false);
						Audio_SetVolume(0, 0x3FFF, 0x0000);
						Audio_SetVolume(1, 0x0000, 0x3FFF);
						
						//Update song time
						stage.interp_ms = Audio_GetTime();
						stage.interp_time = 0;
						stage.song_time = stage.interp_ms;
					}
					else
					{
						//Still scrolling
						//Play menu music
						playing = false;
					}
					
					//Update scroll
					next_scroll = FIXED_MUL(stage.song_time, stage.step_crochet);
				}
				else if (Audio_IsPlaying())
				{
					//Sync to audio
					stage.interp_ms = Audio_GetTime();
					stage.interp_time = 0;
					stage.song_time = stage.interp_ms;
					
					playing = true;
					
					//Update scroll
					next_scroll = ((fixed_t)stage.step_base << FIXED_SHIFT) + FIXED_MUL(stage.song_time - stage.time_base, stage.step_crochet);
				}
				else
				{
					//Song has ended
					playing = false;
					stage.song_time += timer_dt;
					
					//Update scroll
					next_scroll = ((fixed_t)stage.step_base << FIXED_SHIFT) + FIXED_MUL(stage.song_time - stage.time_base, stage.step_crochet);
					
					//Transition to menu or next song
					if (stage.story == false)
					{
						stage.trans = StageTrans_Menu;
						Trans_Start();
					}
					else if (stageoverlay_loadscreen())
					{
						stage.trans = StageTrans_NextStage;
						Trans_Start();
					}
					else
					{
						if (stageoverlay_nextstage())
						{
							printf("test 0 OK");
							stage.stage_def = &stage_defs[stage.stage_id];
							Stage_LoadChart();
							Stage_LoadState();
							Stage_LoadMusic();
							printf("test 1 OK");
							goto SeamLoad;
						}
						else
						{
							stage.trans = StageTrans_Menu;
							Trans_Start();
						}
					}
				}
			
			RecalcScroll:;
			//Update song scroll and step
			if (next_scroll > stage.note_scroll)
			{
				if (((stage.note_scroll / 12) & FIXED_UAND) != ((next_scroll / 12) & FIXED_UAND))
					stage.flag |= STAGE_FLAG_JUST_STEP;
				stage.note_scroll = next_scroll;
				stage.song_step = (stage.note_scroll >> FIXED_SHIFT);
				if (stage.note_scroll < 0)
					stage.song_step -= 11;
				stage.song_step /= 12;
			}
			
			//Update section
			if (stage.note_scroll >= 0)
			{
				//Check if current section has ended
				u16 end = stage.cur_section->end;
				if ((stage.note_scroll >> FIXED_SHIFT) >= end)
				{
					//Increment section pointer
					stage.cur_section++;
					
					//Update BPM
					u16 next_bpm = stage.cur_section->flag & SECTION_FLAG_BPM_MASK;
					Stage_ChangeBPM(next_bpm, end);
					stage.section_base = stage.cur_section;
					
					//Recalculate scroll based off new BPM
					next_scroll = ((fixed_t)stage.step_base << FIXED_SHIFT) + FIXED_MUL(stage.song_time - stage.time_base, stage.step_crochet);
					goto RecalcScroll;
				}
			}

			//note moviment
		    if (stageoverlay_notemoviment != NULL)
			{
			stageoverlay_notemoviment();
			}
			
			//Tick stage
			if (stageoverlay_tick != NULL)
				stageoverlay_tick();
			
			//Handle bump
			if ((stage.bump = FIXED_UNIT + FIXED_MUL(stage.bump - FIXED_UNIT, FIXED_DEC(95,100))) <= FIXED_DEC(1003,1000))
				stage.bump = FIXED_UNIT;
			stage.sbump = FIXED_UNIT + FIXED_MUL(stage.sbump - FIXED_UNIT, FIXED_DEC(60,100));
			
			if (playing && (stage.flag & STAGE_FLAG_JUST_STEP))
			{
				//Check if screen should bump
				boolean is_bump_step = (stage.song_step & 0xF) == 0;
				
				//M.I.L.F bumps
				if (stage.stage_id == StageId_4_3 && stage.song_step >= (168 << 2) && stage.song_step < (200 << 2))
					is_bump_step = (stage.song_step & 0x3) == 0;
				
				//Bump screen
				if (is_bump_step)
					stage.bump = FIXED_DEC(103,100);
				
				//Bump health every 4 steps
				if ((stage.song_step & 0x3) == 0)
					stage.sbump = FIXED_DEC(103,100);
			}
			
			//Scroll camera
			if (stage.cur_section->flag & SECTION_FLAG_OPPFOCUS)
				Stage_FocusCharacter(stage.opponent, FIXED_UNIT / 24);
			else
				Stage_FocusCharacter(stage.player, FIXED_UNIT / 24);
			Stage_ScrollCamera();
			
			switch (stage.mode)
			{
				case StageMode_Normal:
				case StageMode_Swap:
				{
					//Handle player 1 inputs
					Stage_ProcessPlayer(&stage.player_state[0], &pad_state, playing);
					
					//Handle opponent notes
					u8 opponent_anote = CharAnim_Idle;
					u8 opponent_snote = CharAnim_Idle;
					
					for (Note *note = stage.cur_note;; note++)
					{
						if (note->pos > (stage.note_scroll >> FIXED_SHIFT))
							break;
						
						//Opponent note hits
						if (playing && ((note->type ^ stage.note_swap) & NOTE_FLAG_OPPONENT) && !(note->type & NOTE_FLAG_HIT))
						{

							//Opponent hits note
							stage.player_state[1].arrow_hitan[note->type & 0x3] = stage.step_time;
							Stage_StartVocal();
							if (stage.player_state[1].character->ignoreanim != true)
							{
							if (note->type & NOTE_FLAG_SUSTAIN)
								opponent_snote = note_anims[note->type & 0x3][(note->type & NOTE_FLAG_ALT_ANIM) != 0];
							else
								opponent_anote = note_anims[note->type & 0x3][(note->type & NOTE_FLAG_ALT_ANIM) != 0];
							note->type |= NOTE_FLAG_HIT;
						}
					}
				}
					
					if (opponent_anote != CharAnim_Idle)
						stage.player_state[1].character->set_anim(stage.player_state[1].character, opponent_anote);
					else if (opponent_snote != CharAnim_Idle)
						stage.player_state[1].character->set_anim(stage.player_state[1].character, opponent_snote);
					break;
				}
				case StageMode_2P:
				{
					//Handle player 1 and 2 inputs
					Stage_ProcessPlayer(&stage.player_state[0], &pad_state, playing);
					Stage_ProcessPlayer(&stage.player_state[1], &pad_state_2, playing);
					break;
				}
			}
			
			//Tick note splashes
			ObjectList_Tick(&stage.objlist_splash);
			
			//Draw stage notes
			Stage_DrawNotes();
			
			//Draw note HUD
			RECT note_src = {0, 0, 32, 32};
			RECT_FIXED note_dst = {0, 0, FIXED_DEC(32,1), FIXED_DEC(32,1)};
			
			for (u8 i = 0; i < 4; i++)
			{
				//BF
				note_dst.x = stage.note_x[i] - FIXED_DEC(16,1);
				note_dst.y = stage.note_y[i] - FIXED_DEC(16,1);
				if (stage.downscroll)
				note_dst.y = -note_dst.y - note_dst.h;
				Stage_DrawStrum(i, &note_src, &note_dst);
				Stage_DrawTex(&stage.tex_hud0, &note_src, &note_dst, stage.bump);
				
				//Opponent
				note_dst.x = stage.note_x[i | NOTE_FLAG_OPPONENT] - FIXED_DEC(16,1);
				note_dst.y = stage.note_y[i | NOTE_FLAG_OPPONENT] - FIXED_DEC(16,1);
				if (stage.downscroll)
				note_dst.y = -note_dst.y - note_dst.h;
				Stage_DrawStrum(i | 4, &note_src, &note_dst);
				Stage_DrawTex(&stage.tex_hud0, &note_src, &note_dst, stage.bump);
			}
			
			//Draw score
			for (int i = 0; i < ((stage.mode >= StageMode_2P) ? 2 : 1); i++)
			{
				PlayerState *this = &stage.player_state[i];
				
				//Get string representing number
				if (this->refresh_score)
				{
					if (this->score != 0)
						sprintf(this->score_text, "%d0", this->score * stage.max_score / this->max_score);
					else
						strcpy(this->score_text, "0");
					this->refresh_score = false;
				}
				
				//Display score
				RECT score_src = {80, 224, 40, 10};
				RECT_FIXED score_dst = {(i ^ (stage.mode == StageMode_Swap)) ? FIXED_DEC(-100,1) : FIXED_DEC(-150,1), (SCREEN_HEIGHT2 - 22) << FIXED_SHIFT, FIXED_DEC(40,1), FIXED_DEC(10,1)};
				if (stage.downscroll)
					score_dst.y = FIXED_DEC(-87,1);
				
				Stage_DrawTex(&stage.tex_hud0, &score_src, &score_dst, stage.bump);
				
				//Draw number
				score_src.y = 240;
				score_src.w = 8;
				score_dst.x += FIXED_DEC(40,1);
				score_dst.w = FIXED_DEC(8,1);
				
				for (const char *p = this->score_text; ; p++)
				{
					//Get character
					char c = *p;
					if (c == '\0')
						break;
					
					//Draw character
					if (c == '-')
						score_src.x = 160;
					else //Should be a number
						score_src.x = 80 + ((c - '0') << 3);
					
					Stage_DrawTex(&stage.tex_hud0, &score_src, &score_dst, stage.bump);
					
					//Move character right
					score_dst.x += FIXED_DEC(7,1);
				}
			}

			//Draw Combo Break
			for (int i = 0; i < ((stage.mode >= StageMode_2P) ? 2 : 1); i++)
			{
				PlayerState *this = &stage.player_state[i];
				
				//Get string representing number
				if (this->refresh_miss)
				{
					if (this->miss != 0)
						sprintf(this->miss_text, "%d", this->miss);
					else
						strcpy(this->miss_text, "0");
					this->refresh_miss = false;
				}
				
				//Display miss
				RECT miss_src = {163, 155, 36, 9};
				RECT_FIXED miss_dst = {FIXED_DEC(-60,1), (SCREEN_HEIGHT2 - 22) << FIXED_SHIFT, FIXED_DEC(36,1), FIXED_DEC(9,1)};
				if (stage.downscroll)
					miss_dst.y = FIXED_DEC(-87,1);
				
				RECT slash_src = {163, 224, 3, 13};
				RECT_FIXED slash_dst = {FIXED_DEC(-64,1), miss_dst.y - FIXED_DEC(2,1), FIXED_DEC(3,1), FIXED_DEC(13,1)};
				Stage_DrawTex(&stage.tex_huds, &slash_src, &slash_dst, stage.bump);
				
				Stage_DrawTex(&stage.tex_huds, &miss_src, &miss_dst, stage.bump);
				
				//Draw number
				miss_src.y = 240;
				miss_src.w = 8;
				miss_dst.x += FIXED_DEC(45,1);
				miss_dst.w = FIXED_DEC(8,1);
				
				for (const char *p = this->miss_text; ; p++)
				{
					//Get character
					char c = *p;
					if (c == '\0')
						break;
					
					//Draw character
					miss_src.x = 80 + ((c - '0') << 3);
					
					Stage_DrawTex(&stage.tex_huds, &miss_src, &miss_dst, stage.bump);
					
					//Move character right
					miss_dst.x += FIXED_DEC(7,1);
				}
			}
				
			//Draw Accuracy
			for (int i = 0; i < ((stage.mode >= StageMode_2P) ? 2 : 1); i++)
			{
				PlayerState *this = &stage.player_state[i];
				
				this->accuracy = (this->min_accuracy * 100) / (this->max_accuracy);
				
				//Get string representing number
				if (this->refresh_accuracy)
				{
					if (this->accuracy != 0)
						sprintf(this->accuracy_text, "%d", this->accuracy);
					else
						strcpy(this->accuracy_text, "0");
					this->refresh_accuracy = false;
				}
				
				//Display accurate
				RECT accuracy_src = {199, 155, 51, 9};
				RECT_FIXED accuracy_dst = {FIXED_DEC(14,1), (SCREEN_HEIGHT2 - 22) << FIXED_SHIFT, FIXED_DEC(51,1), FIXED_DEC(9,1)};
				if (stage.downscroll)
					accuracy_dst.y = FIXED_DEC(-87,1);
				
				RECT slash_src = {163, 224, 3, 13};
				RECT_FIXED slash_dst = {FIXED_DEC(10,1), accuracy_dst.y - FIXED_DEC(2,1), FIXED_DEC(3,1), FIXED_DEC(13,1)};

				Stage_DrawTex(&stage.tex_huds, &slash_src, &slash_dst, stage.bump);
				
				RECT accur_src = {138, 223, 9, 11};
				u8 accura;
				if (this->accuracy == 100)
					accura = 92;
				else if (this->accuracy > 10)
					accura = 85;
				else
					accura = 77;
				
				RECT_FIXED accur_dst = {FIXED_DEC(accura,1), accuracy_dst.y - FIXED_DEC(1,1), FIXED_DEC(9,1), FIXED_DEC(11,1)};
				Stage_DrawTex(&stage.tex_huds, &accur_src, &accur_dst, stage.bump);

				Stage_DrawTex(&stage.tex_huds, &accuracy_src, &accuracy_dst, stage.bump);
				
				//Draw number
				accuracy_src.y = 240;
				accuracy_src.w = 8;
				accuracy_dst.x += FIXED_DEC(56,1);
				accuracy_dst.w = FIXED_DEC(8,1);
				
				for (const char *p = this->accuracy_text; ; p++)
				{
					//Get character
					char c = *p;
					if (c == '\0')
						break;
					
				   //Draw character
					if (c == '-')
						accuracy_src.x = 160;
					else //Should be a number
						accuracy_src.x = 80 + ((c - '0') << 3);
					
					Stage_DrawTex(&stage.tex_huds, &accuracy_src, &accuracy_dst, stage.bump);
					
					//Move character right
					accuracy_dst.x += FIXED_DEC(7,1);
				}

				if (this->miss == 0)
				{		
				//Draw fc
				accuracy_src.x = 149;
				accuracy_src.y = 226;
				accuracy_src.w = 13;
				accuracy_src.h =  9;
				accuracy_dst.x += FIXED_DEC(16,1);
				accuracy_dst.w = FIXED_DEC(13,1);
				accuracy_dst.h = FIXED_DEC(9,1);

				Stage_DrawTex(&stage.tex_huds, &accuracy_src, &accuracy_dst, stage.bump);
				}
			}
			
			//Draw botplay
			if (stage.botplay)
			{
			RECT bot_fill = {172, 227, 67, 16};
			RECT_FIXED bot_dst = {FIXED_DEC(-33,1), FIXED_DEC(-60,1), FIXED_DEC(67,1), FIXED_DEC(16,1)};
				
			bot_dst.w = bot_fill.w << FIXED_SHIFT;
			if (animf_count & 20)
			Stage_DrawTex(&stage.tex_huds, &bot_fill, &bot_dst, stage.bump);
			}

			//make healthbar color player
			u8 barp_r = (stage.player->health_b >> 16) & 0xFF;
			u8 barp_g = (stage.player->health_b >>  8) & 0xFF;
			u8 barp_b = (stage.player->health_b >>  0) & 0xFF;
			
			//make healthbar color opponent
		    u8 baro_r = (stage.opponent->health_b >> 16) & 0xFF;
			u8 baro_g = (stage.opponent->health_b >>  8) & 0xFF;
		    u8 baro_b = (stage.opponent->health_b >>  0) & 0xFF;

			//health system for normal and swap mode
			if (stage.mode != StageMode_2P)
			{
				//Perform health checks
				if (stage.player_state[0].health <= 0)
				{
					//Player has died
					stage.player_state[0].health = 0;
					stage.state = StageState_Dead;
				}
				if (stage.player_state[0].health > 20000)
					stage.player_state[0].health = 20000;
				
				//Draw health heads
				Stage_DrawHealth(stage.player_state[0].health, stage.player_state[0].character->health_i,    1);
				Stage_DrawHealth(stage.player_state[0].health, stage.player_state[1].character->health_i, -1);
				
				//Draw health bar
				RECT health_fill = {0, 0, 255 - (255 * stage.player_state[0].health / 20000), 5};
				RECT health_back = {0, 7, 255, 5};
				RECT_FIXED health_dst = {FIXED_DEC(-128,1), (SCREEN_HEIGHT2 - 32) << FIXED_SHIFT, 0, FIXED_DEC(5,1)};
				if (stage.downscroll)
					health_dst.y = -health_dst.y - health_dst.h;

		        //draw healthbar and invert if it swap mode
				health_dst.w = health_fill.w << FIXED_SHIFT;
				Stage_DrawTexCol(&stage.tex_huds, &health_fill, &health_dst, stage.bump, baro_r >> 1, baro_g >> 1, baro_b >> 1);
				health_dst.w = health_back.w << FIXED_SHIFT;
				Stage_DrawTexCol(&stage.tex_huds, &health_back, &health_dst, stage.bump, barp_r >> 1, barp_g >> 1, barp_b >> 1);
			}

			//health system for multiplayer
			else
			{
				//Perform health checks
				if (stage.player_state[0].health <= 0)
				{
					//Player has died
					stage.player_state[0].health = 0;
					stage.state = StageState_Dead;
				}
				if (stage.player_state[0].health > 10000)
					stage.player_state[0].health = 10000;

				//Perform health checks
				if (stage.player_state[1].health <= 0)
				{
					//Opponent has died
					stage.player_state[1].health = 0;
					stage.state = StageState_Dead;
				}
				if (stage.player_state[1].health > 10000)
					stage.player_state[1].health = 10000;
				
				//Draw health heads
				Stage_DrawHealth(stage.player_state[0].health, stage.player_state[0].character->health_i,  1);
				Stage_DrawHealth(stage.player_state[1].health, stage.player_state[1].character->health_i, -1);

				
				//Draw health bar
				RECT health_fill = {33, 210, 128 - (128 * stage.player_state[1].health / 20000), 4};
				RECT health_back = {225, 210, 128 - (128 *  stage.player_state[0].health / 20000), 4};
				RECT health_border = {32, 209, 258, 6};

				//draw downscroll healthbar
			    if (stage.downscroll)
                 {
			      health_fill.y = health_back.y = 20;
                  health_border.y = 19;
			     }

		        //draw healthbar
				Gfx_DrawRect(&health_fill, baro_r, baro_g, baro_b);
				Gfx_DrawRect(&health_back, barp_r, barp_g, barp_b);
				Gfx_DrawRect(&health_border, 0,   0,  0);
			}

			FntPrint("step: %d", stage.song_step);
			
			//Tick foreground objects
			ObjectList_Tick(&stage.objlist_fg);
			
			//Draw stage foreground
			if (stageoverlay_drawfg != NULL)
				stageoverlay_drawfg();
			
			//Tick characters
			stage.player->tick(stage.player);
			stage.opponent->tick(stage.opponent);
			
			//Draw stage middle
			if (stageoverlay_drawmd != NULL)
				stageoverlay_drawmd();
			
			//Tick girlfriend
			if (stage.gf != NULL)
				stage.gf->tick(stage.gf);
			
			//Tick background objects
			ObjectList_Tick(&stage.objlist_bg);
			
			//Draw stage background
			if (stageoverlay_drawbg != NULL)
				stageoverlay_drawbg();
			break;
		}
		case StageState_Dead: //Start BREAK animation
		{
		    //Stop music immediately or start pixel gameover song
			if (stage.stage_id >= StageId_6_1 && stage.stage_id <= StageId_6_3)
			{
			Audio_LoadMus("\\MENU\\OVER_3.MUS;1");
	        Audio_PlayMus(true);
	        Audio_SetVolume(0, 0x3FFF, 0x0000);
	        Audio_SetVolume(1, 0x0000, 0x3FFF);
			}
			else
			Audio_StopMus();
	
	        //Free objects
	        ObjectList_Free(&stage.objlist_splash);
	        ObjectList_Free(&stage.objlist_fg);
	        ObjectList_Free(&stage.objlist_bg);
			
			//Free opponent and girlfriend
			Character_Free(stage.opponent);
			stage.opponent = NULL;
			Character_Free(stage.gf);
			stage.gf = NULL;

			//Free stage
	        if (stageoverlay_free != NULL)
		    stageoverlay_free();
	
			
			//Reset stage state
			stage.flag = 0;
			stage.bump = stage.sbump = FIXED_UNIT;
			
			//Change background colour to black
			Gfx_SetClear(0, 0, 0);
			
			//Run death animation, focus on player, and change state
			stage.player->set_anim(stage.player, PlayerAnim_Dead0);
			
			Stage_FocusCharacter(stage.player, 0);
			stage.song_time = 0;
			
			stage.state = StageState_DeadLoad;
		}
	//Fallthrough
		case StageState_DeadLoad:
		{
			//Scroll camera and tick player
			if (stage.song_time < FIXED_UNIT)
				stage.song_time += FIXED_UNIT / 60;
			stage.camera.td = FIXED_DEC(-2, 100) + FIXED_MUL(stage.song_time, FIXED_DEC(45, 1000));
			if (stage.camera.td > 0)
				Stage_ScrollCamera();
			stage.player->tick(stage.player);
			
			//Drop mic and change state if CD has finished reading and animation has ended
			if (stage.player->animatable.anim != PlayerAnim_Dead1)
				break;
			
			stage.player->set_anim(stage.player, PlayerAnim_Dead2);
			stage.camera.td = FIXED_DEC(25, 1000);
			stage.state = StageState_DeadDrop;
			break;
		}
		case StageState_DeadDrop:
		{
			//Scroll camera and tick player
			Stage_ScrollCamera();
			stage.player->tick(stage.player);
			
			//Enter next state once mic has been dropped
			if (stage.player->animatable.anim == PlayerAnim_Dead3)
			{
				stage.state = StageState_DeadRetry;
				if (stage.stage_id < StageId_6_1 || stage.stage_id > StageId_6_3)
				{
				Audio_LoadMus("\\MENU\\OVER_1.MUS;1");
	            Audio_PlayMus(true);
	            Audio_SetVolume(0, 0x3FFF, 0x0000);
	            Audio_SetVolume(1, 0x0000, 0x3FFF);
				}
			}
			break;
		}
		case StageState_DeadRetry:
		{
			//Randomly twitch
			if (stage.player->animatable.anim == PlayerAnim_Dead3)
			{
				if (RandomRange(0, 29) == 0)
					stage.player->set_anim(stage.player, PlayerAnim_Dead4);
				if (RandomRange(0, 29) == 0)
					stage.player->set_anim(stage.player, PlayerAnim_Dead5);
			}
			
			//Scroll camera and tick player
			Stage_ScrollCamera();
			stage.player->tick(stage.player);
			break;
		}
		default:
			break;
	}
}
