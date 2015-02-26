#include "MantidDataHandling/LoadMcStas.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidAPI/IEventWorkspace.h"
#include "MantidKernel/Unit.h"
#include <nexus/NeXusFile.hpp>
#include "MantidAPI/AlgorithmManager.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/InstrumentDefinitionParser.h"
#include "MantidAPI/InstrumentDataService.h"
#include "MantidDataHandling/LoadEventNexus.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidAPI/RegisterFileLoader.h"

#include "MantidAPI/NumericAxis.h"
#include <nexus/NeXusException.hpp>
#include <boost/algorithm/string.hpp>

namespace Mantid {
namespace DataHandling {
using namespace Kernel;
using namespace API;
using namespace DataObjects;

// Register the algorithm into the AlgorithmFactory
DECLARE_NEXUS_FILELOADER_ALGORITHM(LoadMcStas);

//----------------------------------------------------------------------------------------------
/** Constructor
 */
LoadMcStas::LoadMcStas() : m_countNumWorkspaceAdded(1) {}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
LoadMcStas::~LoadMcStas() {}

//----------------------------------------------------------------------------------------------
// Algorithm's name for identification. @see Algorithm::name
const std::string LoadMcStas::name() const { return "LoadMcStas"; };

// Algorithm's version for identification. @see Algorithm::version
int LoadMcStas::version() const { return 1; };

// Algorithm's category for identification. @see Algorithm::category
const std::string LoadMcStas::category() const { return "DataHandling"; }

//----------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void LoadMcStas::init() {
  std::vector<std::string> exts;
  exts.push_back(".h5");
  exts.push_back(".nxs");
  declareProperty(new FileProperty("Filename", "", FileProperty::Load, exts),
                  "The name of the Nexus file to load");

  declareProperty(new WorkspaceProperty<Workspace>("OutputWorkspace", "",
                                                   Direction::Output),
                  "An output workspace.");
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void LoadMcStas::exec() {

  std::string filename = getPropertyValue("Filename");
  g_log.debug() << "Opening file " << filename << std::endl;

  ::NeXus::File nxFile(filename);
  auto entries = nxFile.getEntries();
  auto itend = entries.end();
  WorkspaceGroup_sptr outputGroup(new WorkspaceGroup);

  // here loop over all top level Nexus entries
  // HOWEVER IF IT IS KNOWN THAT MCSTAS NEXUS ONLY EVER HAVE ONE TOP LEVEL ENTRY
  // THIS LOOP CAN BE REMOVED
  for (auto it = entries.begin(); it != itend; ++it) {
    std::string name = it->first;
    std::string type = it->second;

    // open top entry - open data entry
    nxFile.openGroup(name, type);
    nxFile.openGroup("data", "NXdetector");

    auto dataEntries = nxFile.getEntries();

    std::map<std::string, std::string> eventEntries;
    std::map<std::string, std::string> histogramEntries;

    // populate eventEntries and histogramEntries
    for (auto eit = dataEntries.begin(); eit != dataEntries.end(); ++eit) {
      std::string dataName = eit->first;
      std::string dataType = eit->second;
      if (dataName == "content_nxs" || dataType != "NXdata")
        continue; // can be removed if sure no Nexus files contains
                  // "content_nxs"
      g_log.debug() << "Opening " << dataName << "   " << dataType << std::endl;

      // open second level entry
      nxFile.openGroup(dataName, dataType);

      // Find the Neutron_ID tag from McStas event data
      // Each event detector has the nexus attribute:
      // @long_name = data ' Intensity Position Position Neutron_ID Velocity
      // Time_Of_Flight Monitor (Square)'
      // if Neutron_ID present we have event data

      auto nxdataEntries = nxFile.getEntries();

      for (auto nit = nxdataEntries.begin(); nit != nxdataEntries.end();
           ++nit) {
        if (nit->second == "NXparameters")
          continue;
        nxFile.openData(nit->first);
        if (nxFile.hasAttr("long_name")) {
          std::string nameAttrValue;
          nxFile.getAttr("long_name", nameAttrValue);

          if (nameAttrValue.find("Neutron_ID") != std::string::npos) {
            eventEntries[eit->first] = eit->second;
          } else {
            histogramEntries[eit->first] = eit->second;
          }
        }
        nxFile.closeData();
      }
      // close second entry
      nxFile.closeGroup();
    }

    if (!eventEntries.empty()) {
      readEventData(eventEntries, outputGroup, nxFile);
    }

    readHistogramData(histogramEntries, outputGroup, nxFile);

    // close top entery
    nxFile
        .closeGroup(); // corresponds to nxFile.openGroup("data", "NXdetector");
    nxFile.closeGroup();

    setProperty("OutputWorkspace", outputGroup);
  }
} // LoadMcStas::exec()

/**
 * Return the confidence with with this algorithm can load the file
 * @param eventEntries map of the file entries that have events
 * @param outputGroup pointer to the workspace group
 * @param nxFile Reads data from inside first first top entry
 */
void LoadMcStas::readEventData(
    const std::map<std::string, std::string> &eventEntries,
    WorkspaceGroup_sptr &outputGroup, ::NeXus::File &nxFile) {
  std::string filename = getPropertyValue("Filename");
  auto entries = nxFile.getEntries();

  // will assume that each top level entry contain one mcstas
  // generated IDF and any event data entries within this top level
  // entry are data collected for that instrument
  // This code for loading the instrument is for now adjusted code from
  // ExperimentalInfo.

  // Close data folder and go back to top level. Then read and close the
  // Instrument folder.
  nxFile.closeGroup();

  Geometry::Instrument_sptr instrument;

  // Initialize progress reporting
  int reports = 2;
  const double progressFractionInitial = 0.1;
  Progress progInitial(this, 0.0, progressFractionInitial, reports);

  try {
    nxFile.openGroup("instrument", "NXinstrument");
    std::string instrumentXML;
    nxFile.openGroup("instrument_xml", "NXnote");
    nxFile.readData("data", instrumentXML);
    nxFile.closeGroup();
    nxFile.closeGroup();

    progInitial.report("Loading instrument");

    Geometry::InstrumentDefinitionParser parser;
    std::string instrumentName = "McStas";
    parser.initialize(filename, instrumentName, instrumentXML);
    std::string instrumentNameMangled = parser.getMangledName();

    // Check whether the instrument is already in the InstrumentDataService
    if (InstrumentDataService::Instance().doesExist(instrumentNameMangled)) {
      // If it does, just use the one from the one stored there
      instrument =
          InstrumentDataService::Instance().retrieve(instrumentNameMangled);
    } else {
      // Really create the instrument
      instrument = parser.parseXML(NULL);
      // Add to data service for later retrieval
      InstrumentDataService::Instance().add(instrumentNameMangled, instrument);
    }
  } catch (...) {
    // Loader should not stop if there is no IDF.xml
    g_log.warning()
        << "\nCould not find the instrument description in the Nexus file:"
        << filename << " Ignore evntdata from data file" << std::endl;
    return;
  }
  // Finished reading Instrument. Then open new data folder again
  nxFile.openGroup("data", "NXdetector");

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
    eventWS->getEventList(i).addDetectorID(detIDs[i]);
    // spectrum number are treated as equal to detector IDs for McStas data
    eventWS->getEventList(i).setSpectrumNo(detIDs[i]);
  }
  // the one is here for the moment for backward compatibility
  eventWS->rebuildSpectraMapping(true);

