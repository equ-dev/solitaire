#ifndef SOLITAIRE_GAME_H
#define SOLITAIRE_GAME_H

#include "card.h"

#define NUM_TABLEAU 7
#define NUM_FOUNDATIONS 4
#define MAX_PILE 52

typedef struct {
    Card cards[MAX_PILE];
    int count;
} Pile;

typedef struct {
    Pile stock;
    Pile waste;
    Pile foundations[NUM_FOUNDATIONS]; /* indexed by Suit */
    Pile tableau[NUM_TABLEAU];
} GameState;

/* Initializes a new standard Klondike deal: shuffles a fresh deck with
 * `seed`, deals tableau piles of 1..7 cards (top card face up, rest face
 * down), and puts the remaining 24 cards face down in the stock. */
void game_new(GameState *game, unsigned int seed);

/* Draws one card from stock to waste (face up). If stock is empty and
 * waste is not, recycles all of waste back into stock (face down,
 * reversed) instead of drawing. Returns false only if both are empty. */
bool game_draw_from_stock(GameState *game);

/* Moves the top card of waste onto its foundation, if legal. */
bool game_waste_to_foundation(GameState *game);

/* Moves the top card of waste onto tableau pile `tableau_idx`, if legal. */
bool game_waste_to_tableau(GameState *game, int tableau_idx);

/* Moves the top card of tableau pile `tableau_idx` onto its foundation,
 * if legal. */
bool game_tableau_to_foundation(GameState *game, int tableau_idx);

/* Moves the run of cards starting at position `card_pos` (0-based from
 * the bottom of the pile) in tableau pile `from_idx` onto tableau pile
 * `to_idx`, if the run is a legally-sequenced, face-up run and the
 * destination accepts it. */
bool game_tableau_to_tableau(GameState *game, int from_idx, int card_pos, int to_idx);

/* True once all four foundations hold a complete King-high suit. */
bool game_is_won(const GameState *game);

#endif /* SOLITAIRE_GAME_H */
