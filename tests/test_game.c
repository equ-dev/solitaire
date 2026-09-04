#include <check.h>
#include <stdlib.h>
#include <string.h>

#include "../src/model/card.h"
#include "../src/model/deck.h"
#include "../src/model/game.h"

/* ---------- deck tests ---------- */

START_TEST(test_deck_init_has_52_unique_cards)
{
    Deck deck;
    deck_init_standard(&deck);
    ck_assert_int_eq(deck.count, 52);

    bool seen[SUIT_COUNT][RANK_KING + 1];
    memset(seen, 0, sizeof(seen));
    for (int i = 0; i < deck.count; i++) {
        Card c = deck.cards[i];
        ck_assert(!seen[c.suit][c.rank]);
        seen[c.suit][c.rank] = true;
        ck_assert(!c.face_up);
    }
}
END_TEST

START_TEST(test_deck_shuffle_is_deterministic_per_seed)
{
    Deck a, b;
    deck_init_standard(&a);
    deck_init_standard(&b);
    deck_shuffle(&a, 42);
    deck_shuffle(&b, 42);

    for (int i = 0; i < DECK_SIZE; i++) {
        ck_assert_int_eq(a.cards[i].suit, b.cards[i].suit);
        ck_assert_int_eq(a.cards[i].rank, b.cards[i].rank);
    }
}
END_TEST

START_TEST(test_deck_shuffle_still_has_52_unique_cards)
{
    Deck deck;
    deck_init_standard(&deck);
    deck_shuffle(&deck, 7);

    bool seen[SUIT_COUNT][RANK_KING + 1];
    memset(seen, 0, sizeof(seen));
    for (int i = 0; i < deck.count; i++) {
        Card c = deck.cards[i];
        ck_assert(!seen[c.suit][c.rank]);
        seen[c.suit][c.rank] = true;
    }
}
END_TEST

/* ---------- deal tests ---------- */

START_TEST(test_new_game_deal_shape)
{
    GameState game;
    game_new(&game, 1);

    for (int col = 0; col < NUM_TABLEAU; col++) {
        ck_assert_int_eq(game.tableau[col].count, col + 1);
        for (int row = 0; row < game.tableau[col].count; row++) {
            bool expected_face_up = (row == col);
            ck_assert(game.tableau[col].cards[row].face_up == expected_face_up);
        }
    }

    ck_assert_int_eq(game.stock.count, 24);
    ck_assert_int_eq(game.waste.count, 0);
    for (int i = 0; i < NUM_FOUNDATIONS; i++) {
        ck_assert_int_eq(game.foundations[i].count, 0);
    }
}
END_TEST

/* ---------- stock/waste tests ---------- */

START_TEST(test_draw_from_stock_moves_card_face_up)
{
    GameState game;
    game_new(&game, 1);
    int stock_before = game.stock.count;

    ck_assert(game_draw_from_stock(&game));
    ck_assert_int_eq(game.stock.count, stock_before - 1);
    ck_assert_int_eq(game.waste.count, 1);
    ck_assert(game.waste.cards[0].face_up);
}
END_TEST

START_TEST(test_stock_recycles_from_waste_when_empty)
{
    GameState game;
    game_new(&game, 1);

    /* Drain the stock into the waste pile. */
    while (game.stock.count > 0) {
        ck_assert(game_draw_from_stock(&game));
    }
    ck_assert_int_eq(game.stock.count, 0);
    ck_assert_int_eq(game.waste.count, 24);

    /* Next draw should recycle waste back into stock instead of drawing. */
    ck_assert(game_draw_from_stock(&game));
    ck_assert_int_eq(game.waste.count, 0);
    ck_assert_int_eq(game.stock.count, 24);
    for (int i = 0; i < game.stock.count; i++) {
        ck_assert(!game.stock.cards[i].face_up);
    }
}
END_TEST

START_TEST(test_draw_fails_when_stock_and_waste_both_empty)
{
    GameState game;
    memset(&game, 0, sizeof(game));
    ck_assert(!game_draw_from_stock(&game));
}
END_TEST

/* ---------- foundation move tests ---------- */

