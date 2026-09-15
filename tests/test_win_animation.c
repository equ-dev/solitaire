#include <check.h>
#include <math.h>

#include "../src/model/card.h"
#include "../src/view/win_animation.h"

static Card
make_card(Suit suit, int rank)
{
    Card c;
    c.suit = suit;
    c.rank = rank;
    c.face_up = true;
    return c;
}

/* ---------- single-card physics ---------- */

START_TEST(test_inactive_card_does_not_move)
{
    BounceCard c = { .card = make_card(SUIT_HEARTS, RANK_ACE),
                      .origin_x = 10, .x = 10, .y = 10, .vy = 0,
                      .sway_amplitude = 50, .sway_freq = 2.0, .sway_phase = 0,
                      .fold_freq = 3.0, .fold_phase = 0,
                      .active = false, .settled = false };

    win_anim_step_card(&c, 1.0 / 60.0, 500);

    ck_assert(fabs(c.x - 10) < 1e-9);
    ck_assert(fabs(c.y - 10) < 1e-9);
}

START_TEST(test_fall_speed_approaches_terminal_velocity)
{
    /* Drag opposes gravity, so vy should climb quickly at first, then
     * grow by less and less each equal-sized step as it approaches the
     * GRAVITY/DRAG terminal speed, instead of accelerating forever. */
    BounceCard c = { .card = make_card(SUIT_HEARTS, RANK_ACE),
                      .origin_x = 0, .x = 0, .y = 0, .vy = 0,
                      .sway_amplitude = 0, .sway_freq = 1.0, .sway_phase = 0,
                      .fold_freq = 3.0, .fold_phase = 0,
                      .active = true, .settled = false };

    double dt = 1.0 / 60.0;
    win_anim_step_card(&c, dt, 1000.0 /* far below, no landing yet */);
    double vy_after_one = c.vy;

    for (int i = 0; i < 300; i++) {
        win_anim_step_card(&c, dt, 1000.0);
    }
    double vy_late = c.vy;

    double terminal = WIN_ANIM_GRAVITY / WIN_ANIM_DRAG;

    ck_assert(vy_after_one > 0);              /* gravity pulls it downward (+y) */
    ck_assert(c.y > 0);                        /* it has actually fallen */
    ck_assert(vy_late > vy_after_one);         /* still net-accelerating early on */
    ck_assert(vy_late < terminal + 1e-6);       /* never exceeds the terminal speed */
    ck_assert(fabs(vy_late - terminal) < 1.0);  /* has essentially converged after 5s */
}

START_TEST(test_sway_oscillates_around_origin)
{
    /* x is a pure sine function of elapsed active time: it should stay
     * within [origin_x - amplitude, origin_x + amplitude] and actually
     * move both right and left of the origin over one full cycle,
     * rather than drifting off in one direction. */
    BounceCard c = { .card = make_card(SUIT_CLUBS, 2),
                      .origin_x = 100, .x = 100, .y = 0, .vy = 0,
                      .sway_amplitude = 50, .sway_freq = 2.0, .sway_phase = 0,
                      .fold_freq = 3.0, .fold_phase = 0,
                      .active = true, .settled = false };

    double dt = 1.0 / 240.0;
    double min_x = c.x, max_x = c.x;
    double period = 2.0 * M_PI / c.sway_freq;
    int steps = (int)(period / dt) + 1;
    for (int i = 0; i < steps; i++) {
        win_anim_step_card(&c, dt, 100000.0 /* stay airborne the whole cycle */);
        if (c.x < min_x) min_x = c.x;
        if (c.x > max_x) max_x = c.x;
    }

    ck_assert(min_x >= 100 - 50 - 1e-6);
    ck_assert(max_x <= 100 + 50 + 1e-6);
    ck_assert(max_x - min_x > 50.0); /* actually swung across the origin, not stayed put */
}

