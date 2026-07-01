# include "SyscallError.hpp"

SyscallError::SyscallError(const char *msg) : _msg(msg) {}

SyscallError::~SyscallError() const throw() {}

const char	*SyscallError::what() const throw()
{ return (this->_msg); }
