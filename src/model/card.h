#ifndef SOLITAIRE_CARD_H
#define SOLITAIRE_CARD_H

#include <stdbool.h>
#include <stddef.h>

typedef enum {
    SUIT_CLUBS = 0,
    SUIT_DIAMONDS,
    SUIT_HEARTS,
    SUIT_SPADES,
    SUIT_COUNT
} Suit;

#define RANK_ACE 1
#define RANK_JACK 11
#define RANK_QUEEN 12
#define RANK_KING 13

typedef struct {
    Suit suit;
    int rank;       /* 1 (Ace) .. 13 (King) */
    bool face_up;
} Card;

/* Red suits are diamonds and hearts; black suits are clubs and spades. */
bool card_is_red(const Card *c);

/* Returns true if `a` and `b` have opposite colors (one red, one black). */
bool card_colors_differ(const Card *a, const Card *b);

/* Short rank label: "A", "2".."10", "J", "Q", "K". */
const char *card_rank_str(int rank);

/* Short suit label: "Clubs", "Diamonds", "Hearts", "Spades". */
const char *card_suit_str(Suit suit);

/* Writes a human-readable representation, e.g. "K of Spades", into buf. */
void card_to_string(const Card *c, char *buf, size_t buflen);

#endif /* SOLITAIRE_CARD_H */
