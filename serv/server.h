#ifndef SER_TCP_H
#define SER_TCP_H

#define HOSTNAME_LENGTH 20
#define RESP_LENGTH 40
#define WEBPAGE_LENGTH 20
#define REQUEST_PORT 5001
#define BUFFER_LENGTH 1024 
#define MAXPENDING 10
#define MSGHDRSIZE 8 //Message Header Size
#define TIME_LENGTH 100

typedef enum {
	REQ = 1,
	RESP //Message type
} Type;

typedef enum {
	GET = 1,
	POST //Message type
} HTTP_METHOD;



typedef struct
{
	double http_version;
	HTTP_METHOD http_method;
	char hostname[HOSTNAME_LENGTH];
	char serverHostName[HOSTNAME_LENGTH]; // only used in http 1.1 
	char webPageName[WEBPAGE_LENGTH];
	char timeStamp[TIME_LENGTH];
} Req;  //request

typedef struct
{
	int status_code;
	double http_version;
	char timeStamp[TIME_LENGTH];
} Resp; //response


typedef struct
{
	Type type;
	int  length; //length of effective bytes in the buffer
	char buffer[BUFFER_LENGTH];
} Msg; //message format used for sending and receiving


class TcpServer
{
	int serverSock, clientSock;     /* Socket descriptor for server and client*/
	struct sockaddr_in ClientAddr; /* Client address */
	struct sockaddr_in ServerAddr; /* Server address */
	unsigned short ServerPort;     /* Server port */
	int clientLen;            /* Length of Server address data structure */
	char servername[HOSTNAME_LENGTH];

public:
	TcpServer();
	~TcpServer();
	void start();
};

class TcpThread :public Thread
{

	int cs;
public:
	TcpThread(int clientsocket) :cs(clientsocket)
	{}
	virtual void run();
	int msg_recv(int, Msg *);
	int msg_send(int, Msg *);
	unsigned long ResolveName(char name[]);
	static void err_sys(const char * fmt, ...);
};

#endif