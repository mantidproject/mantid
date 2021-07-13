// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataHandling/LoadMuonData.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/FileProperty.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/ListValidator.h"
#include <Poco/Path.h>

namespace Mantid {
namespace DataHandling {

using namespace Kernel;
using namespace API;

DECLARE_ALGORITHM(LoadMuonData)

/// Initialization method.
void LoadMuonData::init() {
  std::vector<std::string> extensions{".nxs", ".nxs_v2", ".bin"};
  declareProperty(std::make_unique<FileProperty>("Filename", "", FileProperty::Load, extensions),
                  "The name of the Nexus file to load");
  const std::vector<std::string> extsTemps{".mon"};
  declareProperty(std::make_unique<Mantid::API::FileProperty>("TemperatureFilename", "",
                                                              Mantid::API::FileProperty::OptionalLoad, extsTemps),
                  "The name of the temperature file to be loaded, this is optional as it "
                  "will be automatically searched for if not provided. For PSI data.");
  declareProperty(std::make_unique<WorkspaceProperty<Workspace>>("OutputWorkspace", "", Direction::Output),
                  "The name of the workspace to be created as the output of the\n"
                  "algorithm. For multiperiod files, one workspace will be\n"
                  "generated for each period");
  declareProperty("SearchForTempFile", true,
                  "If no temp file has been given decide whether the algorithm "
                  "will search for the temperature file. For PSI data.");
  auto mustBePositiveSpectra = std::make_shared<BoundedValidator<specnum_t>>();
  mustBePositiveSpectra->setLower(0);
  declareProperty("SpectrumMin", static_cast<specnum_t>(0), mustBePositiveSpectra);
  declareProperty("SpectrumMax", static_cast<specnum_t>(EMPTY_INT()), mustBePositiveSpectra);
  declareProperty(std::make_unique<ArrayProperty<specnum_t>>("SpectrumList"));
  declareProperty("AutoGroup", false,
                  "Determines whether the spectra are automatically grouped\n"
                  "together based on the groupings in the NeXus file, only\n"
                  "for single period data (default no). Version 1 only.");
  auto mustBePositive = std::make_shared<BoundedValidator<int64_t>>();
  mustBePositive->setLower(0);
  declareProperty("EntryNumber", static_cast<int64_t>(0), mustBePositive,
                  "0 indicates that every entry is loaded, into a separate "
                  "workspace within a group. "
                  "A positive number identifies one entry to be loaded, into "
                  "one workspace");
  std::vector<std::string> FieldOptions{"Transverse", "Longitudinal"};
  declareProperty("MainFieldDirection", "Transverse", std::make_shared<StringListValidator>(FieldOptions),
                  "Output the main field direction if specified in Nexus file "
                  "(default longitudinal).",
                  Direction::Output);
  declareProperty("TimeZero", 0.0, "Time zero in units of micro-seconds (default to 0.0)", Direction::Output);
  declareProperty("FirstGoodData", 0.0, "First good data in units of micro-seconds (default to 0.0)",
                  Direction::Output);
  declareProperty("LastGoodData", 0.0, "Last good data in the OutputWorkspace’s spectra. For PSI data",
                  Direction::Output);
  declareProperty(std::make_unique<ArrayProperty<double>>("TimeZeroList", Direction::Output),
                  "A vector of time zero values");
  declareProperty(
      std::make_unique<WorkspaceProperty<Workspace>>("TimeZeroTable", "", Direction::Output, PropertyMode::Optional),
      "TableWorkspace containing time zero values per spectra.");
  declareProperty("CorrectTime", true, "Boolean flag controlling whether time should be corrected by timezero.",
                  Direction::Input);
  declareProperty(
      std::make_unique<WorkspaceProperty<Workspace>>("DeadTimeTable", "", Direction::Output, PropertyMode::Optional),
      "Table or a group of tables containing detector dead times.");
  declareProperty(std::make_unique<WorkspaceProperty<Workspace>>("DetectorGroupingTable", "", Direction::Output,
                                                                 PropertyMode::Optional),
                  "Table or a group of tables with information about the "
                  "detector grouping.");
}

void LoadMuonData::exec() {
  std::string filePath = getPropertyValue("Filename");
  auto extension = Poco::Path(filePath).getExtension();
  std::shared_ptr<Algorithm> childAlg;
  // Nexus files should use LoadMuonNexus which will decide which loader to use from there based on the nexus file
  // version
  if (extension == "nxs" || extension == "nxs_v2") {
    childAlg = createChildAlgorithm("LoadMuonNexus", 0, 1, true, 2);
  } else if (extension == "bin") {
    childAlg = createChildAlgorithm("LoadPSIMuonBin", 0, 1, true, 1);
  } else {
    throw Kernel::Exception::FileError("Cannot open the file ", filePath);
  }

  // Execute loader
  auto loader = std::dynamic_pointer_cast<API::Algorithm>(childAlg);
  loader->copyPropertiesFrom(*this);
  loader->executeAsChildAlg();
  this->copyPropertiesFrom(*loader);
  API::Workspace_sptr outWS = loader->getProperty("OutputWorkspace");
  setProperty("OutputWorkspace", outWS);
}

} // namespace DataHandling
} // namespace Mantid
