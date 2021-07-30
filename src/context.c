#include "context.h"
#include "common.h"
#include "ui.h"
#include <stdlib.h>

#define OPENCHATS_COLUMN_WIDTH (180)

enum {
    COLUMN_NAME = 0,
    N_COLUMNS
};

static int context_find(ProgramData *data, gchar *name) {
    for (gint i = 0; i < data->chatCount; i++) {
        if (!strcmp(data->chats[i]->name, name)) {
            return i;
        }
    }
    return -1;
}

static void context_add(ProgramData *data, ChatContext *context) {
    assert(data != NULL);
    assert(data->ui != NULL);
    assert(data->ui->openchats_liststore != NULL);
    
    data->chats[data->chatCount++] = context;

    GtkTreeIter iter;
    gtk_list_store_append(data->ui->openchats_liststore, &iter);
    gtk_list_store_set(data->ui->openchats_liststore, &iter,
        COLUMN_NAME, context->name, -1);
}

static void context_buffer_add_tags(GtkTextBuffer *buffer) {
    gtk_text_buffer_create_tag(
        buffer, USER_CHAT_NAME_STYLE, 
        "weight", PANGO_WEIGHT_BOLD, 
        "foreground", "blue", NULL);
    gtk_text_buffer_create_tag(
        buffer, SELF_CHAT_NAME_STYLE, 
        "weight", PANGO_WEIGHT_BOLD, 
        "foreground", "red", NULL);
    gtk_text_buffer_create_tag(
        buffer, SERV_CHAT_NAME_STYLE, 
        "weight", PANGO_WEIGHT_NORMAL, 
        "foreground", "black", NULL);
}

static void context_users_clear(ChatContext *context) {
    gtk_list_store_clear(context->users);
}

static void context_openchats_cell_data_func(
        GtkTreeViewColumn *col, GtkCellRenderer *renderer,
        GtkTreeModel *model, GtkTreeIter *iter, gpointer user_data) {
    ProgramData *data = (ProgramData *)user_data;
    gchar *context_name;
    gtk_tree_model_get(model, iter, COLUMN_NAME, &context_name, -1);
    ChatContext *context = context_get(data, context_name);
    if (context != NULL) {
        if (context->unread) {
            g_object_set(renderer, "weight", PANGO_WEIGHT_BOLD,
                "weight-set", TRUE,
                "foreground", "Red", 
                "foreground-set", TRUE, NULL);
        } else {
            g_object_set(renderer, "weight-set", FALSE, 
                "foreground-set", FALSE, NULL);
        }
    }
}

void context_init(ProgramData *data) {
    data->chatCount = 0;
    ChatContext *main = allocateMemory(sizeof(ChatContext));
    
    main->name = "Main Context";
    main->chat = gtk_text_buffer_new(NULL);
    main->users = gtk_list_store_new(N_COLUMNS, G_TYPE_STRING);
    main->updateList = FALSE;
    main->isChannel = FALSE;
    main->numOfUsers = 0;
    main->autoscroll = TRUE;
    main->unread = FALSE;

    context_buffer_add_tags(main->chat);

    data->ui->openchats_liststore = gtk_list_store_new(N_COLUMNS, G_TYPE_STRING);

    /* use GtkTreeView with GtkListStore */
    if (gtk_tree_view_get_model(GTK_TREE_VIEW(data->ui->openchats_treeview)) == NULL) {
        gtk_tree_view_set_model(
            GTK_TREE_VIEW(data->ui->openchats_treeview),
            GTK_TREE_MODEL(data->ui->openchats_liststore));
        gtk_widget_show_all(data->ui->openchats_treeview);

        data->ui->openchats_renderer = gtk_cell_renderer_text_new();
        gtk_tree_view_insert_column_with_attributes(
                                GTK_TREE_VIEW(data->ui->openchats_treeview),
                                -1, "Chats", data->ui->openchats_renderer, 
                                "text", COLUMN_NAME, NULL);

        GtkTreeViewColumn *name_column = gtk_tree_view_get_column(
            GTK_TREE_VIEW(data->ui->openchats_treeview), COLUMN_NAME);
        gtk_tree_view_column_set_cell_data_func(
            name_column, data->ui->openchats_renderer,
            context_openchats_cell_data_func, data, NULL);
        gtk_tree_view_column_set_max_width(name_column, OPENCHATS_COLUMN_WIDTH);
    }

    context_add(data, main);
    data->context = main;
}

ChatContext *context_create(ProgramData *data, gchar *name, gint isChannel) {
    if (data->chatCount == MAX_CHATS - 1) {
        handleError("createChannel::exceeded maximum amount of channels");
        return NULL;
    }

    ChatContext *context = allocateMemory(sizeof(ChatContext));
    context->isChannel = isChannel;
    context->name = g_strdup(name);
    context->chat = gtk_text_buffer_new(NULL);
    context->users = gtk_list_store_new(N_COLUMNS, G_TYPE_STRING);
    context->updateList = FALSE;
    context->numOfUsers = 0;
    context->unread = FALSE;
    context->autoscroll = TRUE;

    context_buffer_add_tags(context->chat);
    context_add(data, context);
    return context;
}

