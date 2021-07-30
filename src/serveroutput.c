#include "serveroutput.h"
#include <stdio.h>

static char buf[BUF_SIZE];

void sendmsg_quit(ProgramData *data) {
	sprintf(buf, ":%s QUIT", data->conn->nick);
	sendMessage(data->conn->socket, buf);
}

void sendmsg_user(ProgramData *data) {
	sprintf(buf, "USER %s 0 * :Real name", data->conn->nick);
	sendMessage(data->conn->socket, buf);
}

void sendmsg_nick(ProgramData *data) {
	sprintf(buf, "NICK %s", data->conn->nick);
	sendMessage(data->conn->socket, buf);
}

void sendmsg_list(ProgramData *data) {
	sprintf(buf, ":%s LIST", data->conn->nick);
    sendMessage(data->conn->socket, buf);
}

void sendmsg_join(ProgramData *data, gchar *channel) {
	sprintf(buf, ":%s JOIN #%s :reason", data->conn->nick, channel);
	sendMessage(data->conn->socket, buf);
}

void sendmsg_pong(ProgramData *data, gchar *msg) {
	sprintf(buf, ":%s PONG :%s", data->conn->nick, msg);
	sendMessage(data->conn->socket, buf);
}

void sendmsg_privmsg(ProgramData *data, gchar *to, int isChannel, gchar *msg) {
	if (isChannel) {
		sprintf(buf, ":%s PRIVMSG #%s :%s", data->conn->nick, to, msg);	
	} else {
		sprintf(buf, ":%s PRIVMSG %s :%s", data->conn->nick, to, msg);	
	}
	
	sendMessage(data->conn->socket, buf);
}
