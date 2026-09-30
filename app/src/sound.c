/*
 * Author: Mateusz Kluczka
 * Organization: PWr in Space
 * Date: 26.09.2026
 */
#include "sound.h"
#include "buzzer.h"
#include "tim.h"
#include "FreeRTOS.h"
#include "task.h"
#include "cmsis_os2.h"
#include <stdbool.h>
#include <stddef.h>

// TIM2 counter clock after PSC=63 (64 MHz / 64).
#define BUZZER_TIMER_HZ 1000000u

typedef struct {
    uint16_t freq_hz;
    uint16_t ms;
} note_t;

#define REST     0u
#define NOTE_C4  262u
#define NOTE_DS4 311u   // D#4
#define NOTE_F4  349u
#define NOTE_G4  392u
#define NOTE_FS4 370u   // F#4
#define NOTE_GS4 415u   // G#4
#define NOTE_A4  440u
#define NOTE_AS4 466u   // A#4
#define NOTE_B4  494u
#define NOTE_C5  523u
#define NOTE_CS5 554u
#define NOTE_D5  587u
#define NOTE_DS5 622u   // D#5
#define NOTE_E5  659u
// #define NOTE_F5  698u
#define NOTE_FS5 740u   // F#5
#define NOTE_G5  784u
#define NOTE_GS5 831u   // G#5
#define NOTE_A5  880u
#define NOTE_AS5 932u   // A#5
#define NOTE_B5  988u
#define NOTE_C6  1047u
#define NOTE_CS6 1109u  // C#6
#define NOTE_D6  1175u
// #define NOTE_DS6 1245u // D#6
#define NOTE_E6  1319u
#define NOTE_FS6 1480u  // F#6
#define NOTE_G6  1568u
// #define NOTE_GS6 1661u // G#6
#define NOTE_A6  1760u
// #define NOTE_AS6 1865u // A#6
#define NOTE_C7  2093u

