#ifndef SESSION_MANAGER_HPP
# define SESSION_MANAGER_HPP

# include <iostream>
# include <vector>
# include <map>
# include <sstream>
# include <cstdlib> // srand()
// # include <random> // usable for c++98?

typedef struct	s_SessionData
{
	bool		isLoggedIn;
	std::string	name;
} 				t_SessionData;


class SessionManager
{
	public:
		SessionManager();
		~SessionManager();

		std::string	createNewSession();
		std::string	createSessionHeaderForResponse(const std::string &sessionID);
		bool		isValidCookie(const std::string &sessionID);
		std::string	extractCookie(std::vector<std::string>);

		// GETTERS
		size_t	sessionsSize();

	private:
		std::map<std::string, t_SessionData>	_sessions;

		bool		addCookieToSessions(const std::string &newCookie);
		std::string	generateSessionId();
};

#endif