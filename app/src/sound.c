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
#define NOTE_F4  349u
#define NOTE_A4  440u
#define NOTE_C5  523u
#define NOTE_E5  659u
#define NOTE_G5  784u
#define NOTE_A5  880u
#define NOTE_B5  988u
#define NOTE_C6  1047u
#define NOTE_E6  1319u
#define NOTE_G6  1568u
#define NOTE_C7  2093u

static const note_t snd_startup[] = { {NOTE_C6, 90}, {REST, 30}, {NOTE_E6, 90}, {REST, 30}, {NOTE_G6, 120}, {REST, 30}, {NOTE_C7, 320} };
static const note_t snd_ack[] = { {NOTE_E6, 50}, {NOTE_G6, 50}, {NOTE_C7, 90} };
static const note_t snd_error[] = { {NOTE_A4, 200}, {REST, 60}, {NOTE_F4, 260} };
static const note_t snd_warning[] = { {NOTE_B5, 110}, {REST, 70}, {NOTE_B5, 110} };
static const note_t snd_sd_mount[] = { {NOTE_C6, 60}, {NOTE_G6, 90} };
static const note_t snd_sd_unmount[] = { {NOTE_G6, 60}, {NOTE_C6, 90} };

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
