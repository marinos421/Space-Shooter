#ifndef AUDIO_H
#define AUDIO_H

#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

#include <iostream>

class Audio {
private:
    ma_engine engine;
    ma_sound music; // Ειδικό αντικείμενο για τη μουσική
    bool musicInitialized;

public:
    Audio() {
        musicInitialized = false;
        ma_result result = ma_engine_init(NULL, &engine);
        if (result != MA_SUCCESS) {
            std::cout << "ERROR: Failed to initialize audio engine." << std::endl;
        }
    }

    ~Audio() {
        if (musicInitialized) {
            ma_sound_uninit(&music);
        }
        ma_engine_uninit(&engine);
    }

    // Για εφέ (Pew Pew, Boom) - Παίζει μια φορά
    void play(const char* filePath) {
        ma_engine_play_sound(&engine, filePath, NULL);
    }

    // Για Μουσική - Παίζει συνέχεια (Loop)
    void playMusic(const char* filePath) {
        // Αν έπαιζε άλλη μουσική πριν, σταμάτα την
        if (musicInitialized) {
            ma_sound_stop(&music);
            ma_sound_uninit(&music);
        }

        // Φόρτωση και ρυθμιση Loop
        ma_result result = ma_sound_init_from_file(&engine, filePath, 0, NULL, NULL, &music);
        if (result == MA_SUCCESS) {
            ma_sound_set_looping(&music, MA_TRUE); // <--- ΤΟ ΚΛΕΙΔΙ (LOOP)
            ma_sound_start(&music);
            musicInitialized = true;
        }
        else {
            std::cout << "Failed to load music: " << filePath << std::endl;
        }
    }
};

#endif