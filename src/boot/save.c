/*
  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/
#include "save.h"

#include "mem.h"
#include "timer.h"
#include "audio.h"
#include "pad.h"
#include "main.h"
#include "random.h"
#include "movie.h"
#include "psx.h"

#include "menu/menu.h"
#include "trans.h"
#include "loadscr.h"

#include <libds.h>
#include <libsio.h>
#include <libpad.h>
#include <libetc.h>
#include <libgs.h>
#include <libgte.h>
#include <libmcrd.h>
#include <libapi.h>
#include <sys/file.h>
#include <stdlib.h>

//read data from a memory card
//BUG card sync seems to not work correcly
char *memoryCardRead(u32 len)
{
  long cmds;
  long result;
  char *phrase = NULL;
  
  PadStopCom();
  
  phrase = malloc(len);
    
  MemCardInit(1);

  MemCardStart();

  if(MemCardSync(0, &cmds, &result) <= 0)
  {
    printf("\nSync Failed\n");
  }

  MemCardAccept(0);

  if(MemCardSync(0, &cmds, &result) <= 0)
  {
    printf("\nSync Failed\n");
  }

  if(MemCardOpen(0, "test", O_RDONLY) != 0)
  {
    printf("\nOpen Issue\n");
  }

  if(MemCardReadData((unsigned long *)phrase, 0, len)  != 0)
  {
    printf("\nRead Issue\n");
  }

  if(MemCardSync(0, &cmds, &result) <= 0)
  {
    printf("\nSync Failed\n");
  }

  MemCardClose();

  MemCardStop();
  
  PadStartCom();
  
  return phrase;
}

//write data to card
//BUG card sync seems to not work correcly
void memoryCardWrite(char *p_phrase, u32 len)
{
  long cmds;
  long result;
  
  PadStopCom();
    
  MemCardInit(1);

  MemCardStart();

  if(MemCardSync(0, &cmds, &result) <= 0)
  {
    printf("\nSync Failed\n");
  }

  MemCardAccept(0);

  if(MemCardSync(0, &cmds, &result) <= 0)
  {
    printf("\nSync Failed\n");
  }

  if(MemCardOpen(0, "test", O_WRONLY) != 0)
  {
    printf("\nOpen Issue\n");
  }

  if(MemCardWriteData((unsigned long *)p_phrase, 0, len)  != 0)
  {
    printf("\nWrite Issue\n");
  }

  if(MemCardSync(0, &cmds, &result) <= 0)
  {
    printf("\nSync Failed\n");
  }

  MemCardClose();

  MemCardStop();
  
  PadStartCom();
}

