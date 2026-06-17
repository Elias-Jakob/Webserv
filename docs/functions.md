1. getaddrinfo()
	- int getaddrinfo(const char *node, const char *service,  const struct addrinfo *hints, struct addrinfo **res);
		* const char *node: identify an host.
		* const char *service: identify a service.
		* const struct addrinfo *hints: points to an addrinfo to specify criteria for selecting the addrinfo returned in list by "res".
		* struct addrinfo **res: 
		- struct addrinfo {
			int					ai_flags;
			int					ai_family;
			int					ai_socktype;
			int					ai_protocol;
			socklen_t			ai_addrlen;
			struct sockaddr *	ai_addr;
			char				ai_canonname;
			stuct addrinfo *	ai_next;
			}
		+ DESCRIPTION: node and service identify an Internet host and service. getaddrinfo() returns one or more addrinfo structures,
				which contains an Internet address that can be specified by bind() or connect().
		+ RETURN: Success -> 0
					Fail -> EAI Error Codes (EAI_ADDRFAMILY, EAI_AGAIN, EAI_BADFLAGS, ...).

2. socket()
	- int socket(int domain, int type, int protocol);
		* domain:	specifies communication domain, in which a socket is to be created. 
						Address family used in communication domain -> implementation defined by system.
		* type:		specifies type of connection to be created. (SOCK_STREAM, SOCK_DGRAM, SOCK_SEQPACKET)/
		* protocol:	specifies a particular protocol to be used with socket. protocol = 0 => unspecified default protocol appropiate for
						requested socket type. 
		+ DESCRIPTION:
		+ RETURN: Success -> non negative integer.
					Fail -> -1 and errno.

	- SOCKET(Kommunikationsendpunkt) // explanation
		* EXAMPLE: To send a letter to a guest in an hotel. You need an address of the Hotel(IP address) 
			and the room number of the guest(Port).
		* BUILD OF SOCKET: Socket = IP-Address + Port.
			IP-Address(Hotel): Identifies Computer inside Network.
			- Port(room-nr): identifies the concrete programm on this computer(z.B. Port 443/80 for webbrowser || Port 22 for SSH).
		* TYPES OF SOCKETS
			1. TCP SOCKET
				* Functions:	Like a phonecall, connection is built using 3-Way-Handshake.
				* Attributes:	Reliable, checks if all data pakets got received in correct order. If something got lost -> sent again.
				* Usecases:		websites(HTTP/HTTPS), E-Mails(IMAP/SMTP), Filetransfer(FTP).
			2. UDP SOCKET(Datagramm-Sockets)
				* Funcitons:	Like Postcards, just sends data into Network without knowing if Receiver is there and
									if it got received.
				* Attributes:	Very fast. No connection building. No garantie if data got received or if the order is correct.
				* Usecases:		Live-Video-Streaming, Online-Games.
		* SOCKET-CONNECTION (TCP)
			1. Creating socket	-> Server creates a socket.
			2. Bind				-> Server binds socket to specific IP-address and Port.
			3. Listen			-> Server waits if clients knocks.
			4. Connect			-> Client creates own socket and knocks at Server.
			5. Accept			-> Server accepts the connection.
			6. Send & Receive	-> Data flows in both directions.
			7. Close			-> Rebuilt connection.

3. setsockopt()
	- 

4. bind()
	- int bind(int socket, const struct sockaddr *address, socklen_t address_len);
		* socket:		file descriptor of socket.
		* address:		points so sockaddr structure containing address to be bound to socket.
		* address_len:	length of sockaddr structure pointed to be address arg.
	+ DESCRIPTION: Shall assign socket address to a socket. Sockets created with 
					socket() -> unnamed, identified by address family.
	+ RETURN:	Success -> 0.
				Fail -> -1 and errno.

5. listen()
	- int listen(int socket, int backlog);
		* socket:	file descriptor of socket.
		* backlog:	limit number of outstanding connections in socket's listen queue.
	+ DESCRIPTION: Mark a connection-mode Socket as accepting connections.
	+ RETURN:	Success -> 0.
				Fail	-> -1 & errno.

6. fcntl()
	- int fcntl(int fildes, int cmd, ...);
		* fildes:	fildeskriptor.
		* cmd:		different operations to be perfomed on filedeskriptor. (F_DUPFD, F_GETFD, F_SETFL, O_NONBLOCKING)
	+ DESCRIPTION:
		Performs operations defined by cmd on fildes.
	+ RETURN:
		* Success	->	positive int (depend on cmd)
		* Fail		->	-1 && errno.

7. poll()
	- int poll(struct pollfds fds[], nfds_t nfds, int timeout);
		* fds:		filedeskriptors to be examined & events of interest for each fd
		* nfds:		nbr of elements in fds.
		* timeout:	how long poll should wait till returning. -1 -> infinity. 0 -> imediatly.
	+ DESCRIPTION:
		* recv() && accept() are blocking! -> programm would stay in line until something happens. If Server waits for data
			of Client A, Server would not notice Client B already sends Data.
		* poll() not blocking! -> Has list of sockets && tells u if something happens on any of them.
			- struct pollfd {
				int fd;			// socket to monitor.
				int events;		// what to test for. (Input by programmer).
				int revents;	// what happend (Output by OS).
				}
			- EVENT-FLAGS
				*	POLLIN	->	data to read available. // Client sent text || Client knocked for accept().
				*	POLLOUT	->	socket ready to sent data // (without blocking).
				*	POLLERR/
					POLLHUP	->	Error happend.
	+ RETURN:
		* Success	->	non-negative int (indicates nbr of pollfd structures with selected events.)
		* Fail		->	timeout -> 0; failure -> -1;

8. accept()
	- int accept(int socket, struct sockaddr *restrict address, socklen_t *restrict address_len);
		* socket:		fd.
		* address:		Pointer to sockaddr where address of connecting socket shall be returned || NULL
		* address_len:	len of supplied sockaddr *address, on output specifies the length of stored address.
	+ DESCRIPTION:
		* extract first connection of pending connections queue, create new socket with same socket type protocol and address family
			as specified socket and allocate new fd.
	+ RETURN:
		- Success -> fd of accepted socket.
		- Fail -> -1 && errno. address shall remained unchanged.

9. recv()

10. send()