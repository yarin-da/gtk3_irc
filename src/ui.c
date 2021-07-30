#include "ui.h"
#include "common.h"

#define COLUMN_NAME_MAX_WIDTH (150)

enum {
    COLUMN_NAME,
    N_COLUMNS
};

void ui_textViewSetBuffer(GtkWidget *view, GtkTextBuffer *new_buffer) {
    gtk_text_view_set_buffer(GTK_TEXT_VIEW(view), new_buffer);
}

void ui_textViewAppend(GtkWidget *view, char *line, int newLine, char *tag_name) {
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(view));
    ui_textBufferAppend(buffer, line, newLine, tag_name);
}

void ui_textBufferAppend(GtkTextBuffer *buffer, char *line, int newLine, char *tag_name) {
    GtkTextIter iter;
    gtk_text_buffer_get_end_iter(buffer, &iter);
    
    gchar *converted_line = stringToUTF8(line);
    if (tag_name) {
        gtk_text_buffer_insert_with_tags_by_name(
                buffer, &iter, converted_line, -1, tag_name, NULL);       
    } else {
        gtk_text_buffer_insert(buffer, &iter, converted_line, -1);
    }
    g_free(converted_line);

    if (newLine) {
        gtk_text_buffer_insert(buffer, &iter, "\n", -1);
    }
}

void ui_textViewAppendAll(GtkWidget *textView, char **buf, int size) {
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(textView));
    GtkTextIter iter;
    gtk_text_buffer_get_end_iter(buffer, &iter);

    while(size > 0) {
        size--;
        ui_textBufferAppend(buffer, buf[size], TRUE, NULL);
    }
}

void ui_winAutoScroll(GtkWidget *scrolledWin) {
    GtkAdjustment *vadj = gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(scrolledWin));
    gdouble upper = gtk_adjustment_get_upper(vadj); 
    gdouble pagesize = gtk_adjustment_get_page_size(vadj); 
    gtk_adjustment_set_value(vadj, upper - pagesize);
}

void ui_textViewClear(GtkWidget *view) {
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(view));
    ui_textBufferClear(buffer);
}

void ui_textBufferClear(GtkTextBuffer *buffer) {
    GtkTextIter startIter, endIter;
    gtk_text_buffer_get_start_iter(buffer, &startIter);
    gtk_text_buffer_get_end_iter(buffer, &endIter);
    gtk_text_buffer_delete(buffer, &startIter, &endIter);
}

void ui_callback_closeWindow(GtkWidget *widget, gpointer user_data) {
    GtkWidget *parent_window = gtk_widget_get_toplevel(widget);
    if (gtk_widget_is_toplevel(parent_window)) {
        gtk_widget_hide(parent_window);
    }
}

void ui_chat_scroll(ProgramData *data) {
    GtkScrolledWindow *win = GTK_SCROLLED_WINDOW(data->ui->chat_scrolled_win);
    GtkAdjustment *vadj = gtk_scrolled_window_get_vadjustment(win);
    gdouble upper = gtk_adjustment_get_upper(vadj); 
    gdouble pagesize = gtk_adjustment_get_page_size(vadj); 
    gtk_adjustment_set_value(vadj, upper - pagesize);
}

void ui_chat_setContext(ProgramData *data) {
    ui_textViewSetBuffer(data->ui->chat, data->context->chat);
}

void ui_userlist_init(ProgramData *data) {
    ui_userlist_setContext(data);
    gtk_widget_show_all(data->ui->userlist_treeview);
    
    GtkCellRenderer *renderer;
    renderer = gtk_cell_renderer_text_new();
    gtk_tree_view_insert_column_with_attributes(
                            GTK_TREE_VIEW(data->ui->userlist_treeview),
                            -1, "Name", renderer, "text", COLUMN_NAME, NULL);
    GtkTreeViewColumn *column = gtk_tree_view_get_column(
        GTK_TREE_VIEW(data->ui->userlist_treeview), COLUMN_NAME);

    gtk_tree_view_column_set_max_width(column, COLUMN_NAME_MAX_WIDTH);
}

void ui_userlist_setContext(ProgramData *data) {
    assert(data->context != NULL);
    gtk_tree_view_set_model(
        GTK_TREE_VIEW(data->ui->userlist_treeview),
        GTK_TREE_MODEL(data->context->users));
    gtk_widget_show_all(data->ui->userlist_treeview);
}