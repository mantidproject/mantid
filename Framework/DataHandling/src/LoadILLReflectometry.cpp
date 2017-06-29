#include "MantidDataHandling/LoadILLReflectometry.h"

#include "MantidAPI/Axis.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/IPeakFunction.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/RegisterFileLoader.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidHistogramData/LinearGenerator.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/EnabledWhenProperty.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/Quat.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/WorkspaceCreation.h"

using Mantid::HistogramData::Histogram;
using Mantid::HistogramData::Points;
using Mantid::HistogramData::Counts;
using Mantid::HistogramData::LinearGenerator;
using Mantid::DataObjects::create;
using Mantid::DataObjects::Workspace2D;

// formula
/// Returns an iterator for an iterator \a and \b and a double value \c
#define iterator(a, b, c)                                                      \
  std::find_if(a, b, [c](double value) { return value < 0.5 * c; })
// string concatenation
/// Concatenates \string1 and \string2, where the \string1 string will not be
/// modified but copied
#define StringConcat(string1, string2) std::string(string1).append(string2)
// logging
/// Logs a debug message \myString for \myValue
#define debugLog(myString, myValue)                                            \
  g_log.debug(StringConcat(myString, std::to_string(myValue)).append("\n"))
/// Logs a debug message for \myValue explained by two strings \myString and
/// \aString
#define debugLog2(myString, aString, myValue)                                  \
  g_log.debug(StringConcat(myString, aString)                                  \
                  .append(StringConcat(": ", std::to_string(myValue)))         \
                  .append("\n"))
/// Logs a debug message \myString for a \myValue in degrees
#define debugLogWithUnitDegrees(myString, myValue)                             \
  g_log.debug(                                                                 \
      StringConcat(myString, std::to_string(myValue).append(" degrees\n")))
/// Logs a debug message \myString for \myValue in meters
#define debugLogWithUnitMeter(myString, myValue)                               \
  g_log.debug(StringConcat(myString, std::to_string(myValue)).append(" m\n"))
/// Logs an information message \myString using the generated error message
/// what()
#define infoLog(myString)                                                      \
  g_log.information(StringConcat(myString, e.what()).append("\n"))
// get a double value from sample logs of the (output) workspace
/// Get a double value from the workspace to be loaded (SampleLogs) for
/// \myString. Throws error message "Unknown property search object". Possible
/// check via ws->run().hasProperty(myString).
#define getDouble(myString)                                                    \
  m_localWorkspace->run().getPropertyValueAsType<double>(myString)

namespace {
struct Peak {
  double fittedCentre;
  double positionOfMaximum;
};

struct DirectBeamMeasurement {
  double detectorAngle;
  double detectorDistance;
  double fittedPeakCentre;
  double positionOfMaximum;
};

Mantid::API::ITableWorkspace_sptr createBeamPositionTable(const DirectBeamMeasurement &info) {
  auto table = Mantid::API::WorkspaceFactory::Instance().createTable();
  table->addColumn("double", "DetectorAngle");
  table->addColumn("double", "DetectorDistance");
  table->addColumn("double", "FittedPeakCentre");
  table->addColumn("double", "PositionOfMaximum");
  table->appendRow();
  auto col = table->getColumn("DetectorAngle");
  col->cell<double>(0) = info.detectorAngle;
  col = table->getColumn("DetectorDistance");
  col->cell<double>(0) = info.detectorDistance;
  col = table->getColumn("FittedPeakCentre");
  col->cell<double>(0) = info.fittedPeakCentre;
  col = table->getColumn("PositionOfMaximum");
  col->cell<double>(0) = info.positionOfMaximum;
  return table;
}

std::pair<size_t, size_t> fitIntegrationWSIndexRange(const Mantid::API::MatrixWorkspace &ws) {
  const size_t nHisto = ws.getNumberHistograms();
  size_t begin = 0;
  const auto &spectrumInfo = ws.spectrumInfo();
  for (size_t i = 0; i < nHisto; ++i) {
    if (!spectrumInfo.isMonitor(i)) {
      break;
    }
    ++begin;
  }
  size_t end = nHisto - 1;
  for (ptrdiff_t i = static_cast<ptrdiff_t>(nHisto) - 1; i != 0; --i) {
    if (!spectrumInfo.isMonitor(i)) {
      break;
    }
    --end;
  }
  return std::pair<size_t, size_t>{begin, end};
}

DirectBeamMeasurement parseBeamPositionTable(const Mantid::API::ITableWorkspace &table) {
  if (table.rowCount() != 1) {
    throw std::runtime_error("BeamPosition table should have a single row.");
  }
  DirectBeamMeasurement m;
  auto col = table.getColumn("DetectorAngle");
  m.detectorAngle = col->cell<double>(0);
  col = table.getColumn("DetectorDistance");
  m.detectorDistance = col->cell<double>(0);
  col = table.getColumn("FittedPeakCentre");
  m.fittedPeakCentre = col->cell<double>(0);
  col = table.getColumn("PositionOfMaximum");
  m.positionOfMaximum = col->cell<double>(0);
  return m;
}

void rebinIntegralWorkspace(Mantid::API::MatrixWorkspace &ws) {
  auto &xs = ws.mutableX(0);
  std::iota(xs.begin(), xs.end(), 0.0);
}
} // anonymous namespace