void context_select(ProgramData *data, gchar *context_name) {
    GtkTreeIter iter;
    GtkListStore *store = data->ui->openchats_liststore;
    GtkTreeModel *model = GTK_TREE_MODEL(store);
    gchar *curr_name;

    gtk_tree_model_get_iter_first(model, &iter);

    for (gint i = 0; i < data->chatCount; i++) {
        gtk_tree_model_get (model, &iter, 0, &curr_name, -1);

        if (g_strcmp0(curr_name, context_name) == 0) {
            gtk_tree_selection_select_iter(data->ui->openchats_selection, &iter);
            break;
        }

        gtk_tree_model_iter_next(model, &iter);
    }
}

void context_users_add_byName(ProgramData *data, gchar *context_name, gchar *name) {
    ChatContext *context = context_get(data, context_name);
    if (context != NULL) {
        context_users_add(context, name);
    }
}

void context_users_addBulk_byName(ProgramData *data, gchar *context_name, gchar *users) {
    ChatContext *context = context_get(data, context_name);
    if (context != NULL) {
        context_users_addBulk(context, users);
    }    
}

void context_users_add(ChatContext *context, gchar *name) {
    GtkTreeIter iter;
    gtk_list_store_append(context->users, &iter);
    gtk_list_store_set(context->users, &iter, COLUMN_NAME, name, -1);
    context->numOfUsers++;
}

/* users - a string containing usernames separated by a space character */
void context_users_addBulk(ChatContext *context, gchar *users) {
    if (context->updateList == FALSE) {
        context_users_clear(context);
        context->updateList = TRUE;
    }

    gchar *token = strtok(users, " ");
    if (token != NULL) {
        context_users_add(context, token);
        while((token = strtok(NULL, " ")) != NULL) {
            context_users_add(context, token);
        }    
    }
}

void context_users_finish_byName(ProgramData *data, gchar *context_name) {
    ChatContext *context = context_get(data, context_name);
    if (context != NULL) {
        context_users_finish(context);
    }    
}

void context_users_finish(ChatContext *context) {
    context->updateList = FALSE;
}

int context_users_remove_from(ChatContext *context, gchar *username) {
    GtkTreeIter iter;
    gtk_tree_model_get_iter_first(GTK_TREE_MODEL(context->users), &iter);

    gchar *curr_name;
    for (gint i = 0; i < context->numOfUsers; i++) {
        gtk_tree_model_get (
            GTK_TREE_MODEL(context->users), &iter, COLUMN_NAME, &curr_name, -1);

        if (g_strcmp0(curr_name, username) == 0) {
            gtk_list_store_remove(context->users, &iter);
            context->numOfUsers--;
            return TRUE;
        }

        gtk_tree_model_iter_next(GTK_TREE_MODEL(context->users), &iter);
    }
    return FALSE;
}

void context_users_remove(ProgramData *data, gchar *username) {
    for (gint i = 0; i < data->chatCount; i++) {
        if (context_users_remove_from(data->chats[i], username)) {
            break;
        }
    }
}

ChatContext *context_get(ProgramData *data, gchar *name) {
    gint index = context_find(data, name);
    if (index == -1) {
        return NULL;
    }
    return data->chats[index];
}

void context_appendLineByName(
    ProgramData *data, gchar *name, 
    gchar *sender, gchar *line, gchar *tags) {

    ChatContext *context = context_get(data, name);
    if (context != NULL) {
        context_appendLine(data, context, sender, line, tags);
    }
}

void context_appendLine(
    ProgramData *data, ChatContext *context, 
    gchar *sender, gchar *line, gchar *tags) {

    gchar time[10];
    getCurrentTime(time);
    gchar buf[BUF_SIZE];
    sprintf(buf, "[%s] %s$ ", time, sender);
    ui_textBufferAppend(context->chat, buf, FALSE, tags);
    ui_textBufferAppend(context->chat, line, TRUE, NULL);

    if (context->autoscroll) {
        ui_chat_scroll(data);
    }
}

void context_remove(ProgramData *data, gchar *name) {
    gint index = context_find(data, name);
    if (index != -1) {
        GtkTreeIter iter;
        GtkListStore *store = data->ui->openchats_liststore;
        GtkTreeModel *model = GTK_TREE_MODEL(store);
        gchar *curr_name;

        gtk_tree_model_get_iter_first(model, &iter);

        for (gint i = 0; i < data->chatCount; i++) {
            gtk_tree_model_get (model, &iter, 0, &curr_name, -1);

            if (g_strcmp0(curr_name, name) == 0) {
                gtk_list_store_remove(store, &iter);
            }

            gtk_tree_model_iter_next(model, &iter);
        }

        context_destroy(data->chats[index]);
        data->chats[index] = data->chats[data->chatCount - 1];
        data->chats[data->chatCount - 1] = NULL;
        data->chatCount--;
    }
}

void context_set_unread(ChatContext *context) {
    if (context != NULL) {
        context->unread = TRUE;
    }
}

void context_set_read(ChatContext *context) {
    if (context != NULL) {
        context->unread = FALSE;
    }   
}

void context_destroy(ChatContext *context) {
    g_free(context->name);
    g_object_unref(context->chat);
    g_object_unref(context->users);
    free(context);
}
