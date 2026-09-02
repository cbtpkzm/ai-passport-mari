#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    PET_ACTION_FEED = 0,
    PET_ACTION_PLAY,
    PET_ACTION_REST,
    PET_ACTION_COUNT,
} pet_action_t;

typedef enum {
    PET_MOOD_IDLE = 0,
    PET_MOOD_SAD,
    PET_MOOD_COOL,
} pet_mood_t;

typedef enum {
    PET_AFFECTION_LOW = 0,
    PET_AFFECTION_FAMILIAR,
    PET_AFFECTION_HIGH,
} pet_affection_stage_t;

typedef enum {
    PET_EVENT_SUPPLY = 0,
    PET_EVENT_RAIN,
    PET_EVENT_SHORTCUT,
    PET_EVENT_SUNRISE,
    PET_EVENT_COUNT,
} pet_event_t;

#define PET_REWARD_LIMIT 3
#define PET_REWARD_WINDOW_MS (10ULL * 60 * 1000)

typedef struct {
    uint64_t rewarded_at_ms[PET_ACTION_COUNT][PET_REWARD_LIMIT];
    uint8_t used_slots[PET_ACTION_COUNT];
} pet_reward_limiter_t;

typedef struct {
    uint8_t energy;
    uint8_t mood;
    uint8_t hunger;
    pet_action_t selected;
    uint8_t affection;
    pet_action_t last_action;
} pet_state_t;

typedef struct {
    uint8_t affection_gained;
    bool affection_stage_changed;
    bool repeated_action;
    bool reward_granted;
} pet_action_result_t;

void pet_state_init(pet_state_t *state);
void pet_state_move(pet_state_t *state, int direction);
bool pet_reward_try_consume(pet_reward_limiter_t *limiter,
                            pet_action_t action, uint64_t now_ms);
pet_action_result_t pet_state_apply(pet_state_t *state, bool grant_reward);
bool pet_state_apply_event(pet_state_t *state, pet_event_t event);
void pet_state_decay(pet_state_t *state);
pet_mood_t pet_state_mood(const pet_state_t *state);
pet_affection_stage_t pet_state_affection_stage(const pet_state_t *state);
