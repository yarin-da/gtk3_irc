#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <assert.h>
#include "net.h"
#include "common.h"
#include "ui.h"
#include "userinput.h"
#include "serverinput.h"
#include "context.h"
#include "chanlist.h"
#include "serveroutput.h"
#include <gio/gio.h>

int defaultPort = 6667;
char *defaultServer = "chat.freenode.org";
char *defaultNickname = "dudebruh";

int waitForServerResponse(gint socket, gint maxWaitTime);
int receiveFromServer(gint socket, gchar buf[BUF_SIZE]);
void actOnServerMessage(ProgramData *data, ServerMessage *msg);
void handleServerMessage(ProgramData *data, ServerMessage *msg);
void handleUserMessage(ProgramData *data, UserMessage *input, gchar *msg);
void applyUserCommand(ProgramData *data, UserMessage *input, gchar *msg);
void joinChannel(ProgramData *data, gchar *targetChannel);
void updateContext(ProgramData *data, ChatContext *context);
int waitForRPLWELCOME(ProgramData *data, gint maxWaitTimeSeconds);
void UI_init(ProgramData *data);
/* idles */
gboolean mainLoop();
/* callbacks */
void serverprompt_callback_connect(GtkWidget *widget, gpointer user_data);
void userinput_callback_activate(GtkWidget *entry, gpointer user_data);
void cleanUp(ProgramData *data);
void stopProgram(GtkWidget *widget, gpointer user_data);

int main (int argc, char *argv[]) {
    ProgramData *data = allocateMemory(sizeof(ProgramData));
    data->conn = allocateMemory(sizeof(Connection));
    data->ui = allocateMemory(sizeof(UI));

    data->conn->run = TRUE;
    data->conn->connecting = FALSE;
    data->conn->connected = FALSE;
    data->conn->login = FALSE;

    gtk_init(&argc, &argv);
    /* init UI struct */
    UI_init(data);

    gtk_widget_show(data->ui->serverprompt_win);

    g_idle_add(mainLoop, data);
    gtk_main();
    cleanUp(data);
    return EXIT_SUCCESS;
}

void UI_init(ProgramData *data) {
    GtkBuilder *builder = gtk_builder_new();
    gtk_builder_add_from_file(builder, "glade/ui.glade", NULL);

    /* Main Window */
    data->ui->main_win = GTK_WIDGET(gtk_builder_get_object(builder, "main_win"));
    data->ui->chat_scrolled_win = GTK_WIDGET(gtk_builder_get_object(builder, "chat_scroll"));
    data->ui->chat = GTK_WIDGET(gtk_builder_get_object(builder, "chat"));
    data->ui->userlist_treeview = GTK_WIDGET(gtk_builder_get_object(builder, "userlist_treeview"));
    data->ui->msg_entry = GTK_WIDGET(gtk_builder_get_object(builder, "msg_entry"));
    data->ui->button_join_channel = GTK_WIDGET(gtk_builder_get_object(builder, "button_join_channel"));
    data->ui->nickname_label = GTK_WIDGET(gtk_builder_get_object(builder, "nickname_label"));
    data->ui->openchats_treeview = GTK_WIDGET(gtk_builder_get_object(builder, "openchats_treeview"));
    data->ui->openchats_selection = GTK_TREE_SELECTION(gtk_builder_get_object(builder, "openchats_selection"));
    /* User Input (Entry) */
    data->ui->hist_size = 0;
    data->ui->hist_first = 0;
    data->ui->hist_last = 0;
    data->ui->hist_curr = 0;
    /* Channel List */
    data->ui->chanlist_win = GTK_WIDGET(gtk_builder_get_object(builder, "chanlist_win"));
    data->ui->chanlist_treeview = GTK_WIDGET(gtk_builder_get_object(builder, "chanlist_treeview"));
    data->ui->chanlist_entry_filter = GTK_WIDGET(gtk_builder_get_object(builder, "chanlist_entry_filter"));
    data->ui->chanlist_button_filter = GTK_WIDGET(gtk_builder_get_object(builder, "chanlist_button_filter"));
    data->ui->chanlist_liststore = NULL;
    data->ui->chanlist_selected = NULL;
    /* Server Login Prompt*/
    data->ui->serverprompt_win = GTK_WIDGET(gtk_builder_get_object(builder, "server_prompt_win"));
    data->ui->serverprompt_entry_addr = GTK_WIDGET(gtk_builder_get_object(builder, "server_prompt_address_entry"));
    data->ui->serverprompt_entry_port = GTK_WIDGET(gtk_builder_get_object(builder, "server_prompt_port_entry"));
    data->ui->serverprompt_entry_nickname = GTK_WIDGET(gtk_builder_get_object(builder, "server_prompt_nickname_entry"));
    data->ui->serverprompt_button_connect = GTK_WIDGET(gtk_builder_get_object(builder, "server_prompt_button_connect"));

    gtk_builder_connect_signals(builder, data);

    g_object_unref(builder);

    context_init(data);
    chanlist_init(data);    
    ui_userlist_init(data);
}

