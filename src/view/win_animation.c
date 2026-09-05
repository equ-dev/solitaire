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

    /* Spawn cadence: activate the next un-spawned card once enough time
     * has accumulated, regardless of how long this particular frame was. */
    anim->ms_since_spawn += dt * 1000.0;
    while (anim->spawned_count < anim->total_cards &&
           anim->ms_since_spawn >= WIN_ANIM_SPAWN_INTERVAL_MS) {
        anim->cards[anim->spawned_count].active = true;
        anim->spawned_count++;
        anim->ms_since_spawn -= WIN_ANIM_SPAWN_INTERVAL_MS;
    }

    for (int i = 0; i < anim->total_cards; i++) {
        win_anim_step_card(&anim->cards[i], dt, floor_y);
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
