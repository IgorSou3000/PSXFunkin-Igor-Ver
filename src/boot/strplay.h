/*
  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#ifndef PSXF_GUARD_STRPLAY_H
#define PSXF_GUARD_STRPLAY_H

#include "psx.h"
#include "stage.h"

//StrPlayer interface

void strDoPlayback(STRFILE *str);
void strCallback();
void strNextVlc(STRENV *strEnv);
void strSync(STRENV *strEnv, int mode);
u_long *strNext(STRENV *strEnv);
void strKickCD(CdlLOC *loc);


#endif