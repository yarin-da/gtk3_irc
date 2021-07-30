#include "serverinput.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include "common.h"
#include "chanlist.h"
#include "serveroutput.h"
#include "ui.h"

static void setMessageCode(ServerMessage *msg) {
    char *cmd = msg->command;
    if (isdigit(*cmd)) {
        msg->code = atoi(cmd);
    } else if (!strcmp(cmd, "NOTICE")) {
        msg->code = CMD_NOTICE;
    } else if (!strcmp(cmd, "PART")) {
        msg->code = CMD_PART;
    } else if (!strcmp(cmd, "PRIVMSG")) {
        msg->code = CMD_PRIVMSG;
    } else if (!strcmp(cmd, "PING")) {
        msg->code = CMD_PING;
    } else if (!strcmp(cmd, "JOIN")) {
        msg->code = CMD_JOIN;
    } else if (!strcmp(cmd, "QUIT")) {
        msg->code = CMD_QUIT;
    }
}

static int getLastArgIndex(char *buf) {
    int index = -1;
    int len = strlen(buf);
    for (int i = 0; i < len; i++) {
        if (buf[i] == ' ') {
            if (buf[i + 1] == ':') {
                index = i + 1;
                break;
            }
        }
    }
    return index;
}

ServerMessage *parseServerMessage(char *buf) {
    ServerMessage *msg = allocateMemory(sizeof(ServerMessage));
    msg->content = strdup(buf);
    int hasSource = *buf == ':';
    char *pos = buf;
    if (hasSource) {
        pos++;
    }
    int lastArgIdx = getLastArgIndex(pos);
    pos[lastArgIdx] = '\0';
    char *args = pos;
    char *lastArg = NULL;
    if (lastArgIdx != -1) {
        pos[lastArgIdx] = '\0';
        args = pos;
        lastArg = pos + lastArgIdx + 1;
    }
    char *token = strtok(args, " ");
    
    if (hasSource) {
        msg->source = token;
        token = strtok(NULL, " ");
    } else {
        msg->source = NULL;
    }

    msg->command = token;
    token = strtok(NULL, " ");

    /* read the rest of the message and parse into args */
    int numOfArgs = 0;
    do {
        msg->args[numOfArgs] = token;
        numOfArgs++;
        if ((token = strtok(NULL, " ")) == NULL) {
            break;
        }
    } while (1);

    if (lastArg != NULL) {
        msg->args[numOfArgs] = lastArg;
        numOfArgs++;
    }

    msg->numOfArgs = numOfArgs;
    /* keep only the nick in source */
    msg->source = strtok(msg->source, "!");
    /* we don't print messages by default */
    /* set to 0 (FALSE) */
    msg->print = 0;
    msg->context = NULL;
    msg->code = -1;
    setMessageCode(msg);
    return msg;
}

void actOnServerMessage(ProgramData *data, ServerMessage *msg) {
    char *name, *topic;
    int users;
    switch(msg->code) {
    case RPL_WELCOME:
        data->conn->login = TRUE;
        break;
    case RPL_LIST:
        if (data->ui->chanlist_list_end) {
            data->ui->chanlist_list_end = FALSE;
            chanlist_clear(data);
        }
        name = msg->args[1];
        users = atoi(msg->args[2]);
        topic = msg->args[3];
        chanlist_add(data, name, users, topic);
        break;
    case RPL_LISTEND:
        data->ui->chanlist_list_end = TRUE;
        data->ui->chanlist_refresh = FALSE;
        break;
    case RPL_NAMEREPLY:
        /* pass the last argument to updateBulk (list of names) */
        context_users_addBulk_byName(data, msg->args[2] + 1, msg->args[msg->numOfArgs - 1]);
        break;
    case RPL_ENDOFNAMES:
        context_users_finish_byName(data, msg->args[2] + 1);
        break;
    case RPL_MOTD:
    case CMD_NOTICE:
        handleServerNOTICE(data, msg);
        break;
    case CMD_PRIVMSG:
        handleServerPRIVMSG(data, msg);
        break;
    case CMD_PING:
        handleServerPING(data, msg);
        break;
    case CMD_JOIN:
        handleServerJOIN(data, msg);
        break;
    case CMD_PART:
    case CMD_QUIT:
        handleServerQUIT(data, msg);
        break;
    case ERR_NONICKNAMEGIVEN:
        /* shouldn't happen */
        break;
    /* nickname has wrong characters */
    case ERR_ERRONEUSNICKNAME:
    /* nickname is already in use */
    case ERR_NICKNAMEINUSE:
    case ERR_NICKCOLLISION:
        data->conn->changeNick = TRUE;
        break;
    case ERR_ALREADYREGISTRED:
        break;
    case ERR_NEEDMOREPARAMS:
        /* shouldn't happen */
        break;
    }
}

void handleServerQUIT(ProgramData *data, ServerMessage *msg) {
    /* :dudebro    QUIT     :Quit: Leaving */
    /* :source     QUIT     :Quit: Leaving */
    /* msg->source QUIT     msg->args[0] */
    char *user = msg->source;
    context_users_remove(data, user);
}

void handleServerJOIN(ProgramData *data, ServerMessage *msg) {
    /*
    :dudebro    JOIN     #zzz
    :source     JOIN     #channel 
    msg->source msg->cmd msg->args[0] + 1
    */
    char *user = msg->source;
    if (msg->numOfArgs > 0) {
        char *chan_name = msg->args[0] + 1;
        context_users_add_byName(data, chan_name, user);
    } else {
        handleError("join --- missing channel");
    }
}

void handleServerPING(ProgramData *data, ServerMessage *msg) {
    char *text = msg->args[msg->numOfArgs - 1];
    sendmsg_pong(data, text);
}

void handleServerPRIVMSG(ProgramData *data, ServerMessage *msg) {
    /*
    :XXX PRIVMSG #zzz :lol
    :XXX PRIVMSG YYY :hey
    */
    msg->print = TRUE;
    gchar *dest = msg->args[0];
    gchar *src = msg->source;
    
    if (msg->args[0][0] == '#') {
        msg->context = context_get(data, dest + 1);
    /* if message is intended to me (private chat) */
    } else if (g_strcmp0(dest, data->conn->nick) == 0) {    
        msg->context = context_get(data, src);
        if (msg->context == NULL) {
            msg->context = context_create(data, src, FALSE);
        }
    /* if the message is not intended to any of our open chats */
    } else {
        msg->context = context_get(data, "Main Context");
    }

    if (msg->context != data->context) {
        context_set_unread(msg->context);
    }
}

void handleServerNOTICE(ProgramData *data, ServerMessage *msg) {
    msg->print = TRUE;
    msg->context = context_get(data, "Main Context");

    if (msg->context != data->context) {
        context_set_unread(msg->context);
    }
}

void printServerMessage(ServerMessage *msg) {
    printf("====MSG====\n");
    printf("source: %s\n", msg->source == NULL? "null":msg->source);
    printf("code: %d\n", msg->code);
    /* printf("context: %s\n", msg->context->name); */
    printf("args:\n");
    for (int i = 0; i < msg->numOfArgs; i++) {
        printf("\targ[%d]: %s\n", i, msg->args[i]);
    }
    printf("====FIN====\n\n");
}

void destroyServerMessage(ServerMessage *msg) {
    free(msg->content);
	free(msg);
}