#ifndef NET_H
#define NET_H

int receiveDelimiter(const int socket, char *buf, const char *delimiter, int peek);
int sendDelimiter(const int socket, const char *const buf, const unsigned int len, const char *DELIMITER);

int initNetworking(char *server, int port, int nonBlocking);
int hostnameToIP(char *hostname, char *ip);
int sendAll(const int socket, const char *const msg, const unsigned int len);
int receiveAll(const int socket, char *buf);

#endif
