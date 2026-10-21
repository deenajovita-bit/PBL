
#include "deadlock.h"
#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    System *s;
    GtkWidget *window;
    GtkWidget *notebook;
    GtkWidget *n_spin, *m_spin;
    GtkWidget *total[MAX_R];
    GtkWidget *alloc[MAX_P][MAX_R];
    GtkWidget *maximum[MAX_P][MAX_R];
    GtkWidget *need_view, *available_view;
    GtkWidget *graph_area;
    GtkWidget *process_combo;
    GtkWidget *request[MAX_R];
    GtkWidget *use_banker;
    GtkWidget *state_label;
    GtkWidget *status_view, *log_view;
    GtkWidget *safe_view, *dead_view;
    int dead[MAX_P];
    int dead_count;
} Gui;

static void set_view(GtkWidget *view, const char *text)
{
    GtkTextBuffer *buffer =
        gtk_text_view_get_buffer(GTK_TEXT_VIEW(view));
    gtk_text_buffer_set_text(buffer, text ? text : "", -1);
}

static void log_action(Gui *g, const char *message)
{
    GtkTextBuffer *buffer;
    GtkTextIter end;

    buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(g->log_view));
    gtk_text_buffer_get_end_iter(buffer, &end);
    gtk_text_buffer_insert(buffer, &end, message, -1);
    gtk_text_buffer_insert(buffer, &end, "\n", 1);
}

static void show_error(Gui *g, const char *message)
{
    GtkWidget *dialog = gtk_message_dialog_new(
        GTK_WINDOW(g->window),
        GTK_DIALOG_MODAL,
        GTK_MESSAGE_ERROR,
        GTK_BUTTONS_CLOSE,
        "%s", message);

    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

static int read_int(GtkWidget *entry, int *value)
{
    const char *text = gtk_entry_get_text(GTK_ENTRY(entry));
    char *end;
    long number;

    if (text == NULL || *text == '\0')
        return 0;

    number = strtol(text, &end, 10);

    if (*end != '\0' || end == text ||
        number < 0 || number > 1000000)
        return 0;

    *value = (int)number;
    return 1;
}

static GtkWidget *text_view_new(void)
{
    GtkWidget *view = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(view), FALSE);
    gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(view), FALSE);
    return view;
}

static GtkWidget *make_page(void)
{
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_container_set_border_width(GTK_CONTAINER(box), 12);
    return box;
}

static void add_tab(Gui *g, const char *name, GtkWidget *page)
{
    gtk_notebook_append_page(
        GTK_NOTEBOOK(g->notebook),
        page,
        gtk_label_new(name));
}

static void refresh_all(Gui *g)
{
    System *s = g->s;
    char text[8192];
    size_t used;
    int i, j;
    SysState state;

    calc_need(s);
    update_available(s);
    g->dead_count = detect_deadlock(s, g->dead);

    used = (size_t)snprintf(text, sizeof(text), "Need matrix:\n");

    for (i = 0; i < s->n && used < sizeof(text); ++i) {
        used += (size_t)snprintf(
            text + used, sizeof(text) - used, "P%d: ", i);

        for (j = 0; j < s->m && used < sizeof(text); ++j) {
            used += (size_t)snprintf(
                text + used, sizeof(text) - used,
                "%s%d", j ? "  " : "", s->need[i][j]);
        }

        used += (size_t)snprintf(
            text + used, sizeof(text) - used, "\n");
    }

    set_view(g->need_view, text);

    used = (size_t)snprintf(text, sizeof(text), "Available: ");

    for (j = 0; j < s->m && used < sizeof(text); ++j) {
        used += (size_t)snprintf(
            text + used, sizeof(text) - used,
            "%sR%d=%d",
            j ? ", " : "", j, s->available[j]);
    }

    set_view(g->available_view, text);

    state = system_state(s);

    if (state == STATE_SAFE) {
        gtk_label_set_text(GTK_LABEL(g->state_label), "SAFE");
        gtk_widget_set_name(g->state_label, "state-safe");
    } else if (state == STATE_UNSAFE) {
        gtk_label_set_text(GTK_LABEL(g->state_label), "UNSAFE");
        gtk_widget_set_name(g->state_label, "state-unsafe");
    } else {
        gtk_label_set_text(GTK_LABEL(g->state_label), "DEADLOCKED");
        gtk_widget_set_name(g->state_label, "state-deadlock");
    }

    used = (size_t)snprintf(
        text, sizeof(text), "%s\n",
        gtk_label_get_text(GTK_LABEL(g->state_label)));

    used += (size_t)snprintf(
        text + used, sizeof(text) - used, "Available resources:\n");

    for (j = 0; j < s->m && used < sizeof(text); ++j) {
        int in_use = s->total[j] - s->available[j];
        double percent = s->total[j] > 0
            ? 100.0 * in_use / s->total[j] : 0.0;

        used += (size_t)snprintf(
            text + used, sizeof(text) - used,
            "R%d: %d available, %.1f%% utilized\n",
            j, s->available[j], percent);
    }

    set_view(g->status_view, text);
    gtk_widget_queue_draw(g->graph_area);
}

