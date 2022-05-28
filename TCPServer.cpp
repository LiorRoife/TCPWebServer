//******************************************************************* Main File *******************************************************************//
//This project is an implmentation of TCP Web Server which can handle variuos of quweries (SET , GET , POST , PUT , HEAD , TRACE , OPTIONS)
//***************************************************************************************************************************************************//

#include "Header.h"

void main()
{
	time_t now;
	sockaddr_in clientInfo;
	clientInfo.sin_family = AF_INET;
	clientInfo.sin_addr.s_addr = INADDR_ANY;
	clientInfo.sin_port = htons(TIME_PORT);
	const timeval* timeout = new timeval{ 120,0 }; //For timeout cases
	// Initialize Winsock (Windows Sockets).
	// Create a WSADATA object called wsaData.
	// The WSADATA structure contains information about the Windows 
	// Sockets implementation.
	WSAData wsaData;

	// Call WSAStartup and return its value as an integer and check for errors.
	// The WSAStartup function initiates the use of WS2_32.DLL by a process.
	// First parameter is the version number 2.2.
	// The WSACleanup function destructs the use of WS2_32.DLL by a process.
	if (NO_ERROR != WSAStartup(MAKEWORD(2, 2), &wsaData))
	{
		cout << "Time Server: Error at WSAStartup()\n";
		return;
	}
	// Server side:
	// Create and bind a socket to an internet address.
	// Listen through the socket for incoming connections.

	// After initialization, a SOCKET object is ready to be instantiated.

	// Create a SOCKET object called listenSocket. 
	// For this application:	use the Internet address family (AF_INET), 
	//							streaming sockets (SOCK_STREAM), 
	//							and the TCP/IP protocol (IPPROTO_TCP).
	SOCKET listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	// Check for errors to ensure that the socket is a valid socket.
	// Error detection is a key part of successful networking code. 
	// If the socket call fails, it returns INVALID_SOCKET. 
	// The if statement in the previous code is used to catch any errors that
	// may have occurred while creating the socket. WSAGetLastError returns an 
	// error number associated with the last error that occurred.
	if (INVALID_SOCKET == listenSocket)
	{
		cout << "Time Server: Error at socket(): " << WSAGetLastError() << endl;
		WSACleanup();
		return;
	}
	// For a server to communicate on a network, it must bind the socket to 
	// a network address.
	// Need to assemble the required data for connection in sockaddr structure.
	// Create a sockaddr_in object called serverService. 
	sockaddr_in serverService;
	// Address family (must be AF_INET - Internet address family).
	serverService.sin_family = AF_INET;
	// IP address. The sin_addr is a union (s_addr is a unsigned long 
	// (4 bytes) data type).
	// inet_addr (Iternet address) is used to convert a string (char *) 
	// into unsigned long.
	// The IP address is INADDR_ANY to accept connections on all interfaces.
	serverService.sin_addr.s_addr = INADDR_ANY;
	// IP Port. The htons (host to network - short) function converts an
	// unsigned short from host to TCP/IP network byte order 
	// (which is big-endian).
	serverService.sin_port = htons(TIME_PORT);

	// Bind the socket for client's requests.
	// The bind function establishes a connection to a specified socket.
	// The function uses the socket handler, the sockaddr structure (which
	// defines properties of the desired connection) and the length of the
	// sockaddr structure (in bytes).
	if (SOCKET_ERROR == bind(listenSocket, (SOCKADDR*)&serverService, sizeof(serverService)))
	{
		cout << "Time Server: Error at bind(): " << WSAGetLastError() << endl;
		closesocket(listenSocket);
		WSACleanup();
		return;
	}
	// Listen on the Socket for incoming connections.
	// This socket accepts only one connection (no pending connections 
	// from other clients). This sets the backlog parameter.
	if (SOCKET_ERROR == listen(listenSocket, 5))
	{
		cout << "Time Server: Error at listen(): " << WSAGetLastError() << endl;
		closesocket(listenSocket);
		WSACleanup();
		return;
	}
	addSocket(listenSocket, clientInfo, LISTEN);

	// Accept connections and handles them one by one.
	while (true)
	{
		// The select function determines the status of one or more sockets,
		// waiting if necessary, to perform asynchronous I/O. Use fd_sets for
		// sets of handles for reading, writing and exceptions. select gets "timeout" for waiting
		// and still performing other operations (Use NULL for blocking). Finally,
		// select returns the number of descriptors which are ready for use (use FD_ISSET
		// macro to check which descriptor in each set is ready to be used).
		fd_set waitRecv;
		FD_ZERO(&waitRecv);

		for (int i = 0; i < MAX_SOCKETS; i++)
		{
			if ((sockets[i].recv == LISTEN) || (sockets[i].recv == RECEIVE))
				FD_SET(sockets[i].id, &waitRecv);
		}

		fd_set waitSend;
		FD_ZERO(&waitSend);
		for (int i = 0; i < MAX_SOCKETS; i++)
		{
			if (sockets[i].send == SEND)
				FD_SET(sockets[i].id, &waitSend);
		}

		// Wait for interesting event.
		// Note: First argument is ignored. The fourth is for exceptions.
		// And as written above the last is a timeout, hence we are blocked if nothing happens.
		int nfd;

		nfd = select(0, &waitRecv, &waitSend, NULL, timeout);
		if (nfd == SOCKET_ERROR)
		{
			cout << "Time Server: Error at select(): " << WSAGetLastError() << endl;
			WSACleanup();
			return;
		}

		for (int i = 1; i < MAX_SOCKETS; i++)		//remove sockets that past the timeout limit
		{
			char printDisconnectionTime[6];
			if (sockets[i].send == IDLE && sockets[i].recv != LISTEN)
			{
				time(&now);
				int lastTimeAction = now - sockets[i].lastUsed;
				if (lastTimeAction > 120)
				{
					time_t disconnection = sockets[i].lastUsed + 120;
					struct tm* time = localtime(&disconnection);
					cout << "Time Server: Client " << inet_ntoa(sockets[i].addr.sin_addr) << ":" << ntohs(sockets[i].addr.sin_port) << " has disconnected due to timeout.";
					strftime(printDisconnectionTime, 6, "%R", time);
					cout << "at " << printDisconnectionTime << "\n\n";
					closesocket(sockets[i].id);
					removeSocket(i);
				}
			}
		}

		for (int i = 0; i < MAX_SOCKETS && nfd > 0; i++)
		{
			if (FD_ISSET(sockets[i].id, &waitRecv))
			{
				nfd--;
				switch (sockets[i].recv)
				{
				case LISTEN:
					acceptConnection(i);
					break;
				case RECEIVE:
					receiveMessage(i);
					break;
				}
			}
		}

		for (int i = 0; i < MAX_SOCKETS && nfd > 0; i++)
		{
			if (FD_ISSET(sockets[i].id, &waitSend))
			{
				nfd--;
				switch (sockets[i].send)
				{
				case SEND:
					sendMessage(i);
					break;
				}
			}
		}
	}
	// Closing connections and Winsock.
	cout << "Time Server: Closing Connection.\n";
	closesocket(listenSocket);
	WSACleanup();
}