START_TEST(test_rotation_tracks_sway_direction)
{
    /* At age 0 with zero phase, sin(phase)=0 and cos(phase)=1, so the
     * sway velocity (and therefore rotation) should be at its positive
     * extreme: amplitude * freq, scaled by the rotation-coupling
     * constant. */
    BounceCard c = { .card = make_card(SUIT_SPADES, 7),
                      .origin_x = 0, .x = 0, .y = 0, .vy = 0,
                      .sway_amplitude = 60, .sway_freq = 2.0, .sway_phase = 0,
                      .fold_freq = 3.0, .fold_phase = 0,
                      .active = true, .settled = false };

    win_anim_step_card(&c, 1.0 / 240.0, 100000.0);

    double expected = WIN_ANIM_ROT_COUPLING * 60.0 * 2.0 * cos(2.0 * (1.0 / 240.0));
    ck_assert(fabs(c.rotation - expected) < 1e-6);
}

START_TEST(test_fold_scale_oscillates_and_flips_sign)
{
    /* fold_scale is a pure cosine of elapsed active time: over one full
     * fold cycle it should swing from +1 (fully front-facing) down
     * through 0 (edge-on) to -1 (fully back-facing) and back, so both
     * signs and both extremes must actually occur. */
    BounceCard c = { .card = make_card(SUIT_DIAMONDS, 9),
                      .origin_x = 0, .x = 0, .y = 0, .vy = 0,
                      .sway_amplitude = 0, .sway_freq = 1.0, .sway_phase = 0,
                      .fold_freq = 2.0, .fold_phase = 0,
                      .active = true, .settled = false };

    ck_assert(fabs(c.fold_scale) < 1e-9); /* not yet stepped: default-initialized */

    double dt = 1.0 / 240.0;
    double min_fold = 1.0, max_fold = -1.0;
    bool saw_negative = false;
    double period = 2.0 * M_PI / c.fold_freq;
    int steps = (int)(period / dt) + 1;
    for (int i = 0; i < steps; i++) {
        win_anim_step_card(&c, dt, 100000.0);
        if (c.fold_scale < min_fold) min_fold = c.fold_scale;
        if (c.fold_scale > max_fold) max_fold = c.fold_scale;
        if (c.fold_scale < 0.0) saw_negative = true;
    }

    ck_assert(min_fold >= -1.0 - 1e-6);
    ck_assert(max_fold <= 1.0 + 1e-6);
    ck_assert(saw_negative); /* the card actually shows its back at some point */
    ck_assert(max_fold - min_fold > 1.5); /* swings through most of the [-1, 1] range */
}

START_TEST(test_card_settles_on_reaching_floor_without_bouncing)
{
    BounceCard c = { .card = make_card(SUIT_DIAMONDS, 5),
                      .origin_x = 0, .x = 0, .y = 499.0, .vy = 300,
                      .sway_amplitude = 40, .sway_freq = 2.0, .sway_phase = 0,
                      .fold_freq = 3.0, .fold_phase = 0,
                      .active = true, .settled = false };

    win_anim_step_card(&c, 1.0 / 60.0, 500.0);

    ck_assert(c.settled);
    ck_assert(fabs(c.y - 500.0) < 1e-9);
    ck_assert(fabs(c.vy) < 1e-9); /* comes to rest immediately, no rebound */
}

START_TEST(test_settled_card_no_longer_moves)
{
    BounceCard c = { .card = make_card(SUIT_HEARTS, 6),
                      .origin_x = 42, .x = 42, .y = 500, .vy = 0,
                      .sway_amplitude = 40, .sway_freq = 2.0, .sway_phase = 0,
                      .fold_freq = 3.0, .fold_phase = 0,
                      .active = true, .settled = true };

    win_anim_step_card(&c, 1.0 / 60.0, 500.0);

    ck_assert(fabs(c.x - 42) < 1e-9);
    ck_assert(fabs(c.y - 500) < 1e-9);
}

/* ---------- whole-sequence spawning ---------- */

START_TEST(test_new_animation_has_no_cards_and_is_not_running)
{
    WinAnimation anim;
    win_anim_reset(&anim);

    ck_assert_int_eq(anim.total_cards, 0);
    ck_assert_int_eq(anim.spawned_count, 0);
    ck_assert(!anim.running);
    ck_assert(win_anim_finished(&anim));
}

START_TEST(test_adding_a_card_marks_animation_running_but_inactive)
{
    WinAnimation anim;
    win_anim_reset(&anim);

    win_anim_add_card(&anim, make_card(SUIT_HEARTS, RANK_ACE), 100, 100, -20, 50, 2.0, 0.0, 3.0, 0.0);

    ck_assert_int_eq(anim.total_cards, 1);
    ck_assert(anim.running);
    ck_assert(!anim.cards[0].active);
    ck_assert(!win_anim_finished(&anim));
}