static void load_matrix_entries(Gui *g)
{
    int i, j;
    char number[32];

    for (i = 0; i < g->s->n; ++i) {
        for (j = 0; j < g->s->m; ++j) {
            snprintf(number, sizeof(number), "%d",
                     g->s->alloc[i][j]);
            gtk_entry_set_text(GTK_ENTRY(g->alloc[i][j]), number);

            snprintf(number, sizeof(number), "%d",
                     g->s->max[i][j]);
            gtk_entry_set_text(GTK_ENTRY(g->maximum[i][j]), number);
        }
    }

    for (j = 0; j < MAX_R; ++j) {
        snprintf(number, sizeof(number), "%d", g->s->total[j]);
        gtk_entry_set_text(GTK_ENTRY(g->total[j]), number);
    }
}

static void on_create(GtkButton *button, gpointer data)
{
    Gui *g = data;
    int n = gtk_spin_button_get_value_as_int(
        GTK_SPIN_BUTTON(g->n_spin));
    int m = gtk_spin_button_get_value_as_int(
        GTK_SPIN_BUTTON(g->m_spin));
    int total[MAX_R] = {0};
    int i, j;

    (void)button;

    for (j = 0; j < m; ++j) {
        if (!read_int(g->total[j], &total[j])) {
            show_error(g, "Enter valid non-negative resource totals.");
            return;
        }
    }

    sys_init(g->s, n, m, total);

    for (i = 0; i < n; ++i) {
        for (j = 0; j < m; ++j)
            g->s->max[i][j] = total[j];
    }

    calc_need(g->s);
    update_available(g->s);
    load_matrix_entries(g);
    log_action(g, "Created a new system.");
    refresh_all(g);
}

static void on_sample(GtkButton *button, gpointer data)
{
    Gui *g = data;
    const int total[3] = {10, 5, 7};
    const int allocation[5][3] = {
        {0, 1, 0},
        {2, 0, 0},
        {3, 0, 2},
        {2, 1, 1},
        {0, 0, 2}
    };
    const int maximum[5][3] = {
        {7, 5, 3},
        {3, 2, 2},
        {9, 0, 2},
        {2, 2, 2},
        {4, 3, 3}
    };
    int i, j;

    (void)button;

    sys_init(g->s, 5, 3, total);

    for (i = 0; i < 5; ++i) {
        for (j = 0; j < 3; ++j) {
            g->s->alloc[i][j] = allocation[i][j];
            g->s->max[i][j] = maximum[i][j];
        }
    }

    gtk_spin_button_set_value(GTK_SPIN_BUTTON(g->n_spin), 5);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(g->m_spin), 3);

    calc_need(g->s);
    update_available(g->s);
    load_matrix_entries(g);

    log_action(g, "Loaded the textbook sample.");
    refresh_all(g);
}

