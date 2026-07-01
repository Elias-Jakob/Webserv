#ifndef SYSCALLERROR_HPP
# define SYSCALLERROR_HPP

# include <exception>

class CGIError : public std::exception
{
	public:
		CGIError(const char *msg);
		~CGIError() throw();
		const char	*what() const throw();
	private:
		const char	*_msg;
};

#endif // !SYSCALLERROR_HPP
