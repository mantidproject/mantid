#include "MantidVatesAPI/MetaDataExtractorUtils.h"
#include <qwt_double_interval.h>
#include "MantidAPI/IMDIterator.h"
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidAPI/IMDHistoWorkspace.h"
#include "MantidAPI/IPeaksWorkspace.h"
#include "MantidAPI/IMDWorkspace.h"
#include "MantidGeometry/Instrument.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/MultiThreaded.h"
#include "boost/pointer_cast.hpp"
#include <cfloat>

namespace Mantid {
namespace VATES {
namespace {
/// Static logger
Kernel::Logger g_log("MetaDataExtractorUtils");
}

MetaDataExtractorUtils::MetaDataExtractorUtils() = default;

MetaDataExtractorUtils::~MetaDataExtractorUtils() = default;

/**
 * Extract the instrument information from the workspace. If there
 * is more than one instrument involved, then extract the first instrument
 * from the list.
 * @param workspace Shared pointer to the workspace.
 * @returns The instrument name or an empty string.
 */
std::string MetaDataExtractorUtils::extractInstrument(
    const Mantid::API::IMDWorkspace *workspace) {
  auto eventWorkspace =
      dynamic_cast<const Mantid::API::IMDEventWorkspace *>(workspace);
  auto histoWorkspace =
      dynamic_cast<const Mantid::API::IMDHistoWorkspace *>(workspace);

  std::string instrument = "";

  // Check which workspace is currently used and that it contains at least one
  // instrument.
  if (eventWorkspace) {
    if (eventWorkspace->getNumExperimentInfo() > 0) {
      instrument =
          eventWorkspace->getExperimentInfo(0)->getInstrument()->getName();
    } else {
      g_log.notice() << "The event workspace does not have any instruments. \n";

      instrument = "";
    }
  } else if (histoWorkspace) {
    if (histoWorkspace->getNumExperimentInfo() > 0) {
      instrument =
          histoWorkspace->getExperimentInfo(0)->getInstrument()->getName();
    } else {
      g_log.notice() << "The histo workspace does not have any instruments. \n";

      instrument = "";
    }
  } else {
    g_log.warning()
        << "The workspace does not seem to be either event or histo. \n";
    instrument = "";
  }

  return instrument;
}
}
}
