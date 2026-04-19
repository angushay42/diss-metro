#ifndef TEST_H
#define TEST_H

#include "common-defines.h"
#include "stdlib.h"
#include "stdio.h"
#include "time.h"
#ifdef TESTING
#include <sys/time.h>
#endif

#include "dringbuffer.h"
#include "dfft.h"

#define MAX_BPM     (220)
#define MIN_BPM     (40)

#endif // TEST_H