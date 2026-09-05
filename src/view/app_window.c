#include "app_window.h"
#include "board_view.h"
#include "../model/game.h"
#include "../controller/game_controller.h"

#include <stdlib.h>
#include <time.h>

static void
on_window_destroy(GtkWidget *widget, gpointer user_data)
{
    (void)widget;
    GameState *game = (GameState *)user_data;
    free(game);
}

static void
on_new_game_clicked(GtkButton *button, gpointer user_data)
{
    (void)button;
    GtkWidget *board = GTK_WIDGET(user_data);
    game_controller_new_game(board, g_random_int());
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
    game_controller_attach(game, board);

    /* The New Game button lives in its own row above the felt, not
     * overlaid on top of it. An overlay was tried first, but the board
     * lays out its piles internally (stock/waste/foundations/tableau)
     * using its own coordinate math anchored to the same top-right
     * corner as an overlay widget's alignment margins -- so any
     * overlay button wide enough to hold a label always collided with
     * the last foundation pile, at every window size. A separate row
     * avoids that class of bug entirely: the board is simply shorter
     * by the row's height and never draws under it. */
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

    GtkWidget *toolbar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_set_margin_top(toolbar, 8);
    gtk_widget_set_margin_bottom(toolbar, 8);
    gtk_widget_set_margin_end(toolbar, 12);

    GtkWidget *new_game_btn = gtk_button_new_with_label("New Game");
    gtk_widget_set_halign(new_game_btn, GTK_ALIGN_END);
    gtk_widget_set_hexpand(new_game_btn, TRUE);
    g_signal_connect(new_game_btn, "clicked", G_CALLBACK(on_new_game_clicked), board);
    gtk_box_append(GTK_BOX(toolbar), new_game_btn);

    gtk_box_append(GTK_BOX(vbox), toolbar);
    gtk_box_append(GTK_BOX(vbox), board);

    gtk_window_set_child(GTK_WINDOW(window), vbox);

    g_signal_connect(window, "destroy", G_CALLBACK(on_window_destroy), game);

    return window;
}
