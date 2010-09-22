#ifndef NEXUSEXCEPTION_HPP
#define NEXUSEXCEPTION_HPP 1

#include <string>
#include <stdexcept>

namespace NeXus{
  class Exception : public std::runtime_error
  {
  public:
    Exception(const std::string& msg = "GENERIC ERROR", const int status = 0);
    virtual const char* what() const throw();
    int status() throw();
    virtual ~Exception() throw();
  private:
    std::string m_what;
    int m_status;
  };
};

#endif
