/*
 * Author: Mateusz Kluczka
 * Organization: PWr in Space
 * Date: 26.09.2026
 */
#ifndef SOUND_H
#define SOUND_H

typedef enum {
    SOUND_STARTUP = 0,
    SOUND_ACK,
    SOUND_ERROR,
    SOUND_WARNING,
    SOUND_SD_MOUNT,
    SOUND_SD_UNMOUNT,
    SOUND_BLUEGEJ,
    SOUND_RICKROLL,
    SOUND_X_GON_GIVE_IT_TO_YA,
    SOUND_DOOM_E1M1,
    SOUND_BRAINPOWER,
    SOUND_COUNT
} sound_id_t;

void sound_init(void);

void sound_play(sound_id_t id);
void sound_stop(void);

#endif /* SOUND_H */
