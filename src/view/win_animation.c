#include "win_animation.h"
#include <math.h>
#include <string.h>

void
win_anim_reset(WinAnimation *anim)
{
    memset(anim, 0, sizeof(*anim));
}

void
win_anim_add_card(WinAnimation *anim, Card card,
                   double origin_x, double origin_y, double vy,
                   double sway_amplitude, double sway_freq, double sway_phase,
                   double fold_freq, double fold_phase)
{
    if (anim->total_cards >= WIN_ANIM_MAX_CARDS) {
        return;
    }

    BounceCard *c = &anim->cards[anim->total_cards];
    c->card = card;
    c->origin_x = origin_x;
    c->x = origin_x;
    c->y = origin_y;
    c->vy = vy;
    c->sway_amplitude = sway_amplitude;
    c->sway_freq = sway_freq;
    c->sway_phase = sway_phase;
    c->fold_freq = fold_freq;
    c->fold_phase = fold_phase;
    c->fold_scale = cos(fold_phase);
    c->age = 0.0;
    c->rotation = 0.0;
    c->active = false;
    c->settled = false;

    anim->total_cards++;
    anim->running = true;
}

void
win_anim_step_card(BounceCard *card, double dt, double floor_y)
{
    if (!card->active || card->settled) {
        return;
    }

    /* Drag-limited fall: acceleration shrinks as vy approaches the
     * GRAVITY/DRAG terminal speed instead of growing without bound, for
     * the slow, floaty descent of a falling leaf rather than a dropped
     * object. */
    card->vy += (WIN_ANIM_GRAVITY - WIN_ANIM_DRAG * card->vy) * dt;
    card->y += card->vy * dt;
    card->age += dt;

    /* Side-to-side sway: x is a pure function of elapsed active time
     * around the spawn origin, not an integrated velocity, so the card
     * reliably retraces the same back-and-forth drift every cycle
     * instead of wandering. */
    double phase = card->sway_freq * card->age + card->sway_phase;
    card->x = card->origin_x + card->sway_amplitude * sin(phase);

    /* Rotation follows the sway's instantaneous velocity (its time
     * derivative), so the card visibly banks into each turn the way a
     * real leaf tips as it changes lateral direction, instead of
     * spinning freely. */
    double sway_vel = card->sway_amplitude * card->sway_freq * cos(phase);
    card->rotation = WIN_ANIM_ROT_COUPLING * sway_vel;

    /* Front-to-back fold: a card tumbling like a real leaf doesn't just
     * drift side to side, it also turns edge-on toward and away from the
     * viewer. fold_scale oscillates between -1 (fully back-facing) and
     * +1 (fully front-facing); the view layer uses its sign to pick
     * which face to draw and its magnitude as a horizontal squish, so
     * the card visibly narrows to an edge and widens back out each half
     * cycle instead of only ever showing its front. */
    card->fold_scale = cos(card->fold_freq * card->age + card->fold_phase);

    if (card->y >= floor_y) {
        card->y = floor_y;
        card->vy = 0.0;
        card->settled = true;
    }
}

void
win_anim_step(WinAnimation *anim, double dt, double floor_y)
{
    if (!anim->running) {
        return;
    }

    /* Spawn cadence uses the real, un-substepped dt so a card's spawn
     * moment tracks actual elapsed wall-clock time exactly. */
    anim->ms_since_spawn += dt * 1000.0;
    while (anim->spawned_count < anim->total_cards &&
           anim->ms_since_spawn >= WIN_ANIM_SPAWN_INTERVAL_MS) {
        anim->cards[anim->spawned_count].active = true;
        anim->spawned_count++;
        anim->ms_since_spawn -= WIN_ANIM_SPAWN_INTERVAL_MS;
    }

    /* Physics advances in fixed substeps regardless of the actual dt, so
     * fall/sway behavior is deterministic given total elapsed time instead
     * of depending on exactly how that time was sliced into frames. */
    double remaining = dt;
    int substeps = 0;
    while (remaining > 0.0 && substeps < WIN_ANIM_MAX_SUBSTEPS_PER_FRAME) {
        double step_dt = (remaining < WIN_ANIM_FIXED_DT) ? remaining : WIN_ANIM_FIXED_DT;
        for (int i = 0; i < anim->total_cards; i++) {
            win_anim_step_card(&anim->cards[i], step_dt, floor_y);
        }
        remaining -= step_dt;
        substeps++;
    }

    if (win_anim_finished(anim)) {
        anim->running = false;
    }
}

bool
win_anim_finished(const WinAnimation *anim)
{
    if (anim->total_cards == 0) {
        return true;
    }
    if (anim->spawned_count < anim->total_cards) {
        return false;
    }
    for (int i = 0; i < anim->total_cards; i++) {
        if (!anim->cards[i].settled) {
            return false;
        }
    }
    return true;
}
