#ifndef SOLITAIRE_GAME_CONTROLLER_H
#define SOLITAIRE_GAME_CONTROLLER_H

#include <gtk/gtk.h>

#include "../model/game.h"

/* Wires user clicks on `board_view` to mutate `game` via the public
 * game_* move functions (click-to-select, click-to-move), and keeps
 * the view's selection highlight and redraws in sync.
 *
 * Interaction model: clicking the stock always draws (or recycles).
 * Clicking a movable card with nothing selected selects it (and, for
 * a tableau card, the run below it). Clicking the same pile again
 * deselects. Clicking a different pile while something is selected
 * attempts to move the selection there; the selection is cleared
 * afterward whether or not the move was legal.
 *
 * Call once after creating the board view. The controller's lifetime
 * is tied to `board_view` (freed automatically when the widget is
 * destroyed); callers do not need to free anything. */
void game_controller_attach(GameState *game, GtkWidget *board_view);

#endif /* SOLITAIRE_GAME_CONTROLLER_H */
