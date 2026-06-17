1. HTTP-Request-Limits
	* MAX_BODY_SIZE
		- Description: limit for content after headers.
		- Why: 100 GB file upload would break server, cause buffered in RAM.
		- Standard: nginx -> 1MB - 10MB (for file upload sometimes higher). Over Limit => 413 Content too large
	* MAX_HEADER_SIZE
		- Description: Limit for sum of all HTTP-header || max length of single header-value line.
		- Why: Attackers could send infinit long headers -> Not enough Space while searching for \r\n\r\n.
		- Standard: 4KB - 8KB. Over Limit => 431 Request Header Fields Too Large.
	* MAX_URI_LENGTH
		- Description: Limit for length of URL (GET /index.html....).
		- Why: safety for buffer overflow in URI-parsing.
		- Standard: 2KB - 8KB. Over Limit => 414 URI Too Long.

2. NETWORK && CONNECTION limits (poll)
	* MAX_CONNECTIONS
		- Description: limit for sockets, monitored by poll().
		- Why: OS has a limit for open fd's per process (ulimit -n).
		- Standard: Often 512 - 1024 for simple servers.
	* TIMEOUTS:
		* KEEP_ALIVE_TIMEOUT: How long socket stays open without
								getting request of Client.
		* READ_TIMEOUT: How long Client can take to finish sending request.
		* WRITE_TIMEOUT: How long Server waits, while sending data, the client slowly accepts these data.

3. RESOURCE & PERFORMANCE LIMITS
	* MAX_FILE_UPLOADS_PER_REQUEST
		- Description: Limit for number of files, that can be uploader (multipart/form-data)
						per request.
		- Why: To avoid 10 000 small file uploads per request, cause it would result in 10 000 parallel
				file operations.
	* CGI- /PROCESS_TIMEOUT
		- Description: If executing CGI-Script via fork(), need to monitor how long it runs.
		- Why: Script with infinite-loop (while true). -> Zombie-Process would take CPU-Resources forever.
															zb. SIGILL child-process after 30s.

* IMPLEMENTATION
	1. At accept(): if current_connections >= MAX_CONNECTIONS -> close new socket.
	2. At read(): If header-read_bytes > MAX_HEADER_SIZE and !\r\n\r\n -> break && send Code: "431"
	3. At body collecting: if parsing Content-Length header,
							check if content_length >= MAX_BODY_SIZE -> break, not read into RAM 
																		&& send Code: "413"