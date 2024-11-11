// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataHandling/LoadMcStas.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/IEventWorkspace.h"
#include "MantidAPI/InstrumentDataService.h"
#include "MantidAPI/NumericAxis.h"
#include "MantidAPI/RegisterFileLoader.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidDataHandling/H5Util.h"
#include "MantidDataHandling/LoadEventNexus.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/InstrumentDefinitionParser.h"
#include "MantidKernel/RegexStrings.h"
#include "MantidKernel/Strings.h"
#include "MantidKernel/Unit.h"
#include "MantidKernel/UnitFactory.h"

#include <H5Cpp.h>
#include <boost/algorithm/string.hpp>

namespace Mantid::DataHandling {
using namespace Kernel;
using namespace API;
using namespace DataObjects;

// Register the algorithm into the AlgorithmFactory
DECLARE_NEXUS_HDF5_FILELOADER_ALGORITHM(LoadMcStas)

//----------------------------------------------------------------------------------------------
// Algorithm's name for identification. @see Algorithm::name
const std::string LoadMcStas::name() const { return "LoadMcStas"; }

// Algorithm's version for identification. @see Algorithm::version
int LoadMcStas::version() const { return 1; }

// Algorithm's category for identification. @see Algorithm::category
const std::string LoadMcStas::category() const { return "DataHandling\\Nexus"; }

//----------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void LoadMcStas::init() {
  const std::vector<std::string> exts{".h5", ".nxs"};
  declareProperty(std::make_unique<FileProperty>("Filename", "", FileProperty::Load, exts),
                  "The name of the Nexus file to load");

  declareProperty(std::make_unique<WorkspaceProperty<Workspace>>("OutputWorkspace", "", Direction::Output),
                  "An output workspace.");

  declareProperty("ErrorBarsSetTo1", false,
                  "When this property is set to false errors are set equal to data values, "
                  "and when set to true all errors are set equal to one. This property "
                  "defaults to false");

