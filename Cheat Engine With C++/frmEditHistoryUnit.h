#pragma once
#include <cstdint>

boolean logWrites = false;

class TWriteLogEntry
{
public:
	int id;
	uintptr_t address; // Changed ptruint to uintptr_t
	uint32_t originalsize;
	uint8_t* originalbytes;
	uint32_t newsize;
	uint8_t* newbytes;

	~TWriteLogEntry() {}
};


void addWriteLogEntryToList(TWriteLogEntry* wle);
