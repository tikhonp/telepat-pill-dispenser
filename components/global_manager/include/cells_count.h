#pragma once

#include "sdkconfig.h"

#if defined(CONFIG_HW_2X2_V1)
#define CELLS_COUNT 4
#elif defined(CONFIG_HW_4X7_V1)
#define CELLS_COUNT 28
#endif