  bool isAnyNeutrons = false;
  // to store shortest and longest recorded TOF
  double shortestTOF(0.0);
  double longestTOF(0.0);

  const size_t numEventEntries = eventEntries.size();
  Progress progEntries(this, progressFractionInitial, 1.0, numEventEntries * 2);
  for (auto eit = eventEntries.begin(); eit != eventEntries.end(); ++eit) {
    std::string dataName = eit->first;
    std::string dataType = eit->second;

    // open second level entry
    nxFile.openGroup(dataName, dataType);
    std::vector<double> data;
    nxFile.openData("events");
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
    ::NeXus::Info id_info = nxFile.getInfo();
    if (id_info.dims.size() != 2) {
      g_log.error() << "Event data in McStas nexus file not loaded. Expected "
                       "event data block to be two dimensional" << std::endl;
      return;
    }
    int64_t nNeutrons = id_info.dims[0];
    int64_t numberOfDataColumn = id_info.dims[1];
    if (nNeutrons && numberOfDataColumn != 6) {
      g_log.error() << "Event data in McStas nexus file expecting 6 columns"
                    << std::endl;
      return;
    }
    if (isAnyNeutrons == false && nNeutrons > 0)
      isAnyNeutrons = true;

    std::vector<int64_t> start(2);
    std::vector<int64_t> step(2);

    // read the event data in blocks. 1 million event is 1000000*6*8 doubles
    // about 50Mb
    int64_t nNeutronsInBlock = 1000000;
    int64_t nOfFullBlocks = nNeutrons / nNeutronsInBlock;
    int64_t nRemainingNeutrons = nNeutrons - nOfFullBlocks * nNeutronsInBlock;
    // sum over number of blocks + 1 to cover the remainder
    for (int64_t iBlock = 0; iBlock < nOfFullBlocks + 1; iBlock++) {
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
      const int64_t nNeutronsForthisBlock =
          step[0]; // number of neutrons read for this block
      data.resize(nNeutronsForthisBlock * numberOfDataColumn);

      // Check that the type is what it is supposed to be
      if (id_info.type == ::NeXus::FLOAT64) {
        nxFile.getSlab(&data[0], start, step);
      } else {
        g_log.warning()
            << "Entry event field is not FLOAT64! It will be skipped.\n";
        continue;
      }

      // populate workspace with McStas events
      const detid2index_map detIDtoWSindex_map =
          eventWS->getDetectorIDToWorkspaceIndexMap(true);

      progEntries.report("read event data into workspace");
      for (int64_t in = 0; in < nNeutronsForthisBlock; in++) {
        const int detectorID =
            static_cast<int>(data[4 + numberOfDataColumn * in]);
        const double detector_time = data[5 + numberOfDataColumn * in] *
                                     1.0e6; // convert to microseconds
        if (in == 0 && iBlock == 0) {
          shortestTOF = detector_time;
          longestTOF = detector_time;
        } else {
          if (detector_time < shortestTOF)
            shortestTOF = detector_time;
          if (detector_time > longestTOF)
            longestTOF = detector_time;
        }

        const size_t workspaceIndex =
            detIDtoWSindex_map.find(detectorID)->second;

        int64_t pulse_time = 0;
        // eventWS->getEventList(workspaceIndex) +=
        // TofEvent(detector_time,pulse_time);
        // eventWS->getEventList(workspaceIndex) += TofEvent(detector_time);
        eventWS->getEventList(workspaceIndex) += WeightedEvent(
            detector_time, pulse_time, data[numberOfDataColumn * in], 1.0);
      }
    } // end reading over number of blocks of an event dataset

    // nxFile.getData(data);
    nxFile.closeData();
    nxFile.closeGroup();

  } // end reading over number of event datasets

