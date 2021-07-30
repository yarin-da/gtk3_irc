/* Handle output from our client to the server */
#ifndef SERVER_OUTPUT_H
#define SERVER_OUTPUT_H

#include "common.h"

void sendmsg_quit(ProgramData *data);
void sendmsg_user(ProgramData *data);
void sendmsg_nick(ProgramData *data);
void sendmsg_list(ProgramData *data);
void sendmsg_join(ProgramData *data, gchar *channel);
void sendmsg_pong(ProgramData *data, gchar *msg);
void sendmsg_privmsg(ProgramData *data, gchar *to, int isChannel, gchar *msg);

#endif