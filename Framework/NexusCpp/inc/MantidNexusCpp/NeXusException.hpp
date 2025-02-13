#pragma once

#include "MantidNexusCpp/DllConfig.h"
#include "MantidNexusCpp/NeXusFile_fwd.h"
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
  Exception(const std::string &msg = "GENERIC ERROR", const NXstatus status = static_cast<NXstatus>(0));
  /**
   * Get the message associated with the exception
   *
   * \return the message associated with the exception
   */
  virtual const char *what() const throw() override;
  /**
   * Get the status associated with the exception
   *
   * \return the status value associated with the exception
   */
  NXstatus status() throw();
  /** Destructor for exception */
  virtual ~Exception() throw();

private:
  std::string m_what; ///< Error message for the exception
  NXstatus m_status;  ///< Status value for the exception
};
}; // namespace NeXus