static void on_apply_matrices(GtkButton *button, gpointer data)
{
    Gui *g = data;
    int a[MAX_P][MAX_R] = {{0}};
    int mx[MAX_P][MAX_R] = {{0}};
    int i, j, sum;

    (void)button;

    /* Validate all values before changing the system. */
    for (i = 0; i < g->s->n; ++i) {
        for (j = 0; j < g->s->m; ++j) {
            if (!read_int(g->alloc[i][j], &a[i][j]) ||
                !read_int(g->maximum[i][j], &mx[i][j])) {
                show_error(g, "Matrix values must be non-negative integers.");
                return;
            }

            if (a[i][j] > mx[i][j]) {
                show_error(g, "Allocation cannot exceed Maximum.");
                return;
            }
        }
    }

    for (j = 0; j < g->s->m; ++j) {
        sum = 0;

        for (i = 0; i < g->s->n; ++i)
            sum += a[i][j];

        if (sum > g->s->total[j]) {
            show_error(g, "Allocation exceeds total resource instances.");
            return;
        }
    }

    for (i = 0; i < g->s->n; ++i) {
        for (j = 0; j < g->s->m; ++j) {
            g->s->alloc[i][j] = a[i][j];
            g->s->max[i][j] = mx[i][j];
        }
    }

    calc_need(g->s);
    update_available(g->s);
    log_action(g, "Updated Allocation and Maximum matrices.");
    refresh_all(g);
}

static gboolean on_graph_draw(GtkWidget *widget,
                              cairo_t *cr, gpointer data)
{
    Gui *g = data;
    GtkAllocation a;

    gtk_widget_get_allocation(widget, &a);

    graph_draw(cr, a.width, a.height,
               g->s, g->dead, g->dead_count);

    return FALSE;
}

static void on_request(GtkButton *button, gpointer data)
{
    Gui *g = data;
    int p = gtk_combo_box_get_active(
        GTK_COMBO_BOX(g->process_combo));
    int req[MAX_R] = {0};
    int i, any = 0;
    ReqResult result;

    (void)button;

    if (p < 0 || p >= g->s->n || !g->s->active[p]) {
        show_error(g, "Select an active process.");
        return;
    }

    for (i = 0; i < g->s->m; ++i) {
        if (!read_int(g->request[i], &req[i])) {
            show_error(g, "Requests must be non-negative integers.");
            return;
        }

        if (req[i] > 0)
            any = 1;
    }

    if (!any) {
        show_error(g, "Enter at least one positive request value.");
        return;
    }

    result = request_resources(
        g->s, p, req,
        gtk_toggle_button_get_active(
            GTK_TOGGLE_BUTTON(g->use_banker)));

    if (result == REQ_GRANTED)
        log_action(g, "Resource request granted.");
    else if (result == REQ_WAIT)
        log_action(g, "Request waits for available resources.");
    else if (result == REQ_DENIED_UNSAFE)
        log_action(g, "Request waits because the state would be unsafe.");
    else
        show_error(g, "Invalid resource request.");

    refresh_all(g);
}

static void on_banker(GtkButton *button, gpointer data)
{
    Gui *g = data;
    int seq[MAX_P];
    char text[512];
    size_t used;
    int i;

    (void)button;

    if (is_safe(g->s, seq)) {
        used = (size_t)snprintf(text, sizeof(text), "SAFE\nSequence: ");

        for (i = 0; i < g->s->n && seq[i] >= 0 &&
                    used < sizeof(text); ++i) {
            used += (size_t)snprintf(
                text + used, sizeof(text) - used,
                "%sP%d", i ? " -> " : "", seq[i]);
        }

        log_action(g, "Banker's Algorithm returned SAFE.");
    } else {
        snprintf(text, sizeof(text), "UNSAFE\nNo safe sequence.");
        log_action(g, "Banker's Algorithm returned UNSAFE.");
    }

    set_view(g->safe_view, text);
    refresh_all(g);
}

