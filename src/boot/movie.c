

#include "stage.h"
#include "mem.h"
#include "psx.h"
#include "loadscr.h"
#include "movie.h"
#include "font.h"
#include "pad.h"
#include "main.h"
#include "audio.h"

#include <sys/types.h>
#include <libetc.h>
#include <libgte.h>
#include <libgpu.h>
#include <libcd.h>
// CODEC library
#include <libpress.h>

#include "strplay.c"

Movie movie;

STRFILE StrFile[] = {
	// File name	Resolution		Frame count
	{"\\STR\\UGH.STR;1", 320, 240, 180,}
};

void PlayMovie() {
    // Reset and initialize stuff
	Audio_StopMus();
	ResetCallback();
	CdInit();
	ResetGraph(0);
	SetGraphDebug(0);

    movie.playing = 1;
	
}

void Movie_Tick(void) 
{
    Gfx_SetClear(0, 0, 0);

        switch (movie.select)
        {
            case 0:
            PlayMovie();
            PlayStr(320, 240, 0, 0, &StrFile[0]);
            break;
        }

    if(strPlayDone == 1) 
    {
    Stage_LoadScr(movie.id, movie.diff, movie.story);
    }
}

