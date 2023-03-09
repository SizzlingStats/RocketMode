
#pragma once

#include "../public/dt_common.h"

// Bit counts used to encode the information about a property.
#define PROPINFOBITS_NUMPROPS			10
#define PROPINFOBITS_TYPE				5
#define PROPINFOBITS_FLAGS				SPROP_NUMFLAGBITS_NETWORKED
#define PROPINFOBITS_STRINGBUFFERLEN	10
#define PROPINFOBITS_NUMBITS			7
#define PROPINFOBITS_RIGHTSHIFT			6
#define PROPINFOBITS_NUMELEMENTS		10	// For arrays.
