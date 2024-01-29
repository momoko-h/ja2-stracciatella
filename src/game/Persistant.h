#pragma once

// This file introduces the "Persistant" concept for those data types
// that cannot be load or saved with a single read() or write() call
// but must be read or written member-by-member.

// For these types two template functions Load() and Save() take a
// DataReader or DataWriter respectively should be implemented.
// These functions will then be called by the Load() and Save()
// functions that take a SGPFile.

// Such types must specify their size on disk below with the
// PERSISTANTSIZE macro.

#include "Debug.h"
#include "LoadSaveData.h"
#include "SGPFile.h"

// All types without a specialization of this template will not
// satisfy the requirements of th Persistant concept.
template<typename T>
inline constexpr unsigned PersistantSize = 0;

#define PERSISTANTSIZE(NAME, SIZE) struct NAME; template<> inline constexpr unsigned PersistantSize<NAME> = (SIZE);
PERSISTANTSIZE(BULLET, 128)
PERSISTANTSIZE(ROTTING_CORPSE_DEFINITION, 160)
PERSISTANTSIZE(SECTORINFO, 116)
#undef PERSISTANTSIZE


template<typename T>
concept Persistant = (PersistantSize<T> != 0);

void Load(DataReader & reader, Persistant auto & value);
void Save(DataWriter & writer, Persistant auto const& value);

// Adaptor function from SGPFile -> DataReader
template<Persistant T>
void Load(SGPFile * file, T & value)
{
	uint8_t buffer[PersistantSize<T>];
	file->read(buffer, PersistantSize<T>);
	DataReader dr{buffer};
	Load(dr, value);
	Assert(dr.getConsumed() == PersistantSize<T>);
}

// Adaptor function from SGPFile -> DataWriter
template<Persistant T>
void Save(SGPFile * file, T const& value)
{
	uint8_t buffer[PersistantSize<T>]{};
	DataWriter dw{buffer};
	Save(dw, value);
	Assert(dw.getConsumed() == PersistantSize<T>);
	file->write(buffer, PersistantSize<T>);
}
