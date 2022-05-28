//******************************************************************* Header File *******************************************************************//
//The header file of the TCP server project.
//Include all the needed libraries , MACRO defenitions , structs.
//***************************************************************************************************************************************************//

#pragma once
#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <iostream>
#include <winsock2.h>
#include <string.h>
#include <time.h>
#pragma comment(lib, "Ws2_32.lib")

using namespace std;

//SocketStates structure
struct SocketState
{
	SOCKET id;			// Socket handle
	int	recv;			// Receiving?
	int	send;			// Sending?
	int sendSubType;	// Sending sub-type
	char buffer[1024];
	int len;
	time_t lastUsed;
	sockaddr_in addr;
};

//Macros
const int EMPTY = 0;
const int LISTEN = 1;
const int RECEIVE = 2;
const int IDLE = 3;
const int SEND = 4;
const int GET = 3;
const int HEAD = 4;
const int OPTIONS = 5;
const int PUT = 6;
const int _DELETE = 7;
const int TRACE = 8;
const int POST = 9;

const int TIMEOUT = 120;
const int TIME_PORT = 27015;
const int MAX_SOCKETS = 60;
const int SEND_TIME = 1;
const int SEND_SECONDS = 2;

//Message Formats

const char* OK_MSG_200 = "HTTP/1.1 200 OK";
const char* CREATED_MSG_201 = "HTTP/1.1 201 Created";
const char* NO_CONTENT_MSG_204 = "HTTP/1.1 204 No Content";
const char* NOT_FOUND_MSG_404 = "HTTP/1.1 404 Not Found";
const char* NOT_IMPLEMENTED_MSG_501 = "HTTP/1.1 501 Not Implemented";

const char* HTML_CONTENT_TYPE = "Content-Type: text/html";
const char* ALL_ALLOWED_QUWERIES = "The Allowed Quweries: GET, HEAD, PUT, POST, DELETE, TRACE, OPTIONS\n";

const char* NEWLINE = "\r\n"; //empty line

void acceptConnection(int idx);
bool addSocket(SOCKET id, sockaddr_in io_addr, int what);
void removeSocket(int idx);
void receiveMessage(int idx);
void sendMessage(int idx);
char* GetFileType(char* filename, char* sendBuff);
void GetContentOfBody(char* recvBuffer, char* bodyBuffer);
void GET_Quwery(char* path);

struct SocketState sockets[MAX_SOCKETS] = { 0 };
int socketsCount = 0;