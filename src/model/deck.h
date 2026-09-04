#ifndef SOLITAIRE_DECK_H
#define SOLITAIRE_DECK_H

#include "card.h"

#define DECK_SIZE 52

/* A deck is treated as a stack: cards[0] is the bottom, cards[count-1]
 * is the top (the next card that would be drawn/dealt). */
typedef struct {
    Card cards[DECK_SIZE];
    int count;
} Deck;

/* Fills the deck with all 52 standard cards, face down, in a fixed
 * canonical order (clubs A..K, diamonds A..K, hearts A..K, spades A..K). */
void deck_init_standard(Deck *deck);

/* Shuffles the deck in place using a seeded Fisher-Yates shuffle.
 * Same seed always produces the same order, for reproducible tests/deals. */
void deck_shuffle(Deck *deck, unsigned int seed);

/* Removes and returns the top card of the deck. Caller must ensure
 * deck->count > 0. */
Card deck_pop(Deck *deck);

#endif /* SOLITAIRE_DECK_H */
