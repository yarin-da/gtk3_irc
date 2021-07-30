#include "common.h"
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "net.h"

#define DELIMITER "\r\n"

int testConnection(int socket) {
    char buf[BUF_SIZE];
    return receiveDelimiter(socket, buf, DELIMITER, 1 /* PEEK */);
}

int recvMessage(int socket, char *buf) {
    int len = receiveDelimiter(socket, buf, DELIMITER, 0);
#ifdef NET_DEBUG
    if (len > 0) {
        printf("recv: %s\n", buf);
    }
#endif
    return len;
}

void sendMessage(int socket, char *msg) {
#ifdef NET_DEBUG
    printf("send: %s\n", msg);
#endif
    sendDelimiter(socket, msg, strlen(msg), DELIMITER);
}

void *allocateMemory(unsigned long size) {
    void *ptr = malloc(size);
    if (ptr == NULL) {
        handleError("allocateMemory::failed to allocate memory");
        exit(-1);
    }
    return ptr;
}

void *reallocateMemory(void *arr, unsigned long size) {
    void *ptr = realloc(arr, size);
    if (ptr == NULL) {
        handleError("allocateMemory::failed to allocate memory");
        exit(-1);
    }
    return ptr;
}

void getCurrentTime(char *buf){
    time_t now = time(NULL);
    struct tm myTime;
    myTime = *localtime(&now);
    sprintf(buf, "%02d:%02d:%02d", myTime.tm_hour, myTime.tm_min, myTime.tm_sec);
}

int timeDiff(time_t timestamp, int seconds) {
    struct tm before = *localtime(&timestamp);
    time_t now = time(NULL);
    struct tm curr = *localtime(&now);
    return curr.tm_sec - before.tm_sec >= seconds;
}

void handleError(const char *msg){
    static char buf[256];
    static char time[9];
    getCurrentTime(time);
    sprintf(buf, "[%s] %s", time, msg);
    perror(buf);
}

gchar *stringToUTF8(const gchar *str) {
    gchar *ret = g_convert(str, -1, "UTF-8", "ISO-8859-1", NULL, NULL, NULL);
    if (ret == NULL) {
        handleError("stringToUTF8(): g_convert failed");
    }
    return ret;
}

gchar *stringFromUTF8(const gchar *str) {
    gchar *ret = g_convert(str, -1, "ISO-8859-1", "UTF-8", NULL, NULL, NULL);
    if (ret == NULL) {
        handleError("stringToUTF8(): g_convert failed");
    }
    return ret;
}