static const note_t snd_startup[] = { {NOTE_C6, 90}, {REST, 30}, {NOTE_E6, 90}, {REST, 30}, {NOTE_G6, 120}, {REST, 30}, {NOTE_C7, 320} };
static const note_t snd_ack[] = { {NOTE_E6, 50}, {NOTE_G6, 50}, {NOTE_C7, 90} };
static const note_t snd_error[] = { {NOTE_A4, 200}, {REST, 60}, {NOTE_F4, 260} };
static const note_t snd_warning[] = { {NOTE_B5, 110}, {REST, 70}, {NOTE_B5, 110} };
static const note_t snd_sd_mount[] = { {NOTE_C6, 60}, {NOTE_G6, 90} };
static const note_t snd_sd_unmount[] = { {NOTE_G6, 60}, {NOTE_C6, 90} };
static const note_t snd_bluegej[] = {
    {NOTE_B4, 105},  {REST, 15},
    {NOTE_E5, 105},  {REST, 15},
    {NOTE_B4, 105},  {REST, 15},
    {NOTE_FS5, 105},
    {REST, 210},
    {NOTE_E5, 105},
    {NOTE_B5, 210},
    {NOTE_B5, 105}
};
static const note_t snd_rickroll[] = {
    {NOTE_A5, 150},  // Ne-
    {NOTE_B5, 150},  // -ver
    {NOTE_D6, 150},  // gon-
    {NOTE_B5, 150},  // -na
    {NOTE_FS6, 360}, // give
    {REST, 30},
    {NOTE_FS6, 360}, // you
    {NOTE_E6, 720},  // up
    {REST, 220},

    {NOTE_A5, 150},   // Ne-
    {NOTE_B5, 150},  // -ver
    {NOTE_D6, 150},  // gon-
    {NOTE_B5, 150},  // -na
    {NOTE_E6, 360},  // let
    {REST, 30},
    {NOTE_E6, 360},  // you
    {NOTE_D6, 300},  // do-
    {NOTE_B5, 700},  // -wn
    {REST, 160},

    {NOTE_A5, 150},  // Ne-
    {NOTE_B5, 150},  // -ver
    {NOTE_D6, 150},  // gon-
    {NOTE_B5, 150},  // -na
    {NOTE_D6, 300},  // run
    {NOTE_E6, 250},  // a-
    {NOTE_CS6, 420}, // -round
    {NOTE_A5, 280},  // and
    {NOTE_A5, 280},  // de-
    {NOTE_E6, 450},  // -sert
    {NOTE_D6, 900},  // you
    {REST, 220},

    {NOTE_A5, 150},  // Ne-
    {NOTE_B5, 150},  // -ver
    {NOTE_D6, 150},  // gon-
    {NOTE_B5, 150},  // -na
    {NOTE_FS6, 360}, // make
    {REST, 30},
    {NOTE_FS6, 360}, // you
    {NOTE_E6, 720},  // cry
    {REST, 220},

    {NOTE_A5, 150},  // Ne-
    {NOTE_B5, 150},  // -ver
    {NOTE_D6, 150},  // gon-
    {NOTE_B5, 150},  // -na
    {NOTE_A6, 360},  // say
    {NOTE_FS6, 360}, // good-
    {NOTE_D6, 720},  // -bye
    {REST, 220},

    {NOTE_A5, 150},  // Ne-
    {NOTE_B5, 150},  // -ver
    {NOTE_D6, 150},  // gon-
    {NOTE_B5, 150},  // -na
    {NOTE_D6, 340},  // tell
    {NOTE_E6, 150},  // a
    {NOTE_CS6, 420}, // lie
    {NOTE_A5, 200},  // and
    {NOTE_A5, 180},  // hurt
    {NOTE_E6, 360},  // -t
    {NOTE_D6, 850}   // you!
};
static const note_t snd_x_gon_give_it[] = {
    {NOTE_AS4, 310}, {REST, 40},
    {NOTE_AS4, 310}, {REST, 40},
    {NOTE_AS5, 140}, {REST, 30},
    {NOTE_AS5, 140}, {REST, 30},
    {NOTE_AS5, 140}, {REST, 30},
    {NOTE_AS5, 140}, {REST, 30},

    {NOTE_GS4, 310}, {REST, 40},
    {NOTE_GS4, 310}, {REST, 40},
    {NOTE_GS5, 140}, {REST, 30},
    {NOTE_GS5, 140}, {REST, 30},
    {NOTE_GS5, 140}, {REST, 30},
    {NOTE_GS5, 140}, {REST, 30},

    {NOTE_FS4, 310}, {REST, 40},
    {NOTE_FS4, 310}, {REST, 40},
    {NOTE_FS5, 140}, {REST, 30},
    {NOTE_FS5, 140}, {REST, 30},
    {NOTE_FS5, 140}, {REST, 30},
    {NOTE_FS5, 140}, {REST, 30},

    {NOTE_DS4, 310}, {REST, 40},
    {NOTE_DS4, 310}, {REST, 40},
    {NOTE_DS5, 140}, {REST, 30},
    {NOTE_DS5, 140}, {REST, 30},
    {NOTE_DS5, 140}, {REST, 30},
    {NOTE_DS5, 140}, {REST, 30}
};
static const note_t snd_doom_e1m1[] = {
    {NOTE_C4, 70}, {REST, 70}, {NOTE_C4, 140}, {NOTE_C5, 140},
    {NOTE_C4, 70}, {REST, 70}, {NOTE_C4, 140}, {NOTE_AS4, 140},
    {NOTE_C4, 70}, {REST, 70}, {NOTE_C4, 140}, {NOTE_GS4, 140},
    {NOTE_C4, 70}, {REST, 70}, {NOTE_C4, 140}, {NOTE_FS4, 140},
    {NOTE_C4, 70}, {REST, 70}, {NOTE_C4, 140}, {NOTE_G4, 140}, {NOTE_GS4, 140},
    
    {NOTE_C4, 70}, {REST, 70}, {NOTE_C4, 140}, {NOTE_C5, 140},
    {NOTE_C4, 70}, {REST, 70}, {NOTE_C4, 140}, {NOTE_AS4, 140},
    {NOTE_C4, 70}, {REST, 70}, {NOTE_C4, 140}, {NOTE_GS4, 140},
    {NOTE_C4, 70}, {REST, 70}, {NOTE_C4, 140}, {NOTE_FS4, 570}
};
static const note_t snd_brainpower[] = {
    {NOTE_E5, 88}, {REST, 88},
    {NOTE_FS5, 88}, {REST, 88},
    {NOTE_FS5, 44}, {REST, 44}, {NOTE_FS5, 44}, {REST, 44},
    {NOTE_FS5, 44}, {REST, 44}, {NOTE_FS5, 44}, {REST, 44},
    {NOTE_FS5, 44}, {REST, 44}, {NOTE_FS5, 44}, {REST, 44},
    {NOTE_FS5, 44}, {REST, 44}, {NOTE_FS5, 44}, {REST, 44},
    {NOTE_FS5, 44}, {REST, 44}, {NOTE_FS5, 44}, {REST, 44},
    {NOTE_FS5, 44}, {REST, 44}, {NOTE_FS5, 44}, {REST, 44},
    {NOTE_E5, 44}, {REST, 44}, {NOTE_E5, 44}, {REST, 44},
    {NOTE_E5, 44}, {REST, 44}, {NOTE_E5, 44}, {REST, 44},
    {NOTE_B4, 88}, {REST, 88},
    {NOTE_CS5, 88}, {REST, 88},
    {NOTE_A4, 88}, {REST, 88},
    {NOTE_FS5, 88}, {REST, 88},
    {NOTE_FS5, 88}, {REST, 88},
    {NOTE_E5, 88}, {REST, 88},
    {NOTE_FS5, 44}, {REST, 44}, {NOTE_FS5, 44}, {REST, 44},
    {NOTE_FS5, 44}, {REST, 44}, {NOTE_FS5, 44}, {REST, 44},
    {NOTE_FS5, 44}, {REST, 44}, {NOTE_FS5, 44}, {REST, 44},
    {NOTE_FS5, 44}, {REST, 44}, {NOTE_FS5, 44}, {REST, 44},
    {NOTE_FS5, 44}, {REST, 44}, {NOTE_FS5, 44}, {REST, 44},
    {NOTE_FS5, 44}, {REST, 44}, {NOTE_FS5, 44}, {REST, 44},
    {NOTE_D5, 44}, {REST, 44}, {NOTE_D5, 44}, {REST, 44},
    {NOTE_D5, 44}, {REST, 44}, {NOTE_D5, 44}, {REST, 44},
    {NOTE_FS5, 88}, {REST, 88},
    {NOTE_A5, 88}, {REST, 88},
    {NOTE_GS5, 88}, {REST, 88},
    {NOTE_A5, 88}, {REST, 88},
    {NOTE_GS5, 88}, {REST, 88},
    {NOTE_CS5, 88}
};

