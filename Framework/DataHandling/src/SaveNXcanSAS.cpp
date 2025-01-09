// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidDataHandling/SaveNXcanSAS.h"
#include "MantidAPI/Progress.h"

#include <Poco/File.h>

using namespace Mantid::API;

namespace Mantid::DataHandling {
// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(SaveNXcanSAS)

/// constructor
SaveNXcanSAS::SaveNXcanSAS() = default;

void SaveNXcanSAS::init() { initStandardProperties(); }

std::map<std::string, std::string> SaveNXcanSAS::validateInputs() { return validateStandardInputs(); }

void SaveNXcanSAS::exec() {

  std::string &&filename = getPropertyValue("Filename");
  // Remove the file if it already exists
  if (Poco::File(filename).exists()) {
    Poco::File(filename).remove();
  }

  H5::H5File file(filename, H5F_ACC_EXCL);

  Progress progress(this, 0.1, 1.0, 3);
  progress.report("Adding a new entry.");

  Mantid::API::MatrixWorkspace_sptr &&workspace = getProperty("InputWorkspace");
  // add sas entry
  const std::string suffix("01");
  auto sasEntry = addSasEntry(file, workspace, suffix);

  // Add metadata for canSAS file: Instrument, Sample, Process
  progress.report("Adding standard metadata");
  addStandardMetadata(workspace, sasEntry);

  // Add the data
  progress.report("Adding data.");
  addData(sasEntry, workspace);

  file.close();
}
} // namespace Mantid::DataHandling
