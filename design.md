ClientConnection(Controller/Orchestrator)

* 4 Components:
	* HttpRequest
		- Remains mostly unchanged
		- Pure data holder + parse
		- Responsibility: Parse raw HTTP request into structured data
		- No business logic, no response generation
	* RequestProcessor (NEW)
		- Validates the request
		- Creates the appropiate body parser (multipart/form)
		- Parses the request body if needed
		- Returns validation results + parsed data
		- Responsibiliies:
			* ABodyParser *createParser(ContentData &contentData);
			* bool validateMethod(const std::string &method);
			* bool validateHeaders(const Headers& headers);
			* ParsedBody parseBody(HttpRequest *req);

	* MethodExecuter (NEW)
		- Creates the appropiate method (GET/POST/DELETE) via factory
		- Executes the method
		- Returns execution results (status, body, headers)
		- Responsibilities:
			* AMethod* createMethod(const std::string& methodName);
			* ExecutionResult execute(HttpRequest* req, ParsedBody& body);
	
	* ResponseBuilder (NEW)
		- Takes execution results and formats the HTTP response
		- Builds status line, headers, body
		- Handles error responses(405, 500, etc)
		- Responsibilities:
			* std::string buildStatusLine(int code, std::string phrase);
			* std::string buildHeaders(const ResponseData &data);
			* std::string formatResponse(ExecutionResult &result);

* MethodExecuter
	* buildAndExecuteMethod(HttpResponse *response);
	* createMethod(std::string method);
	* t_executionResult method::execute();

* 