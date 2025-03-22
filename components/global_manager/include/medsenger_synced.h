#pragma once

#include <stdbool.h>

/*
Get MEDSENGER_SYNCED varible thread-safe

MEDSENGER_SYNCED is true if there was successfull process of connecting
    to network and fetching schedule data from Medsenger.
*/
bool gm_get_medsenger_synced(void);

/*
set MEDSENGER_SYNCED varible thread-safe

MEDSENGER_SYNCED is true if there was successfull process of connecting
    to network and fetching schedule data from Medsenger.
*/
void gm_set_medsenger_synced(bool value);
