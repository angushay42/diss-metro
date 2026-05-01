#include "ddetect.h"

/* adjust how sensitive detection is. */
uint32_t threshold;
uint32_t average;
/** LSB = (2 * V_Ref) / 8192
 * from DS21700E-page 12 
 */
double LSB;




error_t ddettect_setup(void) {
    threshold = (uint32_t) 350;
    average = 0;
    LSB = 2.0 * 2.5 / 8192.0;

    return OK;
}
/* 
if a value from adc is higher than some threshold, we can consider that a played note.
*/



int detect(short sample) {
    short last = -2303;

    if ((sample - last) > threshold)
        return 1;
    return 0;

}
