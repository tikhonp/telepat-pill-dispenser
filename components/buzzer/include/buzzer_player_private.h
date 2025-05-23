#pragma once

#include "buzzer.h"

const static uint16_t b_notes[] = {
    0,    31,   33,   35,   37,   39,   41,   44,   46,   49,   52,   55,
    58,   62,   65,   69,   73,   78,   82,   87,   93,   98,   104,  110,
    117,  123,  131,  139,  147,  156,  165,  175,  185,  196,  208,  220,
    233,  247,  262,  277,  294,  311,  330,  349,  370,  392,  415,  440,
    466,  494,  523,  554,  587,  622,  659,  698,  740,  784,  831,  880,
    932,  988,  1047, 1109, 1175, 1245, 1319, 1397, 1480, 1568, 1661, 1760,
    1865, 1976, 2093, 2217, 2349, 2489, 2637, 2794, 2960, 3136, 3322, 3520,
    3729, 3951, 4186, 4435, 4699, 4978};

typedef enum {
    NOTE_EMPTY = 0,
    NOTE_A0,
    NOTE_AS0,
    NOTE_B0,
    NOTE_C1,
    NOTE_CS1,
    NOTE_D1,
    NOTE_DS1,
    NOTE_E1,
    NOTE_F1,
    NOTE_FS1,
    NOTE_G1,
    NOTE_GS1,
    NOTE_A1,
    NOTE_AS1,
    NOTE_B1,
    NOTE_C2,
    NOTE_CS2,
    NOTE_D2,
    NOTE_DS2,
    NOTE_E2,
    NOTE_F2,
    NOTE_FS2,
    NOTE_G2,
    NOTE_GS2,
    NOTE_A2,
    NOTE_AS2,
    NOTE_B2,
    NOTE_C3,
    NOTE_CS3,
    NOTE_D3,
    NOTE_DS3,
    NOTE_E3,
    NOTE_F3,
    NOTE_FS3,
    NOTE_G3,
    NOTE_GS3,
    NOTE_A3,
    NOTE_AS3,
    NOTE_B3,
    NOTE_C4,
    NOTE_CS4,
    NOTE_D4,
    NOTE_DS4,
    NOTE_E4,
    NOTE_F4,
    NOTE_FS4,
    NOTE_G4,
    NOTE_GS4,
    NOTE_A4,
    NOTE_AS4,
    NOTE_B4,
    NOTE_C5,
    NOTE_CS5,
    NOTE_D5,
    NOTE_DS5,
    NOTE_E5,
    NOTE_F5,
    NOTE_FS5,
    NOTE_G5,
    NOTE_GS5,
    NOTE_A5,
    NOTE_AS5,
    NOTE_B5,
    NOTE_C6,
    NOTE_CS6,
    NOTE_D6,
    NOTE_DS6,
    NOTE_E6,
    NOTE_F6,
    NOTE_FS6,
    NOTE_G6,
    NOTE_GS6,
    NOTE_A6,
    NOTE_AS6,
    NOTE_B6,
    NOTE_C7,
    NOTE_CS7,
    NOTE_D7,
    NOTE_DS7,
    NOTE_E7,
    NOTE_F7,
    NOTE_FS7,
    NOTE_G7,
    NOTE_GS7,
    NOTE_A7,
    NOTE_AS7,
    NOTE_B7,
    NOTE_C8,
} b_piano_note_t;

typedef struct {
    b_piano_note_t note;
    unsigned int duration_millis;
} b_sample_t;

#define DUR_FOURTH (600u) // millis
#define DUR_EIGHTH DUR_FOURTH / 2
#define DUR_SIXTEENTH DUR_EIGHTH / 2
#define DUR_THREE_SIXTEENTH DUR_SIXTEENTH * 3
#define DUR_THREE_EIGHTH DUR_EIGHTH * 3

static const b_sample_t _b_fatal_err[] = {
    {.note = NOTE_D1, .duration_millis = 1000},
};

