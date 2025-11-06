// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

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

enum class TopicType { Event, Chopper, Sample, Run, Monitor };
/** TopicInfo : Class that holds information on a kafka topic.
 */
class MANTID_KERNEL_DLL TopicInfo {
public:
  TopicInfo(InstrumentInfo const *inst, const Poco::XML::Element *elem);
  TopicInfo(std::string name, TopicType type);
  const std::string &name() const { return m_name; }
  TopicType type() const { return m_type; }

private:
  std::string m_name;
  TopicType m_type;
};

/// Allow this object to be printed to a stream
MANTID_KERNEL_DLL std::ostream &operator<<(std::ostream &buffer, const TopicInfo &topic);

} // namespace Kernel
} // namespace Mantid
