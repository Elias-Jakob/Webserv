#include "SessionManager.hpp"
// =========================================================================
// Constructors & Destructor
// =========================================================================
SessionManager::SessionManager()
{}

SessionManager::~SessionManager()
{}

// =========================================================================
// Public Methods
// =========================================================================

std::string	SessionManager::createNewSession()
{
	std::string	sessionID = generateSessionId();
	addCookieToSessions(sessionID);
	return sessionID;
}

bool	SessionManager::isValidCookie(const std::string &sessionId)
{
	std::map<std::string, t_SessionData>::iterator it;
	it = _sessions.find(sessionId);
	if (it == _sessions.end())
		return false;
	return true;
}

// GETTERS
size_t	SessionManager::sessionsSize()
{
	return _sessions.size();
}

std::string	SessionManager::createSessionHeaderForResponse(const std::string &sessionId)
{
	std::string sessionHeader;
	// look up _session to get the data?
	sessionHeader = "Set-Cookie: session_id=" + sessionId + "; Path=/; Max-Age=30\r\n";
	return sessionHeader;
}

// =========================================================================
// Private Helper Methods
// =========================================================================

std::string	SessionManager::extractCookie(std::vector<std::string> cookieVal)
{
	std::string sessionStr = cookieVal[0];
	size_t	posStart = sessionStr.find('=');
	size_t	posEnd = sessionStr.find(' ');
	if (posStart != std::string::npos) {
		if (posEnd == std::string::npos)
			posEnd = sessionStr.size();
		std::string sessionId = sessionStr.substr(posStart + 1, posEnd - posStart);
		return sessionId;
	}
	return "";
}

std::string	SessionManager::generateSessionId()
{
	std::string	randSessionId;

	int randNbr = rand();
	std::stringstream ss;
	ss << randNbr;
	ss >> randSessionId;
	
	return randSessionId;
}

bool	SessionManager::addCookieToSessions(const std::string &newCookie)
{
	t_SessionData sessionData;
	sessionData.isLoggedIn = false;
	sessionData.name = "";

	std::map<std::string, t_SessionData>::iterator it = _sessions.find(newCookie);
	if (it == _sessions.end()) {
		_sessions[newCookie] = sessionData;
		std::cout << "added new Session..." << std::endl;
		return true;
	}
	return false;
}