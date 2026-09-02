#include "pet_state.h"

static uint8_t clamp_add(uint8_t value, int delta)
{
    int result = (int)value + delta;
    if (result < 0) return 0;
    if (result > 100) return 100;
    return (uint8_t)result;
}

pet_affection_stage_t pet_state_affection_stage(const pet_state_t *state)
{
    if (state->affection < 30) return PET_AFFECTION_LOW;
    if (state->affection < 70) return PET_AFFECTION_FAMILIAR;
    return PET_AFFECTION_HIGH;
}

static bool add_affection(pet_state_t *state, uint8_t amount)
{
    pet_affection_stage_t previous = pet_state_affection_stage(state);
    state->affection = clamp_add(state->affection, amount);
    return pet_state_affection_stage(state) != previous;
}

void pet_state_init(pet_state_t *state)
{
    *state = (pet_state_t) {
        .energy = 72,
        .mood = 84,
        .hunger = 61,
        .selected = PET_ACTION_FEED,
        .affection = 15,
        .last_action = PET_ACTION_COUNT,
    };
}

void pet_state_move(pet_state_t *state, int direction)
{
    int selected = (int)state->selected + direction;
    if (selected < 0) selected = PET_ACTION_COUNT - 1;
    if (selected >= PET_ACTION_COUNT) selected = 0;
    state->selected = (pet_action_t)selected;
}

bool pet_reward_try_consume(pet_reward_limiter_t *limiter,
                            pet_action_t action, uint64_t now_ms)
{
    if (action >= PET_ACTION_COUNT) return false;

    uint8_t used = limiter->used_slots[action];
    for (uint8_t i = 0; i < used; ++i) {
        uint64_t rewarded_at = limiter->rewarded_at_ms[action][i];
        if (now_ms >= rewarded_at &&
            now_ms - rewarded_at >= PET_REWARD_WINDOW_MS) {
            limiter->rewarded_at_ms[action][i] = now_ms;
            return true;
        }
    }
    if (used >= PET_REWARD_LIMIT) return false;

    limiter->rewarded_at_ms[action][used] = now_ms;
    limiter->used_slots[action] = used + 1;
    return true;
}

pet_action_result_t pet_state_apply(pet_state_t *state, bool grant_reward)
{
    static const int deltas[PET_ACTION_COUNT][3] = {
        [PET_ACTION_FEED] = { 14, 3, 18 },
        [PET_ACTION_PLAY] = { -9, 15, -6 },
        [PET_ACTION_REST] = { 20, 5, -4 },
    };
    static const uint8_t affection_gains[PET_ACTION_COUNT] = { 2, 3, 1 };
    pet_action_result_t result = { 0 };
    const int *delta = deltas[state->selected];
    result.repeated_action = state->last_action == state->selected;
    result.reward_granted = grant_reward;
    if (!grant_reward) return result;

    int divisor = result.repeated_action ? 2 : 1;
    state->energy = clamp_add(state->energy, delta[0] / divisor);
    state->mood = clamp_add(state->mood, delta[1] / divisor);
    state->hunger = clamp_add(state->hunger, delta[2] / divisor);
    state->last_action = state->selected;

    result.affection_gained = affection_gains[state->selected];
    if (result.repeated_action) {
        result.affection_gained = (uint8_t)((result.affection_gained + 1) / 2);
    }
    result.affection_stage_changed =
        add_affection(state, result.affection_gained);
    return result;
}

bool pet_state_apply_event(pet_state_t *state, pet_event_t event)
{
    static const int deltas[PET_EVENT_COUNT][3] = {
        [PET_EVENT_SUPPLY] = { 8, 3, 10 },
        [PET_EVENT_RAIN] = { -3, -5, 0 },
        [PET_EVENT_SHORTCUT] = { 4, 6, -2 },
        [PET_EVENT_SUNRISE] = { 2, 8, 0 },
    };
    if (event >= PET_EVENT_COUNT) return false;
    const int *delta = deltas[event];
    state->energy = clamp_add(state->energy, delta[0]);
    state->mood = clamp_add(state->mood, delta[1]);
    state->hunger = clamp_add(state->hunger, delta[2]);
    return add_affection(state, 1);
}

void pet_state_decay(pet_state_t *state)
{
    state->energy = clamp_add(state->energy, -1);
    state->hunger = clamp_add(state->hunger, -1);
}

pet_mood_t pet_state_mood(const pet_state_t *state)
{
    if (state->energy < 25 || state->hunger < 25) return PET_MOOD_SAD;
    if (state->mood >= 95) return PET_MOOD_COOL;
    return PET_MOOD_IDLE;
}