  declareProperty("OutputOnlySummedEventWorkspace", true,
                  "When true the algorithm only outputs the sum of all event data into "
                  "one eventworkspace EventData + _ + name of the OutputWorkspace. "
                  "If false eventworkspaces are also returned for each individual "
                  "McStas components storing event data");
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void LoadMcStas::execLoader() {
  std::string filename = getPropertyValue("Filename");
  H5::H5File file(filename, H5F_ACC_RDONLY);

  auto const &descriptor = getFileInfo();
  auto const &allEntries = descriptor->getAllEntries();

  auto const iterSDS = allEntries.find("SDS");
  if (iterSDS == allEntries.cend()) {
    throw std::runtime_error("Could not find any entries.");
  }
  auto const &entries = iterSDS->second;

  const char *attributeName = "long_name";
  std::vector<std::string> eventEntries;
  std::map<std::string, std::vector<std::string>> histogramEntries;
  for (auto &entry : entries) {
    if (entry.find("/entry1/data") == std::string::npos) {
      continue;
    }

    const auto parts = Strings::StrParts(entry, boost::regex("/"));
    const auto groupPath = "/" + Strings::join(parts.cbegin(), parts.cend() - 1, "/");
    const auto groupName = *(parts.cend() - 2);
    const auto datasetName = parts.back();

    if (groupName == "content_nxs")
      continue;

    const H5::Group group = file.openGroup(groupPath);
    const H5::DataSet dataset = group.openDataSet(datasetName);

    if (!H5Util::hasAttribute(dataset, attributeName)) {
      continue;
    }

    const auto rawString = H5Util::readAttributeAsStrType<char *>(dataset, attributeName);
    if (std::strstr(rawString, "Neutron_ID")) {
      eventEntries.emplace_back(groupPath);
    } else {
      auto foundIt = histogramEntries.find(groupPath);
      if (foundIt == histogramEntries.cend()) {
        histogramEntries[groupPath] = {datasetName};
      } else {
        histogramEntries[groupPath].emplace_back(datasetName);
      }
    }
  }

  std::vector<std::string> scatteringWSNames;
  if (!eventEntries.empty()) {
    scatteringWSNames = readEventData(eventEntries, file);
  }
  const auto histoWSNames = readHistogramData(histogramEntries, file);
  file.close();

  // join two vectors together
  scatteringWSNames.insert(scatteringWSNames.end(), histoWSNames.cbegin(), histoWSNames.cend());

  setProperty("OutputWorkspace", groupWorkspaces(scatteringWSNames));
}

/**
 * Group workspaces
 * @param workspaces workspace names to group
 * @return Workspace group
 */
API::WorkspaceGroup_sptr LoadMcStas::groupWorkspaces(const std::vector<std::string> &workspaces) const {
  auto groupAlgorithm = API::AlgorithmManager::Instance().createUnmanaged("GroupWorkspaces");
  groupAlgorithm->setChild(true);
  groupAlgorithm->setLogging(false);
  groupAlgorithm->initialize();
  groupAlgorithm->setProperty("InputWorkspaces", workspaces);
  groupAlgorithm->setProperty("OutputWorkspace", "__grouped");
  groupAlgorithm->execute();
  return groupAlgorithm->getProperty("OutputWorkspace");
}

/**
 * Read Event Data
 * @param eventEntries map of the file entries that have events
 * @param file Reads data from inside first top entry
 * @return Names of workspaces to include in the output group
 */
std::vector<std::string> LoadMcStas::readEventData(const std::vector<std::string> &eventEntries,
                                                   const H5::H5File &file) {

  // vector to store output workspaces
  std::vector<std::string> scatteringWSNames;

  std::string filename = getPropertyValue("Filename");
  const bool errorBarsSetTo1 = getProperty("ErrorBarsSetTo1");

  Geometry::Instrument_sptr instrument;

  // Initialize progress reporting
  int reports = 2;
  const double progressFractionInitial = 0.1;
  Progress progInitial(this, 0.0, progressFractionInitial, reports);

  std::string instrumentXML;
  progInitial.report("Loading instrument");
  try {
    const H5::Group group = file.openGroup("/entry1/instrument/instrument_xml");
    const H5::DataSet dataset = group.openDataSet("data");
    instrumentXML = H5Util::readString(dataset);
  } catch (...) {
    g_log.warning() << "\nCould not find the instrument description in the Nexus file:" << filename
                    << " Ignore eventdata from the Nexus file\n";
    return scatteringWSNames;
    ;
  }

  try {
    std::string instrumentName = "McStas";
    Geometry::InstrumentDefinitionParser parser(filename, instrumentName, instrumentXML);
    std::string instrumentNameMangled = parser.getMangledName();

    // Check whether the instrument is already in the InstrumentDataService
    if (InstrumentDataService::Instance().doesExist(instrumentNameMangled)) {
      // If it does, just use the one from the one stored there
      instrument = InstrumentDataService::Instance().retrieve(instrumentNameMangled);
    } else {
      // Really create the instrument
      instrument = parser.parseXML(nullptr);
      // Add to data service for later retrieval
      InstrumentDataService::Instance().add(instrumentNameMangled, instrument);
    }
  } catch (Exception::InstrumentDefinitionError &e) {
    g_log.warning() << "When trying to read the instrument description in the Nexus file: " << filename
                    << " the following error is reported: " << e.what() << " Ignore eventdata from the Nexus file\n";
    return scatteringWSNames;
    ;
  } catch (...) {
    g_log.warning() << "Could not parse instrument description in the Nexus file: " << filename
                    << " Ignore eventdata from the Nexus file\n";
    return scatteringWSNames;
    ;
  }

  // create and prepare an event workspace ready to receive the mcstas events
  progInitial.report("Set up EventWorkspace");
  EventWorkspace_sptr eventWS(new EventWorkspace());
  // initialize, where create up front number of eventlists = number of
  // detectors
  eventWS->initialize(instrument->getNumberDetectors(), 1, 1);
  // Set the units
  eventWS->getAxis(0)->unit() = UnitFactory::Instance().create("TOF");
  eventWS->setYUnit("Counts");
  // set the instrument
  eventWS->setInstrument(instrument);
  // assign detector ID to eventlists

  std::vector<detid_t> detIDs = instrument->getDetectorIDs();

  for (size_t i = 0; i < instrument->getNumberDetectors(); i++) {
    eventWS->getSpectrum(i).addDetectorID(detIDs[i]);
    // spectrum number are treated as equal to detector IDs for McStas data
    eventWS->getSpectrum(i).setSpectrumNo(detIDs[i]);
  }
  // the one is here for the moment for backward compatibility
  eventWS->rebuildSpectraMapping(true);
  const auto detIDtoWSIndex = eventWS->getDetectorIDToWorkspaceIndexMap(true);

  bool isAnyNeutrons = false;
  // to store shortest and longest recorded TOF
  double shortestTOF(0.0);
  double longestTOF(0.0);

  // create vector container all the event output workspaces needed
  const size_t numEventEntries = eventEntries.size();
  std::string nameOfGroupWS = getProperty("OutputWorkspace");
  const auto eventDataTotalName = "EventData_" + nameOfGroupWS;
  std::vector<std::pair<EventWorkspace_sptr, std::string>> allEventWS = {{eventWS, eventDataTotalName}};
  // if numEventEntries > 1 also create separate event workspaces
  const bool onlySummedEventWorkspace = getProperty("OutputOnlySummedEventWorkspace");
  if (!onlySummedEventWorkspace && numEventEntries > 1) {
    for (const auto &eventEntry : eventEntries) {
      const auto parts = Strings::StrParts(eventEntry, boost::regex("/"));
      const auto groupName = parts.back();
      // create container to hold partial event data
      // plus the name users will see for it
      const auto ws_name = groupName + "_" + nameOfGroupWS;
      allEventWS.emplace_back(eventWS->clone(), ws_name);
    }
  }

  Progress progEntries(this, progressFractionInitial, 1.0, numEventEntries * 2);

  // Refer to entry in allEventWS. The first non-summed workspace index is 1
  auto eventWSIndex = 1u;
  // Loop over McStas event data components
  for (const auto &groupPath : eventEntries) {
    const H5::Group group = file.openGroup(groupPath);
    const H5::DataSet dataset = group.openDataSet("events");

    // open second level entry
    std::vector<double> data;
    progEntries.report("read event data from nexus");

    // Need to take into account that the nexus readData method reads a
    // multi-column data entry
    // into a vector
    // The number of data column for each neutron is here hardcoded to (p, x,
    // y,  n, id, t)
    // Thus  we have
    // column  0 : p 	neutron wight
    // column  1 : x 	x coordinate
    // column  2 : y 	y coordinate
    // column  3 : n 	accumulated number of neutrons
    // column  4 : id 	pixel id
    // column  5 : t 	time

    // get info about event data
    const H5::DataSpace dataspace = dataset.getSpace();
    const auto rank = dataspace.getSimpleExtentNdims();

    std::vector<hsize_t> dims(rank);
    dataspace.getSimpleExtentDims(dims.data());
    if (dims.size() != 2) {
      g_log.error() << "Event data in McStas nexus file not loaded. Expected "
                       "event data block to be two dimensional\n";
      return scatteringWSNames;
    }
    hsize_t nNeutrons = dims[0];
    hsize_t numberOfDataColumn = dims[1];
    if (nNeutrons && numberOfDataColumn != 6) {
      g_log.error() << "Event data in McStas nexus file expecting 6 columns\n";
      return scatteringWSNames;
      ;
    }
    if (!isAnyNeutrons && nNeutrons > 0)
      isAnyNeutrons = true;

    hsize_t start[2];
    hsize_t step[2];

    // read the event data in blocks. 1 million event is 1000000*6*8 doubles
    // about 50Mb
    hsize_t nNeutronsInBlock = 1000000;
    hsize_t nOfFullBlocks = nNeutrons / nNeutronsInBlock;
    hsize_t nRemainingNeutrons = nNeutrons - nOfFullBlocks * nNeutronsInBlock;
    // sum over number of blocks + 1 to cover the remainder
    for (hsize_t iBlock = 0; iBlock < nOfFullBlocks + 1; iBlock++) {
      if (iBlock == nOfFullBlocks) {
        // read remaining neutrons
        start[0] = nOfFullBlocks * nNeutronsInBlock;
        start[1] = 0;
        step[0] = nRemainingNeutrons;
        step[1] = numberOfDataColumn;
      } else {
        // read neutrons in a full block
        start[0] = iBlock * nNeutronsInBlock;
        start[1] = 0;
        step[0] = nNeutronsInBlock;
        step[1] = numberOfDataColumn;
      }
      const hsize_t nNeutronsForthisBlock = step[0]; // number of neutrons read for this block
      data.resize(nNeutronsForthisBlock * numberOfDataColumn);

      // Check that the type is what it is supposed to be
      const H5::DataType datatype = dataset.getDataType();
      if (datatype.getClass() != H5T_FLOAT) {
        g_log.warning() << "Entry event field is not H5T_FLOAT! It will be skipped.\n";
        continue;
      }

      H5::DataSpace memspace(rank, step);
      dataspace.selectHyperslab(H5S_SELECT_SET, step, start);

      dataset.read(data.data(), H5::PredType::NATIVE_DOUBLE, memspace, dataspace);

      // populate workspace with McStas events
      progEntries.report("read event data into workspace");
      for (hsize_t in = 0; in < nNeutronsForthisBlock; in++) {
        const auto detectorID = static_cast<int>(data[4 + numberOfDataColumn * in]);
        const double detector_time = data[5 + numberOfDataColumn * in] * 1.0e6; // convert to microseconds
        if (in == 0 && iBlock == 0) {
          shortestTOF = detector_time;
          longestTOF = detector_time;
        } else {
          if (detector_time < shortestTOF)
            shortestTOF = detector_time;
          if (detector_time > longestTOF)
            longestTOF = detector_time;
        }

        const size_t workspaceIndex = detIDtoWSIndex.find(detectorID)->second;

        int64_t pulse_time = 0;
        WeightedEvent weightedEvent;
        if (errorBarsSetTo1) {
          weightedEvent = WeightedEvent(detector_time, pulse_time, data[numberOfDataColumn * in], 1.0);
        } else {
          weightedEvent = WeightedEvent(detector_time, pulse_time, data[numberOfDataColumn * in],
                                        data[numberOfDataColumn * in] * data[numberOfDataColumn * in]);
        }
        allEventWS[0].first->getSpectrum(workspaceIndex) += weightedEvent;
        if (!onlySummedEventWorkspace && numEventEntries > 1) {
          allEventWS[eventWSIndex].first->getSpectrum(workspaceIndex) += weightedEvent;
        }
      }
      eventWSIndex++;
    } // end reading over number of blocks of an event dataset
  } // end reading over number of event datasets

  // Create a default TOF-vector for histogramming, for now just 2 bins
  // 2 bins is the standard. However for McStas simulation data it may make
  // sense to
  // increase this number for better initial visual effect

  auto axis = HistogramData::BinEdges{shortestTOF - 1, longestTOF + 1};

  // ensure that specified name is given to workspace (eventWS) when added to
  // outputGroup
  for (const auto &wsAndName : allEventWS) {
    const auto ws = wsAndName.first;
    ws->setAllX(axis);
    AnalysisDataService::Instance().addOrReplace(wsAndName.second, ws);
    scatteringWSNames.emplace_back(wsAndName.second);
  }
  return scatteringWSNames;
}

/**
 * Read histogram data
 * @param histogramEntries map of the file entries that have histogram
 * @param nxFile Reads data from inside first first top entry
 * @return Names of workspaces to include in output group
 */
std::vector<std::string>
LoadMcStas::readHistogramData(const std::map<std::string, std::vector<std::string>> &histogramEntries,
                              const H5::H5File &file) {

  std::string nameAttrValueYLABEL;
  std::vector<std::string> histoWSNames;

  for (const auto &entry : histogramEntries) {
    const auto groupPath = entry.first;
    const H5::Group group = file.openGroup(groupPath);

    std::string nameAttrValueTITLE(H5Util::readAttributeAsStrType<char *>(group, "filename"));

    if (H5Util::hasAttribute(group, "ylabel")) {
      nameAttrValueYLABEL = std::string(H5Util::readAttributeAsStrType<char *>(group, "ylabel"));
    }

    // Find the axis names
    std::string axis1Name, axis2Name;
    for (const auto &datasetName : entry.second) {
      if (datasetName == "ncount")
        continue;
      H5::DataSet dataset = group.openDataSet(datasetName);

      if (H5Util::hasAttribute(dataset, "axis")) {
        const auto axisNo = H5Util::readNumAttributeCoerce<int>(dataset, "axis");
        if (axisNo == 1)
          axis1Name = datasetName;
        else if (axisNo == 2)
          axis2Name = datasetName;
        else
          throw std::invalid_argument("Unknown axis number");
      }
    }

    std::vector<double> axis1Values;
    H5Util::readArray1DCoerce(group, axis1Name, axis1Values);
    std::vector<double> axis2Values;

    if (axis2Name.length() == 0) {
      axis2Name = nameAttrValueYLABEL;
      axis2Values.emplace_back(0.0);
    } else {
      H5Util::readArray1DCoerce(group, axis2Name, axis2Values);
    }

    const size_t axis1Length = axis1Values.size();
    const size_t axis2Length = axis2Values.size();
    g_log.debug() << "Axis lengths=" << axis1Length << " " << axis2Length << '\n';

    // Require "data" field
    std::vector<double> data;
    H5Util::readArray1DCoerce(group, "data", data);

    // Optional errors field
    std::vector<double> errors;
    if (group.exists("errors")) {
      H5Util::readArray1DCoerce(group, "errors", errors);
    }

    MatrixWorkspace_sptr ws = WorkspaceFactory::Instance().create("Workspace2D", axis2Length, axis1Length, axis1Length);
    Axis *axis1 = ws->getAxis(0);
    axis1->title() = axis1Name;
    // Set caption
    auto lblUnit = std::make_shared<Units::Label>();
    lblUnit->setLabel(axis1Name, "");
    axis1->unit() = lblUnit;

    auto axis2 = std::make_unique<NumericAxis>(axis2Length);
    auto axis2Raw = axis2.get();
    axis2->title() = axis2Name;
    // Set caption
    lblUnit = std::make_shared<Units::Label>();
    lblUnit->setLabel(axis2Name, "");
    axis2->unit() = lblUnit;

    ws->setYUnit(axis2Name);
    ws->replaceAxis(1, std::move(axis2));

    for (size_t wsIndex = 0; wsIndex < axis2Length; ++wsIndex) {
      auto &dataX = ws->mutableX(wsIndex);
      auto &dataY = ws->mutableY(wsIndex);
      auto &dataE = ws->mutableE(wsIndex);

      for (size_t j = 0; j < axis1Length; ++j) {
        // Data is stored in column-major order so we are translating to
        // row major for Mantid
        const size_t fileDataIndex = j * axis2Length + wsIndex;

        dataX[j] = axis1Values[j];
        dataY[j] = data[fileDataIndex];
        if (!errors.empty())
          dataE[j] = errors[fileDataIndex];
      }
      axis2Raw->setValue(wsIndex, axis2Values[wsIndex]);
    }

    // set the workspace title
    ws->setTitle(nameAttrValueTITLE);

    // use the workspace title to create the workspace name
    std::replace(nameAttrValueTITLE.begin(), nameAttrValueTITLE.end(), ' ', '_');

    // ensure that specified name is given to workspace (eventWS) when added to
    // outputGroup
    const std::string outputWS = getProperty("OutputWorkspace");
    const std::string nameUserSee = nameAttrValueTITLE + "_" + outputWS;
    AnalysisDataService::Instance().addOrReplace(nameUserSee, ws);

    histoWSNames.emplace_back(ws->getName());
  }
  return histoWSNames;
}

/**
 * Return the confidence with with this algorithm can load the file
 * @param descriptor A descriptor for the file
 * @return An integer specifying the confidence level. 0 indicates it will not
 * be used
 */
int LoadMcStas::confidence(Kernel::NexusHDF5Descriptor &descriptor) const {
  if (!descriptor.isEntry("/entry1/simulation/name")) {
    return 0;
  }
  H5::H5File file(descriptor.getFilename(), H5F_ACC_RDONLY);
  H5::Group group = file.openGroup("/entry1/simulation");
  H5::DataSet dataset = group.openDataSet("name");

  const auto value = H5Util::readString(dataset);
  if (boost::iequals(value, "mccode")) {
    return 98;
  }
  return 0;
}

} // namespace Mantid::DataHandling