void acceptConnection(int idx)
{
	SOCKET id = sockets[idx].id;
	struct sockaddr_in from;		//The IP of sender.
	int fromLen = sizeof(from);

	SOCKET msgSocket = accept(id, (struct sockaddr*) & from, &fromLen);
	if (INVALID_SOCKET == msgSocket)
	{
		cout << "Time Server: Error at accept(): " << WSAGetLastError() << endl;
		return;
	}
	cout << "Time Server: Client " << inet_ntoa(from.sin_addr) << ":" << ntohs(from.sin_port) << " is connected." << endl;

	unsigned long flag = 1;	        //Set the socket to mode: non-blocking.

	if (ioctlsocket(msgSocket, FIONBIO, &flag) != 0)
	{
		cout << "Time Server: Error at ioctlsocket(): " << WSAGetLastError() << endl;
	}

	if (addSocket(msgSocket, from, RECEIVE) == false)
	{
		cout << "\t\tToo many connections, dropped!\n";
		closesocket(id);
	}
	return;
}

bool addSocket(SOCKET id, sockaddr_in io_addr, int what)
{
	for (int i = 0; i < MAX_SOCKETS; i++)
	{
		if (sockets[i].recv == EMPTY)
		{
			sockets[i].id = id;
			sockets[i].addr = io_addr;
			sockets[i].recv = what;
			sockets[i].send = IDLE;
			sockets[i].len = 0;
			sockets[i].lastUsed = time(0);
			socketsCount++;
			return (true);
		}
	}
	return (false);
}

void removeSocket(int idx)
{
	sockets[idx].recv = EMPTY;
	sockets[idx].send = EMPTY;
	socketsCount--;
}