namespace Mantid {
namespace DataHandling {

using namespace Kernel;
using namespace API;
using namespace NeXus;

// Register the algorithm into the AlgorithmFactory
DECLARE_NEXUS_FILELOADER_ALGORITHM(LoadILLReflectometry)

/**
 * Return the confidence with this algorithm can load the file
 * @param descriptor A descriptor for the file
 * @returns An integer specifying the confidence level. 0 indicates it will not
 * be used
 */
int LoadILLReflectometry::confidence(
    Kernel::NexusDescriptor &descriptor) const {

  // fields existent only at the ILL
  if ((descriptor.pathExists("/entry0/wavelength") || // ILL D17
       descriptor.pathExists("/entry0/theta"))        // ILL Figaro
      &&
      descriptor.pathExists("/entry0/experiment_identifier") &&
      descriptor.pathExists("/entry0/mode") &&
      (descriptor.pathExists("/entry0/instrument/VirtualChopper") || // ILL D17
       descriptor.pathExists("/entry0/instrument/Theta")) // ILL Figaro
      )
    return 80;
  else
    return 0;
}

/// Initialize the algorithm's properties.
void LoadILLReflectometry::init() {
  declareProperty(Kernel::make_unique<FileProperty>("Filename", std::string(),
                                                    FileProperty::Load, ".nxs",
                                                    Direction::Input),
                  "Name of the Nexus file to load");

  declareProperty(Kernel::make_unique<WorkspaceProperty<>>(
                      "OutputWorkspace", std::string(), Direction::Output),
                  "Name of the output workspace");

  declareProperty(Kernel::make_unique<WorkspaceProperty<ITableWorkspace>>("OutputBeamPosition", std::string(), Direction::Output, PropertyMode::Optional), "Name of the fitted beam position output workspace");

  declareProperty(Kernel::make_unique<WorkspaceProperty<ITableWorkspace>>("BeamPosition", std::string(), Direction::Input, PropertyMode::Optional), "A workspace defining the beam position; used to calculate the Bragg angle");

  declareProperty("BraggAngle", EMPTY_DBL(),
                  "User defined Bragg angle in degrees");
  const std::vector<std::string> availableUnits{"Wavelength", "TimeOfFlight"};
  declareProperty("XUnit", "Wavelength",
                  boost::make_shared<StringListValidator>(availableUnits),
                  "X unit of the OutputWorkspace");

  const std::vector<std::string> scattering{"coherent", "incoherent"};
  declareProperty("ScatteringType", "incoherent",
                  boost::make_shared<StringListValidator>(scattering),
                  "Scattering type used to calculate the Bragg angle.");

}

/**
 * Validate inputs
 * @returns a string map containing the error messages
 */
std::map<std::string, std::string> LoadILLReflectometry::validateInputs() {
  std::map<std::string, std::string> result;
  if (!getPointerToProperty("BeamPosition")->isDefault() && !getPointerToProperty("BraggAngle")->isDefault()) {
    result["BraggAngle"] = "User defined Bragg angle cannot be given simultaneously to BeamPosition.";
  }
  if (!getPointerToProperty("BraggAngle")->isDefault() && !getPointerToProperty("OutputBeamPosition")->isDefault()) {
    result["OutputBeamPosition"] = "Beam position will not be calculated as user defined Bragg angle was given.";
  }
  return result;
}

/// Execute the algorithm.
void LoadILLReflectometry::exec() {
  // open the root node
  NeXus::NXRoot root(getPropertyValue("Filename"));
  NXEntry firstEntry{root.openFirstEntry()};
  // load Monitor details: n. monitors x monitor contents
  std::vector<std::vector<int>> monitorsData{loadMonitors(firstEntry)};
  // set instrument specific names of Nexus file entries
  initNames(firstEntry);
  // load Data details (number of tubes, channels, etc)
  loadDataDetails(firstEntry);
  // initialise workspace
  initWorkspace(monitorsData);
  // load the instrument from the IDF if it exists
  loadInstrument();
  // get properties
  loadNexusEntriesIntoProperties();
  // load data into the workspace
  loadData(firstEntry, monitorsData, getXValues());
  root.close();
  firstEntry.close();
  // position the source
  placeSource();
  // position the detector
  placeDetector();
  convertTofToWavelength();
  // Set the output workspace property
  setProperty("OutputWorkspace", m_localWorkspace);
} // exec

/// Run the Child Algorithm LoadInstrument.
void LoadILLReflectometry::loadInstrument() {
  // execute the Child Algorithm. Catch and log any error, but don't stop.
  g_log.debug("Loading instrument definition...");
  try {
    IAlgorithm_sptr loadInst = createChildAlgorithm("LoadInstrument");
    loadInst->setPropertyValue("InstrumentName", m_instrumentName);
    loadInst->setProperty("RewriteSpectraMap",
                          Mantid::Kernel::OptionalBool(true));
    loadInst->setProperty<MatrixWorkspace_sptr>("Workspace", m_localWorkspace);
    loadInst->executeAsChildAlg();
  } catch (std::runtime_error &e) {
    infoLog("Unable to successfully run LoadInstrument Child Algorithm : ");
  }
}

/**
  * Init names of member variables based on instrument specific NeXus file
  * entries
  *
  * @params entry :: the NeXus file entry
  */
void LoadILLReflectometry::initNames(NeXus::NXEntry &entry) {
  std::string instrumentNamePath = m_loader.findInstrumentNexusPath(entry);
  m_instrumentName = entry.getString(instrumentNamePath.append("/name"));
  if (m_instrumentName.empty())
    throw std::runtime_error(
        "Cannot set the instrument name from the Nexus file!");
  // In NeXus files names are: D17 and figaro. The instrument
  // definition is independent and names start with a capital letter. This
  // loader follows its convention.
  boost::to_lower(m_instrumentName);
  m_instrumentName[0] = char((std::toupper(m_instrumentName[0])));
  g_log.debug(
      StringConcat("Instrument name : ", m_instrumentName).append("\n"));
  if (m_instrumentName == "D17") {
    m_detectorDistanceName = "det";
    m_detectorAngleName = "dan.value";
    m_sampleAngleName = "san.value";
    m_offsetFrom = "VirtualChopper";
    m_offsetName = "open_offset";
    m_pixelCentre = 135.75; // or 135.75 or 132.5
    m_chopper1Name = "Chopper1";
    m_chopper2Name = "Chopper2";
    // m_wavelength = getFloat("wavelength");
  } else if (m_instrumentName == "Figaro") {
    m_detectorDistanceName = "DTR";
    m_detectorAngleName = "VirtualAxis.DAN_actual_angle";
    m_sampleAngleName = "CollAngle.actual_coll_angle";
    m_offsetFrom = "CollAngle";
    m_offsetName = "openOffset";
    m_pixelCentre = double(m_numberOfHistograms) / 2.0;
    // Figaro: find out which of the four choppers are used
    NXFloat firstChopper =
        entry.openNXFloat("instrument/ChopperSetting/firstChopper");
    firstChopper.load();
    NXFloat secondChopper =
        entry.openNXFloat("instrument/ChopperSetting/secondChopper");
    secondChopper.load();
    m_chopper1Name = StringConcat("CH", std::to_string(int(firstChopper[0])));
    m_chopper2Name = StringConcat("CH", std::to_string(int(secondChopper[0])));
  }
  // get acquisition mode
  NXInt acqMode = entry.openNXInt("acquisition_mode");
  acqMode.load();
  m_acqMode = acqMode[0];
  m_acqMode ? g_log.debug("TOF mode") : g_log.debug("Monochromatic Mode");
}

/// Call child algorithm ConvertUnits for conversion from TOF to wavelength
void LoadILLReflectometry::convertTofToWavelength() {
  if (m_acqMode && (getPropertyValue("XUnit") == "Wavelength")) {
    auto convertToWavelength =
        createChildAlgorithm("ConvertUnits", -1, -1, true);
    convertToWavelength->initialize();
    convertToWavelength->setProperty<MatrixWorkspace_sptr>("InputWorkspace",
                                                           m_localWorkspace);
    convertToWavelength->setProperty<MatrixWorkspace_sptr>("OutputWorkspace",
                                                           m_localWorkspace);
    convertToWavelength->setPropertyValue("Target", "Wavelength");
    convertToWavelength->executeAsChildAlg();
  }
}

/**
 * Creates the workspace and initialises member variables with
 * the corresponding values
 *
 * @param monitorsData :: Monitors data already loaded
 */
void LoadILLReflectometry::initWorkspace(
    const std::vector<std::vector<int>> &monitorsData) {

  debugLog("Number of monitors: ", monitorsData.size());
  for (size_t i = 0; i < monitorsData.size(); ++i) {
    if (monitorsData[i].size() != m_numberOfChannels)
      debugLog2("Data size of monitor", std::to_string(i),
                monitorsData[i].size());
  }
  // create the workspace
  try {
    m_localWorkspace = WorkspaceFactory::Instance().create(
        "Workspace2D", m_numberOfHistograms + monitorsData.size(),
        m_numberOfChannels + 1, m_numberOfChannels);
  } catch (std::out_of_range &) {
    throw std::runtime_error(
        StringConcat("Workspace2D cannot be created, check number of "
                     "histograms (",
                     std::to_string(m_numberOfHistograms))
            .append(StringConcat("), monitors (",
                                 std::to_string(monitorsData.size())))
            .append(StringConcat("), and channels (",
                                 std::to_string(m_numberOfChannels)))
            .append(")\n"));
  }
  if (m_acqMode)
    m_localWorkspace->getAxis(0)->unit() =
        UnitFactory::Instance().create("TOF");
  m_localWorkspace->setYUnitLabel("Counts");
  m_localWorkspace->mutableRun().addProperty("Facility", std::string("ILL"));
  if (m_wavelength > 0.0) {
    double ei = m_loader.calculateEnergy(m_wavelength);
    m_localWorkspace->mutableRun().addProperty<double>("Ei", ei, true);
  }
}

/**
 * Load Data details (number of tubes, channels, etc)
 *
 * @param entry First entry of nexus file
 */
void LoadILLReflectometry::loadDataDetails(NeXus::NXEntry &entry) {
  // PSD data D17 256 x 1 x 1000
  // PSD data Figaro 1 x 256 x 1000

  if (m_acqMode) {
    NXFloat timeOfFlight = entry.openNXFloat("instrument/PSD/time_of_flight");
    timeOfFlight.load();
    m_channelWidth = static_cast<double>(timeOfFlight[0]);
    m_numberOfChannels = size_t(timeOfFlight[1]);
    m_tofDelay = timeOfFlight[2];
    if (m_instrumentName == "Figaro") {
      NXFloat eDelay = entry.openNXFloat("instrument/Theta/edelay_delay");
      eDelay.load();
      m_tofDelay += static_cast<double>(eDelay[0]);
    }
  } else { // monochromatic mode
    m_numberOfChannels = 1;
  }

  NXInt nChannels = entry.openNXInt("instrument/PSD/detsize");
  nChannels.load();
  m_numberOfHistograms = nChannels[0];

  std::string widthName{};
  if (m_instrumentName == "D17")
    widthName = "mppx";
  else if (m_instrumentName == "Figaro")
    widthName = "mppy";

  NXFloat pixelWidth =
      entry.openNXFloat(StringConcat("instrument/PSD/", widthName));
  pixelWidth.load();
  m_pixelWidth = static_cast<double>(pixelWidth[0]) * 1e-3;

  g_log.debug("Please note that ILL reflectometry instruments have "
              "several tubes, after integration one "
              "tube remains in the Nexus file.\n Number of tubes (banks): 1\n");
  debugLog("Number of pixels per tube (number of detectors and number "
           "of histograms): ",
           m_numberOfHistograms);
  debugLog("Number of time channels: ", m_numberOfChannels);
  g_log.debug() << "Channel width: " << m_channelWidth << " 10e-6 sec\n";
  debugLog("TOF delay: ", m_tofDelay);
  debugLogWithUnitMeter("Pixel width ", m_pixelWidth);
}

/**
 * Load single monitor
 *
 * @param entry :: The Nexus entry
 * @param monitor_id :: A std::string containing the Nexus path to the monitor
 *data
 * @return monitor :: A std::vector containing monitor values
 */
std::vector<int>
LoadILLReflectometry::loadSingleMonitor(NeXus::NXEntry &entry,
                                        const std::string &monitor_data) {
  NXData dataGroup = entry.openNXData(monitor_data);
  NXInt data = dataGroup.openIntData();
  // load counts
  data.load();
  std::vector<int> monitor(data(), data() + data.size());
  return monitor;
}

/**
 * Load monitor data
 *
 * @param entry :: The Nexus entry
 * @return :: A std::vector of vectors of monitors containing monitor values
 */
std::vector<std::vector<int>>
LoadILLReflectometry::loadMonitors(NeXus::NXEntry &entry) {
  g_log.debug("Read monitor data...");
  // vector of monitors with one entry
  std::vector<std::vector<int>> monitors{
      loadSingleMonitor(entry, "monitor1/data"),
      loadSingleMonitor(entry, "monitor2/data")};
  return monitors;
}

/**
 * Determine x values (unit time-of-flight)
 *
 * @return :: vector holding the x values
 */
std::vector<double> LoadILLReflectometry::getXValues() {
  std::vector<double> xVals;                  // no initialisation
  xVals.reserve(int(m_numberOfChannels) + 1); // reserve memory
  try {
    if (m_acqMode) {
      std::string chopper{"Chopper"};
      double chop1Speed{0.0}, chop2Speed{0.0}, chop2Phase{0.0};
      if (m_instrumentName == "D17") {
        chop1Speed = getDouble("VirtualChopper.chopper1_speed_average");
        chop2Speed = getDouble("VirtualChopper.chopper2_speed_average");
        chop2Phase = getDouble("VirtualChopper.chopper2_phase_average");
      }
      // use phase of first chopper
      double chop1Phase = getDouble(StringConcat(m_chopper1Name, ".phase"));
      if (m_instrumentName == "Figaro" && chop1Phase > 360.0) {
        // Chopper 1 phase on Figaro is set to an arbitrary value (999.9)
        chop1Phase = 0.0;
      }
      double POFF = getDouble(StringConcat(m_offsetFrom, ".poff"));
      double openOffset =
          getDouble(StringConcat(m_offsetFrom, ".").append(m_offsetName));
      if (m_instrumentName == "D17" && chop1Speed != 0.0 && chop2Speed != 0.0 && chop2Phase != 0.0) {
        // virtual chopper entries are valid
        chopper = "Virtual chopper";
      } else {
        // use chopper values
        chop1Speed = getDouble(StringConcat(m_chopper1Name, ".rotation_speed"));
        chop2Speed = getDouble(StringConcat(m_chopper2Name, ".rotation_speed"));
        chop2Phase = getDouble(StringConcat(m_chopper2Name, ".phase"));
      }
      // logging
      debugLog("Poff: ", POFF);
      debugLog("Open offset: ", openOffset);
      debugLog("Chopper 1 phase : ", chop1Phase);
      debugLog(StringConcat(chopper, " 1 speed : "), chop1Speed);
      debugLog(StringConcat(chopper, " 2 phase : "), chop2Phase);
      debugLog(StringConcat(chopper, " 2 speed : "), chop2Speed);

      if (chop1Speed <= 0.0) {
        g_log.error() << "First chopper velocity " << chop1Speed << ". Check you NeXus file.\n";
      }
      const double t_TOF2 = -1.e+6 * 60.0 *
                 (POFF - 45.0 + chop2Phase - chop1Phase + openOffset) /
                 (2.0 * 360 * chop1Speed);
      debugLog("t_TOF2 : ", t_TOF2);
      // compute tof values
      for (int channelIndex = 0; channelIndex <= int(m_numberOfChannels);
           ++channelIndex) {
        double t_TOF1 = (channelIndex + 0.5) * m_channelWidth + m_tofDelay;
        xVals.push_back(t_TOF1 + t_TOF2);
      }
    } else {
      g_log.debug("Time channel index for axis description \n");
      for (size_t t = 0; t <= m_numberOfChannels; ++t)
        xVals.push_back(double(t));
    }
  } catch (std::runtime_error &e) {
    infoLog("Unable to access Nexus file entry : ");
  }
  return xVals;
}

/**
 * Load data from nexus file
 *
 * @param entry :: The Nexus file entry
 * @param monitorsData :: Monitors data already loaded
 * @param xVals :: X values
 */
void LoadILLReflectometry::loadData(NeXus::NXEntry &entry,
                                    const std::vector<std::vector<int>> &monitorsData,
                                    const std::vector<double> &xVals) {
  g_log.debug("Loading data...");
  NXData dataGroup = entry.openNXData("data");
  NXInt data = dataGroup.openIntData();
  // load the counts from the file into memory
  data.load();
  size_t nb_monitors = monitorsData.size();
  Progress progress(this, 0, 1, m_numberOfHistograms + nb_monitors);

  // write monitors
  if (!xVals.empty()) {
    HistogramData::BinEdges binEdges(xVals);
    for (size_t im = 0; im < nb_monitors; im++) {
      const int *monitor_p = monitorsData[im].data();
      const Counts counts(monitor_p, monitor_p + m_numberOfChannels);
      m_localWorkspace->setHistogram(im, binEdges, std::move(counts));
      progress.report();
    }
    // write data
    for (size_t j = 0; j < m_numberOfHistograms; ++j) {
      const int *data_p = &data(0, static_cast<int>(j), 0);
      const Counts counts(data_p, data_p + m_numberOfChannels);
      m_localWorkspace->setHistogram((j + nb_monitors), binEdges,
                                     std::move(counts));
      progress.report();
    }
  } else
    g_log.debug("Vector of x values is empty");
} // LoadILLIndirect::loadData

/**
 * Use the LoadHelper utility to load most of the nexus entries into workspace
 * sample log properties
 */
void LoadILLReflectometry::loadNexusEntriesIntoProperties() {
  g_log.debug("Building properties...");
  // Open NeXus file
  const std::string filename{getPropertyValue("Filename")};
  NXhandle nxfileID;
  NXstatus stat = NXopen(filename.c_str(), NXACC_READ, &nxfileID);
  if (stat == NX_ERROR)
    throw Kernel::Exception::FileError("Unable to open File:", filename);
  m_loader.addNexusFieldsToWsRun(nxfileID, m_localWorkspace->mutableRun());
  stat = NXclose(&nxfileID);
}

/**
  * Gaussian fit to determine peak position.
  *
  * @return :: detector position of the peak: Gaussian fit and position
  * of the maximum (serves as start value for the optimization)
  */
std::pair<double, double> LoadILLReflectometry::fitReflectometryPeak() {
  size_t startIndex;
  size_t endIndex;
  std::tie(startIndex, endIndex) = fitIntegrationWSIndexRange(*m_localWorkspace);
  IAlgorithm_sptr integration = createChildAlgorithm("Integration");
  integration->initialize();
  integration->setProperty("InputWorkspace", m_localWorkspace);
  integration->setProperty("OutputWorkspace", "__unused_for_child");
  integration->setProperty("StartWorkspaceIndex", static_cast<int>(startIndex));
  integration->setProperty("EndWorkspaceIndex", static_cast<int>(endIndex));
  integration->execute();
  MatrixWorkspace_sptr integralWS = integration->getProperty("OutputWorkspace");
  IAlgorithm_sptr transpose = createChildAlgorithm("Transpose");
  transpose->initialize();
  transpose->setProperty("InputWorkspace", integralWS);
  transpose->setProperty("OutputWorkspace", "__unused_for_child");
  transpose->execute();
  integralWS = transpose->getProperty("OutputWorkspace");
  rebinIntegralWorkspace(*integralWS);
  // determine initial height: maximum value
  const auto maxValueIt =
      std::max_element(integralWS->y(0).cbegin(), integralWS->y(0).cend());
  const double height = *maxValueIt;
  // determine initial centre: index of the maximum value
  const size_t maxIndex = std::distance(integralWS->y(0).cbegin(), maxValueIt);
  const double centreByMax = static_cast<double>(maxIndex);
  debugLog("Peak maximum position ", centreByMax);
  // determine sigma
  const auto minFwhmIt = iterator(maxValueIt, integralWS->y(0).begin(), centreByMax);
  const auto maxFwhmIt = iterator(maxValueIt, integralWS->y(0).end(), centreByMax);
  const double fwhm =
      0.5 * static_cast<double>(std::distance(minFwhmIt, maxFwhmIt) + 1);
  debugLog("Initial fwhm (fixed window at half maximum) ", fwhm);
  // generate Gaussian
  auto func = API::FunctionFactory::Instance().createFunction("Gaussian");
  auto initialGaussian =
      boost::dynamic_pointer_cast<API::IPeakFunction>(func);
  initialGaussian->setHeight(height);
  initialGaussian->setCentre(centreByMax);
  initialGaussian->setFwhm(fwhm);
  // call Fit child algorithm
  API::IAlgorithm_sptr fitGaussian = createChildAlgorithm("Fit");
  fitGaussian->initialize();
  fitGaussian->setProperty(
      "Function",
      boost::dynamic_pointer_cast<API::IFunction>(initialGaussian));
  fitGaussian->setProperty("InputWorkspace", integralWS);
  bool success = fitGaussian->execute();
  if (!success)
    g_log.warning("Fit not successful, using initial values.\n");
  else
    debugLog("Sigma: ", initialGaussian->fwhm());
  const double centreByFit = success ? initialGaussian->centre() : centreByMax;
  debugLog("Estimated peak position ", centreByFit);
  return std::pair<double, double>{centreByFit, centreByMax};
}

/// Compute Bragg angle
double LoadILLReflectometry::computeBraggAngle() {
  const double userAngle = getProperty("BraggAngle");
  if (userAngle != EMPTY_DBL()) {
    return userAngle;
  }
  ITableWorkspace_const_sptr posTable = getProperty("BeamPosition");
  const std::string incidentAngle = posTable ? m_detectorAngleName : m_sampleAngleName;
  // modify error message "Unknown property search object"
  if (!m_localWorkspace->run().hasProperty(incidentAngle))
    throw std::runtime_error(
        std::string(incidentAngle).append(" is not defined in Nexus file"));
  // user angle and sample angle behave equivalently for D17
  const double angle = getDouble(incidentAngle);
  debugLog2("Use angle (degrees), ", incidentAngle, angle);
  // the reflected beam
  double reflectedCentre;
  double reflectedMaxPosition;
  std::tie(reflectedCentre, reflectedMaxPosition) = fitReflectometryPeak();
  if (!getPointerToProperty("OutputBeamPosition")->isDefault()) {
    DirectBeamMeasurement m;
    m.detectorAngle = getDouble(m_detectorAngleName);
    m.detectorDistance = sampleDetectorDistance();
    m.fittedPeakCentre = reflectedCentre;
    m.positionOfMaximum = reflectedMaxPosition;
    setProperty("OutputBeamPosition", createBeamPositionTable(m));
  }
  // Figaro theta sign informs about reflection down (-1.0) or up (1.0)
  double down{1.0}; // default value for D17
  if (m_instrumentName == "Figaro") {
    down = getDouble("theta");
    down > 0. ? down = 1. : down = -1.;
  }
  double sign{-down};
  const std::string scatteringType = getProperty("ScatteringType");
  double angleBragg{angle};
  if (!posTable && scatteringType == "coherent") {
    angleBragg = braggAngleReflectedBeam(angle, reflectedMaxPosition, reflectedCentre, sign);
  } else if (posTable) {
    const auto directBeamMeasurement = parseBeamPositionTable(*posTable);
    const double angleCentre = down * (angle - directBeamMeasurement.detectorAngle) / 2.;
    debugLogWithUnitDegrees("Centre angle ", angleCentre);
    if (scatteringType == "incoherent")
      angleBragg = braggAngleDirectBeam(angleCentre, directBeamMeasurement.fittedPeakCentre, reflectedCentre, sign, directBeamMeasurement.detectorDistance);
    else
      angleBragg = braggAngleDirectBeam(angleCentre, directBeamMeasurement.fittedPeakCentre, reflectedMaxPosition + 0.5, sign, directBeamMeasurement.detectorDistance);
  }
  debugLogWithUnitDegrees("Bragg angle ", angleBragg);
  return angleBragg;
}

/// Update detector position according to data file
void LoadILLReflectometry::placeDetector() {
  g_log.debug("Move the detector bank \n");
  const double dist = sampleDetectorDistance();
  debugLogWithUnitMeter("Sample-detector distance ", dist);
  const double rho = computeBraggAngle() + m_offsetAngle / 2.;
  const double theta_rad = 2 * rho / 180.0 * M_PI;
  // incident angle for using the algorithm ConvertToReflectometryQ
  m_localWorkspace->mutableRun().addProperty("stheta", rho / 180.0 * M_PI);
  const std::string componentName = "bank";
  V3D pos = m_loader.getComponentPosition(m_localWorkspace, componentName);
  V3D newpos(dist * sin(theta_rad), pos.Y(),
             dist * cos(theta_rad));
  m_loader.moveComponent(m_localWorkspace, componentName, newpos);
  // apply a local rotation to stay perpendicular to the beam
  const V3D axis(0.0, 1.0, 0.0);
  const Quat rotation(2. * rho, axis);
  m_loader.rotateComponent(m_localWorkspace, componentName, rotation);
}

/// Update source position.
void LoadILLReflectometry::placeSource() {
  if (m_instrumentName != "Figaro") {
    return;
  }
  const double dist = getDouble("ChopperSetting.chopperpair_sample_distance") * 1e-3;
  debugLogWithUnitMeter("Source-sample distance ", dist);
  const std::string source = "chopper1";
  const V3D newPos{0.0, 0.0, -dist};
  m_loader.moveComponent(m_localWorkspace, source, newPos);
}

/// Computes the atan of an angle in rad used for the coherence equation, where
/// \a is a peak position. This equation requires the sample-detector distance
/// of the direct beam.
double LoadILLReflectometry::detectorAngle(const double peakPosition, const double detectorDistance) const {
  return std::atan((peakPosition - m_pixelCentre) * m_pixelWidth / detectorDistance) / M_PI * 180.0;
}

/** Computes the coherence and incoherence equation for an angle and peak
  * positions of the direct and reflected reflected beams. The signed factor depends
  * on Figaro's reflection down option.
  */
double LoadILLReflectometry::braggAngleDirectBeam(const double angle, const double directBeamPeakPosition, const double reflectedBeamPeakPosition, const double sign, const double directBeamDetectorDistance) const {
  const double d = sampleDetectorDistance();
  return angle - sign * 0.5 * (detectorAngle(directBeamPeakPosition, directBeamDetectorDistance) + sign * detectorAngle(reflectedBeamPeakPosition, d));
}

/** Computes the coherence and incoherence equation for an angle \a, and peak
  * positions of the direct and reflected beam \b and \c, respectively. The
  *  signed factor \sign depends on Figaro's reflection
  * down option.
  */
double LoadILLReflectometry::braggAngleReflectedBeam(const double angle, const double reflectedBeamMaxPosition, const double reflectedBeamPeakPosition, const double sign) const {
  const double d = sampleDetectorDistance();
  return angle - sign * 0.5 * (detectorAngle(reflectedBeamMaxPosition, d) + sign * detectorAngle(reflectedBeamPeakPosition, d));
}

double LoadILLReflectometry::sampleDetectorDistance() const {
  // TODO This might be incorrect for Figaro.
  double dist = getDouble(StringConcat(m_detectorDistanceName, ".value")) * 1e-3;
  if (m_instrumentName == "Figaro")
    dist -= getDouble(StringConcat(m_detectorDistanceName, ".offset_value")) * 1e-3;
  return dist;
}
} // namespace DataHandling
} // namespace Mantid
