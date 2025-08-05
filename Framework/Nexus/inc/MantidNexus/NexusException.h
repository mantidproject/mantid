#pragma once

#include "MantidNexus/DllConfig.h"
#include "MantidNexus/NexusFile_fwd.h"
#include <iosfwd>
#include <stdexcept>
#include <string>

/**
 * Header for a base Nexus::Exception
 * \ingroup cpp_main
 */

namespace Mantid::Nexus {

/**
 * Class that provides for a standard Nexus exception
 * \ingroup cpp_core
 */

class MANTID_NEXUS_DLL Exception : public std::runtime_error {
public:
  /// Create a new Nexus::Exception
  Exception(const std::string &msg = "GENERIC ERROR", const std::string &functionname = "",
            const std::string &filename = "");
  /// \return the message with functionname
  const std::string functionname() const throw();
  /// \return the message with filename
  const std::string filename() const throw();

private:
  std::string m_functionname; ///< Function this exception is associted with
  std::string m_filename;     ///< File this exception is associated with
};

MANTID_NEXUS_DLL std::ostream &operator<<(std::ostream &os, const Exception &err);
}; // namespace Mantid::Nexus
