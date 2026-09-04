#ifndef SOLITAIRE_APP_WINDOW_H
#define SOLITAIRE_APP_WINDOW_H

#include <gtk/gtk.h>

/* Creates the main application window: allocates a fresh GameState,
 * embeds a board_view showing it, and frees the GameState when the
 * window is destroyed. */
GtkWidget *app_window_new(GtkApplication *app);

#endif /* SOLITAIRE_APP_WINDOW_H */
