#include "MantidDataHandling/RenameLog.h"
#include "MantidKernel/MandatoryValidator.h"
#include "MantidKernel/TimeSeriesProperty.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;

namespace Mantid {
namespace DataHandling {

DECLARE_ALGORITHM(RenameLog)

//----------------------------------------------------------------------------------------------
/** Constructor
 */
RenameLog::RenameLog() {}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
RenameLog::~RenameLog() {}

void RenameLog::init() {

  declareProperty(new API::WorkspaceProperty<API::MatrixWorkspace>(
                      "Workspace", "Anonymous", Direction::InOut),
                  "Workspace to have logs merged");
  declareProperty("OriginalLogName", "",
                  boost::make_shared<MandatoryValidator<std::string>>(),
                  "Log's original name.");
  declareProperty("NewLogName", "",
                  boost::make_shared<MandatoryValidator<std::string>>(),
                  "Log's new name.");

  return;
}

void RenameLog::exec() {

  // 1. Get value
  matrixWS = this->getProperty("Workspace");
  std::string origlogname = this->getProperty("OriginalLogName");
  std::string newlogname = this->getProperty("NewLogName");

  Kernel::Property *property = matrixWS->run().getLogData(origlogname);
  Kernel::TimeSeriesProperty<double> *timeprop =
      dynamic_cast<Kernel::TimeSeriesProperty<double> *>(property);

  if (!timeprop) {
    // g_log.error() << "After Log data is removed, TimeSeriesProperty " <<
    // origlogname << " is deleted from memory" << std::endl;
    throw std::runtime_error("Not a TimeSeriesProperty!");
  }

  // std::cout << "Remove log" << origlogname << std::endl;
  matrixWS->mutableRun().removeLogData(origlogname, false);

  // std::cout << "Change log name" << std::endl;
  timeprop->setName(newlogname);
  // std::cout << "Add log" << timeprop->name() << std::endl;
  // std::vector<Kernel::DateAndTime> newtimes = timeprop->timesAsVector();
  // std::cout << "Entries = " << newtimes.size() << std::endl;
  matrixWS->mutableRun().addProperty(timeprop);

  return;
}

} // namespace Mantid
} // namespace DataHandling
