#ifndef SOLITAIRE_BOARD_VIEW_H
#define SOLITAIRE_BOARD_VIEW_H

#include <gtk/gtk.h>

#include "../model/game.h"

typedef enum {
    PILE_NONE = 0,
    PILE_STOCK,
    PILE_WASTE,
    PILE_FOUNDATION,
    PILE_TABLEAU
} PileKind;

typedef struct {
    PileKind kind;
    int index;    /* unused for STOCK/WASTE; 0..3 for FOUNDATION; 0..6 for TABLEAU */
    int card_pos; /* TABLEAU: row index of the clicked card, -1 if pile empty;
                   * WASTE: 0 if non-empty, -1 if empty; unused otherwise */
} BoardHit;

/* Creates a GtkDrawingArea that renders the current state of `game`.
 * The widget holds a pointer to `game` (not owned/copied) and reads
 * from it on every draw. Call board_view_redraw() after the game
 * state changes to request a repaint. */
GtkWidget *board_view_new(GameState *game);

/* Queues a redraw of the board. */
void board_view_redraw(GtkWidget *board_view);

/* Translates a widget-local point (as delivered by a click gesture)
 * into the pile/card it landed on. Returns a BoardHit with
 * kind == PILE_NONE if the point didn't land on any recognized area. */
BoardHit board_view_hit_test(GtkWidget *board_view, double x, double y);

/* Highlights the given pile/card as selected (drawn with a gold
 * outline) and queues a redraw. */
void board_view_set_selection(GtkWidget *board_view, PileKind kind, int index, int card_pos);

/* Removes the selection highlight and queues a redraw. */
void board_view_clear_selection(GtkWidget *board_view);

/* Clears any in-progress or completed win-bounce animation and its
 * "already won" edge-detection state. Call this when starting a new
 * game so a lingering animation from the previous game doesn't carry
 * over or immediately re-trigger. */
void board_view_reset_animation(GtkWidget *board_view);

#endif /* SOLITAIRE_BOARD_VIEW_H */