START_TEST(test_ace_goes_to_empty_foundation)
{
    GameState game;
    memset(&game, 0, sizeof(game));
    game.waste.cards[0] = (Card){ .suit = SUIT_HEARTS, .rank = RANK_ACE, .face_up = true };
    game.waste.count = 1;

    ck_assert(game_waste_to_foundation(&game));
    ck_assert_int_eq(game.waste.count, 0);
    ck_assert_int_eq(game.foundations[SUIT_HEARTS].count, 1);
}
END_TEST

START_TEST(test_two_before_ace_rejected)
{
    GameState game;
    memset(&game, 0, sizeof(game));
    game.waste.cards[0] = (Card){ .suit = SUIT_HEARTS, .rank = 2, .face_up = true };
    game.waste.count = 1;

    ck_assert(!game_waste_to_foundation(&game));
    ck_assert_int_eq(game.waste.count, 1);
    ck_assert_int_eq(game.foundations[SUIT_HEARTS].count, 0);
}
END_TEST

START_TEST(test_sequential_same_suit_accepted)
{
    GameState game;
    memset(&game, 0, sizeof(game));
    game.foundations[SUIT_CLUBS].cards[0] = (Card){ .suit = SUIT_CLUBS, .rank = RANK_ACE, .face_up = true };
    game.foundations[SUIT_CLUBS].count = 1;
    game.waste.cards[0] = (Card){ .suit = SUIT_CLUBS, .rank = 2, .face_up = true };
    game.waste.count = 1;

    ck_assert(game_waste_to_foundation(&game));
    ck_assert_int_eq(game.foundations[SUIT_CLUBS].count, 2);
}
END_TEST

/* ---------- tableau move tests ---------- */

START_TEST(test_red_six_onto_black_seven)
{
    GameState game;
    memset(&game, 0, sizeof(game));
    game.tableau[0].cards[0] = (Card){ .suit = SUIT_SPADES, .rank = 7, .face_up = true };
    game.tableau[0].count = 1;
    game.waste.cards[0] = (Card){ .suit = SUIT_HEARTS, .rank = 6, .face_up = true };
    game.waste.count = 1;

    ck_assert(game_waste_to_tableau(&game, 0));
    ck_assert_int_eq(game.tableau[0].count, 2);
    ck_assert_int_eq(game.waste.count, 0);
}
END_TEST

START_TEST(test_same_color_rejected)
{
    GameState game;
    memset(&game, 0, sizeof(game));
    game.tableau[0].cards[0] = (Card){ .suit = SUIT_SPADES, .rank = 7, .face_up = true };
    game.tableau[0].count = 1;
    game.waste.cards[0] = (Card){ .suit = SUIT_CLUBS, .rank = 6, .face_up = true };
    game.waste.count = 1;

    ck_assert(!game_waste_to_tableau(&game, 0));
}
END_TEST

START_TEST(test_wrong_rank_rejected)
{
    GameState game;
    memset(&game, 0, sizeof(game));
    game.tableau[0].cards[0] = (Card){ .suit = SUIT_SPADES, .rank = 7, .face_up = true };
    game.tableau[0].count = 1;
    game.waste.cards[0] = (Card){ .suit = SUIT_HEARTS, .rank = 5, .face_up = true };
    game.waste.count = 1;

    ck_assert(!game_waste_to_tableau(&game, 0));
}
END_TEST

START_TEST(test_empty_tableau_accepts_only_king)
{
    GameState game;
    memset(&game, 0, sizeof(game));
    game.waste.cards[0] = (Card){ .suit = SUIT_HEARTS, .rank = 5, .face_up = true };
    game.waste.count = 1;
    ck_assert(!game_waste_to_tableau(&game, 0));

    game.waste.cards[0] = (Card){ .suit = SUIT_HEARTS, .rank = RANK_KING, .face_up = true };
    ck_assert(game_waste_to_tableau(&game, 0));
    ck_assert_int_eq(game.tableau[0].count, 1);
}
END_TEST

