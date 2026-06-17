CRITICAL - MUST DO:
1. Request Validation & Error Handling ⚠️
	Currently, your parsing detects errors but doesn't return proper HTTP error codes. You need to implement:

	* 400 Bad Request: When request line format is invalid, malformed headers
	* 411 Length Required: POST requests without Content-Length header
	* 413 Payload Too Large: Body exceeds MAX_BODY_SIZE (you check it but don't return error)
	* 414 URI Too Long: URI exceeds MAX_URI_LENGTH (you check it but don't return error)
	* 415 Unsupported Media Type: Unsupported Content-Type
	* 501 Not Implemented: Unknown HTTP methods (not GET/POST/DELETE)
	* 505 HTTP Version Not Supported: HTTP version validation
	
	Status: HttpStatus has phrases defined, but HttpRequest.cpp sets _state = PARSING_ERROR without generating proper error responses.

2. HTTP Version Validation ⚠️
	* Currently parsing version but not validating it
	* Need to check if version is "HTTP/1.1" or supported version
	* Return 505 if unsupported

3. Host Header Validation ⚠️
	Required by HTTP/1.1 spec:

	* Check if Host header exists
	* Return 400 Bad Request if missing

4. Better Error Response Generation ⚠️
	* ResponseBuilder.cpp only handles successful responses
	* Need method to generate HTML error pages for status codes
	* Should format error responses with proper status codes
	* IMPORTANT - Should Do:

5. HTTP Header Handling Improvements
	* Duplicate headers: Currently overwrites, should merge values (e.g., multiple Cookie headers)
	* Multi-line header values: Handle headers that span multiple lines (deprecated but spec-compliant)
	* Connection header: Partially implemented, but keep-alive is commented out in server.cpp:200-208

6. Content-Length Validation for POST
	In HttpRequest.cpp:248-251, you have:

	But you need to return 411 Length Required if POST/PUT requests don't have Content-Length.

7. Connection Management - Keep-Alive
	* Keep-alive parsing exists in HttpRequest.cpp:132-141
	* But it's commented out in server.cpp:197-208
	* Need to uncomment and test connection reuse

* RECOMMENDED Order of Implementation:
	* First: Add error response generation to ResponseBuilder
		Create method: std::string buildErrorResponse(int statusCode, std::string phrase)

	* Second: Update HttpRequest parsing to return specific error codes
		Modify parseRequest() to return status codes instead of just bool
		Check HTTP version, Host header, Content-Length requirements
	
	* Third: Implement Host header validation
		Add validation after parsing headers
	
	* Fourth: Enable keep-alive connections
		Uncomment in server.cpp and test
	* Fifth: Add Content-Type validation (415 error)