#ifndef MERCPORTRAIT_H
#define MERCPORTRAIT_H

#include <memory>
#include "JA2Types.h"

std::unique_ptr<SGPVObject> Load33Portrait(MERCPROFILESTRUCT const&);
SGPVObject* Load65Portrait(MERCPROFILESTRUCT const&);
SGPVObject* LoadBigPortrait(MERCPROFILESTRUCT const&);
SGPVObject* LoadSmallPortrait(MERCPROFILESTRUCT const&);

#endif
