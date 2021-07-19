// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataHandling/LoadMuonData.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/ListValidator.h"
#include <Poco/Path.h>

namespace Mantid {
namespace DataHandling {

using namespace Kernel;
using namespace API;
using namespace DataObjects;

DECLARE_ALGORITHM(LoadMuonData)

/// Initialization method.
void LoadMuonData::init() {
  // Common properties and output properties
  std::vector<std::string> extensions{".nxs", ".nxs_v2", ".bin"};
  declareProperty(std::make_unique<FileProperty>("Filename", "", FileProperty::Load, extensions),
                  "The name of the Nexus file to load");
  declareProperty(std::make_unique<WorkspaceProperty<Workspace>>("OutputWorkspace", "", Direction::Output),
                  "The name of the workspace to be created as the output of the\n"
                  "algorithm. For multiperiod files, one workspace will be\n"
                  "generated for each period");
  std::vector<std::string> FieldOptions{"Transverse", "Longitudinal", ""};
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

  // Nexus properties
  auto mustBePositiveSpectra = std::make_shared<BoundedValidator<specnum_t>>();
  mustBePositiveSpectra->setLower(0);
  declareProperty("SpectrumMin", static_cast<specnum_t>(0), mustBePositiveSpectra);
  declareProperty("SpectrumMax", static_cast<specnum_t>(EMPTY_INT()), mustBePositiveSpectra);
  declareProperty(std::make_unique<ArrayProperty<specnum_t>>("SpectrumList"),
                  "Array, or comma separated list, of indexes of spectra to\n"
                  "load. If a range and a list of spectra are both supplied,\n"
                  "all the specified spectra will be loaded. For Nexus data.");

  declareProperty("AutoGroup", false,
                  "Determines whether the spectra are automatically grouped\n"
                  "together based on the groupings in the Nexus file, only\n"
                  "for single period data (default no). Version 1 only. For Nexus data.");

  auto mustBePositive = std::make_shared<BoundedValidator<int64_t>>();
  mustBePositive->setLower(0);
  declareProperty("EntryNumber", static_cast<int64_t>(0), mustBePositive,
                  "0 indicates that every entry is loaded, into a separate "
                  "workspace within a group. "
                  "A positive number identifies one entry to be loaded, into "
                  "one workspace. For Nexus data.");

  // PSI properties
  const std::vector<std::string> extsTemps{".mon"};
  declareProperty(std::make_unique<Mantid::API::FileProperty>("TemperatureFilename", "",
                                                              Mantid::API::FileProperty::OptionalLoad, extsTemps),
                  "The name of the temperature file to be loaded, this is optional as it "
                  "will be automatically searched for if not provided. For PSI data.");
  declareProperty("SearchForTempFile", true,
                  "If no temp file has been given decide whether the algorithm "
                  "will search for the temperature file. For PSI data.");
}

void LoadMuonData::exec() {
  std::string filePath = getPropertyValue("Filename");
  auto extension = Poco::Path(filePath).getExtension();
  std::shared_ptr<Algorithm> childAlg;
  Algorithm_sptr loader;
  bool PSIData = false; // Flag to control setting some specific PSI input/output properties
  // Nexus files should use LoadMuonNexus which will decide which loader to use from there based on the Nexus file
  // version
  if (extension == "nxs" || extension == "nxs_v2") {
    // Create Nexus loader and set Nexus inputs
    childAlg = createChildAlgorithm("LoadMuonNexus", 0, 1, true, 2);
    loader = std::dynamic_pointer_cast<API::Algorithm>(childAlg);
    specnum_t specMin = getProperty("SpectrumMin");
    loader->setProperty("SpectrumMin", specMin);
    specnum_t specMax = getProperty("SpectrumMax");
    loader->setProperty("SpectrumMax", specMax);
    std::vector<specnum_t> specList = getProperty("SpectrumList");
    loader->setProperty("SpectrumList", specList);
    bool autoGroup = getProperty("AutoGroup");
    loader->setProperty("AutoGroup", autoGroup);
    int64_t entryNumber = getProperty("EntryNumber");
    loader->setProperty("EntryNumber", entryNumber);
    if (!getPropertyValue("DetectorGroupingTable").empty())
      loader->setProperty("DetectorGroupingTable", getPropertyValue("DetectorGroupingTable"));

  } else if (extension == "bin") {
    // Set PSI flag, create PSI load alg and set PSI inputs
    PSIData = true;
    childAlg = createChildAlgorithm("LoadPSIMuonBin", 0, 1, true, 1);
    loader = std::dynamic_pointer_cast<API::Algorithm>(childAlg);
    loader->setProperty("TemperatureFilename", getPropertyValue("TemperatureFilename"));
    bool searchForTempFile = getProperty("SearchForTempFile");
    loader->setProperty("SearchForTempFile", searchForTempFile);

  } else {
    throw Kernel::Exception::FileError("Cannot open the file ", filePath);
  }

  // Set common input properties
  loader->setProperty("Filename", getPropertyValue("Filename"));
  loader->setProperty("OutputWorkspace", getPropertyValue("OutputWorkspace"));
  bool correctTime = getProperty("CorrectTime");
  loader->setProperty("CorrectTime", correctTime);
  if (!getPropertyValue("TimeZeroTable").empty())
    loader->setProperty("TimeZeroTable", getPropertyValue("TimeZeroTable"));
  if (!getPropertyValue("DeadTimeTable").empty())
    loader->setProperty("DeadTimeTable", getPropertyValue("DeadTimeTable"));

  // Exec loader
  loader->executeAsChildAlg();

  // Set common output properties
  Workspace_sptr ws = loader->getProperty("OutputWorkspace");
  setProperty("OutputWorkspace", ws);
  double timeZero = loader->getProperty("TimeZero");
  setProperty("TimeZero", timeZero);
  double firstGoodData = loader->getProperty("FirstGoodData");
  setProperty("FirstGoodData", firstGoodData);
  std::vector<double> timeZeroList = loader->getProperty("TimeZeroList");
  setProperty("TimeZeroList", timeZeroList);
  if (!getPropertyValue("TimeZeroTable").empty()) {
    Workspace_sptr table = loader->getProperty("TimeZeroTable");
    setProperty("TimeZeroTable", table);
  }
  if (!getPropertyValue("DeadTimeTable").empty()) {
    Workspace_sptr table = loader->getProperty("DeadTimeTable");
    setProperty("DeadTimeTable", table);
  }

  // PSI output properties
  if (PSIData) {
    // MainFieldDirection not used in PSI
    setProperty("MainFieldDirection", "");

    double lastGoodData = loader->getProperty("LastGoodData");
    setProperty("LastGoodData", lastGoodData);

    // PSI loader does not output a detector grouping table so make an empty one
    if (!getPropertyValue("DetectorGroupingTable").empty()) {
      Mantid::DataObjects::TableWorkspace_sptr detectorGroupingTable =
          std::dynamic_pointer_cast<Mantid::DataObjects::TableWorkspace>(
              Mantid::API::WorkspaceFactory::Instance().createTable("TableWorkspace"));
      detectorGroupingTable->addColumn("int", "Detectors");
      setProperty("DetectorGroupingTable", detectorGroupingTable);
    }
  }
  // Nexus output properties
  else {
    std::string field = loader->getProperty("MainFieldDirection");
    setProperty("MainFieldDirection", field);

    // Get detector grouping table from load alg
    if (!getPropertyValue("DetectorGroupingTable").empty()) {
      Workspace_sptr table = loader->getProperty("DetectorGroupingTable");
      setProperty("DetectorGroupingTable", table);
    }
  }
}

int LoadMuonData::confidence(Kernel::FileDescriptor &descriptor) const { return false; }
bool LoadMuonData::loadMutipleAsOne() { return false; }

} // namespace DataHandling
} // namespace Mantid
