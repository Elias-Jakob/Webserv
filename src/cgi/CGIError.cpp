# include "CGIError.hpp"

CGIError::CGIError(const char *msg) : _msg(msg) {}

CGIError::~CGIError() throw() {}

const char	*CGIError::what() const throw()
{ return (this->_msg); }
