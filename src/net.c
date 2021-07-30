#include <sys/socket.h>
#include <string.h>
#include <stdio.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <unistd.h>
#include <netinet/in.h>
#include <errno.h>
#include "net.h"
#include "common.h"

#define BUFFER_SIZE  (8192)
#define MESSAGE_SIZE (512)

int initNetworking(char *server, int port, int nonBlocking){
    int mySocket = socket(AF_INET, SOCK_STREAM, 0);
    if (mySocket == -1){
        handleError("initNetworking::socket()");
        return -1;
    }

    
    if (nonBlocking) {
        /* set socket to non-blocking mode */
        if (fcntl(mySocket, F_SETFL, fcntl(mySocket, F_GETFL) | O_NONBLOCK) < 0) {
            handleError("initNetworking::fcntl()");
            close(mySocket);
            return -1;
        }
    }

    /* make sure we can reuse the socket */
    int enable = 1;
    if (setsockopt(mySocket, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &enable, sizeof(int)) == ERR){
        handleError("initNetworking::setsockopt()");    
        close(mySocket);
    	return -1;
    }

    char ip[16];
    hostnameToIP(server, ip);
    struct sockaddr_in serverAddress = { 0 };
    inet_aton(ip, &serverAddress.sin_addr);
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(port);
    if(connect(mySocket, (struct sockaddr *)&serverAddress,
           sizeof(struct sockaddr)) == ERR){
        if (errno != EINPROGRESS) {
            handleError("initNetworking::connect()");
            close(mySocket);
            return -1;
        } else {
            /* wait until socket is connected? */
        }
    }

    return mySocket;
}

int32_t hostnameToIP(char *hostname, char *ip) {
    struct hostent *he = gethostbyname(hostname);
    if(he == NULL) {
		return -1;
    }

    struct in_addr **addr_list = (struct in_addr **)he->h_addr_list;
    for(int i = 0; addr_list[i] != NULL; i++) {
		strcpy(ip, inet_ntoa(*addr_list[i]));
		return 0;
    }
    
    return -1;
}

/* Read full messages ending with a certain delimiter
   save data read so far inside buffer */
int receiveDelimiter(const int socket, char *buf, const char *delimiter, int peek) {
	/* store partial messages inside buffer */
    static unsigned int bufferPos = 0;
    static char buffer[BUFFER_SIZE];
    
    char *delimiterPos;
    char *pos = &buffer[bufferPos];
    while(1) {
		delimiterPos = strstr(buffer, delimiter);
		if(delimiterPos != NULL){
		    break;
		}
		
		unsigned int bytes = recv(socket, pos, BUFFER_SIZE - bufferPos, peek ? MSG_PEEK : 0);
		/* got error (EWOULDBLOCK|EAGAIN if socket is non-blocking) */
		if(bytes == -1) {
		    return -1;
		}
		/* no data to read - we can leave */
		if(bytes == 0) {
			return 0;
		}
		bufferPos += bytes;
		pos = &buffer[bufferPos];
		*pos = '\0';
    }

    int msgSize = delimiterPos - buffer;
    memcpy(buf, buffer, msgSize);
    
    char *newBufferStart = delimiterPos + strlen(delimiter);
    int newBufferLen = (int)(buffer + bufferPos - newBufferStart);
    memmove(buffer, newBufferStart, newBufferLen);
    memset(&buffer[newBufferLen], 0, bufferPos - newBufferLen);
    bufferPos = newBufferLen;
    
    return msgSize;    
}

int sendDelimiter(const int socket, const char *const buf, const unsigned int len, const char *DELIMITER) {
    if(len <= 0) {
		return 0; 
    }

    const char *pos = buf;
    unsigned int bytesLeft = len;
    while(bytesLeft > 0) {	
		unsigned int bytes = send(socket, pos, bytesLeft, 0);
		if(bytes == -1) {
		    return -1;
		}
		bytesLeft -= bytes;
		pos += bytes;
    }

    send(socket, DELIMITER, strlen(DELIMITER), 0);
    return 0;    
}

/*  Send the length of the message (4 bytes) 
	before you send the actual message
	
	Loop until the whole message is delivered */
int sendAll(const int socket, const char *const msg, const unsigned int len) {
	if(len == 0) {
		return 0; 
	}

	unsigned int size = sizeof(unsigned int);
	char buf[size];
	memcpy(buf, &len, size);

	if(send(socket, buf, size, 0) < size) {
		return -1;
	}

	char buffer[len];
	strncpy(buffer, msg, len);

	char *pos = buffer;
	unsigned int bytesLeft = len;
	while(bytesLeft > 0) {	
		unsigned int bytes = send(socket, pos, bytesLeft, 0);
		if(bytes == -1) {
			return -1;
		}
		bytesLeft -= bytes;
		pos += bytes;
	}

	return 0;
}

int receiveAll(const int socket, char *buf) {	
	unsigned int size = sizeof(size);
	if(recv(socket, buf, size, MSG_PEEK) < size) {
		/* Socket not ready to read */
		return 0;
	}
	recv(socket, buf, size, 0);

	int len, total;
	memcpy(&len, buf, size);
	total = len;
	
	char *pos = buf;
	while(len > 0) {
		unsigned int bytes = recv(socket, pos, len, 0);
		if(bytes == -1){
			return -1;
		}
		len -= bytes;
		pos += bytes;
	}
	*pos = '\0';
	return total;
}