static const b_sample_t _b_pill_not[] = {
    {.note = NOTE_GS4, .duration_millis = DUR_EIGHTH},

    {.note = NOTE_CS5, .duration_millis = DUR_EIGHTH},
    {.note = NOTE_CS5, .duration_millis = DUR_EIGHTH},
    {.note = NOTE_F5, .duration_millis = DUR_EIGHTH},
    {.note = NOTE_F5, .duration_millis = DUR_EIGHTH},

    {.note = NOTE_CS5, .duration_millis = DUR_FOURTH},
    {.note = NOTE_GS4, .duration_millis = DUR_EIGHTH},
    {.note = NOTE_EMPTY, .duration_millis = DUR_SIXTEENTH},
    {.note = NOTE_GS4, .duration_millis = DUR_SIXTEENTH},

    {.note = NOTE_GS4, .duration_millis = DUR_THREE_SIXTEENTH},
    {.note = NOTE_GS4, .duration_millis = DUR_SIXTEENTH},
    {.note = NOTE_DS5, .duration_millis = DUR_SIXTEENTH},
    {.note = NOTE_CS5, .duration_millis = DUR_SIXTEENTH},
    {.note = NOTE_C5, .duration_millis = DUR_SIXTEENTH},
    {.note = NOTE_AS4, .duration_millis = DUR_SIXTEENTH},

    {.note = NOTE_GS4, .duration_millis = DUR_FOURTH},
    {.note = NOTE_EMPTY, .duration_millis = DUR_EIGHTH},
    {.note = NOTE_GS4, .duration_millis = DUR_EIGHTH},

    {.note = NOTE_CS5, .duration_millis = DUR_THREE_SIXTEENTH},
    {.note = NOTE_CS5, .duration_millis = DUR_SIXTEENTH},
    {.note = NOTE_F5, .duration_millis = DUR_EIGHTH},
    {.note = NOTE_F5, .duration_millis = DUR_EIGHTH},

    {.note = NOTE_CS5, .duration_millis = DUR_FOURTH},
    {.note = NOTE_GS4, .duration_millis = DUR_EIGHTH},
    {.note = NOTE_CS5, .duration_millis = DUR_EIGHTH},

    {.note = NOTE_C5, .duration_millis = DUR_EIGHTH},
    {.note = NOTE_AS4, .duration_millis = DUR_SIXTEENTH},
    {.note = NOTE_C5, .duration_millis = DUR_SIXTEENTH},
    {.note = NOTE_CS5, .duration_millis = DUR_EIGHTH},
    {.note = NOTE_G4, .duration_millis = DUR_EIGHTH},

    {.note = NOTE_GS4, .duration_millis = DUR_FOURTH},
    {.note = NOTE_EMPTY, .duration_millis = DUR_EIGHTH},
    {.note = NOTE_GS4, .duration_millis = DUR_EIGHTH},

    {.note = NOTE_C5, .duration_millis = DUR_EIGHTH},
    {.note = NOTE_C5, .duration_millis = DUR_EIGHTH},
    {.note = NOTE_CS5, .duration_millis = DUR_SIXTEENTH},
    {.note = NOTE_C5, .duration_millis = DUR_SIXTEENTH},
    {.note = NOTE_AS4, .duration_millis = DUR_SIXTEENTH},
    {.note = NOTE_C5, .duration_millis = DUR_SIXTEENTH},

    {.note = NOTE_CS5, .duration_millis = DUR_FOURTH},
    {.note = NOTE_GS4, .duration_millis = DUR_EIGHTH},
    {.note = NOTE_CS5, .duration_millis = DUR_EIGHTH},

    {.note = NOTE_C5, .duration_millis = DUR_EIGHTH},
    {.note = NOTE_C5, .duration_millis = DUR_EIGHTH},
    {.note = NOTE_C5, .duration_millis = DUR_SIXTEENTH},
    {.note = NOTE_FS5, .duration_millis = DUR_SIXTEENTH},
    {.note = NOTE_CS5, .duration_millis = DUR_SIXTEENTH},
    {.note = NOTE_C5, .duration_millis = DUR_SIXTEENTH},

    {.note = NOTE_CS5, .duration_millis = DUR_THREE_EIGHTH},
    {.note = NOTE_CS5, .duration_millis = DUR_EIGHTH},

    {.note = NOTE_AS4, .duration_millis = DUR_EIGHTH},
    {.note = NOTE_AS4, .duration_millis = DUR_EIGHTH},
    {.note = NOTE_AS4, .duration_millis = DUR_EIGHTH},
    {.note = NOTE_CS5, .duration_millis = DUR_EIGHTH},

    {.note = NOTE_CS5, .duration_millis = DUR_FOURTH},
    {.note = NOTE_GS4, .duration_millis = DUR_EIGHTH},
    {.note = NOTE_GS4, .duration_millis = DUR_EIGHTH},

    {.note = NOTE_GS4, .duration_millis = DUR_THREE_SIXTEENTH},
    {.note = NOTE_GS4, .duration_millis = DUR_SIXTEENTH},
    {.note = NOTE_DS5, .duration_millis = DUR_EIGHTH},
    {.note = NOTE_C5, .duration_millis = DUR_EIGHTH},

    {.note = NOTE_CS5, .duration_millis = DUR_THREE_EIGHTH},
};

void b_play_notification_task(enum b_notification_t notification);
void b_buzzer_channel_init(void);
