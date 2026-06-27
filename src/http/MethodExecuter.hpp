#ifndef METHOD_EXECUTER_HPP
# define METHOD_EXECUTER_HPP

# include <iostream>
# include <string>
# include <map>
# include <algorithm>

# include <sstream>
# include <vector>
# include "HttpRequest.hpp"
# include "../methods/AMethod.hpp"
# include "../methods/Get.hpp"
# include "../methods/Post.hpp"
# include "../methods/Delete.hpp"
# include "../parsers/ABodyParser.hpp"
# include "../parsers/FormParser.hpp"
# include "../parsers/MultipartParser.hpp"
# include "../config/ConfigFileParser.hpp"
# include "../../structs.h"

class MethodExecuter
{
	public:
		MethodExecuter();
		~MethodExecuter();

		bool				isImplementedMethod(const std::string &methodName);
		AMethod				*createMethod(const std::string &methodName, const std::string &path);
		t_executionResult	execute(AMethod *method, HttpRequest *request);
		bool				setConfig(t_Server *serverConfig);
		t_Location			*availableLocation(const std::string &path);
		std::string			modifyRequestURI(HttpRequest *req);

	private:
		t_Server	*_serverConfig;
		std::map<std::string, std::string>	_rootedLocations;

		static AMethod	*createGet(std::string name);
		static AMethod	*createPost(std::string name);
		static AMethod	*createDelete(std::string name);
		static AMethod	*createGet(std::string name, t_Location *locationObj);
		
		bool	isAllowedMethod(t_Location *location, const std::string &method);
		std::vector<std::string> splitPath(const std::string &path);
		std::vector<std::string> splitPathDir(const std::string &path);
};

#endif