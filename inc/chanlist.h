#ifndef CHANLIST_H
#define CHANLIST_H

#include "common.h"

void chanlist_init(ProgramData *data);
void chanlist_add(ProgramData *data, char *name, int users, char *topic);
void chanlist_clear(ProgramData *data);
void chanlist_refresh(ProgramData *data);

/* called when user clicks the search button */
void chanlist_callback_search_clicked(GtkWidget *widget, gpointer user_data);
/* called after user requested to open this window */
void chanlist_callback_listChannels(GtkWidget *widget, gpointer user_data);
/* called when user selected a row in the list */
void chanlist_callback_selected(GtkTreeSelection *selection, gpointer user_data);
/* called when user pressed the join button */
void chanlist_callback_join_pressed(GtkWidget *widget, gpointer user_data);

#endif