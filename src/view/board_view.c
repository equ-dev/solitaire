#include "board_view.h"
#include "win_animation.h"

#define CARD_W 80.0
#define CARD_H 112.0
#define MARGIN 24.0
#define GAP 16.0
#define STACK_OFFSET 26.0
#define TOP_ROW_GAP 40.0
#define DEG (3.14159265358979323846 / 180.0)

typedef struct {
    GameState *game;
    PileKind sel_kind;
    int sel_index;
    int sel_card_pos;

    WinAnimation win_anim;
    bool was_won;        /* edge-detects the moment game_is_won() becomes true */
    gint64 last_frame_us; /* frame-clock timestamp of the previous tick, 0 = none yet */
} BoardViewData;

static void
rounded_rect(cairo_t *cr, double x, double y, double w, double h, double r)
{
    cairo_new_sub_path(cr);
    cairo_arc(cr, x + w - r, y + r, r, -90 * DEG, 0 * DEG);
    cairo_arc(cr, x + w - r, y + h - r, r, 0 * DEG, 90 * DEG);
    cairo_arc(cr, x + r, y + h - r, r, 90 * DEG, 180 * DEG);
    cairo_arc(cr, x + r, y + r, r, 180 * DEG, 270 * DEG);
    cairo_close_path(cr);
}

static void
draw_empty_slot(cairo_t *cr, double x, double y, double w, double h)
{
    rounded_rect(cr, x, y, w, h, 10);
    cairo_set_source_rgba(cr, 1, 1, 1, 0.12);
    cairo_fill_preserve(cr);
    cairo_set_source_rgba(cr, 1, 1, 1, 0.4);
    double dash[] = { 4, 4 };
    cairo_set_dash(cr, dash, 2, 0);
    cairo_set_line_width(cr, 1.5);
    cairo_stroke(cr);
    cairo_set_dash(cr, NULL, 0, 0);
}

static void
draw_card_back(cairo_t *cr, double x, double y, double w, double h)
{
    rounded_rect(cr, x, y, w, h, 8);
    cairo_set_source_rgb(cr, 0.10, 0.20, 0.55);
    cairo_fill_preserve(cr);
    cairo_set_source_rgb(cr, 0.85, 0.85, 0.95);
    cairo_set_line_width(cr, 2);
    cairo_stroke(cr);

    rounded_rect(cr, x + 6, y + 6, w - 12, h - 12, 6);
    cairo_set_source_rgba(cr, 1, 1, 1, 0.25);
    cairo_set_line_width(cr, 1.5);
    cairo_stroke(cr);
}

/* Suit icons are drawn as filled vector shapes rather than text glyphs.
 * Unicode suit characters (\u2663 etc.) depend on the system's default
 * font actually covering those code points, which is not guaranteed --
 * on some setups "Sans" falls back to a font with no card-suit glyphs
 * and renders empty tofu boxes instead. Vector shapes render identically
 * everywhere regardless of installed fonts. */

static void
draw_diamond(cairo_t *cr, double cx, double cy, double s)
{
    cairo_move_to(cr, cx, cy - s * 0.9);
    cairo_line_to(cr, cx + s * 0.6, cy);
    cairo_line_to(cr, cx, cy + s * 0.9);
    cairo_line_to(cr, cx - s * 0.6, cy);
    cairo_close_path(cr);
    cairo_fill(cr);
}

static void
draw_heart(cairo_t *cr, double cx, double cy, double s)
{
    double r = s * 0.32;
    cairo_new_sub_path(cr);
    cairo_arc(cr, cx - r * 0.9, cy - r * 0.3, r, 0, 2 * G_PI);
    cairo_new_sub_path(cr);
    cairo_arc(cr, cx + r * 0.9, cy - r * 0.3, r, 0, 2 * G_PI);
    cairo_new_sub_path(cr);
    cairo_move_to(cr, cx - r * 1.9, cy - r * 0.1);
    cairo_line_to(cr, cx + r * 1.9, cy - r * 0.1);
    cairo_line_to(cr, cx, cy + s * 0.9);
    cairo_close_path(cr);
    cairo_fill(cr);
}

