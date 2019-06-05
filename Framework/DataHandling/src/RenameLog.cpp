// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataHandling/RenameLog.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Run.h"
#include "MantidKernel/MandatoryValidator.h"
#include "MantidKernel/TimeSeriesProperty.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;

namespace Mantid {
namespace DataHandling {

DECLARE_ALGORITHM(RenameLog)

void RenameLog::init() {

  declareProperty(
      std::make_unique<API::WorkspaceProperty<API::MatrixWorkspace>>(
          "Workspace", "Anonymous", Direction::InOut),
      "Workspace to have logs merged");
  declareProperty("OriginalLogName", "",
                  boost::make_shared<MandatoryValidator<std::string>>(),
                  "Log's original name.");
  declareProperty("NewLogName", "",
                  boost::make_shared<MandatoryValidator<std::string>>(),
                  "Log's new name.");
}

void RenameLog::exec() {

  // 1. Get value
  matrixWS = this->getProperty("Workspace");
  std::string origlogname = this->getProperty("OriginalLogName");
  std::string newlogname = this->getProperty("NewLogName");

  Kernel::Property *property = matrixWS->run().getLogData(origlogname)->clone();
  Kernel::TimeSeriesProperty<double> *timeprop =
      dynamic_cast<Kernel::TimeSeriesProperty<double> *>(property);

  if (!timeprop) {
    // g_log.error() << "After Log data is removed, TimeSeriesProperty " <<
    // origlogname << " is deleted from memory\n";
    throw std::runtime_error("Not a TimeSeriesProperty!");
  }

  // std::cout << "Remove log" << origlogname << '\n';
  matrixWS->mutableRun().removeLogData(origlogname);

  // std::cout << "Change log name\n";
  timeprop->setName(newlogname);
  // std::cout << "Add log" << timeprop->name() << '\n';
  // std::vector<Types::Core::DateAndTime> newtimes = timeprop->timesAsVector();
  // std::cout << "Entries = " << newtimes.size() << '\n';
  matrixWS->mutableRun().addProperty(timeprop);
}

} // namespace DataHandling
} // namespace Mantid
