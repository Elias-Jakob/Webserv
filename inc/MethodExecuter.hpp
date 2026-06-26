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
# include "ABodyParser.hpp"
# include "FormParser.hpp"
# include "MultipartParser.hpp"
# include "structs.h"

class MethodExecuter
{
	private:
		// create AMethod
		static AMethod	*createGet(std::string name);
		static AMethod	*createPost(std::string name);
		static AMethod	*createDelete(std::string name);

	public:
		// construction & deconstruction
		MethodExecuter();
		~MethodExecuter();

		bool				isValidMethod(const std::string &methodName);
		AMethod				*createMethod(const std::string &methodName);
		t_executionResult	execute(AMethod *method, HttpRequest *request);
};

#endif