static void
draw_spade(cairo_t *cr, double cx, double cy, double s)
{
    double r = s * 0.32;
    /* Point-up heart shape for the top of the spade. */
    cairo_new_sub_path(cr);
    cairo_arc(cr, cx - r * 0.9, cy + r * 0.3, r, 0, 2 * G_PI);
    cairo_new_sub_path(cr);
    cairo_arc(cr, cx + r * 0.9, cy + r * 0.3, r, 0, 2 * G_PI);
    cairo_new_sub_path(cr);
    cairo_move_to(cr, cx - r * 1.9, cy + r * 0.1);
    cairo_line_to(cr, cx + r * 1.9, cy + r * 0.1);
    cairo_line_to(cr, cx, cy - s * 0.9);
    cairo_close_path(cr);
    /* Stem. */
    cairo_new_sub_path(cr);
    cairo_move_to(cr, cx - s * 0.12, cy + r * 0.9);
    cairo_line_to(cr, cx + s * 0.12, cy + r * 0.9);
    cairo_line_to(cr, cx + s * 0.35, cy + s * 0.9);
    cairo_line_to(cr, cx - s * 0.35, cy + s * 0.9);
    cairo_close_path(cr);
    cairo_fill(cr);
}

static void
draw_club(cairo_t *cr, double cx, double cy, double s)
{
    double r = s * 0.30;
    cairo_new_sub_path(cr);
    cairo_arc(cr, cx, cy - r * 0.95, r, 0, 2 * G_PI);
    cairo_new_sub_path(cr);
    cairo_arc(cr, cx - r * 0.9, cy + r * 0.5, r, 0, 2 * G_PI);
    cairo_new_sub_path(cr);
    cairo_arc(cr, cx + r * 0.9, cy + r * 0.5, r, 0, 2 * G_PI);
    /* Stem. */
    cairo_new_sub_path(cr);
    cairo_move_to(cr, cx - s * 0.12, cy + r * 0.3);
    cairo_line_to(cr, cx + s * 0.12, cy + r * 0.3);
    cairo_line_to(cr, cx + s * 0.32, cy + s * 0.9);
    cairo_line_to(cr, cx - s * 0.32, cy + s * 0.9);
    cairo_close_path(cr);
    cairo_fill(cr);
}

static void
draw_suit_icon(cairo_t *cr, Suit suit, double cx, double cy, double size)
{
    switch (suit) {
    case SUIT_CLUBS:    draw_club(cr, cx, cy, size);    break;
    case SUIT_DIAMONDS: draw_diamond(cr, cx, cy, size); break;
    case SUIT_HEARTS:   draw_heart(cr, cx, cy, size);   break;
    case SUIT_SPADES:   draw_spade(cr, cx, cy, size);   break;
    default: break;
    }
}

static void
draw_card_face(cairo_t *cr, double x, double y, double w, double h, const Card *card)
{
    rounded_rect(cr, x, y, w, h, 8);
    cairo_set_source_rgb(cr, 0.98, 0.98, 0.96);
    cairo_fill_preserve(cr);
    cairo_set_source_rgb(cr, 0.15, 0.15, 0.15);
    cairo_set_line_width(cr, 1.5);
    cairo_stroke(cr);

    if (card_is_red(card)) {
        cairo_set_source_rgb(cr, 0.75, 0.08, 0.08);
    } else {
        cairo_set_source_rgb(cr, 0.08, 0.08, 0.08);
    }

    const char *rank = card_rank_str(card->rank);

    cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size(cr, 16);
    cairo_move_to(cr, x + 6, y + 20);
    cairo_show_text(cr, rank);

    draw_suit_icon(cr, card->suit, x + 14, y + 38, 9);
    draw_suit_icon(cr, card->suit, x + w / 2, y + h / 2, 24);
}

static void
draw_selection_highlight(cairo_t *cr, double x, double y, double w, double h)
{
    rounded_rect(cr, x - 3, y - 3, w + 6, h + 6, 10);
    cairo_set_source_rgb(cr, 0.95, 0.75, 0.15);
    cairo_set_line_width(cr, 3);
    cairo_stroke(cr);
}

