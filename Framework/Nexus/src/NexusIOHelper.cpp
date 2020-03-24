#include "MantidNexus/NexusIOHelper.h"
#include "MantidKernel/Logger.h"
#include "MantidTypes/Core/DateAndTime.h"

using namespace Mantid::Types::Core;
namespace {
Mantid::Kernel::Logger g_log("NeXusIOHelper");
}

namespace Mantid {
namespace NeXus {
namespace NeXusIOHelper {

std::pair<::NeXus::Info, bool> checkIfOpenAndGetInfo(::NeXus::File &file,
                                                     std::string entry) {
  std::pair<::NeXus::Info, bool> info_and_close;
  info_and_close.second = false;
  if (!file.isDataSetOpen()) {
    file.openData(entry);
    info_and_close.second = true;
  }
  info_and_close.first = file.getInfo();
  return info_and_close;
}

const std::string readStartTimeOffset(::NeXus::File &file) {
  std::string startTime;
  // Read the offset (time zero)
  if (file.hasAttr("offset"))
    file.getAttr("offset", startTime);
  else if (file.hasAttr("start"))
    file.getAttr("start", startTime);
  else {
    // The ESS uses the unix epoch as the default offset and this is not
    // stored in the nexus file. So here we assume the unix epoch and post a
    // warning to the logs.
    auto epoch = DateAndTime(DateAndTime::UNIX_EPOCH);
    startTime = epoch.toISO8601String();
    g_log.warning() << "No offset provided for pulse times in the nexus file. "
                       "Assuming Unix epoch. \n";
  }
  return startTime;
}
} // namespace NeXusIOHelper
} // namespace NeXus
} // namespace Mantid
