// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataHandling/LoadMuonData.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/FileProperty.h"
#include "MantidDataHandling/LoadMuonNexus1.h"
#include "MantidDataHandling/LoadMuonNexus2.h"
#include "MantidDataHandling/LoadMuonNexusV2.h"
#include "MantidDataHandling/LoadPSIMuonBin.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/FileDescriptor.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/NexusDescriptor.h"
#include "MantidKernel/NexusHDF5Descriptor.h"
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
                  "will be automatically searched for if not provided.");
  declareProperty(std::make_unique<WorkspaceProperty<Workspace>>("OutputWorkspace", "", Direction::Output),
                  "The name of the workspace to be created as the output of the\n"
                  "algorithm. For multiperiod files, one workspace will be\n"
                  "generated for each period");
  declareProperty("SearchForTempFile", true,
                  "If no temp file has been given decide whether the algorithm "
                  "will search for the temperature file.");
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
  if (extension == "nxs" || extension == "nxs_v2") {
    // LoadMuonNexus
    std::cout << "Using LoadMuonNexus" << std::endl;
    IAlgorithm_sptr loadMuonNexusAlg = AlgorithmManager::Instance().create("LoadMuonNexus");
    loadMuonNexusAlg->setProperty("Filename", getPropertyValue("Filename"));
    loadMuonNexusAlg->setProperty("OutputWorkspace", getPropertyValue("OutputWorkspace"));
    loadMuonNexusAlg->executeAsChildAlg();
    setProperty("OutputWorkspace", loadMuonNexusAlg->getPropertyValue("OutputWorkspace"));
  } else if (extension == "bin") {
    // LoadPSIMuonBin
    std::cout << "Using LoadPSIMuonBin" << std::endl;
    IAlgorithm_sptr loadPSIMuonBinAlg = AlgorithmManager::Instance().create("LoadPSIMuonBin");
    loadPSIMuonBinAlg->setProperty("Filename", getPropertyValue("Filename"));
    loadPSIMuonBinAlg->setProperty("OutputWorkspace", getPropertyValue("OutputWorkspace"));
    loadPSIMuonBinAlg->execute();
    setProperty("OutputWorkspace", loadPSIMuonBinAlg->getPropertyValue("OutputWorkspace"));
  } else {
    throw Kernel::Exception::FileError("Cannot open the file ", filePath);
  }

  /*
  LoadMuonNexus1 loadMuonNexus;
  LoadMuonNexus2 loadMuonNexus2;
  LoadMuonNexusV2 loadMuonNexusV2;
  LoadPSIMuonBin loadPSIMuonBin;
  loadMuonNexus.initialize();
  loadMuonNexus2.initialize();
  loadPSIMuonBin.initialize();
  loadMuonNexusV2.initialize();

  std::string filePath = getPropertyValue("Filename");
  FileDescriptor fileDescriptor(filePath);
  int confidence1 = 0;
  int confidence2 = 0;
  int confidence3 = 0;
  int confidence4 = loadPSIMuonBin.confidence(fileDescriptor);
  if (Kernel::NexusDescriptor::isReadable(filePath, Kernel::NexusDescriptor::Version4)) {
    Kernel::NexusDescriptor descriptor(filePath);
    confidence1 = loadMuonNexus.confidence(descriptor);
    confidence2 = loadMuonNexus2.confidence(descriptor);
  }
  if (Kernel::NexusDescriptor::isReadable(filePath, Kernel::NexusDescriptor::Version5)) {
    Kernel::NexusHDF5Descriptor descriptorHDF5(filePath);
    confidence3 = loadMuonNexusV2.confidence(descriptorHDF5);
  };

  // None can load
  if (confidence1 < 80 && confidence2 < 80 && confidence3 < 80 && confidence4 < 80) {
    throw Kernel::Exception::FileError("Cannot open the file ", filePath);
  }

  if ((confidence1 > confidence3 && confidence1 > confidence4) ||
      (confidence2 > confidence3 && confidence2 > confidence4)) {
    // LoadMuonNexus
    std::cout << "using LoadMuonNexus" << std::endl;
    IAlgorithm_sptr loadMuonNexusAlg = AlgorithmManager::Instance().create("LoadMuonNexus");
    loadMuonNexusAlg->setProperty("Filename", getPropertyValue("Filename"));
    loadMuonNexusAlg->setProperty("OutputWorkspace", getPropertyValue("OutputWorkspace"));
    loadMuonNexusAlg->execute();
    setProperty("OutputWorkspace", loadMuonNexusAlg->getPropertyValue("OutputWorkspace"));
  } else if (confidence3 > confidence1 && confidence3 > confidence2 && confidence3 > confidence4){
    // LoadMuonNexusV2
    std::cout << "using LoadMuonNexusV2" << std::endl;
    IAlgorithm_sptr loadMuonNexusV2Alg = AlgorithmManager::Instance().create("LoadMuonNexusV2");
    loadMuonNexusV2Alg->setProperty("Filename", getPropertyValue("Filename"));
    loadMuonNexusV2Alg->setProperty("OutputWorkspace", getPropertyValue("OutputWorkspace"));
    loadMuonNexusV2Alg->execute();
    setProperty("OutputWorkspace", loadMuonNexusV2Alg->getPropertyValue("OutputWorkspace"));
  }
  else {
    // LoadPSIMuonBin
    std::cout << "using LoadPSIMuonBin" << std::endl;
    IAlgorithm_sptr loadPSIMuonBinAlg = AlgorithmManager::Instance().create("LoadPSIMuonBin");
    loadPSIMuonBinAlg->setProperty("Filename", getPropertyValue("Filename"));
    loadPSIMuonBinAlg->setProperty("OutputWorkspace", getPropertyValue("OutputWorkspace"));
    loadPSIMuonBinAlg->execute();
    setProperty("OutputWorkspace", loadPSIMuonBinAlg->getPropertyValue("OutputWorkspace"));
  }

  /*
  IAlgorithm_sptr loadMuonNexusAlg = AlgorithmManager::Instance().create("LoadMuonNexus");
  loadMuonNexusAlg->setProperty("Filename", getPropertyValue("Filename"));
  loadMuonNexusAlg->setProperty("OutputWorkspace", getPropertyValue("OutputWorkspace"));
  loadMuonNexusAlg->execute();
  setProperty("OutputWorkspace", loadMuonNexusAlg->getPropertyValue("OutputWorkspace"));*/

  /*auto childAlg = createChildAlgorithm("LoadMuonNexus", 0, 1, true, 2);
  auto loadMuonNexusAlg = std::dynamic_pointer_cast<API::Algorithm>(childAlg);
  loadMuonNexus->copyPropertiesFrom(*this);
  loadMuonNexus->execute();
  this->copyPropertiesFrom(*loadMuonNexus);*/

  /*auto childAlg = createChildAlgorithm("LoadPSIMuonBin", 0, 1, true, 1);
  auto loadMuonNexusAlg = std::dynamic_pointer_cast<API::Algorithm>(childAlg);
  loadMuonNexusAlg->copyPropertiesFrom(*this);
  loadMuonNexusAlg->execute();
  this->copyPropertiesFrom(*loadMuonNexusAlg);*/
}

} // namespace DataHandling
} // namespace Mantid