#include "game_controller.h"
#include "../view/board_view.h"

#include <stdbool.h>
#include <stdlib.h>

typedef struct {
    GameState *game;
    GtkWidget *board_view;
    bool has_selection;
    PileKind sel_kind;
    int sel_index;
    int sel_card_pos;
} Controller;

static void
clear_selection(Controller *ctrl)
{
    ctrl->has_selection = false;
    board_view_clear_selection(ctrl->board_view);
}

static void
set_selection(Controller *ctrl, PileKind kind, int index, int card_pos)
{
    ctrl->has_selection = true;
    ctrl->sel_kind = kind;
    ctrl->sel_index = index;
    ctrl->sel_card_pos = card_pos;
    board_view_set_selection(ctrl->board_view, kind, index, card_pos);
}

/* Attempts to move the current selection onto `dest`. Whether this
 * succeeds or not is left to the caller to handle (currently: the
 * selection is cleared either way, so an illegal target is simply a
 * no-op click). */
static void
try_move_to(Controller *ctrl, BoardHit dest)
{
    switch (ctrl->sel_kind) {
    case PILE_WASTE:
        if (dest.kind == PILE_TABLEAU) {
            game_waste_to_tableau(ctrl->game, dest.index);
        } else if (dest.kind == PILE_FOUNDATION) {
            game_waste_to_foundation(ctrl->game);
        }
        break;
    case PILE_TABLEAU:
        if (dest.kind == PILE_TABLEAU && dest.index != ctrl->sel_index) {
            game_tableau_to_tableau(ctrl->game, ctrl->sel_index, ctrl->sel_card_pos, dest.index);
        } else if (dest.kind == PILE_FOUNDATION) {
            game_tableau_to_foundation(ctrl->game, ctrl->sel_index);
        }
        break;
    default:
        break;
    }
}

static void
on_pressed(GtkGestureClick *gesture, int n_press, double x, double y, gpointer user_data)
{
    (void)gesture;
    (void)n_press;
    Controller *ctrl = (Controller *)user_data;
    BoardHit hit = board_view_hit_test(ctrl->board_view, x, y);

    if (hit.kind == PILE_STOCK) {
        game_draw_from_stock(ctrl->game);
        clear_selection(ctrl);
        board_view_redraw(ctrl->board_view);
        return;
    }

    if (!ctrl->has_selection) {
        if (hit.kind == PILE_WASTE && hit.card_pos >= 0) {
            set_selection(ctrl, PILE_WASTE, 0, -1);
        } else if (hit.kind == PILE_TABLEAU && hit.card_pos >= 0) {
            set_selection(ctrl, PILE_TABLEAU, hit.index, hit.card_pos);
        }
        board_view_redraw(ctrl->board_view);
        return;
    }

    if (hit.kind == ctrl->sel_kind && hit.index == ctrl->sel_index) {
        clear_selection(ctrl);
        board_view_redraw(ctrl->board_view);
        return;
    }

    try_move_to(ctrl, hit);
    clear_selection(ctrl);
    board_view_redraw(ctrl->board_view);
}

static void
on_board_destroy(GtkWidget *widget, gpointer user_data)
{
    (void)widget;
    free(user_data);
}

void
game_controller_attach(GameState *game, GtkWidget *board_view)
{
    Controller *ctrl = malloc(sizeof(Controller));
    ctrl->game = game;
    ctrl->board_view = board_view;
    ctrl->has_selection = false;
    ctrl->sel_kind = PILE_NONE;
    ctrl->sel_index = -1;
    ctrl->sel_card_pos = -1;

    GtkGesture *click = gtk_gesture_click_new();
    g_signal_connect(click, "pressed", G_CALLBACK(on_pressed), ctrl);
    gtk_widget_add_controller(board_view, GTK_EVENT_CONTROLLER(click));

    g_signal_connect(board_view, "destroy", G_CALLBACK(on_board_destroy), ctrl);
}
