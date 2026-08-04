#ifndef SESSION_MANAGER_HPP
# define SESSION_MANAGER_HPP

# include <iostream>
# include <vector>
# include <map>
# include <sstream>
# include <cstdlib> // srand()
# include "limits_defines.hpp" // For color printing

//TODO How to handle cgiResponse if new session created?

typedef struct	s_SessionData
{
	time_t		createdAt;
	std::map<std::string, std::string> data;
} 				t_SessionData;

// Server: session store -- generic key-value bag + ID lifecycle.
/**
 * 
 */
class SessionManager
{
	public:
		SessionManager();
		~SessionManager();

		std::string	createNewSession();
		std::string	createSessionHeaderForResponse(const std::string &sessionID);
		bool		isValidCookie(const std::string &sessionID);
		std::string	extractCookie(std::vector<std::string>);

		// delete Sessions
		void	removeExpiredSessions();
		void	deleteSession(const std::string &sessionCookie);

		// generic session-data access (application-defined keys, e.g. "isLoggedIn", "name")
		std::string	getData(const std::string &sessionId, const std::string &key);
		void		setData(const std::string &sessionId, const std::string &key, const std::string &value);
		std::map<std::string, std::string>	getAllData(const std::string &sessionId);

		// GETTERS
		size_t	sessionsSize();
		
	private:
		std::map<std::string, t_SessionData>	_sessions;

		time_t	_lastSweep;
		
		// settings(configuration) variables.
		std::string	_flags;
		int			_maxAge;

		// private-methods
		bool		addCookieToSessions(const std::string &newCookie);
		std::string	generateSessionId();
};

// TODO
/*
	Session expiration:
*/
#endif