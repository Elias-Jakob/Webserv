#ifndef SYSCALLERROR_HPP
# define SYSCALLERROR_HPP

# include <exception>

class SyscallError : public std::exception
{
	public:
		SyscallError(const char *msg);
		~SyscallError() throw();
		const char	*what() const throw();
	private:
		const char	*_msg;
};

#endif // !SYSCALLERROR_HPP
