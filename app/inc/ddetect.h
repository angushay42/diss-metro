#ifndef DDETECT_H
#define DDETECT_H

#include "common-defines.h"

error_t ddettect_setup(void);
extern bool ddetect_detect_note(uint64_t *note_stamp, bool *ans, short *samples, size_t n);

#endif  // DDETECT_H 