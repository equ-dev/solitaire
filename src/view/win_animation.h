#ifndef SOLITAIRE_WIN_ANIMATION_H
#define SOLITAIRE_WIN_ANIMATION_H

#include <stdbool.h>
#include "../model/card.h"

#define WIN_ANIM_MAX_CARDS   52
#define WIN_ANIM_GRAVITY     1400.0 /* px/s^2, downward acceleration before drag opposes it */
#define WIN_ANIM_DRAG        9.0    /* 1/s, opposes vy; caps fall at a GRAVITY/DRAG terminal speed */
#define WIN_ANIM_ROT_COUPLING 0.0015 /* radians of tilt per px/s of sway velocity ("banking") */
#define WIN_ANIM_SPAWN_INTERVAL_MS 90 /* time between successive card spawns */

/* Physics is advanced in fixed-size substeps rather than directly with
 * whatever dt a frame happens to report. A real display's frame times
 * aren't perfectly uniform (compositor scheduling, other load, etc.);
 * feeding that raw, slightly-jittery dt straight into the drag/gravity
 * integrator makes the fall speed (and therefore the sway/rotation
 * derived from elapsed time) drift slightly out of sync frame to frame.
 * Fixed substeps make the simulation's result depend only on total
 * elapsed time, not on how that time happened to be sliced into frames. */
#define WIN_ANIM_FIXED_DT (1.0 / 240.0)  /* seconds per physics substep */
#define WIN_ANIM_MAX_SUBSTEPS_PER_FRAME 16 /* bounds worst case work per win_anim_step call */

/* One falling card, drifting down like a leaf in the wind rather than
 * bouncing. Pure data; no GTK/Cairo types so this can be exercised (and
 * unit-tested) independently of the view layer. */
typedef struct {
    Card card;
    double origin_x;   /* px; horizontal center the sway oscillates around */
    double x, y;        /* top-left corner, px, in board_view's coordinate space */
    double vy;           /* px/s, vertical only; horizontal motion is purely a
                           * function of elapsed time (see sway_* below) */
    double sway_amplitude; /* px; how far left/right of origin_x the card drifts */
    double sway_freq;       /* rad/s; how quickly it sways back and forth */
    double sway_phase;      /* rad; per-card offset so cards don't sway in lockstep */
    double fold_freq;        /* rad/s; how quickly the card flips face/back */
    double fold_phase;       /* rad; per-card offset so flips don't sync up */
    double fold_scale;       /* -1..1, derived each step: cos of the fold angle.
                               * Sign selects which face is showing (>=0 front,
                               * <0 back); magnitude is the horizontal squish
                               * that sells the card turning edge-on, like a
                               * leaf tumbling toward/away from the viewer. */
    double age;              /* s; elapsed time since this card became active */
    double rotation;         /* radians, derived each step from sway velocity */
    bool active;             /* has been spawned; false = not yet in play */
    bool settled;             /* has reached the floor and stopped moving */
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
 * (top-left x/y, e.g. a foundation pile's on-screen position), an initial
 * vertical velocity (small flutter, positive or negative), a sway
 * amplitude/frequency/phase describing its side-to-side drift, and a
 * fold frequency/phase describing how quickly it flips face/back as it
 * turns edge-on toward the viewer. The card starts inactive; it becomes
 * active once the spawn cadence reaches it. Does nothing once
 * WIN_ANIM_MAX_CARDS is reached. Marks the animation as running. */
void win_anim_add_card(WinAnimation *anim, Card card,
                        double origin_x, double origin_y, double vy,
                        double sway_amplitude, double sway_freq, double sway_phase,
                        double fold_freq, double fold_phase);

/* Advances one card's physics by dt seconds against a floor whose
 * top-left resting y-coordinate is floor_y (i.e. floor_y = screen_floor -
 * card_h). Vertical speed is pulled down by gravity and opposed by drag,
 * approaching a terminal fall speed rather than accelerating forever.
 * Horizontal position and rotation are both derived from elapsed active
 * time (a sine sway and its coupled derivative), and fold_scale is
 * likewise derived as a cosine of elapsed time, so the card drifts back
 * and forth, tilts into each turn, and flips face/back like a falling
 * leaf tumbling in three dimensions. The card settles the instant it
 * reaches the floor, with no bounce. No-op if the card is inactive or
 * already settled. */
void win_anim_step_card(BounceCard *card, double dt, double floor_y);

/* Advances the whole animation by dt seconds: activates newly-due cards
 * per WIN_ANIM_SPAWN_INTERVAL_MS (using the raw, un-substepped dt, so
 * spawn timing tracks real elapsed time exactly), then advances physics
 * in fixed WIN_ANIM_FIXED_DT substeps (capped at
 * WIN_ANIM_MAX_SUBSTEPS_PER_FRAME) so fall/sway behavior is independent
 * of how dt happens to be sliced across frames. Clears `running` once
 * every added card has settled. Safe to call every frame regardless of
 * state. */
void win_anim_step(WinAnimation *anim, double dt, double floor_y);

/* True once every card that was added has settled (or no cards were ever
 * added). Used to know when to reveal the "You Win!" banner. */
bool win_anim_finished(const WinAnimation *anim);

#endif /* SOLITAIRE_WIN_ANIMATION_H */
