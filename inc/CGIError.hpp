#ifndef SYSCALLERROR_HPP
# define SYSCALLERROR_HPP

class CGIError
{
	public:
		CGIError();
		CGIError(const CGIError &other);
		~CGIError();
		CGIError	&operator=(const CGIError &other);
};

#endif
