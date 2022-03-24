/*
  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifndef PSXF_GUARD_SAVE_H
#define PSXF_GUARD_SAVE_H

#include "io.h"
#include "gfx.h"
#include "pad.h"

#include "fixed.h"
#include "character.h"
#include "player.h"
#include "object.h"
#include "psx.h"

#include "network.h"

char *memoryCardRead(u32 len);
void memoryCardWrite(char *p_phrase, u32 len);
  
#endif
