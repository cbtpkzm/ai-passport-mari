#include "pet_state.h"
#include <assert.h>

int main(void)
{
    pet_state_t state;
    pet_state_init(&state);
    assert(state.energy == 72 && state.mood == 84 && state.hunger == 61);
    assert(state.affection == 15);
    assert(state.selected == PET_ACTION_FEED);
    assert(pet_state_mood(&state) == PET_MOOD_IDLE);
    assert(pet_state_affection_stage(&state) == PET_AFFECTION_LOW);

    pet_state_move(&state, -1);
    assert(state.selected == PET_ACTION_REST);
    pet_state_move(&state, 1);
    assert(state.selected == PET_ACTION_FEED);

    pet_action_result_t result = pet_state_apply(&state, true);
    assert(state.energy == 86 && state.mood == 87 && state.hunger == 79);
    assert(state.affection == 17 && result.affection_gained == 2);
    assert(result.reward_granted);
    assert(!result.repeated_action && !result.affection_stage_changed);

    result = pet_state_apply(&state, false);
    assert(!result.reward_granted && result.affection_gained == 0);
    assert(state.energy == 86 && state.mood == 87 && state.hunger == 79);
    assert(state.affection == 17);

    state.selected = PET_ACTION_PLAY;
    result = pet_state_apply(&state, true);
    assert(state.energy == 77 && state.mood == 100 && state.hunger == 73);
    assert(state.affection == 20 && result.affection_gained == 3);
    assert(pet_state_mood(&state) == PET_MOOD_COOL);

    result = pet_state_apply(&state, true);
    assert(result.repeated_action);
    assert(state.energy == 73 && state.hunger == 70);
    assert(state.affection == 22 && result.affection_gained == 2);

    state.affection = 29;
    state.selected = PET_ACTION_FEED;
    result = pet_state_apply(&state, true);
    assert(result.affection_stage_changed);
    assert(state.affection == 31);
    assert(pet_state_affection_stage(&state) == PET_AFFECTION_FAMILIAR);

    state.affection = 69;
    state.selected = PET_ACTION_PLAY;
    result = pet_state_apply(&state, true);
    assert(result.affection_stage_changed);
    assert(state.affection == 72);
    assert(pet_state_affection_stage(&state) == PET_AFFECTION_HIGH);

    assert(pet_state_apply_event(&state, PET_EVENT_SUPPLY) == false);
    assert(state.affection == 73);
    assert(state.hunger <= 100);

    state.hunger = 1;
    pet_state_decay(&state);
    assert(state.hunger == 0);
    assert(pet_state_mood(&state) == PET_MOOD_SAD);
    return 0;
}