START_TEST(test_move_run_of_cards_between_tableau)
{
    GameState game;
    memset(&game, 0, sizeof(game));

    /* Source pile: [.. black 8 (face down), red 7, black 6] all face up,
     * a valid descending alternating run starting at index 1. */
    game.tableau[0].cards[0] = (Card){ .suit = SUIT_CLUBS, .rank = 9, .face_up = false };
    game.tableau[0].cards[1] = (Card){ .suit = SUIT_HEARTS, .rank = 7, .face_up = true };
    game.tableau[0].cards[2] = (Card){ .suit = SUIT_CLUBS, .rank = 6, .face_up = true };
    game.tableau[0].count = 3;

    /* Destination top: black 8, accepts red 7 run. */
    game.tableau[1].cards[0] = (Card){ .suit = SUIT_SPADES, .rank = 8, .face_up = true };
    game.tableau[1].count = 1;

    ck_assert(game_tableau_to_tableau(&game, 0, 1, 1));
    ck_assert_int_eq(game.tableau[0].count, 1);
    ck_assert(game.tableau[0].cards[0].face_up); /* auto-flip newly exposed card */
    ck_assert_int_eq(game.tableau[1].count, 3);
    ck_assert_int_eq(game.tableau[1].cards[1].rank, 7);
    ck_assert_int_eq(game.tableau[1].cards[2].rank, 6);
}
END_TEST

START_TEST(test_move_invalid_run_rejected)
{
    GameState game;
    memset(&game, 0, sizeof(game));

    /* Not a valid run: 7 then 6 of the SAME color. */
    game.tableau[0].cards[0] = (Card){ .suit = SUIT_HEARTS, .rank = 7, .face_up = true };
    game.tableau[0].cards[1] = (Card){ .suit = SUIT_DIAMONDS, .rank = 6, .face_up = true };
    game.tableau[0].count = 2;

    game.tableau[1].cards[0] = (Card){ .suit = SUIT_SPADES, .rank = 8, .face_up = true };
    game.tableau[1].count = 1;

    ck_assert(!game_tableau_to_tableau(&game, 0, 0, 1));
}
END_TEST

/* ---------- win detection ---------- */

START_TEST(test_win_detected_when_all_foundations_complete)
{
    GameState game;
    memset(&game, 0, sizeof(game));
    ck_assert(!game_is_won(&game));

    for (int i = 0; i < NUM_FOUNDATIONS; i++) {
        game.foundations[i].count = RANK_KING;
    }
    ck_assert(game_is_won(&game));
}
END_TEST

Suite *
game_suite(void)
{
    Suite *s = suite_create("Game");

    TCase *tc = tcase_create("Core");
    tcase_add_test(tc, test_deck_init_has_52_unique_cards);
    tcase_add_test(tc, test_deck_shuffle_is_deterministic_per_seed);
    tcase_add_test(tc, test_deck_shuffle_still_has_52_unique_cards);
    tcase_add_test(tc, test_new_game_deal_shape);
    tcase_add_test(tc, test_draw_from_stock_moves_card_face_up);
    tcase_add_test(tc, test_stock_recycles_from_waste_when_empty);
    tcase_add_test(tc, test_draw_fails_when_stock_and_waste_both_empty);
    tcase_add_test(tc, test_ace_goes_to_empty_foundation);
    tcase_add_test(tc, test_two_before_ace_rejected);
    tcase_add_test(tc, test_sequential_same_suit_accepted);
    tcase_add_test(tc, test_red_six_onto_black_seven);
    tcase_add_test(tc, test_same_color_rejected);
    tcase_add_test(tc, test_wrong_rank_rejected);
    tcase_add_test(tc, test_empty_tableau_accepts_only_king);
    tcase_add_test(tc, test_move_run_of_cards_between_tableau);
    tcase_add_test(tc, test_move_invalid_run_rejected);
    tcase_add_test(tc, test_win_detected_when_all_foundations_complete);
    suite_add_tcase(s, tc);

    return s;
}

int
main(void)
{
    Suite *s = game_suite();
    SRunner *sr = srunner_create(s);
    srunner_run_all(sr, CK_NORMAL);
    int failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return failed == 0 ? 0 : 1;
}
