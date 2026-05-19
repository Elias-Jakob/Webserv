#include "HttpResponse.hpp"

HttpResponse::HttpResponse() : method(NULL)
{}

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
	* check if requested Method is implemented, to create AMethod * => (Post || Get || Delete).
	* if requested Method is "POST", check "Content-Type" to create ABodyParser * => (Multipart || Form).
*/
HttpResponse::HttpResponse(HttpRequest *request) : 
	_reqLine(request->getRequestLine()),
	_reqHeaders(request->getRequestHeaders()),
	_reqBody(request->getRequestBody()),
	method(NULL),
	_parser(NULL)
{
	std::cout << "\33[36m" << "_____________________\nHTTP_RESPONSE building..." << std::endl;
	if (_reqLine.method == "GET" || _reqLine.method == "POST")
	{
		if (_reqLine.method == "GET")
			method = createGet(_reqLine.method);
		else if (_reqLine.method == "POST")
		{
			createBodyParser();
			_parser->parse(_reqBody);
			_parsedResult = _parser->getResult();
			if (_parsedResult.size() > 0)
				std::cout << "_parsedResult returned something..." << std::endl;
			method = createPost(_reqLine.method);
		}
		method->setResource(_reqLine.requestURI, _reqHeaders["Host"][0]);
		method->setHeaders(_reqHeaders);
		method->setBody(_parsedResult);
		method->setContentData(_contentData);
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

std::string HttpResponse::parseContentType(std::vector<std::string> value)
{
	std::cout << "parsing contentType..." << std::endl;
	std::string	temp;
	std::string	parameter;
	std::string type;
	size_t posSemiColon = 0;
	size_t posSlash = 0;

	temp = value.at(0);
	if ((posSlash = temp.find('/', 0)) < temp.size())
	{
		std::cout << "\tfound type/subtype..." << std::endl;
		_contentData.type = temp.substr(0, posSlash);
		if ((posSemiColon = temp.find(';', posSlash)) < temp.size())
		{
			std::cout << "\tfound parameter..." << std::endl;
			parameter = temp.substr(posSemiColon, temp.size() - posSemiColon);
			_contentData.subtype = temp.substr(posSlash + 1, posSemiColon - posSlash);
			size_t posEqual = 0;
			if ((posEqual = parameter.find("=", 0)) < parameter.size())
			{
				std::cout << "\tfound parameter-value..." << std::endl;
				_contentData.boundary = parameter.substr(posEqual + 1, parameter.size() - posEqual);
			}
		}
		else
		{
			std::cout << "\tno parameter found..." << std::endl;
			_contentData.subtype = temp.substr(posSlash + 1, temp.size() - posSlash);
		}
		type = temp.substr(0, posSemiColon);
		parameter = temp.substr(posSemiColon + 1, temp.size());
		std::cout << "s_ContentData = {" << std::endl;
		std::cout << "\ttype: \"" << _contentData.type << "\"" << std::endl;
		std::cout << "\tsubtype: \"" << _contentData.subtype << "\"" << std::endl;
		std::cout << "\tboundary: \"" << _contentData.boundary << "\"\n}" << std::endl;
	}
	return type;
}

AMethod *HttpResponse::createGet(std::string name)
{
	return new Get(name);
}

AMethod *HttpResponse::createPost(std::string name)
{
	return new Post(name);
}

/**
	*creates A ABodyParser based on the "Content-Type" request-header.
**/
bool HttpResponse::createBodyParser()
{
	parseContentType(_reqHeaders["Content-Type"]);
	if (_contentData.type == "multipart")
	{
		std::cout << "Want to create MultipartParser" << std::endl;
		_parser = createMultiParser();
	}
	else if (_contentData.type == "application")
	{
		std::cout << "Want to create FormParser" << std::endl;
		_parser = createFormParser();
	}
	return true;
}

ABodyParser *HttpResponse::createMultiParser()
{
	return new MultipartParser();
}

ABodyParser *HttpResponse::createFormParser()
{
	return new FormParser();
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