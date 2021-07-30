#ifndef COMMON_H
#define COMMON_H

#include <gtk/gtk.h>
#include <assert.h>
#include <time.h>

#define WINDOW_TITLE "My IRC client" 
#define MAX_SERVER_REPONSE_TIME_SECS (1)

#define ERR (-1)
#define BUF_SIZE    (2048)
#define TARGET_SIZE (256)

#define MAX_ENTRY_LEN (150)
#define MAX_HISTORY_LEN (20)

#define MAX_CHATS (32)

#define RPL_WELCOME          (1)
#define RPL_LISTSTART        (321)
#define RPL_LIST             (322)
#define RPL_LISTEND          (323)
#define RPL_NAMEREPLY        (353)
#define RPL_ENDOFNAMES       (366)
#define RPL_MOTD             (372)
#define ERR_NONICKNAMEGIVEN  (431)
#define ERR_ERRONEUSNICKNAME (432)
#define ERR_NICKNAMEINUSE    (433)
#define ERR_NICKCOLLISION    (436)
#define ERR_ALREADYREGISTRED (451)
#define ERR_NEEDMOREPARAMS   (461)
#define CMD_JOIN             (10005)
#define CMD_PING             (10004)
#define CMD_NOTICE           (10003)
#define CMD_PRIVMSG          (10002)
#define CMD_PART             (10001)
#define CMD_QUIT             (10000)

#define SELF_CHAT_NAME_STYLE "red_bold"
#define USER_CHAT_NAME_STYLE "blue_bold"
#define SERV_CHAT_NAME_STYLE "black_normal"

typedef struct {
    /* channel/user name*/
    gchar *name;
    /* holds the chat's text */
    GtkTextBuffer *chat;
    gboolean autoscroll;
    /* userlist */
    GtkListStore *users;
    gint numOfUsers;
    gboolean updateList;
    gboolean isChannel;
    gboolean unread;
} ChatContext;

typedef struct {
    /* Main window */
    GtkWidget *main_win;
    GtkWidget *chat_scrolled_win;
    GtkWidget *chat;
    GtkWidget *userlist_treeview;
    GtkWidget *nickname_label;
    GtkWidget *button_join_channel;
    GtkWidget *msg_entry;

    /* Open-Chats */
    GtkCellRenderer *openchats_renderer;
    GtkListStore *openchats_liststore;
    GtkWidget *openchats_treeview;
    GtkTreeSelection *openchats_selection;

    /* User Input (Entry) */
    char history[MAX_HISTORY_LEN][MAX_ENTRY_LEN];
    int hist_size;
    int hist_first;
    int hist_last;
    int hist_curr;

    /* Channel-List window */
    GtkWidget *chanlist_win;
    GtkWidget *chanlist_entry_filter;
    GtkWidget *chanlist_button_filter;
    GtkWidget *chanlist_treeview;
    GtkTreeModel *chanlist_filter;
    GtkListStore *chanlist_liststore;
    char *chanlist_selected;
    gboolean chanlist_list_end;
    gboolean chanlist_refresh;
    time_t chanlist_last_refresh;

    /* Server-Prompt window */
    GtkWidget *serverprompt_win;
    GtkWidget *serverprompt_entry_addr;
    GtkWidget *serverprompt_entry_port;
    GtkWidget *serverprompt_entry_nickname;
    GtkWidget *serverprompt_button_connect;
} UI;

typedef struct {
    /* running state of the program */
    gboolean run;
    /* socket */
    gint socket;
    /* nickname */
    gchar *nick;
    /* server address */
    gchar *server;
    /* server port */
    gint port;
    /* connection state */
    gboolean connected;
    gboolean connecting;
    /* true after message code 001 is sent to us from the server */
    gboolean login;
    /* if we received an error from the server (e.g. already used nickname) */
    gboolean error;
    /* true if we need to choose a different nickname */
    gboolean changeNick;
} Connection;

typedef struct {
    UI *ui;
    Connection *conn;
    /* open chats (contexts) */
    ChatContext *chats[MAX_CHATS];
    gint chatCount;
    /* current context (chat) */
    ChatContext *context;
    /* FLAGS */
    gboolean sendLIST;
} ProgramData;

/* peek at socket for received messages (return true if there is once) */
gint testConnection(gint socket);
/* receive a message from socket */
gint recvMessage(gint socket, gchar *msg);
/* send msg to socket */
void sendMessage(gint socket, gchar *msg);
/* malloc and exit if there's an error */
void *allocateMemory(unsigned long size);
/* realloc and exit if there's an error */
void *reallocateMemory(void *arr, gulong size);
/* fill buf with current time in format hh:mm:ss */
void getCurrentTime(char *buf);
/* return true if AT LEAST 'seconds' amount of seconds passed since 'timestamp' */
gint timeDiff(time_t timestamp, gint seconds);
/* print msg and add perror message */
void handleError(const gchar *msg);
/* convert strings from ASCII to UTF8 */
gchar *stringToUTF8(const gchar *str);
/* convert strings from UTF8 to ASCII */
gchar *stringFromUTF8(const gchar *str);

#endif