#ifndef SMACK_STUB_H
#define SMACK_STUB_H

#include "Types.h"

struct Smack;

Smack* SmackOpen(SGPFile* FileHandle);
void SmackDoFrame(Smack* Smk, UINT32 Left, UINT32 Top);
INT8 SmackNextFrame(Smack* Smk);
void SmackClose(Smack* Smk);

#endif