void sendMessage(int idx)
{
	int bytesSent = 0;
	char sendBuff[1024];
	char tempBuff[1024];
	char* filename, * fileExtension;
	int len;
	char fileLenStr[50];
	SOCKET msgSocket = sockets[idx].id;
	sockets[idx].lastUsed = time(0);
	//******************************************************************* GET or HEAD action *******************************************************************//
	if (sockets[idx].sendSubType == GET || sockets[idx].sendSubType == HEAD)
	{
		filename = sockets[idx].buffer;
		filename = strtok(filename, " ");
		memmove(filename, filename + 1, strlen(filename));
		GET_Quwery(filename);
		FILE* file = fopen(filename, "r");                                   //for reading
		if (file == NULL)                                                    //in case file does not exsist, Error MSG CODE - 404
		{
			sprintf(sendBuff, "%s%s%s%s%s%s%s",
				NOT_FOUND_MSG_404, NEWLINE, "Content-Length: 0", NEWLINE, HTML_CONTENT_TYPE, NEWLINE, NEWLINE);
		}
		else
		{
			fseek(file, 0, SEEK_END);
			len = ftell(file);
			fseek(file, 0, SEEK_SET);
			int lines = 0;
			char enterCheck;
			while (!feof(file))
			{
				enterCheck = fgetc(file);
				if (enterCheck == '\n')
				{
					lines++;
				}
			}
			fseek(file, 0, SEEK_SET);
			len -= lines;
			_itoa(len, fileLenStr, 10);
			sprintf(sendBuff, "%s%s", OK_MSG_200, NEWLINE);                   //in case file found MSG CODE - 200
			sprintf(tempBuff, "%s%s%s", "Content-Length: ", fileLenStr, NEWLINE);
			strcat(sendBuff, tempBuff);

			fileExtension = GetFileType(filename, sendBuff);
			if ((strcmp(fileExtension, ".html") == 0))
			{
				sprintf(tempBuff, "%s%s%s", HTML_CONTENT_TYPE, NEWLINE, NEWLINE);
				strcat(sendBuff, tempBuff);
			}
			if (sockets[idx].sendSubType == GET)
			{
				while (fgets(tempBuff, 1024, file))
				{
					strcat(sendBuff, tempBuff);
				}
			}
			fclose(file);
		}
	}
	//******************************************************************* POST action *******************************************************************//
	else if (sockets[idx].sendSubType == POST)
	{
		char bodyBuff[1024];
		GetContentOfBody(sockets[idx].buffer, bodyBuff);
		cout << "Client POST: " << bodyBuff << endl;//we assume that the body is not empty.
		sprintf(sendBuff, "%s%s%s%s%s%s%s%s",
			OK_MSG_200, NEWLINE, NEWLINE, "Content-Length: 0", NEWLINE, HTML_CONTENT_TYPE, NEWLINE, NEWLINE);
	}
	else if (sockets[idx].sendSubType == OPTIONS)
	{
		sprintf(sendBuff, "%s%s%s%s%s%s%s%s",
			OK_MSG_200, NEWLINE, ALL_ALLOWED_QUWERIES, "Content-Length: 0", NEWLINE, HTML_CONTENT_TYPE, NEWLINE, NEWLINE);
	}
	//******************************************************************* PUT action *******************************************************************//
	else if (sockets[idx].sendSubType == PUT)
	{
		char CopyBuffer[1024];
		FILE* file = NULL;
		char doubleNewLine[50];
		bool isNotExist = false;
		memcpy(CopyBuffer, sockets[idx].buffer, 1024);
		filename = CopyBuffer;
		filename = strtok(filename, " ");
		memmove(filename, filename + 1, strlen(filename));
		file = fopen(filename, "r");
		if (file == NULL)
		{
			isNotExist = true;
		}
		else
		{
			fclose(file);
		}
		file = fopen(filename, "w");
		if (file == NULL)
		{
			sprintf(sendBuff, "%s %s%s%s%s%s%s",
				NOT_IMPLEMENTED_MSG_501, NEWLINE, "Content-Length: 0", NEWLINE, HTML_CONTENT_TYPE, NEWLINE, NEWLINE);
		}
		else
		{
			if (isNotExist)
			{
				sprintf(sendBuff, "%s%s%s%s%s%s%s",
					CREATED_MSG_201, NEWLINE, "Content-Length: 0", NEWLINE, HTML_CONTENT_TYPE, NEWLINE, NEWLINE);
			}
			else
			{
				sprintf(sendBuff, "%s%s%s%s%s%s%s",
					NO_CONTENT_MSG_204, NEWLINE, "Content-Length: 0", NEWLINE, HTML_CONTENT_TYPE, NEWLINE, NEWLINE);
			}
			string msgBody;
			msgBody = sockets[idx].buffer;
			sprintf(doubleNewLine, "%s%s", NEWLINE, NEWLINE);
			int startOfBodyidx = msgBody.find(doubleNewLine);
			fputs(&(msgBody[startOfBodyidx + 4]), file);
			fclose(file);
		}
	}
	//******************************************************************* DELETE action *******************************************************************//
	else if (sockets[idx].sendSubType == _DELETE)
	{
		filename = sockets[idx].buffer;
		filename = strtok(filename, " ");
		memmove(filename, filename + 1, strlen(filename));

		if (remove(filename) != 0)
		{
			sprintf(sendBuff, "%s%s%s%s%s%s%s", NO_CONTENT_MSG_204, NEWLINE, "Content-Length: 0",
				NEWLINE, HTML_CONTENT_TYPE, NEWLINE, NEWLINE);
		}
		else
		{
			sprintf(sendBuff, "%s%s%s%s%s%s%s", OK_MSG_200, NEWLINE, "Content-Length: 0", NEWLINE,
				HTML_CONTENT_TYPE, NEWLINE, NEWLINE);
		}
	}
	//******************************************************************* TRACE action *******************************************************************//
	else if (sockets[idx].sendSubType == TRACE)
	{
		filename = sockets[idx].buffer;
		int len = strlen(sockets[idx].buffer);
		char* lenStr = new char[1024];
		_itoa(len, lenStr, 10);
		sprintf(sendBuff, "%s%s%s%s%s%s%s%s", OK_MSG_200, NEWLINE, "Content-Length: ", lenStr,
			NEWLINE, HTML_CONTENT_TYPE, NEWLINE, NEWLINE);
		strcat(sendBuff, filename);
		strcat(sendBuff, NEWLINE);
	}

	bytesSent = send(msgSocket, sendBuff, (int)strlen(sendBuff), 0); //sned the answer

	if (SOCKET_ERROR == bytesSent)                                   //in case there is error with the socket
	{
		cout << "TCP Server: Error at send(): " << WSAGetLastError() << endl;
		return;
	}
	cout << "TCP Server: Sent: " << bytesSent << "\\" << strlen(sendBuff) << " bytes of \"" << sendBuff << "\" message.\n";
	sockets[idx].send = IDLE;
	sockets[idx].recv = RECEIVE;
	sockets[idx].buffer[0] = '\0';
	sockets[idx].len = 0;
}

