#ifndef NEXUSEXCEPTION_HPP
#define NEXUSEXCEPTION_HPP 1

#include "MantidNexusCpp/DllConfig.h"
#include <stdexcept>
#include <string>

/**
 * \file NeXusException.hpp
 * Header for a base NeXus::Exception
 * \ingroup cpp_main
 */

namespace NeXus {

/**
 * Class that provides for a standard NeXus exception
 * \ingroup cpp_core
 */

class MANTID_NEXUSCPP_DLL Exception : public std::runtime_error {
public:
  /**
   * Create a new NeXus::Exception
   *
   * \param msg the string to pass a the error message
   * \param status
   */
  Exception(const std::string &msg = "GENERIC ERROR", const int status = 0);
  /**
   * Get the message associated with the exception
   *
   * \return the message associated with the exception
   */
  virtual const char *what() const throw();
  /**
   * Get the status associated with the exception
   *
   * \return the status value associated with the exception
   */
  int status() throw();
  /** Destructor for exception */
  virtual ~Exception() throw();

private:
  std::string m_what; ///< Error message for the exception
  int m_status;       ///< Status value for the exception
};
}; // namespace NeXus

#endif
