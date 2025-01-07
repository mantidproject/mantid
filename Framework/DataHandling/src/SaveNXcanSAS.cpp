// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataHandling/SaveNXcanSAS.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/CommonBinsValidator.h"
#include "MantidAPI/ExperimentInfo.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/InstrumentFileFinder.h"
#include "MantidAPI/Progress.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/WorkspaceUnitValidator.h"
#include "MantidDataHandling/NXcanSASDefinitions.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/MDGeometry/IMDDimension.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/CompositeValidator.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/MDUnit.h"
#include "MantidKernel/MantidVersion.h"
#include "MantidKernel/VectorHelper.h"
#include "MantidNexus/H5Util.h"

#include <H5Cpp.h>
#include <boost/algorithm/string/trim.hpp>
#include <boost/regex.hpp>
#include <memory>

#include <Poco/File.h>
#include <Poco/Path.h>
#include <algorithm>
#include <cctype>
#include <functional>
#include <iterator>
#include <utility>

using namespace Mantid::Kernel;
using namespace Mantid::Geometry;
using namespace Mantid::API;
using namespace Mantid::DataHandling::NXcanSAS;

namespace Mantid::DataHandling {
// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(SaveNXcanSAS)

/// constructor
SaveNXcanSAS::SaveNXcanSAS() = default;

void SaveNXcanSAS::init() { initStandardProperties(); }

std::map<std::string, std::string> SaveNXcanSAS::validateInputs() {
  // The input should be a Workspace2D
  Mantid::API::MatrixWorkspace_sptr workspace = getProperty("InputWorkspace");
  std::map<std::string, std::string> result;
  if (!workspace || !std::dynamic_pointer_cast<const Mantid::DataObjects::Workspace2D>(workspace)) {
    result.emplace("InputWorkspace", "The InputWorkspace must be a Workspace2D.");
  }

  // Transmission data should be 1D
  Mantid::API::MatrixWorkspace_sptr transmission = getProperty("Transmission");
  Mantid::API::MatrixWorkspace_sptr transmissionCan = getProperty("TransmissionCan");

  auto checkTransmission = [&result](const Mantid::API::MatrixWorkspace_sptr &trans, const std::string &propertyName) {
    if (trans->getNumberHistograms() != 1) {
      result.emplace(propertyName, "The input workspaces for transmissions have to be 1D.");
    }
  };

  if (transmission) {
    checkTransmission(transmission, "Transmission");
  }

  if (transmissionCan) {
    checkTransmission(transmissionCan, "TransmissionCan");
  }

  return result;
}

void SaveNXcanSAS::exec() {
  Mantid::API::MatrixWorkspace_sptr &&workspace = getProperty("InputWorkspace");
  std::string &&filename = getPropertyValue("Filename");
  // Remove the file if it already exists
  if (Poco::File(filename).exists()) {
    Poco::File(filename).remove();
  }

  H5::H5File file(filename, H5F_ACC_EXCL);
  auto numberOfSteps = 4;
  Progress progress(this, 0.1, 1.0, numberOfSteps);
  progress.report("Adding a new entry.");
  // add sas entry
  const std::string suffix("01");
  auto sasEntry = addSasEntry(file, workspace, suffix);
  // Add metadata for canSAS file: Instrument, Sample, Process
  addStandardMetadata(workspace, sasEntry, &progress);
  // Add the data
  progress.report("Adding data.");
  addData(sasEntry, workspace);

  file.close();
}
} // namespace Mantid::DataHandling