void receiveMessage(int idx)
{
	SOCKET msgSocket = sockets[idx].id;

	int len = sockets[idx].len;
	int bytesRecv = recv(msgSocket, &sockets[idx].buffer[len], sizeof(sockets[idx].buffer) - len, 0);

	if (SOCKET_ERROR == bytesRecv)                                  //Check if there is socket error
	{
		cout << "Time Server: Error at recv(): " << WSAGetLastError() << endl;
		closesocket(msgSocket);
		removeSocket(idx);
		return;
	}
	if (bytesRecv == 0)                                             //in case nothing been sent
	{
		closesocket(msgSocket);
		removeSocket(idx);
		return;
	}
	else
	{
		sockets[idx].buffer[len + bytesRecv] = '\0';               //add the null-terminating to make it a string
		cout << "Time Server: Recieved: " << bytesRecv << " bytes of \"" << &sockets[idx].buffer[len] << "\" message.\n";

		sockets[idx].len += bytesRecv;

		if (sockets[idx].len > 0)                                  //in case there are requests pending for answer
		{
			if (strncmp(sockets[idx].buffer, "GET", 3) == 0)
			{
				sockets[idx].send = SEND;
				sockets[idx].sendSubType = GET;
				memcpy(sockets[idx].buffer, &sockets[idx].buffer[3], sockets[idx].len - 3);
				sockets[idx].len -= 3;
				sockets[idx].buffer[sockets[idx].len] = '\0';      //add the null-terminating to make it a string
				return;
			}
			if (strncmp(sockets[idx].buffer, "PUT", 3) == 0)
			{
				sockets[idx].send = SEND;
				sockets[idx].sendSubType = PUT;
				memcpy(sockets[idx].buffer, &sockets[idx].buffer[3], sockets[idx].len - 3);
				sockets[idx].len -= 3;
				sockets[idx].buffer[sockets[idx].len] = '\0';      //add the null-terminating to make it a string
				return;
			}
			if (strncmp(sockets[idx].buffer, "HEAD", 4) == 0)
			{
				sockets[idx].send = SEND;
				sockets[idx].sendSubType = HEAD;
				memcpy(sockets[idx].buffer, &sockets[idx].buffer[4], sockets[idx].len - 4);
				sockets[idx].len -= 4;
				sockets[idx].buffer[sockets[idx].len] = '\0';      //add the null-terminating to make it a string
				return;
			}
			if (strncmp(sockets[idx].buffer, "POST", 4) == 0)
			{
				sockets[idx].send = SEND;
				sockets[idx].sendSubType = POST;
				memcpy(sockets[idx].buffer, &sockets[idx].buffer[4], sockets[idx].len - 4);
				sockets[idx].len -= 4;
				sockets[idx].buffer[sockets[idx].len] = '\0';      //add the null-terminating to make it a string
				return;
			}
			if (strncmp(sockets[idx].buffer, "TRACE", 5) == 0)
			{
				sockets[idx].send = SEND;
				sockets[idx].sendSubType = TRACE;
				memcpy(sockets[idx].buffer, &sockets[idx].buffer[5], sockets[idx].len - 5);
				sockets[idx].len -= 5;
				sockets[idx].buffer[sockets[idx].len] = '\0';      //add the null-terminating to make it a string
				return;
			}
			if (strncmp(sockets[idx].buffer, "DELETE", 6) == 0)
			{
				sockets[idx].send = SEND;
				sockets[idx].sendSubType = _DELETE;
				memcpy(sockets[idx].buffer, &sockets[idx].buffer[6], sockets[idx].len - 6);
				sockets[idx].len -= 6;
				sockets[idx].buffer[sockets[idx].len] = '\0';      //add the null-terminating to make it a string
				return;
			}
			if (strncmp(sockets[idx].buffer, "OPTIONS", 7) == 0)
			{
				sockets[idx].send = SEND;
				sockets[idx].sendSubType = OPTIONS;
				memcpy(sockets[idx].buffer, &sockets[idx].buffer[7], sockets[idx].len - 7);
				sockets[idx].len -= 7;
				sockets[idx].buffer[sockets[idx].len] = '\0';      //add the null-terminating to make it a string
				return;
			}
			else if (strncmp(sockets[idx].buffer, "Exit", 4) == 0) //in case the client wants to exit the program
			{
				closesocket(msgSocket);
				removeSocket(idx);
				return;
			}
		}
	}
}

