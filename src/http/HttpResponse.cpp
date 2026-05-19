#include "HttpResponse.hpp"

HttpResponse::HttpResponse() : method(NULL) {}
HttpResponse::~HttpResponse()
{
	if (method)
		delete method;
}

std::string getHttpDate()
{
	time_t now = time(0);
	struct tm tm = *gmtime(&now);
	char buf[100];

	strftime(buf, sizeof(buf), "%a, %d %b %Y %H:%M:%S GMT", &tm);
	return std::string(buf);
}

/*
check which AMethod is requestet and execute the AMethod,
then retrieve the status-code and reasonphrase for the status-line
of the response
*/
HttpResponse::HttpResponse(HttpRequest *request) : 
	_reqLine(request->getRequestLine()),
	_reqHeaders(request->getRequestHeaders()),
	_reqBody(request->getRequestBody()),
	method(NULL)
{
	std::cout << "\33[36m" << "_____________________\nHTTP_RESPONSE building..." << std::endl;

	if (_reqLine.method == "GET" || _reqLine.method == "POST")
	{
		if (_reqLine.method == "GET")
			method = createGet(_reqLine.method);
		else if (_reqLine.method == "POST")
			method = createPost(_reqLine.method);
		method->setResource(_reqLine.requestURI, _reqHeaders["Host"][0]);
		method->setHeaders(_reqHeaders);
		method->setBody(_reqBody);
		method->execute();

		status.httpVersion = _reqLine.version;
		status.statusCode = method->getCode();
		status.reasonPhrase = method->getPhrase();

		messageBody = method->getBody();
		setContentLength();
		_resHeads.contentType = method->getContentType();
		_resHeads.Date = getHttpDate();
	}
	else
	{
		// Unsupported AMethod
		std::cout << "Unsupported AMethod: " << _reqLine.method << std::endl;
		status.httpVersion = _reqLine.version;
		status.statusCode = "405";
		status.reasonPhrase = "AMethod Not Allowed";
		messageBody = "<html><body><h1>405 AMethod Not Allowed</h1></body></html>";
		_resHeads.contentType = "text/html";
		_resHeads.contentLength = "";
		setContentLength();
		_resHeads.Date = getHttpDate();
	}

	buildStatusLine();
	buildHeaders();
}

void HttpResponse::buildResponse(HttpRequest *request)
{
	std::cout << "build respone...:" << std::endl;
	_reqLine = request->getRequestLine();
	_reqHeaders = request->getRequestHeaders();
	
	if (_reqLine.method == "GET")
	{
		method = createGet("Gett");
		method->setResource(_reqLine.requestURI, _reqHeaders["Host"][0]);
		method->setHeaders(_reqHeaders);
		method->execute();

		status.httpVersion = _reqLine.version;
		status.statusCode = method->getCode();
		status.reasonPhrase = method->getPhrase();

		messageBody = method->getBody();
		setContentLength();
		_resHeads.contentType = method->getContentType();
		_resHeads.Date = getHttpDate();
	}
	else
	{
		// Unsupported AMethod
		status.httpVersion = _reqLine.version;
		status.statusCode = "405";
		status.reasonPhrase = "AMethod Not Allowed";
		messageBody = "<html><body><h1>405 AMethod Not Allowed</h1></body></html>";
		_resHeads.contentType = "text/html";
		setContentLength();
		_resHeads.Date = getHttpDate();
	}

	buildStatusLine();
	buildHeaders();
}

AMethod *HttpResponse::createGet(std::string name)
{
	return new Get(name);
}

AMethod *HttpResponse::createPost(std::string name)
{
	return new Post(name);
}

bool	HttpResponse::buildStatusLine()
{
	status.httpVersion = _reqLine.version;
	status.statusCode = method->getCode();
	status.reasonPhrase = method->getPhrase();
	statusLine = status.httpVersion  + " " + status.statusCode + " " + status.reasonPhrase + "\r\n";
	// std::cout << statusLine << std::cout;
	return true;
}

std::string &HttpResponse::getStatusLine()
{
	return statusLine;
}

std::string &HttpResponse::getMessageBody()
{
	return messageBody;
}

void HttpResponse::setContentLength()
{
	std::stringstream	ss;
	ss << messageBody.size();
	_resHeads.contentLength = ss.str();
}

// builds the headers for the Response
// should be more dynamic not hardcoded
void HttpResponse::buildHeaders()
{
	messageHeaders = "Content-Type: " + _resHeads.contentType + "\r\n";
	messageHeaders += "Content-Length: " + _resHeads.contentLength + "\r\n";
	messageHeaders += "Date: " + _resHeads.Date + "\r\n";
	messageHeaders += "Server: webserv/1.0\r\n";
	messageHeaders += "Connection: keep-alive\r\n";
	messageHeaders += "\r\n";
}

std::string &HttpResponse::getMessageHeaders()
{
	return messageHeaders;
}