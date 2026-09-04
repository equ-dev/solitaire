#include "app_window.h"
#include "board_view.h"
#include "../model/game.h"

#include <stdlib.h>
#include <time.h>

static void
on_window_destroy(GtkWidget *widget, gpointer user_data)
{
    (void)widget;
    GameState *game = (GameState *)user_data;
    free(game);
}

GtkWidget *
app_window_new(GtkApplication *app)
{
    GameState *game = malloc(sizeof(GameState));
    game_new(game, (unsigned int)time(NULL));

    GtkWidget *window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "Solitaire");
    gtk_window_set_default_size(GTK_WINDOW(window), 1024, 768);

    GtkWidget *board = board_view_new(game);
    gtk_window_set_child(GTK_WINDOW(window), board);

    g_signal_connect(window, "destroy", G_CALLBACK(on_window_destroy), game);

    return window;
}
