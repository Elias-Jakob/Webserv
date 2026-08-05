#ifndef METHOD_EXECUTER_HPP
# define METHOD_EXECUTER_HPP

# include <iostream>
# include <string>
# include <map>
# include <algorithm>

# include <sstream>
# include <vector>
# include "HttpRequest.hpp"
# include "AMethod.hpp"
# include "Get.hpp"
# include "Post.hpp"
# include "Delete.hpp"
# include "Head.hpp"
# include "ABodyParser.hpp"
# include "FormParser.hpp"
# include "MultipartParser.hpp"
# include "ConfigFileParser.hpp"
# include "structs.h"

class MethodExecuter
{
	public:
		MethodExecuter();
		~MethodExecuter();

		bool				isImplementedMethod(const std::string &methodName);
		AMethod				*createMethod(const std::string &methodName, t_Location *locationObj);
		t_executionResult	execute(AMethod *method, HttpRequest *request);
		bool				setConfig(std::vector<t_Configs> serverConfigs);
		t_Location			*availableLocation(const std::string &path, const std::string &listeningInterface, const std::string &methodName);
		std::string			modifyRequestURI(HttpRequest *req, const std::string &listeningInterface);

		std::vector<t_Configs>	getServerConfigs();

	private:
		std::vector<t_Configs>	_serverConfigs;
		
		std::map<std::string, std::map<std::string, std::string> >	_rootedLocs;
		//			IP:PORT				loc/path		root
		// t_Configs	*_serverConfig;
		t_Location	_defaultLocation;

		std::map<std::string, std::string>	_rootedLocations;

		static AMethod	*createGet(std::string name);
		static AMethod	*createPost(std::string name);
		static AMethod	*createDelete(std::string name);
		static AMethod	*createGet(std::string name, t_Location *locationObj);
		static AMethod	*createDelete(std::string name, t_Location *location);
		static AMethod	*createPost(std::string name, t_Location *location);
		static AMethod	*createHead(std::string name);
		static AMethod	*createHead(std::string name, t_Location *location);
		
		void						setDefaultLocation();
		bool						isAllowedMethod(t_Location *location, const std::string &method);
		std::vector<std::string>	splitPath(const std::string &path);
		std::vector<std::string>	splitPathDir(const std::string &path);
		bool 						isListening(size_t i, const std::string &host);
};

#endif
