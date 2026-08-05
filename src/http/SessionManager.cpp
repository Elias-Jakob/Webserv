#include "SessionManager.hpp"
// =========================================================================
// Constructors & Destructor
// =========================================================================
SessionManager::SessionManager() :
	_flags("HttpOnly"),
	_maxAge(30)
{
	_lastSweep = time(NULL);
}

SessionManager::~SessionManager()
{}

// =========================================================================
// Public Methods
// =========================================================================

std::string	SessionManager::createNewSession()
{
	std::string	sessionID;
	const int	maxAttempts = 10;

	for (int attempts = 0; attempts < maxAttempts; attempts++) {
		sessionID = generateSessionId();
		if (addCookieToSessions(sessionID))
			return sessionID;
	}
	return "";
}

bool	SessionManager::isValidCookie(const std::string &sessionId)
{
	std::map<std::string, t_SessionData>::iterator it;
	it = _sessions.find(sessionId);
	if (it == _sessions.end())
		return false;
	time_t now = time(NULL);

	if (now > it->second.createdAt + _maxAge) {
		return false;
	}
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
	std::stringstream	maxAgeVal;
	maxAgeVal << _maxAge;
	std::string maxAge = "Max-Age=" + maxAgeVal.str();
	sessionHeader = "Set-Cookie: session_id=" + sessionId + "; Path=/; " + maxAge + "; " + _flags + "\r\n";// HttpOnly\r\n";
	return sessionHeader;
}

/**
 * @brief checks if it is time to remove cookies. Then removes expired cookies.
 */
void	SessionManager::removeExpiredSessions()
{
	time_t	now = time(NULL);
	if (now < _lastSweep + SWEEP_INTERVAL) {
		return ;
	}
	_lastSweep = now;
	std::map<std::string, t_SessionData>::iterator it = _sessions.begin();
	std::map<std::string, t_SessionData>::iterator itEnd = _sessions.end();
	while (it != itEnd) {
		if (now > it->second.createdAt + _maxAge) {
			_sessions.erase(it++);
		}
		else {
			++it;
		}
	}
}

/**
 * @brief deletes the given session-cookie frome _sessions.
 */
void	SessionManager::deleteSession(const std::string &sessionCookie)
{
	if (sessionCookie.empty())
		return ;

	std::map<std::string, t_SessionData>::iterator	it = _sessions.find(sessionCookie);
	if (it != _sessions.end()) {
		_sessions.erase(it);
	}
}

/**
 * @brief generic getter for application-defined session data (e.g. "isLoggedIn", "name").
 * @return the stored value, or "" if the session or key does not exist.
 */
std::string	SessionManager::getData(const std::string &sessionId, const std::string &key)
{
	std::map<std::string, t_SessionData>::iterator sessionIt = _sessions.find(sessionId);
	if (sessionIt == _sessions.end())
		return "";

	std::map<std::string, std::string>::iterator dataIt = sessionIt->second.data.find(key);
	if (dataIt == sessionIt->second.data.end())
		return "";
	return dataIt->second;
}

/**
 * @brief generic setter for application-defined session data. Server has no
 *  knowledge of what "key" means; the application (CGI, login-handler, ...) decides.
 */
void	SessionManager::setData(const std::string &sessionId, const std::string &key, const std::string &value)
{
	std::map<std::string, t_SessionData>::iterator sessionIt = _sessions.find(sessionId);
	if (sessionIt == _sessions.end())
		return ;
	sessionIt->second.data[key] = value;
}

/**
 * @brief returns every application-defined key/value pair stored for a session.
 *  Returns an empty map if the session does not exist.
 */
std::map<std::string, std::string>	SessionManager::getAllData(const std::string &sessionId)
{
	std::map<std::string, t_SessionData>::iterator sessionIt = _sessions.find(sessionId);
	if (sessionIt == _sessions.end())
		return (std::map<std::string, std::string>());
	return (sessionIt->second.data);
}

// =========================================================================
// Private Helper Methods
// =========================================================================

std::string	SessionManager::extractCookie(std::vector<std::string> cookieVal)
{
	size_t	posStart = 0;
	const std::string prefix = "session_id=";
	for (size_t i = 0; i < cookieVal.size(); i++) {
		posStart = cookieVal[i].find(prefix);
		if (posStart != std::string::npos) {
			posStart += prefix.size();
			size_t posEnd = cookieVal[i].find(";", posStart);
			if (posEnd == std::string::npos)
				posEnd = cookieVal[i].size();
			std::string sessionId = cookieVal[i].substr(posStart, posEnd - posStart);
			return sessionId;
		}
	}
	return "";
}

/**
 * @brief generates a unique session_id, by combining 
 * 	two rand() and one time() call
 */
std::string	SessionManager::generateSessionId()
{
	std::stringstream ss;
	ss << rand() << "-" << rand() << "-" << time(NULL);
	return ss.str();
}

bool	SessionManager::addCookieToSessions(const std::string &newCookie)
{
	t_SessionData sessionData;
	sessionData.createdAt = time(NULL);

	std::map<std::string, t_SessionData>::iterator it = _sessions.find(newCookie);
	if (it == _sessions.end()) {
		_sessions[newCookie] = sessionData;
		return true;
	}
	return false;
}