static void on_detect(GtkButton *button, gpointer data)
{
    Gui *g = data;
    char text[512];
    size_t used;
    int i;

    (void)button;

    g->dead_count = detect_deadlock(g->s, g->dead);

    if (g->dead_count == 0) {
        snprintf(text, sizeof(text), "No deadlocked processes.");
        log_action(g, "No deadlock found.");
    } else {
        used = (size_t)snprintf(text, sizeof(text),
                                "Deadlocked processes: ");

        for (i = 0; i < g->dead_count && used < sizeof(text); ++i) {
            used += (size_t)snprintf(
                text + used, sizeof(text) - used,
                "%sP%d", i ? ", " : "", g->dead[i]);
        }

        log_action(g, "Deadlocked processes detected.");
    }

    set_view(g->dead_view, text);
    refresh_all(g);
}

static void on_recover(GtkButton *button, gpointer data)
{
    Gui *g = data;
    int victim;

    (void)button;

    g->dead_count = detect_deadlock(g->s, g->dead);

    if (g->dead_count == 0) {
        show_error(g, "There are no deadlocked processes to recover.");
        return;
    }

    victim = pick_victim(g->s, g->dead, g->dead_count);

    if (victim < 0 || victim >= g->s->n) {
        show_error(g, "No valid recovery victim was selected.");
        return;
    }

    recover_terminate(g->s, victim);
    log_action(g, "Recovery terminated a deadlocked process.");
    refresh_all(g);
}

static GtkWidget *make_setup(Gui *g)
{
    GtkWidget *box = make_page();
    GtkWidget *row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    GtkWidget *grid = gtk_grid_new();
    GtkWidget *create = gtk_button_new_with_label("Create");
    GtkWidget *sample = gtk_button_new_with_label("Load sample");
    int i;

    gtk_box_pack_start(GTK_BOX(row), gtk_label_new("Processes"),
                       FALSE, FALSE, 0);
    g->n_spin = gtk_spin_button_new_with_range(1, MAX_P, 1);
    gtk_box_pack_start(GTK_BOX(row), g->n_spin, FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(row), gtk_label_new("Resources"),
                       FALSE, FALSE, 0);
    g->m_spin = gtk_spin_button_new_with_range(1, MAX_R, 1);
    gtk_box_pack_start(GTK_BOX(row), g->m_spin, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box), row, FALSE, FALSE, 0);

    gtk_grid_set_row_spacing(GTK_GRID(grid), 5);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 8);

    for (i = 0; i < MAX_R; ++i) {
        char label[32];

        snprintf(label, sizeof(label), "Total R%d", i);
        gtk_grid_attach(GTK_GRID(grid), gtk_label_new(label),
                        0, i, 1, 1);

        g->total[i] = gtk_entry_new();
        gtk_entry_set_width_chars(GTK_ENTRY(g->total[i]), 8);
        gtk_entry_set_text(GTK_ENTRY(g->total[i]), "3");

        gtk_grid_attach(GTK_GRID(grid), g->total[i],
                        1, i, 1, 1);
    }

    gtk_box_pack_start(GTK_BOX(box), grid, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box), create, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box), sample, FALSE, FALSE, 0);

    g_signal_connect(create, "clicked", G_CALLBACK(on_create), g);
    g_signal_connect(sample, "clicked", G_CALLBACK(on_sample), g);

    return box;
}

