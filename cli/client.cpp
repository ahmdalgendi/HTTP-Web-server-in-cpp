#define _CRT_SECURE_NO_WARNINGS
#include <winsock.h>
#include <stdio.h>
#include <iostream>
#include <string.h>
#include <windows.h>
#include <complex>
#include <ctime>
#include <chrono>
using namespace std;
#define HOSTNAME_LENGTH 20
#define RESP_LENGTH 40
#define WEBPAGE_LENGTH 20
#define REQUEST_PORT 5001
#define BUFFER_LENGTH 1024
#define TRACE 0
#define MSGHDRSIZE 8 //Message Header Size
#define TIME_LENGTH 100
#define NUMBER_OF_PARAMETER 5

typedef enum
{
	REQ = 1,
	RESP //Message type
} Type;

typedef enum
{
	GET = 1,
	POST //Message type
} HTTP_METHOD;


typedef struct
{
	double http_version;
	HTTP_METHOD http_method;
	char serverHostName[HOSTNAME_LENGTH]; // only used in http 1.1 
	char hostname[HOSTNAME_LENGTH];
	char webPageName[WEBPAGE_LENGTH];
	char timeStamp[TIME_LENGTH];
} Req; //request

typedef struct
{
	int status_code;
	double http_version;
	char timeStamp[TIME_LENGTH];
} Resp; //response


typedef struct
{
	Type type;
	int length; //length of effective bytes in the buffer
	char buffer[BUFFER_LENGTH];
} Msg; //message format used for sending and receiving


class TcpClient
{
	int sock; /* Socket descriptor */
	struct sockaddr_in ServAddr; /* server socket address */
	unsigned short ServPort; /* server port */
	Req* reqp; /* pointer to request */
	Resp* respp; /* pointer to response*/
	Msg smsg, rmsg; /* receive_message and send_message */
	WSADATA wsadata;
public:
	TcpClient()
	{
	}
	void run();
	~TcpClient();
	int msg_recv(int, Msg*);
	int msg_send(int, Msg*);
	unsigned long ResolveName(char name[]);
	void err_sys(const char* fmt,...);
};

void TcpClient::run()
{
	char argv[NUMBER_OF_PARAMETER][WEBPAGE_LENGTH];
	char method[HOSTNAME_LENGTH];
	struct _stat stat_buf;

	//initilize winsocket
	if (WSAStartup(0x0202, &wsadata) != 0)
	{
		WSACleanup();
		err_sys("Error in starting WSAStartup()\n");
	}


	reqp = (Req *)smsg.buffer;

	//Display name of local host and copy it to the req
	if (gethostname(reqp->hostname,HOSTNAME_LENGTH) != 0) //get the hostname
		err_sys("can not get the host name,program exit");
	printf("%s%s\n", "Client starting at host:", reqp->hostname);
	cout << "Type name of web server:";
	scanf("%s", argv[1]); // read server host name
	cout << "Requesting web page..\n";
	cout << "HTTP version:";
	scanf("%lf", &(reqp->http_version));
	cout << "Method:";
	scanf("%s", method);

	if (strcmp(method, "GET") == 0)
		reqp->http_method = GET;
	else if (strcmp(argv[3], "POST") == 0)
		reqp->http_method = POST;
	else err_sys("Wrong Method type\n");
	smsg.type = REQ;
	cout << "Webpage Name:";
	//read webpage name
	scanf("%s", argv[2]);
	strcpy(reqp->webPageName, argv[2]);


	memset(reqp->timeStamp, 0, sizeof(reqp->timeStamp));
	auto end = std::chrono::system_clock::now();
	std::time_t end_time = std::chrono::system_clock::to_time_t(end);
	sprintf(reqp->timeStamp, "%s", std::ctime(&end_time));


	if(reqp->http_version == 1.1) // set the sever host name only when the version is  1.1
		strcpy(reqp->serverHostName, argv[1]);
	else memset(reqp->serverHostName, 0, sizeof(reqp->serverHostName));

	puts("Webpage requested, waiting for web server to respond... ");
	//Create the socket
	if ((sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) //create the socket 
		err_sys("Socket Creating Error");

	//connect to the server
	ServPort = REQUEST_PORT;
	memset(&ServAddr, 0, sizeof(ServAddr)); /* Zero out structure */
	ServAddr.sin_family = AF_INET; /* Internet address family */
	ServAddr.sin_addr.s_addr = ResolveName(argv[1]); /* Server IP address */
	ServAddr.sin_port = htons(ServPort); /* Server port */
	if (connect(sock, (struct sockaddr *)&ServAddr, sizeof(ServAddr)) < 0)
		err_sys("Socket Creating Error");

	//send out the message
	smsg.length = sizeof(Req);
	fprintf(stdout, "Send reqest to %s\n", argv[1]);
	if (msg_send(sock, &smsg) != sizeof(Req))
		err_sys("Sending req packet error.,exit");

	//receive the response
	if (msg_recv(sock, &rmsg) != rmsg.length)
		err_sys("recv response error,exit");

	//cast it to the response structure
	respp = (Resp *)rmsg.buffer;
	printf("HTTP response to your request is  %s, response received at %s \n\n\n",
	       respp->status_code == 200 ? "[200 OK]" : "[501 Error]", respp->timeStamp);

	//close the client socket
	closesocket(sock);
}
TcpClient::~TcpClient()
{
	/* When done uninstall winsock.dll (WSACleanup()) and exit */
	WSACleanup();
}


void TcpClient::err_sys(const char* fmt,...) //from Richard Stevens's source code
{
	perror(NULL);
	va_list args;
	va_start(args,fmt);
	fprintf(stderr, "error: ");
	vfprintf(stderr, fmt, args);
	fprintf(stderr, "\n");
	va_end(args);
	exit(1);
}

unsigned long TcpClient::ResolveName(char name[])
{
	struct hostent* host; /* Structure containing host information */

	if ((host = gethostbyname(name)) == NULL)
		err_sys("gethostbyname() failed");

	/* Return the binary, network byte ordered address */
	return *((unsigned long *)host->h_addr_list[0]);
}

/*
msg_recv returns the length of bytes in the msg_ptr->buffer,which have been recevied successfully.
*/
int TcpClient::msg_recv(int sock, Msg* msg_ptr)
{
	int rbytes, n;

	for (rbytes = 0; rbytes < MSGHDRSIZE; rbytes += n)
		if ((n = recv(sock, (char *)msg_ptr + rbytes,MSGHDRSIZE - rbytes, 0)) <= 0)
			err_sys("Recv MSGHDR Error");

	for (rbytes = 0; rbytes < msg_ptr->length; rbytes += n)
		if ((n = recv(sock, (char *)msg_ptr->buffer + rbytes, msg_ptr->length - rbytes, 0)) <= 0)
			err_sys("Recevier Buffer Error");

	return msg_ptr->length;
}

/* msg_send returns the length of bytes in msg_ptr->buffer,which have been sent out successfully
 */
int TcpClient::msg_send(int sock, Msg* msg_ptr)
{
	int n;
	if ((n = send(sock, (char *)msg_ptr,MSGHDRSIZE + msg_ptr->length, 0)) != (MSGHDRSIZE + msg_ptr->length))
		err_sys("Send MSGHDRSIZE+length Error");
	return (n - MSGHDRSIZE);
}

int main(int argc, char* argv[]) 
{
	TcpClient* tc = new TcpClient();
	tc->run();
	system("pause");
	return 0;
}