  // Create a default TOF-vector for histogramming, for now just 2 bins
  // 2 bins is the standard. However for McStas simulation data it may make
  // sense to
  // increase this number for better initial visual effect
  Kernel::cow_ptr<MantidVec> axis;
  MantidVec &xRef = axis.access();
  xRef.resize(2, 0.0);
  // if ( nNeutrons > 0)
  if (isAnyNeutrons) {
    xRef[0] = shortestTOF - 1; // Just to make sure the bins hold it all
    xRef[1] = longestTOF + 1;
  }
  // Set the binning axis
  eventWS->setAllX(axis);

  // ensure that specified name is given to workspace (eventWS) when added to
  // outputGroup
  std::string nameOfGroupWS = getProperty("OutputWorkspace");
  std::string nameUserSee = std::string("EventData_") + nameOfGroupWS;
  std::string extraProperty =
      "Outputworkspace_dummy_" +
      boost::lexical_cast<std::string>(m_countNumWorkspaceAdded);
  declareProperty(new WorkspaceProperty<Workspace>(extraProperty, nameUserSee,
                                                   Direction::Output));
  setProperty(extraProperty, boost::static_pointer_cast<Workspace>(eventWS));
  m_countNumWorkspaceAdded++; // need to increment to ensure extraProperty are
                              // unique