static GtkWidget *make_matrices(Gui *g)
{
    GtkWidget *box = make_page();
    GtkWidget *scroll = gtk_scrolled_window_new(NULL, NULL);
    GtkWidget *grid = gtk_grid_new();
    GtkWidget *apply = gtk_button_new_with_label("Apply matrices");
    GtkWidget *details = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    int i, j;

    gtk_grid_set_row_spacing(GTK_GRID(grid), 5);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 5);

    gtk_grid_attach(GTK_GRID(grid), gtk_label_new("Process"),
                    0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), gtk_label_new("Allocation"),
                    1, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), gtk_label_new("Maximum"),
                    2, 0, 1, 1);

    for (i = 0; i < MAX_P; ++i) {
        char p[24];
        GtkWidget *ab = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 3);
        GtkWidget *mb = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 3);

        snprintf(p, sizeof(p), "P%d", i);
        gtk_grid_attach(GTK_GRID(grid), gtk_label_new(p),
                        0, i + 1, 1, 1);

        for (j = 0; j < MAX_R; ++j) {
            g->alloc[i][j] = gtk_entry_new();
            g->maximum[i][j] = gtk_entry_new();

            gtk_entry_set_width_chars(GTK_ENTRY(g->alloc[i][j]), 3);
            gtk_entry_set_width_chars(GTK_ENTRY(g->maximum[i][j]), 3);
            gtk_entry_set_text(GTK_ENTRY(g->alloc[i][j]), "0");
            gtk_entry_set_text(GTK_ENTRY(g->maximum[i][j]), "0");

            gtk_box_pack_start(GTK_BOX(ab), g->alloc[i][j],
                               FALSE, FALSE, 0);
            gtk_box_pack_start(GTK_BOX(mb), g->maximum[i][j],
                               FALSE, FALSE, 0);
        }

        gtk_grid_attach(GTK_GRID(grid), ab, 1, i + 1, 1, 1);
        gtk_grid_attach(GTK_GRID(grid), mb, 2, i + 1, 1, 1);
    }

    gtk_container_add(GTK_CONTAINER(scroll), grid);
    gtk_widget_set_vexpand(scroll, TRUE);
    gtk_box_pack_start(GTK_BOX(box), scroll, TRUE, TRUE, 0);

    g->need_view = text_view_new();
    g->available_view = text_view_new();

    gtk_widget_set_size_request(g->need_view, -1, 130);
    gtk_widget_set_size_request(g->available_view, -1, 55);

    gtk_box_pack_start(GTK_BOX(details), g->need_view,
                       TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(details), g->available_view,
                       TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(box), details, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box), apply, FALSE, FALSE, 0);

    g_signal_connect(apply, "clicked",
                     G_CALLBACK(on_apply_matrices), g);

    return box;
}

static GtkWidget *make_graph(Gui *g)
{
    GtkWidget *box = make_page();

    g->graph_area = gtk_drawing_area_new();
    gtk_widget_set_size_request(g->graph_area, 800, 420);
    gtk_widget_set_hexpand(g->graph_area, TRUE);
    gtk_widget_set_vexpand(g->graph_area, TRUE);

    g_signal_connect(g->graph_area, "draw",
                     G_CALLBACK(on_graph_draw), g);

    gtk_box_pack_start(GTK_BOX(box), g->graph_area, TRUE, TRUE, 0);
    return box;
}

static GtkWidget *make_request(Gui *g)
{
    GtkWidget *box = make_page();
    GtkWidget *grid = gtk_grid_new();
    GtkWidget *send = gtk_button_new_with_label("Send request");
    int i;

    g->process_combo = gtk_combo_box_text_new();

    for (i = 0; i < MAX_P; ++i) {
        char p[24];
        snprintf(p, sizeof(p), "P%d", i);
        gtk_combo_box_text_append_text(
            GTK_COMBO_BOX_TEXT(g->process_combo), p);
    }

    gtk_combo_box_set_active(GTK_COMBO_BOX(g->process_combo), 0);
    gtk_box_pack_start(GTK_BOX(box), gtk_label_new("Process"),
                       FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box), g->process_combo,
                       FALSE, FALSE, 0);

    gtk_grid_set_row_spacing(GTK_GRID(grid), 5);

    for (i = 0; i < MAX_R; ++i) {
        char label[32];

        snprintf(label, sizeof(label), "Request R%d", i);
        gtk_grid_attach(GTK_GRID(grid), gtk_label_new(label),
                        0, i, 1, 1);

        g->request[i] = gtk_entry_new();
        gtk_entry_set_text(GTK_ENTRY(g->request[i]), "0");
        gtk_entry_set_width_chars(GTK_ENTRY(g->request[i]), 8);

        gtk_grid_attach(GTK_GRID(grid), g->request[i],
                        1, i, 1, 1);
    }

    gtk_box_pack_start(GTK_BOX(box), grid, FALSE, FALSE, 0);

    g->use_banker = gtk_check_button_new_with_label(
        "Use Banker's Algorithm");
    gtk_toggle_button_set_active(
        GTK_TOGGLE_BUTTON(g->use_banker), TRUE);

    gtk_box_pack_start(GTK_BOX(box), g->use_banker,
                       FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box), send, FALSE, FALSE, 0);

    g_signal_connect(send, "clicked", G_CALLBACK(on_request), g);

    return box;
}

