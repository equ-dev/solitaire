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
                      .x = 10, .y = 10, .vx = 50, .vy = 0,
                      .active = false, .settled = false };

    win_anim_step_card(&c, 1.0 / 60.0, 500);

    ck_assert(fabs(c.x - 10) < 1e-9);
    ck_assert(fabs(c.y - 10) < 1e-9);
}

START_TEST(test_gravity_accelerates_fall)
{
    BounceCard c = { .card = make_card(SUIT_HEARTS, RANK_ACE),
                      .x = 0, .y = 0, .vx = 0, .vy = 0,
                      .active = true, .settled = false };

    double dt = 1.0 / 60.0;
    win_anim_step_card(&c, dt, 1000.0 /* far below, no bounce yet */);
    double vy_after_one = c.vy;
    win_anim_step_card(&c, dt, 1000.0);

    ck_assert(vy_after_one > 0);       /* gravity pulls it downward (+y) */
    ck_assert(c.vy > vy_after_one);    /* still accelerating */
    ck_assert(c.y > 0);                /* it has actually fallen */
}

START_TEST(test_bounce_reverses_and_dampens_vertical_speed)
{
    BounceCard c = { .card = make_card(SUIT_SPADES, RANK_KING),
                      .x = 0, .y = 490, .vx = 0, .vy = 600,
                      .active = true, .settled = false };

    win_anim_step_card(&c, 1.0 / 60.0, 500.0);

    ck_assert(c.y <= 500.0);
    /* Went from falling (+vy) to rising (-vy): direction flipped, and the
     * new speed is exactly the configured restitution fraction of the
     * impact speed (600 + one frame of gravity), not the raw pre-impact
     * speed, since gravity is applied before the floor check. */
    double expected_impact_speed = 600.0 + WIN_ANIM_GRAVITY * (1.0 / 60.0);
    ck_assert(c.vy < 0);
    ck_assert(fabs(-c.vy - expected_impact_speed * WIN_ANIM_RESTITUTION) < 1e-6);
}

START_TEST(test_bounce_applies_floor_friction_horizontally)
{
    BounceCard c = { .card = make_card(SUIT_CLUBS, 2),
                      .x = 0, .y = 490, .vx = 200, .vy = 600,
                      .active = true, .settled = false };

    win_anim_step_card(&c, 1.0 / 60.0, 500.0);

    ck_assert(fabs(c.vx - 200.0 * WIN_ANIM_FLOOR_FRICTION) < 1e-6);
}

START_TEST(test_card_settles_once_bounce_speed_drops_below_threshold)
{
    /* A slow tap on the floor: post-bounce speed should land under the
     * settle threshold immediately, marking the card as settled and
     * zeroing all velocity/rotation. */
    BounceCard c = { .card = make_card(SUIT_DIAMONDS, 5),
                      .x = 0, .y = 499.5, .vx = 30, .vy = 10,
                      .rotation = 1.0, .vrot = 2.0,
                      .active = true, .settled = false };

    win_anim_step_card(&c, 1.0 / 60.0, 500.0);

    ck_assert(c.settled);
    ck_assert(fabs(c.vx) < 1e-9);
    ck_assert(fabs(c.vy) < 1e-9);
    ck_assert(fabs(c.vrot) < 1e-9);
}

START_TEST(test_settled_card_no_longer_moves)
{
    BounceCard c = { .card = make_card(SUIT_HEARTS, 6),
                      .x = 42, .y = 500, .vx = 0, .vy = 0,
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

    win_anim_add_card(&anim, make_card(SUIT_HEARTS, RANK_ACE), 100, 100, 50, -300, 1.0);

    ck_assert_int_eq(anim.total_cards, 1);
    ck_assert(anim.running);
    ck_assert(!anim.cards[0].active);
    ck_assert(!win_anim_finished(&anim));
}

START_TEST(test_cards_spawn_one_at_a_time_on_interval)
{
    WinAnimation anim;
    win_anim_reset(&anim);
    win_anim_add_card(&anim, make_card(SUIT_HEARTS, RANK_ACE), 0, 0, 0, 0, 0);
    win_anim_add_card(&anim, make_card(SUIT_HEARTS, 2), 0, 0, 0, 0, 0);

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
    win_anim_add_card(&anim, make_card(SUIT_SPADES, RANK_ACE), 0, 400, 0, 0, 0);

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
        win_anim_add_card(&anim, make_card(SUIT_CLUBS, RANK_ACE), 0, 0, 0, 0, 0);
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
    tcase_add_test(tc, test_gravity_accelerates_fall);
    tcase_add_test(tc, test_bounce_reverses_and_dampens_vertical_speed);
    tcase_add_test(tc, test_bounce_applies_floor_friction_horizontally);
    tcase_add_test(tc, test_card_settles_once_bounce_speed_drops_below_threshold);
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
