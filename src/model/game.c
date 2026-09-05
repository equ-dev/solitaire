#include "game.h"
#include "deck.h"

#include <string.h>

static void
pile_push(Pile *pile, Card card)
{
    pile->cards[pile->count++] = card;
}

static Card
pile_pop(Pile *pile)
{
    return pile->cards[--pile->count];
}

static Card *
pile_top(Pile *pile)
{
    if (pile->count == 0) {
        return NULL;
    }
    return &pile->cards[pile->count - 1];
}

static void
flip_new_top_face_up(Pile *pile)
{
    Card *top = pile_top(pile);
    if (top != NULL && !top->face_up) {
        top->face_up = true;
    }
}

void
game_new(GameState *game, unsigned int seed)
{
    memset(game, 0, sizeof(*game));

    Deck deck;
    deck_init_standard(&deck);
    deck_shuffle(&deck, seed);

    for (int col = 0; col < NUM_TABLEAU; col++) {
        for (int row = 0; row <= col; row++) {
            Card c = deck_pop(&deck);
            c.face_up = (row == col); /* only the last card dealt is face up */
            pile_push(&game->tableau[col], c);
        }
    }

    while (deck.count > 0) {
        Card c = deck_pop(&deck);
        c.face_up = false;
        pile_push(&game->stock, c);
    }
}

bool
game_draw_from_stock(GameState *game)
{
    if (game->stock.count > 0) {
        Card c = pile_pop(&game->stock);
        c.face_up = true;
        pile_push(&game->waste, c);
        return true;
    }

    if (game->waste.count == 0) {
        return false; /* nothing to draw or recycle */
    }

    /* Recycle: move waste back to stock, face down, reversed so the
     * card that was drawn first ends up on the bottom again. */
    while (game->waste.count > 0) {
        Card c = pile_pop(&game->waste);
        c.face_up = false;
        pile_push(&game->stock, c);
    }
    return true;
}

static bool
can_place_on_foundation(const Pile *foundation, const Card *card)
{
    if (foundation->count == 0) {
        return card->rank == RANK_ACE;
    }
    const Card *top = &foundation->cards[foundation->count - 1];
    return top->suit == card->suit && top->rank + 1 == card->rank;
}

static bool
can_place_on_tableau(const Pile *tableau, const Card *card)
{
    if (tableau->count == 0) {
        return card->rank == RANK_KING;
    }
    const Card *top = &tableau->cards[tableau->count - 1];
    return top->face_up && card_colors_differ(top, card) && top->rank == card->rank + 1;
}

bool
game_waste_to_foundation(GameState *game)
{
    Card *top = pile_top(&game->waste);
    if (top == NULL) {
        return false;
    }
    Pile *foundation = &game->foundations[top->suit];
    if (!can_place_on_foundation(foundation, top)) {
        return false;
    }
    pile_push(foundation, pile_pop(&game->waste));
    return true;
}

bool
game_waste_to_tableau(GameState *game, int tableau_idx)
{
    if (tableau_idx < 0 || tableau_idx >= NUM_TABLEAU) {
        return false;
    }
    Card *top = pile_top(&game->waste);
    if (top == NULL) {
        return false;
    }
    Pile *dest = &game->tableau[tableau_idx];
    if (!can_place_on_tableau(dest, top)) {
        return false;
    }
    pile_push(dest, pile_pop(&game->waste));
    return true;
}

bool
game_tableau_to_foundation(GameState *game, int tableau_idx)
{
    if (tableau_idx < 0 || tableau_idx >= NUM_TABLEAU) {
        return false;
    }
    Pile *src = &game->tableau[tableau_idx];
    Card *top = pile_top(src);
    if (top == NULL || !top->face_up) {
        return false;
    }
    Pile *foundation = &game->foundations[top->suit];
    if (!can_place_on_foundation(foundation, top)) {
        return false;
    }
    pile_push(foundation, pile_pop(src));
    flip_new_top_face_up(src);
    return true;
}

bool
game_foundation_to_tableau(GameState *game, int foundation_idx, int tableau_idx)
{
    if (foundation_idx < 0 || foundation_idx >= NUM_FOUNDATIONS) {
        return false;
    }
    if (tableau_idx < 0 || tableau_idx >= NUM_TABLEAU) {
        return false;
    }
    Pile *src = &game->foundations[foundation_idx];
    Card *top = pile_top(src);
    if (top == NULL) {
        return false;
    }
    Pile *dest = &game->tableau[tableau_idx];
    if (!can_place_on_tableau(dest, top)) {
        return false;
    }
    pile_push(dest, pile_pop(src));
    return true;
}

/* Checks that cards[pos..count-1] form a legally-sequenced, all-face-up
 * run: strictly descending rank, alternating colors. A single card is
 * trivially a valid run. */
static bool
is_valid_run(const Pile *pile, int pos)
{
    if (pos < 0 || pos >= pile->count) {
        return false;
    }
    for (int i = pos; i < pile->count; i++) {
        if (!pile->cards[i].face_up) {
            return false;
        }
    }
    for (int i = pos; i < pile->count - 1; i++) {
        const Card *upper = &pile->cards[i];
        const Card *lower = &pile->cards[i + 1];
        if (!card_colors_differ(upper, lower) || upper->rank != lower->rank + 1) {
            return false;
        }
    }
    return true;
}

bool
game_tableau_to_tableau(GameState *game, int from_idx, int card_pos, int to_idx)
{
    if (from_idx < 0 || from_idx >= NUM_TABLEAU || to_idx < 0 || to_idx >= NUM_TABLEAU) {
        return false;
    }
    if (from_idx == to_idx) {
        return false;
    }
    Pile *src = &game->tableau[from_idx];
    if (!is_valid_run(src, card_pos)) {
        return false;
    }

    Card *moving_bottom = &src->cards[card_pos]; /* the card that will touch the dest pile */
    Pile *dest = &game->tableau[to_idx];
    if (!can_place_on_tableau(dest, moving_bottom)) {
        return false;
    }

    int run_len = src->count - card_pos;
    for (int i = 0; i < run_len; i++) {
        pile_push(dest, src->cards[card_pos + i]);
    }
    src->count = card_pos;
    flip_new_top_face_up(src);
    return true;
}

bool
game_is_won(const GameState *game)
{
    for (int i = 0; i < NUM_FOUNDATIONS; i++) {
        if (game->foundations[i].count != RANK_KING) {
            return false;
        }
    }
    return true;
}
