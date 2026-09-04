#include "deck.h"

#include <assert.h>
#include <stdlib.h>

void
deck_init_standard(Deck *deck)
{
    int i = 0;
    for (Suit s = SUIT_CLUBS; s < SUIT_COUNT; s++) {
        for (int rank = RANK_ACE; rank <= RANK_KING; rank++) {
            deck->cards[i].suit = s;
            deck->cards[i].rank = rank;
            deck->cards[i].face_up = false;
            i++;
        }
    }
    deck->count = DECK_SIZE;
}

void
deck_shuffle(Deck *deck, unsigned int seed)
{
    unsigned int state = seed;

    /* Fisher-Yates, using rand_r for reproducibility independent of any
     * other rand() usage elsewhere in the program. */
    for (int i = deck->count - 1; i > 0; i--) {
        int j = rand_r(&state) % (i + 1);
        Card tmp = deck->cards[i];
        deck->cards[i] = deck->cards[j];
        deck->cards[j] = tmp;
    }
}

Card
deck_pop(Deck *deck)
{
    assert(deck->count > 0);
    return deck->cards[--deck->count];
}