char* GetFileType(char* filename, char* sendBuff)
{
	char* type = NULL;
	for (int i = strlen(filename) - 1; i >= 0; i--)
	{
		if (filename[i] == '.')//find the '.' (dot) in the filename 
		{
			type = filename + i;
		}
	}
	return type;
}

void GET_Quwery(char* path)
{
	char tempStr[50] = "";                         //used in the procces of connecting filnename string and language string
	char resultOfQuery[50] = "";                   //init to empty string
	char filename[50] = "";                        //init to empty string
	int i = 0, j = 0;                              //indexs

	while (path[j] != ' ')
	{
		filename[j] = path[j];
		j++;
		if (path[j] == '?')                        //language type comes after the '?'
		{
			j++;
			break;
		}
	}
	while (path[j] != ' ')                         //Copy path to the result string
	{
		resultOfQuery[i] = path[j];
		i++;
		j++;
	}
	if (strcmp(resultOfQuery, "lang=he") == 0)     //in case the client requests the hebrew site
	{
		strcpy(tempStr, filename);
		strcpy(filename, (char*)"he\\");
		strcat(filename, tempStr);
	}
	else if (strcmp(resultOfQuery, "lang=en") == 0)//in case the client requests the english site
	{
		strcpy(tempStr, filename);
		strcpy(filename, (char*)"en\\");
		strcat(filename, tempStr);
	}
	else if (strcmp(resultOfQuery, "lang=fr") == 0)//in case the client requests the french site
	{
		strcpy(tempStr, filename);
		strcpy(filename, (char*)"fr\\");
		strcat(filename, tempStr);
	}
	else                                          //else, use Default language - hebrew
	{
		strcpy(tempStr, filename);
		strcpy(filename, (char*)"he\\");
		strcat(filename, tempStr);
	}
	strcat(filename, ".html");                   //add the .html file extension
	strcpy(path, filename);
}

void GetContentOfBody(char* recvBuffer, char* bodyBuffer)
{
	int idx = 0;
	int resultidx = -1;
	char TwoNewLines[4] = { '\r','\n','\r','\n' };
	printf("%s \n", recvBuffer);
	if (strlen(recvBuffer) < 4)
	{
		cout << "Error at recvBuffer.\n";
	}
	while (recvBuffer[idx + 4] != '\0')
	{
		if (strncmp(recvBuffer + idx, TwoNewLines, 4) == 0)
		{
			resultidx = idx;
			break;
		}
		idx++;
	}
	if (resultidx != -1)
		strcpy(bodyBuffer, recvBuffer + idx + 4);
}