int waitForServerResponse(int socket, int maxWaitTimeSeconds) {
    /* used for timing */
    time_t now = time(NULL);

    /* wait for response or until time runs out */
    while (testConnection(socket)) {
        if (timeDiff(now, maxWaitTimeSeconds)) {
            /* time ran out and we didn't receive a message from the server */
            return FALSE;
        }
    }
    /* if we got here, then we're connected */
    return TRUE;
}

void handleServerMessage(ProgramData *data, ServerMessage *msg) {
    actOnServerMessage(data, msg);
    if (msg->print) {
        gchar *message = msg->args[msg->numOfArgs - 1];
        gchar *sender = msg->source;
        context_appendLine(data, msg->context, sender, 
            message, USER_CHAT_NAME_STYLE);
    }
    destroyServerMessage(msg);
}

void handleUserMessage(ProgramData *data, UserMessage *input, gchar *msg) {
    assert(input != NULL);
    msg[0] = '\0';

    /* custom debugging messages */
    if (input->isDebug) {
        sprintf(msg, "%s", input->content + 1);
    } else if (input->isCommand) {
        /* commands start with '/' */
        applyUserCommand(data, input, msg);
    } else if (data->context != NULL) {
        sendmsg_privmsg(data, data->context->name, TRUE, input->content);
    }
}

void applyUserCommand(ProgramData *data, UserMessage *input, gchar *msg) {
    assert(input->command != NULL);
    if (!strcmp(input->command, "join")) {
        if (input->numOfArgs == 1) {
            joinChannel(data, input->args[0]);
        } else {
            /* wrong number of arguments */
            handleError("applyUserCommand::join wrong number of args");
        }
    /* also: query, m */
    } else if (!strcmp(input->command, "msg")) {
        if (input->numOfArgs < 1) {
            handleError("applyUserCommand::msg wrong number of args");
        }
        gchar *target = input->args[0];
        /* if the user wants to send a message to the target */
        if (input->numOfArgs > 1) {
            /* send message to 'target' */
            ChatContext *context = context_get(data, target);
            if (context == NULL) {
                context = context_create(data, target, FALSE);
                sendmsg_privmsg(data, target, FALSE, "");
                gchar *pos = msg + strlen(msg);
                for (gint i = 0; i < input->numOfArgs; i++) {
                    gchar *arg = input->args[i];
                    strcpy(pos, arg);
                    gint len = strlen(pos);
                    pos += len;
                    *pos = ' ';
                    pos++;
                }
                *pos = '\0';
            }
        }
    }
}

void updateContext(ProgramData *data, ChatContext *context) {
    assert(context != NULL);
    data->context = context;
    context_set_read(context);
    ui_chat_setContext(data);
    ui_userlist_setContext(data);
    context_select(data, data->context->name);
}

void cleanUp(ProgramData *data) {
    close(data->conn->socket);
    free(data->ui);
    free(data->conn);
    free(data);
}

void stopProgram(GtkWidget *widget, gpointer user_data) {
    ProgramData *data = (ProgramData *)user_data;

    /* logout from the server */
    if (data->conn->login) {
        sendmsg_quit(data);
    }

    /* close connection */
    if (data->conn->connected) {
        close(data->conn->socket);
    }
    
    /* set main loop's ret val to FALSE */
    data->conn->run = FALSE;

    gtk_main_quit();
}

void joinChannel(ProgramData *data, gchar *targetChannel) {
    ChatContext *context = context_get(data, targetChannel);
    if (context == NULL) {
        context = context_create(data, targetChannel, TRUE);
        sendmsg_join(data, targetChannel);
    }
    updateContext(data, context);
}

int waitForRPLWELCOME(ProgramData *data, gint maxWaitTimeSeconds) {    
    time_t now = time(NULL);
    int code;
    while (TRUE) {
        gchar buf[BUF_SIZE];
        guint len = receiveFromServer(data->conn->socket, buf);
        if (len > 0) {
            ServerMessage *msg = parseServerMessage(buf);
            code = msg->code;
            destroyServerMessage(msg);
            switch(code) {
            case RPL_WELCOME:
            case ERR_NONICKNAMEGIVEN :
            case ERR_ERRONEUSNICKNAME:
            case ERR_NICKNAMEINUSE:
            case ERR_NICKCOLLISION:
            case ERR_ALREADYREGISTRED:
            case ERR_NEEDMOREPARAMS:
                return code;
            } 
            /* time's up - didn't receive RPL_WELCOME */
            if (timeDiff(now, maxWaitTimeSeconds)) {
                return FALSE;
            }
        }
    }
}

int receiveFromServer(int socket, char buf[BUF_SIZE]){
    int fromServerLen = recvMessage(socket, buf);
    if(fromServerLen == ERR){
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            handleError("receiveFromServer:: socket recv");
        }
        fromServerLen = 0;
    }else if(fromServerLen != 0){
        buf[fromServerLen] = '\0';
    }

    return fromServerLen;
}