typedef struct {
    const note_t *seq;
    uint16_t len;
} melody_t;

#define MELODY(arr) { (arr), (uint16_t)(sizeof(arr) / sizeof((arr)[0])) }

static const melody_t g_melodies[SOUND_COUNT] = {
    [SOUND_STARTUP] = MELODY(snd_startup),
    [SOUND_ACK] = MELODY(snd_ack),
    [SOUND_ERROR] = MELODY(snd_error),
    [SOUND_WARNING] = MELODY(snd_warning),
    [SOUND_SD_MOUNT] = MELODY(snd_sd_mount),
    [SOUND_SD_UNMOUNT] = MELODY(snd_sd_unmount),
    [SOUND_BLUEGEJ] = MELODY(snd_bluegej),
    [SOUND_RICKROLL] = MELODY(snd_rickroll),
    [SOUND_X_GON_GIVE_IT_TO_YA] = MELODY(snd_x_gon_give_it),
    [SOUND_DOOM_E1M1] = MELODY(snd_doom_e1m1),
    [SOUND_BRAINPOWER] = MELODY(snd_brainpower)
};

static buzzer_t g_buzzer;
static osTimerId_t g_timer;
static const note_t *g_seq;
static uint16_t g_len;
static uint16_t g_idx;

static void snd_cb(void *arg) {
    (void)arg;
    note_t n;
    bool play;

    taskENTER_CRITICAL();
    play = (g_seq != NULL) && (g_idx < g_len);
    if (play) n = g_seq[g_idx++];
    taskEXIT_CRITICAL();

    if (!play) {
        buzzer_mute(&g_buzzer);
        return;
    }

    if (n.freq_hz != 0u) buzzer_set_tone(&g_buzzer, n.freq_hz);
    else buzzer_mute(&g_buzzer);

    osTimerStart(g_timer, pdMS_TO_TICKS(n.ms != 0u ? n.ms : 1u));
}

void sound_init(void) {
    const buzzer_t cfg = {
        .htim = &htim2,
        .channel = TIM_CHANNEL_1,
        .timer_hz = BUZZER_TIMER_HZ,
    };
    buzzer_init(&g_buzzer, &cfg);

    if (g_timer == NULL) {
        g_timer = osTimerNew(snd_cb, osTimerOnce, NULL, NULL);
    }
}

void sound_play(sound_id_t id) {
    if (id >= SOUND_COUNT || g_timer == NULL) return;

    const melody_t *m = &g_melodies[id];
    if (m->seq == NULL || m->len == 0u) return;

    taskENTER_CRITICAL();
    g_seq = m->seq;
    g_len = m->len;
    g_idx = 0u;
    taskEXIT_CRITICAL();

    osTimerStart(g_timer, 1u);
}

void sound_stop(void) {
    taskENTER_CRITICAL();
    g_seq = NULL;
    g_len = 0u;
    g_idx = 0u;
    taskEXIT_CRITICAL();

    if (g_timer != NULL) osTimerStop(g_timer);
    buzzer_mute(&g_buzzer);
}
