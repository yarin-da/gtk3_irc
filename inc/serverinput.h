#ifndef SERVER_INPUT_H
#define SERVER_INPUT_H

#include "context.h"
#include "common.h"

#define MAX_ARGS (50)

typedef struct {
    char *source;
    int code;
    char *command;
    char *args[MAX_ARGS];
    int numOfArgs;
    int print;
    ChatContext *context;
    char *content;
} ServerMessage;

ServerMessage *parseServerMessage(char *buf);
void printServerMessage(ServerMessage *msg);
void destroyServerMessage(ServerMessage *msg);

void handleServerJOIN(ProgramData *data, ServerMessage *msg);
void handleServerQUIT(ProgramData *data, ServerMessage *msg);
void handleServerPING(ProgramData *data, ServerMessage *msg);
void handleServerPRIVMSG(ProgramData *data, ServerMessage *msg);
void handleServerNOTICE(ProgramData *data, ServerMessage *msg);

#endif