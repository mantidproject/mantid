// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_KERNEL_LIVELISTENERINFO_H_
#define MANTID_KERNEL_LIVELISTENERINFO_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/DllConfig.h"
#include <string>

//----------------------------------------------------------------------
// Forward declarations
//----------------------------------------------------------------------
namespace Poco {
namespace XML {
class Element;
}
} // namespace Poco

namespace Mantid {
namespace Kernel {

//----------------------------------------------------------------------
// Forward declarations
//----------------------------------------------------------------------
class InstrumentInfo;

/**
 * A class that holds information about a LiveListener connection.
 *
 *
 */
class MANTID_KERNEL_DLL LiveListenerInfo {
public:
  LiveListenerInfo(InstrumentInfo *inst, const Poco::XML::Element *elem);
  LiveListenerInfo(const std::string &listener = "",
                   const std::string &address = "",
                   const std::string &name = "");

  /// Required for Python bindings
  bool operator==(const LiveListenerInfo &rhs) const;

  /// Return the name of this LiveListener connection
  const std::string &name() const;

  /// Returns the address string of this LiveListener connection
  const std::string &address() const;

  /// Returns the classname of the specific LiveListener to use
  const std::string &listener() const;

private:
  std::string m_name;     ///< Listener name
  std::string m_address;  ///< Listener address
  std::string m_listener; ///< Listener classname
};

/// Allow this object to be printed to a stream
MANTID_KERNEL_DLL std::ostream &operator<<(std::ostream &buffer,
                                           const LiveListenerInfo &listener);

} // namespace Kernel
} // namespace Mantid

#endif /* MANTID_KERNEL_LIVELISTENERINFO_H_ */
