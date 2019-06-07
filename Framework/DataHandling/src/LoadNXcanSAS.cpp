// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataHandling/LoadNXcanSAS.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/FileFinder.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/NumericAxis.h"
#include "MantidAPI/Progress.h"
#include "MantidAPI/RegisterFileLoader.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/Workspace.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/WorkspaceUnitValidator.h"
#include "MantidDataHandling/H5Util.h"
#include "MantidDataHandling/NXcanSASDefinitions.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/UnitFactory.h"

#include <H5Cpp.h>
#include <Poco/DirectoryIterator.h>
#include <Poco/Path.h>
#include <nexus/NeXusFile.hpp>
#include <type_traits>

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::DataHandling::NXcanSAS;
using Mantid::HistogramData::HistogramE;
using Mantid::HistogramData::HistogramX;
using Mantid::HistogramData::HistogramY;

namespace {

Mantid::Kernel::Logger g_log("LoadNXcanSAS");

struct DataSpaceInformation {
  DataSpaceInformation(size_t dimSpectrumAxis = 0, size_t dimBin = 0)
      : dimSpectrumAxis(dimSpectrumAxis), dimBin(dimBin) {}
  size_t dimSpectrumAxis;
  size_t dimBin;
};

DataSpaceInformation getDataSpaceInfo(H5::DataSet &dataSet) {
  DataSpaceInformation dataSpaceInfo;
  auto dataSpace = dataSet.getSpace();
  const auto rank = dataSpace.getSimpleExtentNdims();
  if (rank > 2) {
    std::invalid_argument("LoadNXcanSAS: Cannot load a data set "
                          "with more than 2 dimensions.");
  }

  hsize_t dims[2] = {0, 0};
  dataSpace.getSimpleExtentDims(dims);
  // If dims[1] is 0, then the first entry is the number of data points
  // If dims[1] is not 0, then the second entry is the number of data points
  if (dims[1] == 0) {
    dims[1] = dims[0];
    dims[0] = 1; // One histogram
  }

  dataSpaceInfo.dimSpectrumAxis = dims[0];
  dataSpaceInfo.dimBin = dims[1];
  return dataSpaceInfo;
}

std::string getNameOfEntry(H5::H5File &root) {
  auto numberOfObjects = root.getNumObjs();
  if (numberOfObjects != 1) {
    throw std::invalid_argument("LoadNXcanSAS: Trying to load multiperiod "
                                "data. This is currently not supported.");
  }

  auto objectType = root.getObjTypeByIdx(0);
  if (objectType != H5G_GROUP) {
    throw std::invalid_argument(
        "LoadNXcanSAS: The object below the root is not a H5::Group.");
  }

  return root.getObjnameByIdx(0);
}

Mantid::API::MatrixWorkspace_sptr createWorkspace(H5::DataSet &dataSet) {
  auto dimInfo = getDataSpaceInfo(dataSet);

  // Create a workspace based on the dataSpace information
  return Mantid::API::WorkspaceFactory::Instance().create(
      "Workspace2D", dimInfo.dimSpectrumAxis /*NHisto*/,
      dimInfo.dimBin /*xdata*/, dimInfo.dimBin /*ydata*/);
}

Mantid::API::MatrixWorkspace_sptr
createWorkspaceForHistogram(H5::DataSet &dataSet) {
  auto dimInfo = getDataSpaceInfo(dataSet);

  // Create a workspace based on the dataSpace information
  return Mantid::API::WorkspaceFactory::Instance().create(
      "Workspace2D", dimInfo.dimSpectrumAxis /*NHisto*/,
      dimInfo.dimBin + 1 /*xdata*/, dimInfo.dimBin /*ydata*/);
}

// ----- LOGS

void loadLogs(H5::Group &entry, Mantid::API::MatrixWorkspace_sptr workspace) {
  auto &run = workspace->mutableRun();

  // Load UserFile (optional)
  auto process = entry.openGroup(sasProcessGroupName);
  auto userFile =
      Mantid::DataHandling::H5Util::readString(process, sasProcessTermUserFile);
  if (!userFile.empty()) {
    run.addLogData(
        new PropertyWithValue<std::string>(sasProcessUserFileInLogs, userFile));
  }

  // Load Run (optional)
  auto runNumber = Mantid::DataHandling::H5Util::readString(entry, sasEntryRun);
  if (!runNumber.empty()) {
    run.addLogData(
        new PropertyWithValue<std::string>(sasEntryRunInLogs, runNumber));
  }

  // Load Title (optional)
  auto title = Mantid::DataHandling::H5Util::readString(entry, sasEntryTitle);
  if (!title.empty()) {
    workspace->setTitle(title);
  }
}

// ----- INSTRUMENT
std::string extractIdfFileOnCurrentSystem(std::string idf) {
  // If the idf is is not empty extract the last element from the file
  if (idf.empty()) {
    return "";
  }

  // Get the specified IDF name
  Poco::Path path(idf);
  const auto &fileName = path.getFileName();

  // Compare against all available IDFs
  const std::vector<std::string> &directoryNames =
      Mantid::Kernel::ConfigService::Instance().getInstrumentDirectories();
  Poco::DirectoryIterator end_iter;
  for (const auto &directoryName : directoryNames) {
    for (Poco::DirectoryIterator dir_itr(directoryName); dir_itr != end_iter;
         ++dir_itr) {
      if (Poco::File(dir_itr->path()).isFile()) {
        if (fileName == Poco::Path(dir_itr->path()).getFileName()) {
          return Poco::Path(dir_itr->path()).absolute().toString();
        }
      }
    }
  }
  return "";
}

void loadInstrument(H5::Group &entry,
                    Mantid::API::MatrixWorkspace_sptr workspace) {
  auto instrument = entry.openGroup(sasInstrumentGroupName);

  // Get instrument name
  auto instrumentName =
      Mantid::DataHandling::H5Util::readString(instrument, sasInstrumentName);
  if (instrumentName.empty()) {
    return;
  }

  // Get IDF
  auto idf =
      Mantid::DataHandling::H5Util::readString(instrument, sasInstrumentIDF);
  idf = extractIdfFileOnCurrentSystem(idf);

  // Try to load the instrument. If it fails we will continue nevertheless.
  try {
    auto instAlg = Mantid::API::AlgorithmManager::Instance().createUnmanaged(
        "LoadInstrument");
    instAlg->initialize();
    instAlg->setChild(true);
    instAlg->setProperty("Workspace", workspace);
    instAlg->setProperty("InstrumentName", instrumentName);
    if (!idf.empty()) {
      instAlg->setProperty("Filename", idf);
    }
    instAlg->setProperty("RewriteSpectraMap", "False");
    instAlg->execute();
  } catch (std::invalid_argument &) {
    g_log.information("Invalid argument to LoadInstrument Child Algorithm.");
  } catch (std::runtime_error &) {
    g_log.information(
        "Unable to successfully run LoadInstrument Child Algorithm.");
  }
}

// ----- DATA
WorkspaceDimensionality getWorkspaceDimensionality(H5::Group &dataGroup) {
  WorkspaceDimensionality dimensionality(WorkspaceDimensionality::other);
  auto intensity = dataGroup.openDataSet(sasDataI);
  auto dataSpaceInfo = getDataSpaceInfo(intensity);

  if (dataSpaceInfo.dimSpectrumAxis == 1 && dataSpaceInfo.dimBin > 0) {
    dimensionality = WorkspaceDimensionality::oneD;
  } else if (dataSpaceInfo.dimSpectrumAxis > 1 && dataSpaceInfo.dimBin > 0) {
    dimensionality = WorkspaceDimensionality::twoD;
  } else {
    dimensionality = WorkspaceDimensionality::other;
  }
  return dimensionality;
}

std::string getUnit(H5::DataSet &dataSet) {
  return Mantid::DataHandling::H5Util::readAttributeAsString(dataSet,
                                                             sasUnitAttr);
}

bool hasQDev(H5::Group &dataGroup) {
  bool hasQDev(true);
  try {
    dataGroup.openDataSet(sasDataQdev);
  } catch (H5::GroupIException &) {
    hasQDev = false;
  } catch (H5::FileIException &) {
    hasQDev = false;
  }

  return hasQDev;
}

void loadData1D(H5::Group &dataGroup,
                Mantid::API::MatrixWorkspace_sptr workspace) {
  // General
  workspace->setDistribution(true);

  // Load the Q value
  workspace->mutableX(0) =
      Mantid::DataHandling::H5Util::readArray1DCoerce<double>(dataGroup,
                                                              sasDataQ);
  workspace->getAxis(0)->setUnit("MomentumTransfer");

  // Load the I value + units
  workspace->mutableY(0) =
      Mantid::DataHandling::H5Util::readArray1DCoerce<double>(dataGroup,
                                                              sasDataI);

  auto iDataSet = dataGroup.openDataSet(sasDataI);
  auto yUnit = getUnit(iDataSet);
  workspace->setYUnit(yUnit);

  // Load the Idev value
  workspace->mutableE(0) =
      Mantid::DataHandling::H5Util::readArray1DCoerce<double>(dataGroup,
                                                              sasDataIdev);

  // Load the Qdev value (optional)
  bool hasQResolution = hasQDev(dataGroup);
  if (hasQResolution) {
    workspace->setPointStandardDeviations(
        0, Mantid::DataHandling::H5Util::readArray1DCoerce<double>(
               dataGroup, sasDataQdev));
  }
}

template <typename Functor>
void read2DWorkspace(H5::DataSet &dataSet,
                     Mantid::API::MatrixWorkspace_sptr workspace, Functor func,
                     H5::DataType memoryDataType) {
  using namespace Mantid::DataHandling::NXcanSAS;
  auto dimInfo = getDataSpaceInfo(dataSet);

  // File Data Space
  auto fileSpace = dataSet.getSpace();

  // Size of single slab
  const hsize_t rank = 2;
  hsize_t sizeOfSingleSlab[rank] = {1, dimInfo.dimBin};
  hsize_t start[rank] = {0, 0};

  // Memory Data Space
  hsize_t memSpaceDimension[1] = {dimInfo.dimBin};
  H5::DataSpace memSpace(1, memSpaceDimension);

  auto &dataHist = func(workspace, 0);
  Mantid::MantidVec data(dataHist.size());
  for (size_t index = 0; index < dimInfo.dimSpectrumAxis; ++index) {
    // Set the dataSpace to a 1D HyperSlab
    fileSpace.selectHyperslab(H5S_SELECT_SET, sizeOfSingleSlab, start);
    dataSet.read(data.data(), memoryDataType, memSpace, fileSpace);
    auto &dat = func(workspace, index);
    dat = data;
    ++start[0];
  }
}

void readQyInto2DWorkspace(H5::DataSet &dataSet,
                           Mantid::API::MatrixWorkspace_sptr workspace,
                           H5::DataType memoryDataType) {
  using namespace Mantid::DataHandling::NXcanSAS;

  // Get info about the data set
  auto dimInfo = getDataSpaceInfo(dataSet);

  // If axis 1 is spectra axis we need to convert it to numeric axes.
  if (workspace->getAxis(1)->isSpectra()) {
    auto const newAxis = new Mantid::API::NumericAxis(dimInfo.dimSpectrumAxis);
    workspace->replaceAxis(1, newAxis);
  }

  // File Data Space
  auto fileSpace = dataSet.getSpace();

  // Size of single slab
  const hsize_t rank = 2;
  hsize_t sizeOfSingleSlab[rank] = {dimInfo.dimSpectrumAxis, 1};
  hsize_t start[rank] = {0, 0};

  // Memory Data Space
  hsize_t memSpaceDimension[1] = {dimInfo.dimSpectrumAxis};
  H5::DataSpace memSpace(1, memSpaceDimension);

  // Select the HyperSlab
  fileSpace.selectHyperslab(H5S_SELECT_SET, sizeOfSingleSlab, start);

  // Read the data
  Mantid::MantidVec data;
  data.resize(dimInfo.dimSpectrumAxis);
  dataSet.read(data.data(), memoryDataType, memSpace, fileSpace);

  auto axis1 = workspace->getAxis(1);
  for (size_t index = 0; index < dimInfo.dimSpectrumAxis; ++index) {
    axis1->setValue(index, data[index]);
  }

  // Set the axis units
  axis1->unit() =
      Mantid::Kernel::UnitFactory::Instance().create("MomentumTransfer");
}

void loadData2D(H5::Group &dataGroup,
                Mantid::API::MatrixWorkspace_sptr workspace) {
  // General
  workspace->setDistribution(true);
  //-----------------------------------------
  // Load the I value.
  auto iDataSet = dataGroup.openDataSet(sasDataI);
  auto iExtractor = [](Mantid::API::MatrixWorkspace_sptr ws,
                       size_t index) -> HistogramY & {
    return ws->mutableY(index);
  };
  auto iDataType = Mantid::DataHandling::H5Util::getType<double>();
  read2DWorkspace(iDataSet, workspace, iExtractor, iDataType);
  auto yUnit = getUnit(iDataSet);
  workspace->setYUnit(yUnit);

  //-----------------------------------------
  // Load the Idev value
  auto eDataSet = dataGroup.openDataSet(sasDataIdev);
  auto eExtractor = [](Mantid::API::MatrixWorkspace_sptr ws,
                       size_t index) -> HistogramE & {
    return ws->mutableE(index);
  };
  read2DWorkspace(eDataSet, workspace, eExtractor, iDataType);

  //-----------------------------------------
  // Load the Qx value + units
  auto qxDataSet = dataGroup.openDataSet(sasDataQx);
  auto qxExtractor = [](Mantid::API::MatrixWorkspace_sptr ws,
                        size_t index) -> HistogramX & {
    return ws->mutableX(index);
  };
  auto qxDataType = Mantid::DataHandling::H5Util::getType<double>();
  read2DWorkspace(qxDataSet, workspace, qxExtractor, qxDataType);
  workspace->getAxis(0)->setUnit("MomentumTransfer");

  //-----------------------------------------
  // Load the Qy value
  auto qyDataSet = dataGroup.openDataSet(sasDataQy);
  auto qyDataType = Mantid::DataHandling::H5Util::getType<double>();
  readQyInto2DWorkspace(qyDataSet, workspace, qyDataType);
}

void loadData(H5::Group &entry, Mantid::API::MatrixWorkspace_sptr workspace) {
  auto dataGroup = entry.openGroup(sasDataGroupName);
  auto dimensionality = getWorkspaceDimensionality(dataGroup);
  switch (dimensionality) {
  case (WorkspaceDimensionality::oneD):
    loadData1D(dataGroup, workspace);
    break;
  case (WorkspaceDimensionality::twoD):
    loadData2D(dataGroup, workspace);
    break;
  default:
    throw std::invalid_argument(
        "LoadNXcanSAS: Cannot load workspace which not 1D or 2D.");
  }
}

bool findDefinition(::NeXus::File &file) {
  bool foundDefinition = false;
  auto entries = file.getEntries();

  for (auto &entry : entries) {
    if (entry.second == sasEntryClassAttr || entry.second == nxEntryClassAttr) {
      file.openGroup(entry.first, entry.second);
      file.openData(sasEntryDefinition);
      auto definitionFromFile = file.getStrData();
      if (definitionFromFile == sasEntryDefinitionFormat) {
        foundDefinition = true;
        break;
      }
      file.closeData();
      file.closeGroup();
    }
  }
  return foundDefinition;
}

// --- TRANSMISSION
bool hasTransmissionEntry(H5::Group &entry, const std::string &name) {
  bool hasTransmission(false);
  try {
    entry.openGroup(sasTransmissionSpectrumGroupName + "_" + name);
    hasTransmission = true;
  } catch (H5::GroupIException &) {
  } catch (H5::FileIException &) {
  }
  return hasTransmission;
}

void loadTransmissionData(H5::Group &transmission,
                          Mantid::API::MatrixWorkspace_sptr workspace) {
  //-----------------------------------------
  // Load T
  workspace->mutableY(0) =
      Mantid::DataHandling::H5Util::readArray1DCoerce<double>(
          transmission, sasTransmissionSpectrumT);
  //-----------------------------------------
  // Load Tdev
  workspace->mutableE(0) =
      Mantid::DataHandling::H5Util::readArray1DCoerce<double>(
          transmission, sasTransmissionSpectrumTdev);
  //-----------------------------------------
  // Load Lambda
  workspace->setPoints(0,
                       Mantid::DataHandling::H5Util::readArray1DCoerce<double>(
                           transmission, sasTransmissionSpectrumLambda));

  workspace->getAxis(0)->unit() =
      Mantid::Kernel::UnitFactory::Instance().create("Wavelength");
}
} // namespace