static double
foundation_x(int width, int idx)
{
    double found_total_w = NUM_FOUNDATIONS * CARD_W + (NUM_FOUNDATIONS - 1) * GAP;
    double found_x0 = width - MARGIN - found_total_w;
    return found_x0 + idx * (CARD_W + GAP);
}

/* Draws one bouncing card, rotated about its own center. Used only by
 * the win animation; ordinary pile rendering has no rotation. */
static void
draw_bouncing_card(cairo_t *cr, const BounceCard *bc)
{
    cairo_save(cr);
    double cx = bc->x + CARD_W / 2.0;
    double cy = bc->y + CARD_H / 2.0;
    cairo_translate(cr, cx, cy);
    cairo_rotate(cr, bc->rotation);
    cairo_translate(cr, -cx, -cy);
    draw_card_face(cr, bc->x, bc->y, CARD_W, CARD_H, &bc->card);
    cairo_restore(cr);
}

/* Populates `anim` with all 52 foundation cards, spawning in round-robin
 * order across the four piles (one card from each in turn) so the
 * cascade fans out from all four corners at once rather than draining
 * one pile at a time. Each card gets a randomized initial "pop" velocity
 * and spin so the shower doesn't look mechanical. */
static void
start_win_animation(WinAnimation *anim, GameState *game, int width)
{
    win_anim_reset(anim);
    for (int rank = 0; rank < MAX_PILE / NUM_FOUNDATIONS; rank++) {
        for (int suit = 0; suit < NUM_FOUNDATIONS; suit++) {
            Pile *pile = &game->foundations[suit];
            if (rank >= pile->count) {
                continue;
            }
            double origin_x = foundation_x(width, suit);
            double origin_y = MARGIN;
            double vx = g_random_double_range(-160.0, 160.0);
            double vy = g_random_double_range(-420.0, -180.0); /* upward pop, gravity takes over */
            double vrot = g_random_double_range(-4.0, 4.0);
            win_anim_add_card(anim, pile->cards[rank], origin_x, origin_y, vx, vy, vrot);
        }
    }
}

