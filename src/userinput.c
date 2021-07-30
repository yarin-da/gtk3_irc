#include "userinput.h"
#include <string.h>
#include <stdio.h>
#include "context.h"

#define COMMAND_PREFIX '/'
#define DEBUG_PREFIX '$'
#define TOKEN_DELIM " "

gboolean userinput_callback_keyboard(GtkWidget *entry, GdkEventKey *event, gpointer user_data) {
    ProgramData *data = (ProgramData *)user_data;
    switch (event->keyval) {
    case GDK_KEY_Up:
        userinput_scroll_history(data, FALSE);
        break;
    case GDK_KEY_Down:
        userinput_scroll_history(data, TRUE);
        break;
    default:
        return gtk_entry_im_context_filter_keypress(GTK_ENTRY(entry), event);
    }
    return TRUE;
}

void userinput_scroll_history(ProgramData *data, int forwards) {
	if (forwards) {
		if (data->ui->hist_curr != data->ui->hist_last) {
			data->ui->hist_curr = (data->ui->hist_curr + 1) % MAX_HISTORY_LEN;
		}
	/* backwards */
	} else {
		if (data->ui->hist_curr != data->ui->hist_first) {
			data->ui->hist_curr = (data->ui->hist_curr - 1) % MAX_HISTORY_LEN;
		}
	}
	gtk_entry_set_text(GTK_ENTRY(data->ui->msg_entry), data->ui->history[data->ui->hist_curr]);
	GtkEntryBuffer *buffer = gtk_entry_get_buffer(GTK_ENTRY(data->ui->msg_entry));
	int bufferLength = gtk_entry_buffer_get_length(buffer);
	gtk_editable_set_position(GTK_EDITABLE(data->ui->msg_entry), bufferLength);
}

void userinput_clear(ProgramData *data) {
	int len = gtk_entry_get_text_length(GTK_ENTRY(data->ui->msg_entry));
	if (len != 0) {
		const char *text = gtk_entry_get_text(GTK_ENTRY(data->ui->msg_entry));
		strcpy(data->ui->history[data->ui->hist_last], text);
		data->ui->hist_last = (data->ui->hist_last + 1) % MAX_HISTORY_LEN;
		strcpy(data->ui->history[data->ui->hist_last], "");
		data->ui->hist_curr = data->ui->hist_last;
		if (data->ui->hist_size == MAX_HISTORY_LEN) {
			data->ui->hist_first = (data->ui->hist_first + 1) % MAX_HISTORY_LEN;
		} else {
			data->ui->hist_size++;
		}
	}
	gtk_entry_set_text(GTK_ENTRY(data->ui->msg_entry), data->ui->history[data->ui->hist_last]);
}

UserMessage *parseUserMessage(char *buf) {
	if (buf == NULL) {
		return NULL;
	}
	if (*buf == '\0') {
		return NULL;
	}

	UserMessage *msg = allocateMemory(sizeof(UserMessage));
	msg->content = allocateMemory(strlen(buf) + 1);
	msg->isCommand = *buf == COMMAND_PREFIX;
	msg->isDebug = *buf == DEBUG_PREFIX;
	msg->isPrintable = !msg->isCommand && !msg->isDebug;

	strcpy(msg->content, buf);

	if (msg->isCommand) {
		msg->command = strtok(buf + 1, TOKEN_DELIM);
		char *arg = NULL;
		int numOfArgs = 0;
		while ((arg = strtok(NULL, TOKEN_DELIM)) != NULL) {
			msg->args[numOfArgs] = arg;
			numOfArgs++;
			if (numOfArgs > MAX_USER_ARGS) {
				printf("parseUserMessage: exceeded maximum arguments\n");
				destroyUserMessage(msg);
				return NULL;
			}
		}
		msg->numOfArgs = numOfArgs;
	}
	return msg;
}

void destroyUserMessage(UserMessage *msg) {
	free(msg->content);
	free(msg);
}