void userinput_callback_activate(GtkWidget *entry, gpointer user_data) {
    ProgramData *data = (ProgramData *)user_data;
    assert(data != NULL);
    assert(data->conn != NULL);
    GtkEntryBuffer *currentBuffer = gtk_entry_get_buffer(GTK_ENTRY(entry));
    gchar *buftxt = (gchar *)gtk_entry_buffer_get_text(currentBuffer);

    gchar buf[BUF_SIZE];
    UserMessage *input = parseUserMessage(buftxt);
    /* return if there's no input (entry is empty) */
    if (input == NULL) {
        return;
    }
    
    handleUserMessage(data, input, buf);

    /* add user input to chat view */
    if (input->isPrintable) {
        context_appendLine(data, data->context, data->conn->nick, 
            input->content, SELF_CHAT_NAME_STYLE);
    }
    userinput_clear(data);
    destroyUserMessage(input);
}

gboolean mainLoop(gpointer user_data) {
    ProgramData *data = (ProgramData *)user_data;
    gchar buf[BUF_SIZE];
    if (data->conn->login) {
        guint len = receiveFromServer(data->conn->socket, buf);
        if (len > 0) {
            ServerMessage *msg = parseServerMessage(buf);
            handleServerMessage(data, msg);
        }    
    }   
    return data->conn->run;
}

void serverprompt_callback_connect(GtkWidget *widget, gpointer user_data) {
    ProgramData *data = (ProgramData *)user_data;
    /* get input from the entries */
    const gchar *address_text = gtk_entry_get_text(
        GTK_ENTRY(data->ui->serverprompt_entry_addr));
    const gchar *port_text = gtk_entry_get_text(
        GTK_ENTRY(data->ui->serverprompt_entry_port));
    const gchar *nickname_text = gtk_entry_get_text(
        GTK_ENTRY(data->ui->serverprompt_entry_nickname));
    
    /* convert ui input from to ascii */
    address_text = stringFromUTF8(address_text);
    port_text = stringFromUTF8(port_text);
    nickname_text = stringFromUTF8(nickname_text);

    /* set connection variables (defaults if there's no input) */
    /* set port */
    data->conn->port = atoi(port_text);
    if (!port_text || strlen(port_text) == 0) {
        data->conn->port = defaultPort;
    } else {
        data->conn->port = atoi(port_text);
    }
    /* set address (server) */
    if (!address_text || strlen(address_text) == 0) {
        data->conn->server = defaultServer;
    } else {
        data->conn->server = address_text;
    }
    /* set nickname */
    if (!nickname_text || strlen(nickname_text) == 0) {
        data->conn->nick = defaultNickname;
    } else {
        data->conn->nick = nickname_text;
    }

    /* connect to the server */
    if (data->conn->connected == FALSE) {
        data->conn->connecting = TRUE;
        /* create a non-blocking socket and connect to the server */
        data->conn->socket = initNetworking(
            data->conn->server, data->conn->port, TRUE);

        if (data->conn->socket == ERR) {
            handleError("failed to connect to the server");
        } else {
            data->conn->connected = TRUE;
            data->conn->connecting = FALSE;
        }
    }

    /* login to the server */
    if (data->conn->connected && data->conn->login == FALSE) {
        /* wait a few seconds before sending login info */
        waitForServerResponse(data->conn->socket, MAX_SERVER_REPONSE_TIME_SECS);

        /* login to the server */
        sendmsg_nick(data);
        sendmsg_user(data);

        /* wait for RPLWELCOME - which implies we're logged in */
        data->conn->login = waitForRPLWELCOME(data, MAX_SERVER_REPONSE_TIME_SECS);

        /* set label to the chosen nickname */
        if (data->conn->login) {
            gtk_label_set_text(GTK_LABEL(data->ui->nickname_label), data->conn->nick);    
        }

        /* "close" the login window */
        gtk_widget_hide(data->ui->serverprompt_win);

        /* open main window */
        gtk_widget_show(data->ui->main_win);
        /* focus on entry (for user input) */
        gtk_widget_grab_focus(data->ui->msg_entry);

        /* load and buffer the server's channel list */ 
        chanlist_refresh(data);
    }
}

void context_callback_row_selected(GtkTreeSelection *selection, gpointer user_data) {
    ProgramData *data = (ProgramData *)user_data;
    assert(data->ui->openchats_liststore != NULL);

    GtkTreeIter iter;
    GtkTreeModel *model = GTK_TREE_MODEL(data->ui->openchats_liststore);
    char *chat_name = NULL;
    if (gtk_tree_selection_get_selected(selection, &model, &iter)) {
        gtk_tree_model_get(model, &iter, 0 /* COLUMN_CHATS */, &chat_name, -1);
        ChatContext *context = context_get(data, chat_name);
        assert(context != NULL);
        updateContext(data, context);
    }
}
