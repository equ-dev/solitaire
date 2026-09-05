#ifndef SOLITAIRE_WIN_ANIMATION_H
#define SOLITAIRE_WIN_ANIMATION_H

#include <stdbool.h>
#include "../model/card.h"

#define WIN_ANIM_MAX_CARDS   52
#define WIN_ANIM_GRAVITY     1400.0 /* px/s^2, downward */
#define WIN_ANIM_RESTITUTION 0.62   /* fraction of vertical speed kept after a bounce */
#define WIN_ANIM_FLOOR_FRICTION 0.90 /* fraction of horizontal speed kept per bounce */
#define WIN_ANIM_SETTLE_VY   40.0   /* below this post-bounce speed (px/s), a card is settled */
#define WIN_ANIM_SPAWN_INTERVAL_MS 90 /* time between successive card spawns */

/* One bouncing card. Pure data; no GTK/Cairo types so this can be
 * exercised (and unit-tested) independently of the view layer. */
typedef struct {
    Card card;
    double x, y;      /* top-left corner, px, in board_view's coordinate space */
    double vx, vy;    /* px/s */
    double rotation;  /* radians, for visual spin */
    double vrot;      /* radians/s */
    bool active;      /* has been spawned; false = not yet in play */
    bool settled;      /* has come to rest on the floor and stopped moving */
} BounceCard;

/* The full animation sequence: a fixed-size pool of cards plus how many
 * have been spawned so far and the elapsed time since the last spawn. */
typedef struct {
    BounceCard cards[WIN_ANIM_MAX_CARDS];
    int total_cards;      /* how many cards[] slots are actually in use (<= 52) */
    int spawned_count;    /* how many have been made active so far */
    double ms_since_spawn; /* accumulator driving the spawn cadence */
    bool running;          /* true from the first spawn until every card has settled */
} WinAnimation;

/* Resets `anim` to a fresh, not-yet-started state. Does not populate any
 * cards; callers add cards via win_anim_add_card before starting. */
void win_anim_reset(WinAnimation *anim);

/* Appends one card to the animation's pool with the given spawn origin
 * (top-left x/y, e.g. a foundation pile's on-screen position) and initial
 * velocity/spin. The card starts inactive; it becomes active once the
 * spawn cadence reaches it. Does nothing once WIN_ANIM_MAX_CARDS is
 * reached. Marks the animation as running. */
void win_anim_add_card(WinAnimation *anim, Card card,
                        double origin_x, double origin_y,
                        double vx, double vy, double vrot);

/* Advances one card's physics by dt seconds against a floor whose
 * top-left resting y-coordinate is floor_y (i.e. floor_y = screen_floor -
 * card_h), handling gravity, a bounce off the floor, and settling once
 * post-bounce vertical speed drops below WIN_ANIM_SETTLE_VY. No-op if the
 * card is inactive or already settled. */
void win_anim_step_card(BounceCard *card, double dt, double floor_y);

/* Advances the whole animation by dt seconds: activates newly-due cards
 * per WIN_ANIM_SPAWN_INTERVAL_MS, steps every active card's physics
 * against floor_y, and clears `running` once every added card has
 * settled. Safe to call every frame regardless of state. */
void win_anim_step(WinAnimation *anim, double dt, double floor_y);

/* True once every card that was added has settled (or no cards were ever
 * added). Used to know when to reveal the "You Win!" banner. */
bool win_anim_finished(const WinAnimation *anim);

#endif /* SOLITAIRE_WIN_ANIMATION_H */
