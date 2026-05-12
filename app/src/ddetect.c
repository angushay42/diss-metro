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
float ddetect_get_probability(short *samples, size_t n) {

}

/** TODO: Unsure if this will work... */
/* Detect a note from a given set of samples. Returns 0 if successful. */
 extern bool ddetect_detect_note(uint64_t *note_stamp, bool *ans, short *samples, size_t n) {
    /**
     * Plan:
     * take n samples of guitar input
     * get a probability for if a note was played (like 0.8 is 80% certain its a note)
     * if probability is above some threshold *AND* the timestamp of this note probability
     * is within _another_ threshold, then we will say the user played a note correctly. 
     */
}