static void
board_draw_func(GtkDrawingArea *area, cairo_t *cr, int width, int height, gpointer user_data)
{
    (void)area;
    BoardViewData *data = (BoardViewData *)user_data;
    GameState *game = data->game;

    /* Felt background. */
    cairo_set_source_rgb(cr, 0.06, 0.35, 0.14);
    cairo_paint(cr);

    /* Stock. */
    double x = MARGIN;
    double y = MARGIN;
    if (game->stock.count > 0) {
        draw_card_back(cr, x, y, CARD_W, CARD_H);
    } else {
        draw_empty_slot(cr, x, y, CARD_W, CARD_H);
    }

    /* Waste. */
    double waste_x = MARGIN + CARD_W + GAP;
    if (game->waste.count > 0) {
        draw_card_face(cr, waste_x, y, CARD_W, CARD_H, &game->waste.cards[game->waste.count - 1]);
    } else {
        draw_empty_slot(cr, waste_x, y, CARD_W, CARD_H);
    }

    /* Foundations, right-aligned. */
    for (int i = 0; i < NUM_FOUNDATIONS; i++) {
        double fx = foundation_x(width, i);
        Pile *foundation = &game->foundations[i];
        if (foundation->count > 0) {
            draw_card_face(cr, fx, MARGIN, CARD_W, CARD_H, &foundation->cards[foundation->count - 1]);
        } else {
            draw_empty_slot(cr, fx, MARGIN, CARD_W, CARD_H);
        }
    }

    /* Tableau. */
    double tableau_y0 = MARGIN + CARD_H + TOP_ROW_GAP;
    for (int col = 0; col < NUM_TABLEAU; col++) {
        double cx = MARGIN + col * (CARD_W + GAP);
        Pile *pile = &game->tableau[col];
        if (pile->count == 0) {
            draw_empty_slot(cr, cx, tableau_y0, CARD_W, CARD_H);
            continue;
        }
        for (int row = 0; row < pile->count; row++) {
            double cy = tableau_y0 + row * STACK_OFFSET;
            Card *c = &pile->cards[row];
            if (c->face_up) {
                draw_card_face(cr, cx, cy, CARD_W, CARD_H, c);
            } else {
                draw_card_back(cr, cx, cy, CARD_W, CARD_H);
            }
        }
    }

    /* Selection highlight, drawn last so it overlays everything else. */
    if (data->sel_kind == PILE_WASTE && game->waste.count > 0) {
        draw_selection_highlight(cr, waste_x, MARGIN, CARD_W, CARD_H);
    } else if (data->sel_kind == PILE_FOUNDATION && game->foundations[data->sel_index].count > 0) {
        double fx = foundation_x(width, data->sel_index);
        draw_selection_highlight(cr, fx, MARGIN, CARD_W, CARD_H);
    } else if (data->sel_kind == PILE_TABLEAU) {
        Pile *pile = &game->tableau[data->sel_index];
        if (data->sel_card_pos >= 0 && data->sel_card_pos < pile->count) {
            double cx = MARGIN + data->sel_index * (CARD_W + GAP);
            double row_y = tableau_y0 + data->sel_card_pos * STACK_OFFSET;
            double run_h = (pile->count - 1 - data->sel_card_pos) * STACK_OFFSET + CARD_H;
            draw_selection_highlight(cr, cx, row_y, CARD_W, run_h);
        }
    }

    /* Win animation: on the frame the game first becomes won, spawn all
     * 52 foundation cards into a bouncing cascade. While it's playing,
     * draw the bouncing cards over the (still-intact) foundation piles;
     * the "You Win!" banner only appears once every card has settled. */
    bool won_now = game_is_won(game);
    if (won_now && !data->was_won) {
        start_win_animation(&data->win_anim, game, width);
    }
    data->was_won = won_now;

    if (won_now) {
        for (int i = 0; i < data->win_anim.total_cards; i++) {
            const BounceCard *bc = &data->win_anim.cards[i];
            if (bc->active) {
                draw_bouncing_card(cr, bc);
            }
        }
    }

    if (won_now && win_anim_finished(&data->win_anim)) {
        cairo_set_source_rgba(cr, 0, 0, 0, 0.55);
        cairo_paint(cr);

        const char *msg = "You Win!";
        cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
        cairo_set_font_size(cr, 48);
        cairo_text_extents_t ext;
        cairo_text_extents(cr, msg, &ext);
        cairo_set_source_rgb(cr, 1, 1, 1);
        cairo_move_to(cr,
                      width / 2.0 - ext.width / 2.0 - ext.x_bearing,
                      height / 2.0 - ext.height / 2.0 - ext.y_bearing);
        cairo_show_text(cr, msg);
    }
}

static bool
point_in_rect(double x, double y, double rx, double ry, double rw, double rh)
{
    return x >= rx && x <= rx + rw && y >= ry && y <= ry + rh;
}

/* Advances the win animation's physics once per display frame. Registered
 * via gtk_widget_add_tick_callback so it's driven by the real frame clock
 * (matches the display's refresh rate) rather than a manual timer. Cheap
 * to call even when no animation is running, since win_anim_step() is a
 * no-op unless `running` is set. */
static gboolean
board_tick_cb(GtkWidget *widget, GdkFrameClock *frame_clock, gpointer user_data)
{
    BoardViewData *data = (BoardViewData *)user_data;
    gint64 now_us = gdk_frame_clock_get_frame_time(frame_clock);

    if (data->last_frame_us == 0) {
        /* First tick: nothing to advance yet, just establish a baseline
         * so the next tick's dt isn't measured from widget-creation time. */
        data->last_frame_us = now_us;
        return G_SOURCE_CONTINUE;
    }

    double dt = (now_us - data->last_frame_us) / 1000000.0;
    data->last_frame_us = now_us;
    /* Clamp dt so a stall (e.g. the window was backgrounded) can't feed a
     * huge single physics step that tunnels a card through the floor. */
    if (dt > 0.05) {
        dt = 0.05;
    }

    if (data->win_anim.running) {
        double floor_y = gtk_widget_get_height(widget) - CARD_H - MARGIN;
        win_anim_step(&data->win_anim, dt, floor_y);
        gtk_widget_queue_draw(widget);
    }

    return G_SOURCE_CONTINUE;
}

