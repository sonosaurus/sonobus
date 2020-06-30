

// All that we need to do is include the c-like functions defined in the other file
// N.b. I assume that RunningCumulant.c is set up so that for a default intel build,
// any inlines or other SPU specific instructions are not set.
#include "RunningCumulant.c"
