#ifndef HEAD_HPP
# define HEAD_HPP
# include "Get.hpp"


class Head : public Get
{
	public:

	private:
		Head();
		Head(std::string name);
		Head(std::string name, t_Location *location);
		~Head();
		bool serveFile(struct stat &fileInfo);
};

#endif