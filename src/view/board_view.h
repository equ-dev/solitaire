#ifndef SOLITAIRE_BOARD_VIEW_H
#define SOLITAIRE_BOARD_VIEW_H

#include <gtk/gtk.h>

#include "../model/game.h"

/* Creates a GtkDrawingArea that renders the current state of `game`.
 * The widget holds a pointer to `game` (not owned/copied) and reads
 * from it on every draw. Call board_view_redraw() after the game
 * state changes to request a repaint. */
GtkWidget *board_view_new(GameState *game);

/* Queues a redraw of the board. */
void board_view_redraw(GtkWidget *board_view);

#endif /* SOLITAIRE_BOARD_VIEW_H */
