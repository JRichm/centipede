#pragma once

#include <cstdio>
#include <SDL3_mixer/SDL_mixer.h>

struct Audio {

    MIX_Mixer *mixer = nullptr;

    // loaded sounds
    MIX_Audio *shoot     = nullptr;
    MIX_Audio *kill      = nullptr;
    MIX_Audio *death     = nullptr;
    MIX_Audio *bonus     = nullptr;
    MIX_Audio *centipede = nullptr;
    MIX_Audio *spider    = nullptr;
    MIX_Audio *flea      = nullptr;
    MIX_Audio *scorpion  = nullptr;

    // all sounds on dedicated tracks for gain control
    MIX_Track *track_shoot      = nullptr;
    MIX_Track *track_kill       = nullptr;
    MIX_Track *track_death      = nullptr;
    MIX_Track *track_bonus      = nullptr;
    MIX_Track *track_centipede  = nullptr;
    MIX_Track *track_spider     = nullptr;
    MIX_Track *track_flea       = nullptr;
    MIX_Track *track_scorpion   = nullptr;

    // tune these to taste (1.0 = normal, 0.5 = half, 2.0 = double)
    static constexpr float GAIN_SHOOT     = 0.5f;
    static constexpr float GAIN_KILL      = 0.8f;
    static constexpr float GAIN_DEATH     = 1.0f;
    static constexpr float GAIN_BONUS     = 0.9f;
    static constexpr float GAIN_CENTIPEDE = 0.2f;
    static constexpr float GAIN_SPIDER    = 1.2f;
    static constexpr float GAIN_FLEA      = 0.8f;
    static constexpr float GAIN_SCORPION  = 0.1f;

    bool init() {
        if (!MIX_Init()) {
            fprintf(stderr, "MIX_Init error: %s\n", SDL_GetError());
            return false;
        }

        mixer = MIX_CreateMixerDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, nullptr);
        if (!mixer) {
            fprintf(stderr, "MIX_CreateMixerDevice error: %s\n", SDL_GetError());
            return false;
        }

        // load all sounds
        shoot     = MIX_LoadAudio(mixer, "assets/sounds/shoot.wav",     true);
        kill      = MIX_LoadAudio(mixer, "assets/sounds/kill.wav",      true);
        death     = MIX_LoadAudio(mixer, "assets/sounds/death.wav",     true);
        bonus     = MIX_LoadAudio(mixer, "assets/sounds/bonus.wav",     true);
        centipede = MIX_LoadAudio(mixer, "assets/sounds/centipede.wav", true);
        spider    = MIX_LoadAudio(mixer, "assets/sounds/spider.wav",    true);
        flea      = MIX_LoadAudio(mixer, "assets/sounds/flea.wav",      true);
        scorpion  = MIX_LoadAudio(mixer, "assets/sounds/scorpion.wav",  true);

        if (!shoot || !kill || !death || !bonus ||
            !centipede || !spider || !flea || !scorpion) {
            fprintf(stderr, "Error loading sounds: %s\n", SDL_GetError());
            return false;
        }

        // create all tracks
        track_shoot     = MIX_CreateTrack(mixer);
        track_kill      = MIX_CreateTrack(mixer);
        track_death     = MIX_CreateTrack(mixer);
        track_bonus     = MIX_CreateTrack(mixer);
        track_centipede = MIX_CreateTrack(mixer);
        track_spider    = MIX_CreateTrack(mixer);
        track_flea      = MIX_CreateTrack(mixer);
        track_scorpion  = MIX_CreateTrack(mixer);

        if (!track_shoot || !track_kill || !track_death || !track_bonus ||
            !track_centipede || !track_spider || !track_flea || !track_scorpion) {
            fprintf(stderr, "Error creating tracks: %s\n", SDL_GetError());
            return false;
        }

        // assign audio to tracks
        MIX_SetTrackAudio(track_shoot,     shoot);
        MIX_SetTrackAudio(track_kill,      kill);
        MIX_SetTrackAudio(track_death,     death);
        MIX_SetTrackAudio(track_bonus,     bonus);
        MIX_SetTrackAudio(track_centipede, centipede);
        MIX_SetTrackAudio(track_spider,    spider);
        MIX_SetTrackAudio(track_flea,      flea);
        MIX_SetTrackAudio(track_scorpion,  scorpion);

        // set looping tracks to loop forever
        MIX_SetTrackLoops(track_centipede, -1);
        MIX_SetTrackLoops(track_spider,    -1);
        MIX_SetTrackLoops(track_flea,      -1);
        MIX_SetTrackLoops(track_scorpion,  -1);

        // set gain for each track
        MIX_SetTrackGain(track_shoot,     GAIN_SHOOT);
        MIX_SetTrackGain(track_kill,      GAIN_KILL);
        MIX_SetTrackGain(track_death,     GAIN_DEATH);
        MIX_SetTrackGain(track_bonus,     GAIN_BONUS);
        MIX_SetTrackGain(track_centipede, GAIN_CENTIPEDE);
        MIX_SetTrackGain(track_spider,    GAIN_SPIDER);
        MIX_SetTrackGain(track_flea,      GAIN_FLEA);
        MIX_SetTrackGain(track_scorpion,  GAIN_SCORPION);

        return true;
    }

    void free() {
        MIX_StopAllTracks(mixer, 0);
        MIX_DestroyTrack(track_shoot);
        MIX_DestroyTrack(track_kill);
        MIX_DestroyTrack(track_death);
        MIX_DestroyTrack(track_bonus);
        MIX_DestroyTrack(track_centipede);
        MIX_DestroyTrack(track_spider);
        MIX_DestroyTrack(track_flea);
        MIX_DestroyTrack(track_scorpion);
        MIX_DestroyAudio(shoot);
        MIX_DestroyAudio(kill);
        MIX_DestroyAudio(death);
        MIX_DestroyAudio(bonus);
        MIX_DestroyAudio(centipede);
        MIX_DestroyAudio(spider);
        MIX_DestroyAudio(flea);
        MIX_DestroyAudio(scorpion);
        MIX_DestroyMixer(mixer);
        MIX_Quit();
    }

    // one-shot helpers — stop, rewind, play so rapid firing restarts cleanly
    void play_oneshot(MIX_Track *track) {
        if (!track) return;
        MIX_StopTrack(track, 0);
        MIX_SetTrackPlaybackPosition(track, 0);
        MIX_PlayTrack(track, 0);
    }

    void play_shoot() { play_oneshot(track_shoot); }
    void play_kill()  { play_oneshot(track_kill);  }
    void play_death() { play_oneshot(track_death); }
    void play_bonus() { play_oneshot(track_bonus); }

    void start_loop(MIX_Track *track) {
        if (track && !MIX_TrackPlaying(track))
            MIX_PlayTrack(track, 0);
    }
    void stop_loop(MIX_Track *track) {
        if (track && MIX_TrackPlaying(track))
            MIX_StopTrack(track, 0);
    }

    void update_centipede(bool playing) {
        playing ? start_loop(track_centipede) : stop_loop(track_centipede);
    }
    void update_spider(bool playing) {
        playing ? start_loop(track_spider) : stop_loop(track_spider);
    }
    void update_flea(bool playing) {
        playing ? start_loop(track_flea) : stop_loop(track_flea);
    }
    void update_scorpion(bool playing) {
        playing ? start_loop(track_scorpion) : stop_loop(track_scorpion);
    }
};