GtkWidget *
board_view_new(GameState *game)
{
    BoardViewData *data = g_new0(BoardViewData, 1);
    data->game = game;
    data->sel_kind = PILE_NONE;
    data->sel_index = -1;
    data->sel_card_pos = -1;
    win_anim_reset(&data->win_anim);
    data->was_won = false;
    data->last_frame_us = 0;

    GtkWidget *area = gtk_drawing_area_new();
    gtk_widget_set_hexpand(area, TRUE);
    gtk_widget_set_vexpand(area, TRUE);
    gtk_drawing_area_set_draw_func(GTK_DRAWING_AREA(area), board_draw_func, data, NULL);
    gtk_widget_add_tick_callback(area, board_tick_cb, data, NULL);
    g_object_set_data_full(G_OBJECT(area), "board-view-data", data, g_free);

    return area;
}

void
board_view_redraw(GtkWidget *board_view)
{
    gtk_widget_queue_draw(board_view);
}

BoardHit
board_view_hit_test(GtkWidget *board_view, double x, double y)
{
    BoardHit result = { PILE_NONE, -1, -1 };
    BoardViewData *data = g_object_get_data(G_OBJECT(board_view), "board-view-data");
    if (data == NULL) {
        return result;
    }
    GameState *game = data->game;
    int width = gtk_widget_get_width(board_view);

    if (point_in_rect(x, y, MARGIN, MARGIN, CARD_W, CARD_H)) {
        result.kind = PILE_STOCK;
        return result;
    }

    double waste_x = MARGIN + CARD_W + GAP;
    if (point_in_rect(x, y, waste_x, MARGIN, CARD_W, CARD_H)) {
        result.kind = PILE_WASTE;
        result.card_pos = (game->waste.count > 0) ? 0 : -1;
        return result;
    }

    for (int i = 0; i < NUM_FOUNDATIONS; i++) {
        double fx = foundation_x(width, i);
        if (point_in_rect(x, y, fx, MARGIN, CARD_W, CARD_H)) {
            result.kind = PILE_FOUNDATION;
            result.index = i;
            return result;
        }
    }

    double tableau_y0 = MARGIN + CARD_H + TOP_ROW_GAP;
    for (int col = 0; col < NUM_TABLEAU; col++) {
        double cx = MARGIN + col * (CARD_W + GAP);
        if (x < cx || x > cx + CARD_W) {
            continue;
        }
        Pile *pile = &game->tableau[col];
        if (pile->count == 0) {
            if (point_in_rect(x, y, cx, tableau_y0, CARD_W, CARD_H)) {
                result.kind = PILE_TABLEAU;
                result.index = col;
                result.card_pos = -1;
                return result;
            }
            continue;
        }
        for (int row = pile->count - 1; row >= 0; row--) {
            double row_y = tableau_y0 + row * STACK_OFFSET;
            double band_h = (row == pile->count - 1) ? CARD_H : STACK_OFFSET;
            if (y >= row_y && y <= row_y + band_h) {
                result.kind = PILE_TABLEAU;
                result.index = col;
                result.card_pos = row;
                return result;
            }
        }
    }

    return result;
}

void
board_view_set_selection(GtkWidget *board_view, PileKind kind, int index, int card_pos)
{
    BoardViewData *data = g_object_get_data(G_OBJECT(board_view), "board-view-data");
    if (data == NULL) {
        return;
    }
    data->sel_kind = kind;
    data->sel_index = index;
    data->sel_card_pos = card_pos;
    gtk_widget_queue_draw(board_view);
}

void
board_view_clear_selection(GtkWidget *board_view)
{
    board_view_set_selection(board_view, PILE_NONE, -1, -1);
}

void
board_view_reset_animation(GtkWidget *board_view)
{
    BoardViewData *data = g_object_get_data(G_OBJECT(board_view), "board-view-data");
    if (data == NULL) {
        return;
    }
    win_anim_reset(&data->win_anim);
    data->was_won = false;
    gtk_widget_queue_draw(board_view);
}
