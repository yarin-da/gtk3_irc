#ifndef UI_H
#define UI_H

#include <gtk/gtk.h>
#include "common.h"

void ui_textBufferAppend(GtkTextBuffer *buffer, gchar *line, gint newLine, gchar *tag_name);
void ui_textViewSetBuffer(GtkWidget *view, GtkTextBuffer *new_buffer);
void ui_textViewAppend(GtkWidget *view, gchar *line, gint newLine, gchar *tag_name);
void ui_textViewAppendAll(GtkWidget *textView, gchar **buf, gint size);
void ui_winAutoScroll(GtkWidget *scrolledWin);
void ui_textViewClear(GtkWidget *view);
void ui_textBufferClear(GtkTextBuffer *buffer);
void ui_closeWindow();
void ui_chat_scroll(ProgramData *data);
void ui_chat_setContext(ProgramData *data);
void ui_userlist_init(ProgramData *data);
void ui_userlist_setContext(ProgramData *data);

#endif