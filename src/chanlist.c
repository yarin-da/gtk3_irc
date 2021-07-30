#include "chanlist.h"
#include "common.h"
#include "serveroutput.h"

#define COLUMN_NAME_MAX_WIDTH (200)
#define COLUMN_USERS_MAX_WIDTH (50)
#define COLUMN_TOPIC_MAX_WIDTH (-1)

enum {
    COLUMN_NAME = 0,
    COLUMN_USERS,
    COLUMN_TOPIC,
    N_COLUMNS
};

/* used to filter rows in our channel list view */ 
static gboolean visible_func(
    GtkTreeModel *model, GtkTreeIter *iter, gpointer user_data) {

    ProgramData *data = (ProgramData *)user_data;
    assert(data->ui->chanlist_entry_filter != NULL);

    /* get the user's input from the search entry */
    GtkEntry *entry = GTK_ENTRY(data->ui->chanlist_entry_filter);
    const gchar *search_term = gtk_entry_get_text(entry);
    /* return if there's no input from the user */
    if (search_term == NULL) {
        return FALSE;
    }

    gchar *name;
    gtk_tree_model_get(model, iter, COLUMN_NAME, &name, -1);
    if (name && g_str_match_string(search_term, name, TRUE)) {
        return TRUE;
    }
    g_free(name);

    gchar *topic;
    gtk_tree_model_get(model, iter, COLUMN_TOPIC, &topic, -1);
    if (topic && g_str_match_string(search_term, topic, TRUE)) {
        return TRUE;
    }
    g_free(topic);

    return FALSE;
}

void chanlist_init(ProgramData *data) {
    data->ui->chanlist_liststore = gtk_list_store_new(
                                        N_COLUMNS, G_TYPE_STRING, 
                                        G_TYPE_INT, G_TYPE_STRING);
    data->ui->chanlist_filter = gtk_tree_model_filter_new(
        GTK_TREE_MODEL(data->ui->chanlist_liststore), NULL);
    gtk_tree_model_filter_set_visible_func(
        GTK_TREE_MODEL_FILTER(data->ui->chanlist_filter), 
        (GtkTreeModelFilterVisibleFunc)visible_func, 
        data, NULL);

    data->ui->chanlist_list_end = FALSE;
    data->sendLIST = FALSE;
}

void chanlist_clear(ProgramData *data) {
    gtk_list_store_clear(data->ui->chanlist_liststore);
}

void chanlist_refresh(ProgramData *data) {
    if (!data->conn->login) {
        handleError("chanlist_refresh(): user is not logged in");
    } else {
        data->ui->chanlist_refresh = TRUE;
        /* send :nick LIST to server */
        if (data->sendLIST) {
            sendmsg_list(data);
        }
        chanlist_clear(data);
    }
}

void chanlist_add(ProgramData *data, char *name, int users, char *topic) {
    /* :source     322       my_nick      chan_name    chan_users   :chan_topic */
    /* msg->source msg->code msg->args[0] msg->args[1] msg->args[2] msg->args[3] */
    assert(data->ui->chanlist_liststore != NULL);
    GtkTreeIter iter;
    gtk_list_store_append(data->ui->chanlist_liststore, &iter);

    gchar *converted_name = stringToUTF8(name);
    gchar *converted_topic = stringToUTF8(topic);

    gtk_list_store_set(data->ui->chanlist_liststore, &iter,
        COLUMN_NAME, converted_name,
        COLUMN_USERS, users,
        COLUMN_TOPIC, converted_topic, -1);
}

void chanlist_callback_listChannels(GtkWidget *widget, gpointer user_data) {
    ProgramData *data = (ProgramData *)user_data;
    UI *ui = data->ui;
    
    /* refresh list only if more than 5 minutes passed since last refresh */
    if (!ui->chanlist_list_end || timeDiff(ui->chanlist_last_refresh, 300)) {
        chanlist_refresh(data);
    }

    /* use GtkTreeView with GtkListStore */
    if (gtk_tree_view_get_model(GTK_TREE_VIEW(ui->chanlist_treeview)) == NULL) {
        gtk_tree_view_set_model(
            GTK_TREE_VIEW(ui->chanlist_treeview), ui->chanlist_filter);
        gtk_widget_show_all(ui->chanlist_treeview);
        GtkCellRenderer *renderer;

        renderer = gtk_cell_renderer_text_new();
        gtk_tree_view_insert_column_with_attributes(
                                GTK_TREE_VIEW(ui->chanlist_treeview),
                                -1, "Name", renderer, "text", COLUMN_NAME, NULL);
        renderer = gtk_cell_renderer_text_new();
        gtk_tree_view_insert_column_with_attributes(
                                GTK_TREE_VIEW(ui->chanlist_treeview),
                                -1, "Users", renderer, "text", COLUMN_USERS, NULL);
        renderer = gtk_cell_renderer_text_new();
        gtk_tree_view_insert_column_with_attributes(
                                GTK_TREE_VIEW(ui->chanlist_treeview),
                                -1, "Topic", renderer, "text", COLUMN_TOPIC, NULL);

        GtkTreeViewColumn *name_column = gtk_tree_view_get_column(
            GTK_TREE_VIEW(ui->chanlist_treeview), COLUMN_NAME);
        GtkTreeViewColumn *users_column = gtk_tree_view_get_column(
            GTK_TREE_VIEW(ui->chanlist_treeview), COLUMN_USERS);
        GtkTreeViewColumn *topic_column = gtk_tree_view_get_column(
            GTK_TREE_VIEW(ui->chanlist_treeview), COLUMN_TOPIC);

        gtk_tree_view_column_set_max_width(name_column, COLUMN_NAME_MAX_WIDTH);
        gtk_tree_view_column_set_max_width(users_column, COLUMN_USERS_MAX_WIDTH);
        gtk_tree_view_column_set_max_width(topic_column, COLUMN_TOPIC_MAX_WIDTH);
    }
    gtk_widget_show(ui->chanlist_win);
}

void chanlist_callback_join_pressed(GtkWidget *widget, gpointer user_data) {
    ProgramData *data = (ProgramData *)user_data;
    if (data->ui->chanlist_selected) {
        joinChannel(data, data->ui->chanlist_selected + 1);
        /* if connected - hide window */
        gtk_widget_hide(data->ui->chanlist_win);
    }
    /* set to n*/
    data->ui->chanlist_selected = NULL;
}

void chanlist_callback_selected(GtkTreeSelection *selection, gpointer user_data) {
    ProgramData *data = (ProgramData *)user_data;
    assert(data != NULL);
    assert(data->ui != NULL);
    assert(data->ui->chanlist_liststore != NULL);
    GtkTreeIter iter;
    GtkTreeModel *model = GTK_TREE_MODEL(data->ui->chanlist_liststore);
    char *chan_name = NULL;
    if (gtk_tree_selection_get_selected(selection, &model, &iter)) {
        gtk_tree_model_get(model, &iter, COLUMN_NAME, &chan_name, -1);
        if (data->ui->chanlist_selected) {
            g_free(data->ui->chanlist_selected);
        }
        data->ui->chanlist_selected = chan_name;
    }
}

void chanlist_callback_search_clicked(GtkWidget *widget, gpointer user_data) {
    ProgramData *data = (ProgramData *)user_data;
    gtk_tree_model_filter_refilter(
        GTK_TREE_MODEL_FILTER(data->ui->chanlist_filter));
}