static GtkWidget *make_banker(Gui *g)
{
    GtkWidget *box = make_page();
    GtkWidget *button =
        gtk_button_new_with_label("Run Banker's Algorithm");

    g->safe_view = text_view_new();

    gtk_box_pack_start(GTK_BOX(box), button, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box), g->safe_view, TRUE, TRUE, 0);

    g_signal_connect(button, "clicked", G_CALLBACK(on_banker), g);

    return box;
}

static GtkWidget *make_detect(Gui *g)
{
    GtkWidget *box = make_page();
    GtkWidget *button =
        gtk_button_new_with_label("Detect deadlock");

    g->dead_view = text_view_new();

    gtk_box_pack_start(GTK_BOX(box), button, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box), g->dead_view, TRUE, TRUE, 0);

    g_signal_connect(button, "clicked", G_CALLBACK(on_detect), g);

    return box;
}

static GtkWidget *make_recover(Gui *g)
{
    GtkWidget *box = make_page();
    GtkWidget *button =
        gtk_button_new_with_label("Recover from deadlock");

    gtk_box_pack_start(GTK_BOX(box), gtk_label_new(
        "The victim is selected using pick_victim()."),
        FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box), button, FALSE, FALSE, 0);

    g_signal_connect(button, "clicked", G_CALLBACK(on_recover), g);

    return box;
}

static GtkWidget *make_status(Gui *g)
{
    GtkWidget *box = make_page();
    GtkWidget *scroll = gtk_scrolled_window_new(NULL, NULL);
    GtkWidget *log_scroll = gtk_scrolled_window_new(NULL, NULL);
    GtkWidget *heading = gtk_label_new("System status");
    GtkWidget *log_heading = gtk_label_new("Action log");

    g->state_label = gtk_label_new("SAFE");
    g->status_view = text_view_new();
    g->log_view = text_view_new();

    gtk_label_set_xalign(GTK_LABEL(heading), 0.0f);
    gtk_label_set_xalign(GTK_LABEL(log_heading), 0.0f);

    gtk_container_add(GTK_CONTAINER(scroll), g->status_view);
    gtk_container_add(GTK_CONTAINER(log_scroll), g->log_view);

    gtk_box_pack_start(GTK_BOX(box), heading, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box), g->state_label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box), scroll, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(box), log_heading, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box), log_scroll, TRUE, TRUE, 0);

    return box;
}

int gui_run(System *s, int argc, char **argv)
{
    Gui *g;
    GtkWidget *window;
    (void)argc;
    (void)argv;

    if (s == NULL)
        return 1;

    g = g_new0(Gui, 1);
    g->s = s;

    /*
     * The caller must initialize GTK before gui_run().
     * For example, use gtk_init(&argc, &argv) in the test main.
     */
    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    g->window = window;

    gtk_window_set_title(GTK_WINDOW(window),
                         "Interactive Deadlock Simulation System");
    gtk_window_set_default_size(GTK_WINDOW(window), 1100, 760);

    g_signal_connect(window, "destroy",
                     G_CALLBACK(gtk_main_quit), NULL);

    g->notebook = gtk_notebook_new();
    gtk_container_add(GTK_CONTAINER(window), g->notebook);

    add_tab(g, "Setup", make_setup(g));
    add_tab(g, "Matrices", make_matrices(g));
    add_tab(g, "Graph", make_graph(g));
    add_tab(g, "Request", make_request(g));
    add_tab(g, "Banker", make_banker(g));
    add_tab(g, "Detect", make_detect(g));
    add_tab(g, "Recover", make_recover(g));
    add_tab(g, "Status", make_status(g));

    load_matrix_entries(g);
    refresh_all(g);

    gtk_widget_show_all(window);
    gtk_main();

    g_free(g);
    return 0;
}