START_TEST(test_cards_spawn_one_at_a_time_on_interval)
{
    WinAnimation anim;
    win_anim_reset(&anim);
    win_anim_add_card(&anim, make_card(SUIT_HEARTS, RANK_ACE), 0, 0, 0, 0, 1.0, 0.0, 3.0, 0.0);
    win_anim_add_card(&anim, make_card(SUIT_HEARTS, 2), 0, 0, 0, 0, 1.0, 0.0, 3.0, 0.0);

    /* Advance by less than one spawn interval: nothing should be active yet. */
    win_anim_step(&anim, (WIN_ANIM_SPAWN_INTERVAL_MS / 1000.0) * 0.5, 10000.0);
    ck_assert_int_eq(anim.spawned_count, 0);

    /* Cross the first interval: exactly one card active. */
    win_anim_step(&anim, (WIN_ANIM_SPAWN_INTERVAL_MS / 1000.0) * 0.6, 10000.0);
    ck_assert_int_eq(anim.spawned_count, 1);
    ck_assert(anim.cards[0].active);
    ck_assert(!anim.cards[1].active);

    /* Cross the second interval: both active. */
    win_anim_step(&anim, WIN_ANIM_SPAWN_INTERVAL_MS / 1000.0, 10000.0);
    ck_assert_int_eq(anim.spawned_count, 2);
    ck_assert(anim.cards[1].active);
}

START_TEST(test_animation_reports_finished_only_after_all_cards_settle)
{
    WinAnimation anim;
    win_anim_reset(&anim);
    win_anim_add_card(&anim, make_card(SUIT_SPADES, RANK_ACE), 0, 400, 0, 40, 2.0, 0.0, 3.0, 0.0);

    ck_assert(!win_anim_finished(&anim));

    /* Drive it with realistic ~16ms frames, the way the real GTK tick
     * callback will, until it settles or we give up (bug, not a slow
     * settle) after a generous number of frames. */
    const double dt = 1.0 / 60.0;
    bool finished = false;
    for (int frame = 0; frame < 600 && !finished; frame++) {
        win_anim_step(&anim, dt, 500.0);
        finished = win_anim_finished(&anim);
    }

    ck_assert(finished);
    ck_assert(!anim.running);
}

START_TEST(test_adding_more_than_max_cards_is_ignored)
{
    WinAnimation anim;
    win_anim_reset(&anim);
    for (int i = 0; i < WIN_ANIM_MAX_CARDS + 5; i++) {
        win_anim_add_card(&anim, make_card(SUIT_CLUBS, RANK_ACE), 0, 0, 0, 0, 1.0, 0.0, 3.0, 0.0);
    }
    ck_assert_int_eq(anim.total_cards, WIN_ANIM_MAX_CARDS);
}
END_TEST

Suite *
win_animation_suite(void)
{
    Suite *s = suite_create("WinAnimation");

    TCase *tc = tcase_create("Physics");
    tcase_add_test(tc, test_inactive_card_does_not_move);
    tcase_add_test(tc, test_fall_speed_approaches_terminal_velocity);
    tcase_add_test(tc, test_sway_oscillates_around_origin);
    tcase_add_test(tc, test_rotation_tracks_sway_direction);
    tcase_add_test(tc, test_fold_scale_oscillates_and_flips_sign);
    tcase_add_test(tc, test_card_settles_on_reaching_floor_without_bouncing);
    tcase_add_test(tc, test_settled_card_no_longer_moves);
    tcase_add_test(tc, test_new_animation_has_no_cards_and_is_not_running);
    tcase_add_test(tc, test_adding_a_card_marks_animation_running_but_inactive);
    tcase_add_test(tc, test_cards_spawn_one_at_a_time_on_interval);
    tcase_add_test(tc, test_animation_reports_finished_only_after_all_cards_settle);
    tcase_add_test(tc, test_adding_more_than_max_cards_is_ignored);
    suite_add_tcase(s, tc);

    return s;
}

int
main(void)
{
    Suite *s = win_animation_suite();
    SRunner *sr = srunner_create(s);
    srunner_run_all(sr, CK_NORMAL);
    int failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return failed == 0 ? 0 : 1;
}