namespace Mantid {
namespace DataHandling {

// Register the algorithm into the AlgorithmFactory
DECLARE_NEXUS_FILELOADER_ALGORITHM(LoadNXcanSAS)

/// constructor
LoadNXcanSAS::LoadNXcanSAS() {}

int LoadNXcanSAS::confidence(Kernel::NexusDescriptor &descriptor) const {
  const std::string &extn = descriptor.extension();
  if (extn != ".nxs" && extn != ".h5") {
    return 0;
  }

  int confidence(0);

  ::NeXus::File &file = descriptor.data();
  // Check if there is an entry root/SASentry/definition->NXcanSAS
  try {
    bool foundDefinition = findDefinition(file);
    if (foundDefinition) {
      confidence = 95;
    }
  } catch (...) {
  }

  return confidence;
}

void LoadNXcanSAS::init() {
  // Declare required input parameters for algorithm
  const std::vector<std::string> exts{".nxs", ".h5"};
  declareProperty(
      std::make_unique<Mantid::API::FileProperty>("Filename", "",
                                                  FileProperty::Load, exts),
      "The name of the NXcanSAS file to read, as a full or relative path.");
  declareProperty(
      std::make_unique<Mantid::API::WorkspaceProperty<Mantid::API::Workspace>>(
          "OutputWorkspace", "", Direction::Output),
      "The name of the workspace to be created as the output of "
      "the algorithm.  A workspace of this name will be created "
      "and stored in the Analysis Data Service. For multiperiod "
      "files, one workspace may be generated for each period. "
      "Currently only one workspace can be saved at a time so "
      "multiperiod Mantid files are not generated.");

  declareProperty(
      std::make_unique<PropertyWithValue<bool>>("LoadTransmission", false,
                                                Direction::Input),
      "Load the transmission related data from the file if it is present "
      "(optional, default False).");
}

void LoadNXcanSAS::exec() {
  const std::string fileName = getPropertyValue("Filename");
  const bool loadTransmissions = getProperty("LoadTransmission");
  H5::H5File file(fileName, H5F_ACC_RDONLY);

  // Setup progress bar
  const int numberOfSteps = loadTransmissions ? 4 : 3;
  Progress progress(this, 0.1, 1.0, numberOfSteps);

  auto entryName = getNameOfEntry(file);
  auto entry = file.openGroup(entryName);

  // Create the output workspace
  auto dataGroup = entry.openGroup(sasDataGroupName);
  auto intensity = dataGroup.openDataSet(sasDataI);
  auto ws = createWorkspace(intensity);

  // Load the logs
  progress.report("Loading logs.");
  loadLogs(entry, ws);

  // Load instrument
  progress.report("Loading instrument.");
  loadInstrument(entry, ws);

  // Load data
  progress.report("Loading data.");
  loadData(entry, ws);

  // Load Transmissions

  if (loadTransmissions) {
    progress.report("Loading transmissions.");
    // Load sample transmission
    loadTransmission(entry, sasTransmissionSpectrumNameSampleAttrValue);

    // Load can transmission
    loadTransmission(entry, sasTransmissionSpectrumNameCanAttrValue);
  }
  file.close();
  setProperty("OutputWorkspace", ws);
}

void LoadNXcanSAS::loadTransmission(H5::Group &entry, const std::string &name) {
  if (!hasTransmissionEntry(entry, name)) {
    g_log.information("NXcanSAS file does not contain transmission for " +
                      name);
    return;
  }

  auto transmission =
      entry.openGroup(sasTransmissionSpectrumGroupName + "_" + name);

  // Create a 1D workspace
  auto tDataSet = transmission.openDataSet(sasTransmissionSpectrumT);
  auto workspace = createWorkspaceForHistogram(tDataSet);

  // Load logs
  loadLogs(entry, workspace);
  auto title = workspace->getTitle();
  const std::string transExtension = "_trans_" + name;
  title += transExtension;
  workspace->setTitle(transExtension);

  // Load instrument
  loadInstrument(entry, workspace);

  // Load transmission data
  loadTransmissionData(transmission, workspace);

  // Set to distribution
  workspace->setDistribution(true);
  workspace->setYUnitLabel("Transmission");

  // Set the workspace on the output

  std::string propertyName;
  if (name == "sample")
    propertyName = "TransmissionWorkspace";
  else
    propertyName = "TransmissionCanWorkspace";
  const std::string doc = "The transmission workspace";

  declareProperty(
      std::make_unique<
          Mantid::API::WorkspaceProperty<Mantid::API::MatrixWorkspace>>(
          propertyName, title, Direction::Output),
      doc);
  setProperty(propertyName, workspace);
}

} // namespace DataHandling
} // namespace Mantid