  outputGroup->addWorkspace(eventWS);
}

/**
 * Return the confidence with with this algorithm can load the file
 * @param histogramEntries map of the file entries that have histogram
 * @param outputGroup pointer to the workspace group
 * @param nxFile Reads data from inside first first top entry
 * @returns An integer specifying the confidence level. 0 indicates it will not
 * be used
 */
void LoadMcStas::readHistogramData(
    const std::map<std::string, std::string> &histogramEntries,
    WorkspaceGroup_sptr &outputGroup, ::NeXus::File &nxFile) {

  std::string nameAttrValueYLABEL;

  for (auto eit = histogramEntries.begin(); eit != histogramEntries.end();
       ++eit) {
    std::string dataName = eit->first;
    std::string dataType = eit->second;

    // open second level entry
    nxFile.openGroup(dataName, dataType);

    // grap title to use to e.g. create workspace name
    std::string nameAttrValueTITLE;
    nxFile.getAttr("filename", nameAttrValueTITLE);

    if (nxFile.hasAttr("ylabel")) {
      nxFile.getAttr("ylabel", nameAttrValueYLABEL);
    }

    // Find the axis names
    auto nxdataEntries = nxFile.getEntries();
    std::string axis1Name, axis2Name;
    for (auto nit = nxdataEntries.begin(); nit != nxdataEntries.end(); ++nit) {
      if (nit->second == "NXparameters")
        continue;
      if (nit->first == "ncount")
        continue;
      nxFile.openData(nit->first);

      if (nxFile.hasAttr("axis")) {
        int axisNo(0);
        nxFile.getAttr("axis", axisNo);
        if (axisNo == 1)
          axis1Name = nit->first;
        else if (axisNo == 2)
          axis2Name = nit->first;
        else
          throw std::invalid_argument("Unknown axis number");
      }
      nxFile.closeData();
    }

    std::vector<double> axis1Values, axis2Values;
    nxFile.readData<double>(axis1Name, axis1Values);
    if (axis2Name.length() == 0) {
      axis2Name = nameAttrValueYLABEL;
      axis2Values.push_back(0.0);
    } else {
      nxFile.readData<double>(axis2Name, axis2Values);
    }

    const size_t axis1Length = axis1Values.size();
    const size_t axis2Length = axis2Values.size();
    g_log.debug() << "Axis lengths=" << axis1Length << " " << axis2Length
                  << std::endl;

    // Require "data" field
    std::vector<double> data;
    nxFile.readData<double>("data", data);

    // Optional errors field
    std::vector<double> errors;
    try {
      nxFile.readData<double>("errors", errors);
    } catch (::NeXus::Exception &) {
      g_log.information() << "Field " << dataName
                          << " contains no error information." << std::endl;
    }

    // close second level entry
    nxFile.closeGroup();

    MatrixWorkspace_sptr ws = WorkspaceFactory::Instance().create(
        "Workspace2D", axis2Length, axis1Length, axis1Length);
    Axis *axis1 = ws->getAxis(0);
    axis1->title() = axis1Name;
    // Set caption
    boost::shared_ptr<Units::Label> lblUnit(new Units::Label);
    lblUnit->setLabel(axis1Name, "");
    axis1->unit() = lblUnit;

    Axis *axis2 = new NumericAxis(axis2Length);
    axis2->title() = axis2Name;
    // Set caption
    lblUnit = boost::shared_ptr<Units::Label>(new Units::Label);
    lblUnit->setLabel(axis2Name, "");
    axis2->unit() = lblUnit;

    ws->setYUnit(axis2Name);
    ws->replaceAxis(1, axis2);

    for (size_t wsIndex = 0; wsIndex < axis2Length; ++wsIndex) {
      auto &dataY = ws->dataY(wsIndex);
      auto &dataE = ws->dataE(wsIndex);
      auto &dataX = ws->dataX(wsIndex);

      for (size_t j = 0; j < axis1Length; ++j) {
        // Data is stored in column-major order so we are translating to
        // row major for Mantid
        const size_t fileDataIndex = j * axis2Length + wsIndex;

        dataY[j] = data[fileDataIndex];
        dataX[j] = axis1Values[j];
        if (!errors.empty())
          dataE[j] = errors[fileDataIndex];
      }
      axis2->setValue(wsIndex, axis2Values[wsIndex]);
    }

    // set the workspace title
    ws->setTitle(nameAttrValueTITLE);

    // use the workspace title to create the workspace name
    std::replace(nameAttrValueTITLE.begin(), nameAttrValueTITLE.end(), ' ',
                 '_');

    // ensure that specified name is given to workspace (eventWS) when added to
    // outputGroup
    std::string nameOfGroupWS = getProperty("OutputWorkspace");
    std::string nameUserSee = nameAttrValueTITLE + "_" + nameOfGroupWS;
    std::string extraProperty =
        "Outputworkspace_dummy_" +
        boost::lexical_cast<std::string>(m_countNumWorkspaceAdded);
    declareProperty(new WorkspaceProperty<Workspace>(extraProperty, nameUserSee,
                                                     Direction::Output));
    setProperty(extraProperty, boost::static_pointer_cast<Workspace>(ws));
    m_countNumWorkspaceAdded++; // need to increment to ensure extraProperty are
                                // unique

    // Make Mantid store the workspace in the group
    outputGroup->addWorkspace(ws);
  }
  nxFile.closeGroup();

} // finish

/**
 * Return the confidence with with this algorithm can load the file
 * @param descriptor A descriptor for the file
 * @returns An integer specifying the confidence level. 0 indicates it will not
 * be used
 */
int LoadMcStas::confidence(Kernel::NexusDescriptor &descriptor) const {
  using namespace ::NeXus;
  // look at to see if entry1/simulation/name exist first and then
  // if its value = mccode
  int confidence(0);
  if(descriptor.pathExists("/entry1/simulation/name")) {
    try {
        // need to look inside file to check value of entry1/simulation/name 
        ::NeXus::File file = ::NeXus::File(descriptor.filename());
        file.openGroup( descriptor.firstEntryNameType().first, descriptor.firstEntryNameType().second);
        file.openGroup("simulation", "NXnote");
        std::string value;
        // check if entry1/simulation/name equals mccode
        file.readData("name", value);
        if (boost::iequals(value, "mccode"))
          confidence = 98;
        file.closeGroup();
        file.closeGroup();
    } catch (::NeXus::Exception &) {
    }
  }
  return confidence;
}

} // namespace DataHandling
} // namespace Mantid
