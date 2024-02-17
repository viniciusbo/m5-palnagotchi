#include "M5Cardputer.h"

#define MOOD_BROKEN 19

// ASCII equivalent
const String palnagotchi_moods[] = {
    "(v__v)",   // 0 - sleeping
    "(=__=)",   // 1 - awakening
    "(O__O)",   // 2 - awake
    "( O_O)",   // 3 - observing (neutral) right
    "(O_O )",   // 4 - observig (neutral) left
    "( 0_0)",   // 5 - observing (happy) right
    "(0_0 )",   // 6 - observing (happy) left
    "(+__+)",   // 7 - intense
    "(,-@_@)",  // 8 - cool
    "(0__0)",   // 9 - happy
    "(^__^)",   // 10 - grateful
    "(a__a)",   // 11 - excited
    "(+__+)",   // 12 - smart
    "(*__*)",   // 13 - friendly
    "(@__@)",   // 14 - motivated
    "(>__<)",   // 15 - demotivated
    "(-__-)",   // 16 - bored
    "(T_T )",   // 17 - sad
    "(;__;)",   // 18 - lonely
    "(X__X)",   // 19 - broken
    "(#__#)",   // 20 - debugging,
    "8====D",   // 21 - ultra random easter egg
};

const String palnagotchi_moods_desc[] = {
    "Zzzz...",                               // 0 - sleeping
    "...",                                   // 1 - awakening
    "Hey, let's MAKE FRIENDS!",              // 2 - awake
    "WANTED: FRIENDS",                       // 3 - observing (neutral) right
    "WANTED: FRIENDS",                       // 4 - observig (neutral) left
    "Can we have even more friends?",        // 5 - observing (happy) right
    "Can we have even more friends?",        // 6 - observing (happy) left
    "YEAH! So many pwnagotchis!",            // 7 - intense
    "The coolest pal in the neighbourhood",  // 8 - cool
    "Can we have even more friends?",        // 9 - happy
    "I LOVE PWNAGOTCHIS!",                   // 10 - grateful
    "That's how I like it.",                 // 11 - excited
    "3.1415926535897932384626433832795",     // 12 - smart
    "HEY YOU! LETS BE FRIENDS!",             // 13 - friendly
    "IT RUNS! SUCK MA BALLZ!",               // 14 - motivated
    "Why my life sucks? WHY",                // 15 - demotivated
    "Seriously, let's go for a walk...",     // 16 - bored
    "Just don't...",                         // 17 - sad
    "Where are all the Pwnagotchis?",        // 18 - lonely
    "It works on my end.",                   // 19 - broken
    "Wtf? I didn't even touch it...",        // 20 - debugging,
    "What?",                                 // 21 - ultra random easter egg
};

void initUi();
void wakeUp();
void showMood(uint8_t mood, String phrase = "");
