#ifndef USER_INPUT_H
#define USER_INPUT_H

#define MAX_USER_ARGS (10)

#include "common.h"

typedef struct {
	int isDebug;
	int isCommand;
	int isPrintable;
	char *content;
	char *command;
	char *args[MAX_USER_ARGS];
	int numOfArgs;
} UserMessage;

void userinput_callback_activate(GtkWidget *entry, gpointer user_data);
void userinput_scroll_history(ProgramData *data, int forwards);
void userinput_clear(ProgramData *data);
void userinput_init(GtkWidget *chatView, GtkWidget *entryView);

UserMessage *parseUserMessage(char *buf);
void destroyUserMessage(UserMessage *input);

#endif