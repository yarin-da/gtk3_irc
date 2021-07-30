#ifndef CONTEXT_H
#define CONTEXT_H

#include "common.h"

void context_select(ProgramData *data, gchar *context_name);
void context_set_read(ChatContext *context);
void context_set_unread(ChatContext *context);
gint context_users_remove_from(ChatContext *context, gchar *username);
void context_users_remove(ProgramData *data, gchar *username);
void context_users_finish_byName(ProgramData *data, gchar *context_name);
void context_users_add_byName(ProgramData *data, gchar *context_name, gchar *name);
void context_users_addBulk_byName(ProgramData *data, gchar *context_name, gchar *users);
void context_users_add(ChatContext *context, gchar *name);
/* users - a string containing usernames separated by a space character */
void context_users_addBulk(ChatContext *context, gchar *users);
void context_users_finish(ChatContext *context);
void context_init(ProgramData *data);
void context_appendLine(ProgramData *data, ChatContext *context, gchar *sender, gchar *line, gchar *tags);
void context_appendLineByName(ProgramData *data, gchar *name, gchar *sender, gchar *line, char *tags);
ChatContext *context_create(ProgramData *data, gchar *name, gint isChannel);
ChatContext *context_get(ProgramData *data, gchar *name);
void context_remove(ProgramData *data, gchar *name);
void context_destroy(ChatContext *context);

#endif