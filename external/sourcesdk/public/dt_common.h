
#pragma once

#define SPROP_EXCLUDE			(1<<6)	// This is an exclude prop (not excludED, but it points at another prop to be excluded).

#define SPROP_NUMFLAGBITS_NETWORKED		16

enum SendPropType
{
    DPT_Int = 0,
    DPT_Float,
    DPT_Vector,
    DPT_VectorXY, // Only encodes the XY of a vector, ignores Z
    DPT_String,
    DPT_Array,	// An array of the base types (can't be of datatables).
    DPT_DataTable,
    DPT_NUMSendPropTypes
};
