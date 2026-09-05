#include "game_controller.h"
#include "../view/board_view.h"

#include <stdbool.h>
#include <stdlib.h>

#define UNDO_INITIAL_CAPACITY 64

typedef struct {
    GameState *game;
    GtkWidget *board_view;
    bool has_selection;
    PileKind sel_kind;
    int sel_index;
    int sel_card_pos;

    GameState *undo_stack;
    int undo_count;
    int undo_capacity;
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

static void
push_undo(Controller *ctrl, const GameState *snapshot)
{
    if (ctrl->undo_count == ctrl->undo_capacity) {
        ctrl->undo_capacity = (ctrl->undo_capacity == 0) ? UNDO_INITIAL_CAPACITY : ctrl->undo_capacity * 2;
        ctrl->undo_stack = g_realloc(ctrl->undo_stack, (gsize)ctrl->undo_capacity * sizeof(GameState));
    }
    ctrl->undo_stack[ctrl->undo_count++] = *snapshot;
}

static void
do_undo(Controller *ctrl)
{
    if (ctrl->undo_count == 0) {
        return;
    }
    *ctrl->game = ctrl->undo_stack[--ctrl->undo_count];
    clear_selection(ctrl);
    board_view_redraw(ctrl->board_view);
}

/* --- Move wrappers: snapshot the state beforehand and push it onto the
 * undo stack only if the move actually succeeded. --- */

static bool
try_draw(Controller *ctrl)
{
    GameState snap = *ctrl->game;
    bool ok = game_draw_from_stock(ctrl->game);
    if (ok) {
        push_undo(ctrl, &snap);
    }
    return ok;
}

static bool
try_waste_to_foundation(Controller *ctrl)
{
    GameState snap = *ctrl->game;
    bool ok = game_waste_to_foundation(ctrl->game);
    if (ok) {
        push_undo(ctrl, &snap);
    }
    return ok;
}

static bool
try_waste_to_tableau(Controller *ctrl, int tableau_idx)
{
    GameState snap = *ctrl->game;
    bool ok = game_waste_to_tableau(ctrl->game, tableau_idx);
    if (ok) {
        push_undo(ctrl, &snap);
    }
    return ok;
}

static bool
try_tableau_to_foundation(Controller *ctrl, int tableau_idx)
{
    GameState snap = *ctrl->game;
    bool ok = game_tableau_to_foundation(ctrl->game, tableau_idx);
    if (ok) {
        push_undo(ctrl, &snap);
    }
    return ok;
}

static bool
try_foundation_to_tableau(Controller *ctrl, int foundation_idx, int tableau_idx)
{
    GameState snap = *ctrl->game;
    bool ok = game_foundation_to_tableau(ctrl->game, foundation_idx, tableau_idx);
    if (ok) {
        push_undo(ctrl, &snap);
    }
    return ok;
}

static bool
try_tableau_to_tableau(Controller *ctrl, int from_idx, int card_pos, int to_idx)
{
    GameState snap = *ctrl->game;
    bool ok = game_tableau_to_tableau(ctrl->game, from_idx, card_pos, to_idx);
    if (ok) {
        push_undo(ctrl, &snap);
    }
    return ok;
}

/* Once the stock/waste are empty and every tableau card is face up, the
 * game is guaranteed winnable by repeatedly sending any playable top
 * card to its foundation. Auto-complete does exactly that, so the
 * player doesn't have to walk it out card by card. */
static bool
tableau_all_face_up(const GameState *game)
{
    for (int i = 0; i < NUM_TABLEAU; i++) {
        const Pile *pile = &game->tableau[i];
        for (int j = 0; j < pile->count; j++) {
            if (!pile->cards[j].face_up) {
                return false;
            }
        }
    }
    return true;
}

static void
try_auto_complete(Controller *ctrl)
{
    GameState *game = ctrl->game;
    if (game->stock.count != 0 || game->waste.count != 0) {
        return;
    }
    if (!tableau_all_face_up(game)) {
        return;
    }
    if (game_is_won(game)) {
        return;
    }

    /* One snapshot for the whole run, so a single undo reverts back to
     * the pre-auto-complete state rather than needing many undos. */
    GameState snapshot = *game;
    bool any_progress = false;
    bool progressed = true;
    while (progressed) {
        progressed = false;
        for (int i = 0; i < NUM_TABLEAU; i++) {
            if (game_tableau_to_foundation(game, i)) {
                progressed = true;
                any_progress = true;
            }
        }
    }
    if (any_progress) {
        push_undo(ctrl, &snapshot);
    }
}

static void
try_move_to(Controller *ctrl, BoardHit dest)
{
    switch (ctrl->sel_kind) {
    case PILE_WASTE:
        if (dest.kind == PILE_TABLEAU) {
            try_waste_to_tableau(ctrl, dest.index);
        } else if (dest.kind == PILE_FOUNDATION) {
            try_waste_to_foundation(ctrl);
        }
        break;
    case PILE_TABLEAU:
        if (dest.kind == PILE_TABLEAU && dest.index != ctrl->sel_index) {
            try_tableau_to_tableau(ctrl, ctrl->sel_index, ctrl->sel_card_pos, dest.index);
        } else if (dest.kind == PILE_FOUNDATION) {
            try_tableau_to_foundation(ctrl, ctrl->sel_index);
        }
        break;
    case PILE_FOUNDATION:
        if (dest.kind == PILE_TABLEAU) {
            try_foundation_to_tableau(ctrl, ctrl->sel_index, dest.index);
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
    Controller *ctrl = (Controller *)user_data;
    BoardHit hit = board_view_hit_test(ctrl->board_view, x, y);

    /* Double-click: send the clicked card straight to its foundation,
     * regardless of any prior selection. */
    if (n_press >= 2) {
        if (hit.kind == PILE_WASTE && hit.card_pos >= 0) {
            try_waste_to_foundation(ctrl);
            clear_selection(ctrl);
            try_auto_complete(ctrl);
            board_view_redraw(ctrl->board_view);
            return;
        }
        if (hit.kind == PILE_TABLEAU && hit.card_pos >= 0) {
            Pile *pile = &ctrl->game->tableau[hit.index];
            if (hit.card_pos == pile->count - 1) {
                try_tableau_to_foundation(ctrl, hit.index);
                clear_selection(ctrl);
                try_auto_complete(ctrl);
                board_view_redraw(ctrl->board_view);
                return;
            }
        }
    }

    if (hit.kind == PILE_STOCK) {
        try_draw(ctrl);
        clear_selection(ctrl);
        try_auto_complete(ctrl);
        board_view_redraw(ctrl->board_view);
        return;
    }

    if (!ctrl->has_selection) {
        if (hit.kind == PILE_WASTE && hit.card_pos >= 0) {
            set_selection(ctrl, PILE_WASTE, 0, -1);
        } else if (hit.kind == PILE_TABLEAU && hit.card_pos >= 0) {
            set_selection(ctrl, PILE_TABLEAU, hit.index, hit.card_pos);
        } else if (hit.kind == PILE_FOUNDATION && ctrl->game->foundations[hit.index].count > 0) {
            set_selection(ctrl, PILE_FOUNDATION, hit.index, -1);
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
    try_auto_complete(ctrl);
    board_view_redraw(ctrl->board_view);
}

static gboolean
on_key_pressed(GtkEventControllerKey *controller, guint keyval, guint keycode, GdkModifierType state, gpointer user_data)
{
    (void)controller;
    (void)keycode;
    Controller *ctrl = (Controller *)user_data;
    if ((state & GDK_CONTROL_MASK) && (keyval == GDK_KEY_z || keyval == GDK_KEY_Z)) {
        do_undo(ctrl);
        return TRUE;
    }
    return FALSE;
}

static void
on_board_destroy(GtkWidget *widget, gpointer user_data)
{
    (void)widget;
    Controller *ctrl = (Controller *)user_data;
    g_free(ctrl->undo_stack);
    free(ctrl);
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
    ctrl->undo_stack = NULL;
    ctrl->undo_count = 0;
    ctrl->undo_capacity = 0;

    GtkGesture *click = gtk_gesture_click_new();
    g_signal_connect(click, "pressed", G_CALLBACK(on_pressed), ctrl);
    gtk_widget_add_controller(board_view, GTK_EVENT_CONTROLLER(click));

    /* Ctrl+Z for undo. Attached to the window (root) with CAPTURE phase
     * so it works regardless of which widget currently has focus. */
    GtkRoot *root = gtk_widget_get_root(board_view);
    if (root != NULL) {
        GtkEventController *key_ctrl = gtk_event_controller_key_new();
        gtk_event_controller_set_propagation_phase(key_ctrl, GTK_PHASE_CAPTURE);
        g_signal_connect(key_ctrl, "key-pressed", G_CALLBACK(on_key_pressed), ctrl);
        gtk_widget_add_controller(GTK_WIDGET(root), key_ctrl);
    }

    g_signal_connect(board_view, "destroy", G_CALLBACK(on_board_destroy), ctrl);

    g_object_set_data(G_OBJECT(board_view), "game-controller", ctrl);
}

void
game_controller_new_game(GtkWidget *board_view, unsigned int seed)
{
    Controller *ctrl = g_object_get_data(G_OBJECT(board_view), "game-controller");
    if (ctrl == NULL) {
        return;
    }
    game_new(ctrl->game, seed);
    clear_selection(ctrl);
    ctrl->undo_count = 0; /* a new deal isn't undoable back into the old one */
    board_view_reset_animation(board_view);
    board_view_redraw(board_view);
}
