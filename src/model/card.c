#include "card.h"

#include <stdio.h>

bool
card_is_red(const Card *c)
{
    return c->suit == SUIT_DIAMONDS || c->suit == SUIT_HEARTS;
}

bool
card_colors_differ(const Card *a, const Card *b)
{
    return card_is_red(a) != card_is_red(b);
}

const char *
card_rank_str(int rank)
{
    switch (rank) {
    case RANK_ACE:   return "A";
    case 2:          return "2";
    case 3:          return "3";
    case 4:          return "4";
    case 5:          return "5";
    case 6:          return "6";
    case 7:          return "7";
    case 8:          return "8";
    case 9:          return "9";
    case 10:         return "10";
    case RANK_JACK:  return "J";
    case RANK_QUEEN: return "Q";
    case RANK_KING:  return "K";
    default:         return "?";
    }
}

const char *
card_suit_str(Suit suit)
{
    switch (suit) {
    case SUIT_CLUBS:    return "Clubs";
    case SUIT_DIAMONDS: return "Diamonds";
    case SUIT_HEARTS:   return "Hearts";
    case SUIT_SPADES:   return "Spades";
    default:            return "Unknown";
    }
}

void
card_to_string(const Card *c, char *buf, size_t buflen)
{
    snprintf(buf, buflen, "%s of %s", card_rank_str(c->rank), card_suit_str(c->suit));
}
