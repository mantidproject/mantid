#pragma once

#include "MantidNexus/DllConfig.h"
#include "MantidNexus/NeXusFile_fwd.h"
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

class MANTID_NEXUS_DLL Exception : public std::runtime_error {
public:
  /// Create a new NeXus::Exception
  Exception(const std::string &msg = "GENERIC ERROR", const std::string &filename = "");
  /// \return the message with filename
  const std::string filename() const throw();

private:
  std::string m_filename; ///< File this exceptoin is associated with
};
}; // namespace NeXus
