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
                   double origin_x, double origin_y,
                   double vx, double vy, double vrot)
{
    if (anim->total_cards >= WIN_ANIM_MAX_CARDS) {
        return;
    }

    BounceCard *c = &anim->cards[anim->total_cards];
    c->card = card;
    c->x = origin_x;
    c->y = origin_y;
    c->vx = vx;
    c->vy = vy;
    c->rotation = 0.0;
    c->vrot = vrot;
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

    card->vy += WIN_ANIM_GRAVITY * dt;
    card->y += card->vy * dt;
    card->x += card->vx * dt;
    card->rotation += card->vrot * dt;

    if (card->y >= floor_y) {
        card->y = floor_y;
        card->vy = -card->vy * WIN_ANIM_RESTITUTION;
        card->vx *= WIN_ANIM_FLOOR_FRICTION;

        if (fabs(card->vy) < WIN_ANIM_SETTLE_VY) {
            card->vy = 0.0;
            card->vx = 0.0;
            card->vrot = 0.0;
            card->settled = true;
        }
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
     * bounce behavior is deterministic given total elapsed time instead
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
