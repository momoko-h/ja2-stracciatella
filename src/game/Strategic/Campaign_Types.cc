#include <stdint.h>
#include "Campaign_Types.h"

bool IS_VALID_SECTOR(uint8_t const x, uint8_t const y)
{
  return 1 <= x && x <= 16 && 1 <= y && y <= 16;
}

//Function to convert sector coordinates (1-16,1-16) to 0-255
uint8_t SECTOR(uint8_t const x, uint8_t const y)
{
  Assert(IS_VALID_SECTOR(x, y));
  return (y - 1) * 16 + x - 1;
}

SECTORINFO& GetSectorInfo(uint8_t const x, uint8_t const y)
{
  return SectorInfo[SECTOR(x, y)];
}
