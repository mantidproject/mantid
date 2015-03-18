//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidWorkflowAlgorithms/EQSANSLoad.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidAPI/AnalysisDataService.h"
#include <MantidAPI/FileFinder.h>
#include <MantidAPI/FileProperty.h>
#include "MantidKernel/TimeSeriesProperty.h"
#include "Poco/DirectoryIterator.h"
#include "Poco/NumberParser.h"
#include "Poco/NumberFormatter.h"
#include "Poco/String.h"
#include <iostream>
#include <fstream>
#include <istream>
#include <boost/algorithm/string.hpp>
#include <boost/tokenizer.hpp>
#include <boost/regex.hpp>
#include "MantidWorkflowAlgorithms/EQSANSInstrument.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AlgorithmProperty.h"
#include "MantidAPI/PropertyManagerDataService.h"
#include "MantidKernel/PropertyManager.h"

namespace Mantid {
namespace WorkflowAlgorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(EQSANSLoad)

using namespace Kernel;
using namespace API;
using namespace Geometry;
using namespace DataObjects;

void EQSANSLoad::init() {
  declareProperty(new API::FileProperty("Filename", "",
                                        API::FileProperty::OptionalLoad,
                                        "_event.nxs"),
                  "The name of the input event Nexus file to load");

  auto wsValidator = boost::make_shared<CompositeValidator>();
  wsValidator->add<WorkspaceUnitValidator>("TOF");
  declareProperty(new WorkspaceProperty<EventWorkspace>(
                      "InputWorkspace", "", Direction::Input,
                      PropertyMode::Optional, wsValidator),
                  "Input event workspace. Assumed to be unmodified events "
                  "straight from LoadEventNexus");

  declareProperty(
      new WorkspaceProperty<>("OutputWorkspace", "", Direction::Output),
      "Then name of the output EventWorkspace");
  declareProperty(
      "NoBeamCenter", false,
      "If true, the detector will not be moved according to the beam center");
  declareProperty("UseConfigBeam", false, "If true, the beam center defined in "
                                          "the configuration file will be "
                                          "used");
  declareProperty("BeamCenterX", EMPTY_DBL(), "Beam position in X pixel "
                                              "coordinates (used only if "
                                              "UseConfigBeam is false)");
  declareProperty("BeamCenterY", EMPTY_DBL(), "Beam position in Y pixel "
                                              "coordinates (used only if "
                                              "UseConfigBeam is false)");
  declareProperty("UseConfigTOFCuts", false,
                  "If true, the edges of the TOF distribution will be cut "
                  "according to the configuration file");
  declareProperty("LowTOFCut", 0.0, "TOF value below which events will not be "
                                    "loaded into the workspace at load-time");
  declareProperty("HighTOFCut", 0.0, "TOF value above which events will not be "
                                     "loaded into the workspace at load-time");
  declareProperty("SkipTOFCorrection", false,
                  "IF true, the EQSANS TOF correction will be skipped");
  declareProperty("WavelengthStep", 0.1, "Wavelength steps to be used when "
                                         "rebinning the data before performing "
                                         "the reduction");
  declareProperty("UseConfigMask", false, "If true, the masking information "
                                          "found in the configuration file "
                                          "will be used");
  declareProperty("UseConfig", true,
                  "If true, the best configuration file found will be used");
  declareProperty("CorrectForFlightPath", false,
                  "If true, the TOF will be modified for the true flight path "
                  "from the sample to the detector pixel");
  declareProperty("PreserveEvents", true,
                  "If true, the output workspace will be an event workspace");
  declareProperty(
      "SampleDetectorDistance", EMPTY_DBL(),
      "Sample to detector distance to use (overrides meta data), in mm");
  declareProperty("SampleDetectorDistanceOffset", EMPTY_DBL(),
                  "Offset to the sample to detector distance (use only when "
                  "using the distance found in the meta data), in mm");
  declareProperty("LoadMonitors", true,
                  "If true, the monitor workspace will be loaded");
  declareProperty("OutputMessage", "", Direction::Output);
  declareProperty("ReductionProperties", "__sans_reduction_properties",
                  Direction::Input);
}

/// Returns the value of a run property from a given workspace
/// @param inputWS :: input workspace
/// @param pname :: name of the property to retrieve
double getRunPropertyDbl(MatrixWorkspace_sptr inputWS,
                         const std::string &pname) {
  Mantid::Kernel::Property *prop = inputWS->run().getProperty(pname);
  Mantid::Kernel::PropertyWithValue<double> *dp =
      dynamic_cast<Mantid::Kernel::PropertyWithValue<double> *>(prop);
  if (!dp) {
    throw std::runtime_error("Could not cast (interpret) the property " +
                             pname + " as a floating point numeric value.");
  }
  return *dp;
}

/// Find the most appropriate configuration file for a given run
/// @param run :: run number
std::string EQSANSLoad::findConfigFile(const int &run) {
  // Append the standard location of EQSANS config file to the data search
  // directory list
  std::string sns_folder = "/SNS/EQSANS/shared/instrument_configuration";
  if (Poco::File(sns_folder).exists())
    Kernel::ConfigService::Instance().appendDataSearchDir(sns_folder);

  const std::vector<std::string> &searchPaths =
      Kernel::ConfigService::Instance().getDataSearchDirs();
  std::vector<std::string>::const_iterator it = searchPaths.begin();

  int max_run_number = 0;
  std::string config_file = "";
  static boost::regex re1("eqsans_configuration\\.([0-9]+)$");
  boost::smatch matches;
  for (; it != searchPaths.end(); ++it) {
    Poco::DirectoryIterator file_it(*it);
    Poco::DirectoryIterator end;
    for (; file_it != end; ++file_it) {
      if (boost::regex_search(file_it.name(), matches, re1)) {
        std::string s = matches[1];
        int run_number = 0;
        Poco::NumberParser::tryParse(s, run_number);
        if (run_number > max_run_number && run_number <= run) {
          max_run_number = run_number;
          config_file = file_it.path().toString();
        }
      }
    }
  }
  return config_file;
}

/// Read rectangular masks from a config file string
/// @param line :: line (string) from the config file
void EQSANSLoad::readRectangularMasks(const std::string &line) {
  // Looking for rectangular mask
  // Rectangular mask         = 7, 0; 7, 255
  boost::regex re_key("rectangular mask", boost::regex::icase);
  boost::regex re_key_alt("elliptical mask", boost::regex::icase);
  if (boost::regex_search(line, re_key) ||
      boost::regex_search(line, re_key_alt)) {
    boost::regex re_sig("=[ ]*([0-9]+)[ ]*[ ,][ ]*([0-9]+)[ ]*[ ;,][ "
                        "]*([0-9]+)[ ]*[ ,][ ]*([0-9]+)");
    boost::smatch posVec;
    if (boost::regex_search(line, posVec, re_sig)) {
      if (posVec.size() == 5) {
        for (int i = 0; i < 4; i++) {
          std::string num_str = posVec[i + 1];
          m_mask_as_string = m_mask_as_string + " " + num_str;
        }
        m_mask_as_string += ",";
      }
    }
  }
}

/// Read the TOF cuts from a config file string
/// @param line :: line (string) from the config file
void EQSANSLoad::readTOFcuts(const std::string &line) {
  boost::regex re_key("tof edge discard", boost::regex::icase);
  if (boost::regex_search(line, re_key)) {
    boost::regex re_sig("=[ ]*([0-9]+)[ ]*[ ,][ ]*([0-9]+)");
    boost::smatch posVec;
    if (boost::regex_search(line, posVec, re_sig)) {
      if (posVec.size() == 3) {
        std::string num_str = posVec[1];
        Poco::NumberParser::tryParseFloat(num_str, m_low_TOF_cut);
        num_str = posVec[2];
        Poco::NumberParser::tryParseFloat(num_str, m_high_TOF_cut);
      }
    }
  }
}

/// Read the beam center from a config file string
/// @param line :: line (string) from the config file
void EQSANSLoad::readBeamCenter(const std::string &line) {
  boost::regex re_key("spectrum center", boost::regex::icase);
  if (boost::regex_search(line, re_key)) {
    boost::regex re_sig("=[ ]*([0-9]+.[0-9]*)[ ]*[ ,][ ]*([0-9]+.[0-9]+)");
    boost::smatch posVec;
    if (boost::regex_search(line, posVec, re_sig)) {
      if (posVec.size() == 3) {
        std::string num_str = posVec[1];
        Poco::NumberParser::tryParseFloat(num_str, m_center_x);
        num_str = posVec[2];
        Poco::NumberParser::tryParseFloat(num_str, m_center_y);
      }
    }
  }
}

/// Read the moderator position from a config file string
/// @param line :: line (string) from the config file
void EQSANSLoad::readModeratorPosition(const std::string &line) {
  boost::regex re_key("sample location", boost::regex::icase);
  if (boost::regex_search(line, re_key)) {
    boost::regex re_sig("=[ ]*([0-9]+)");
    boost::smatch posVec;
    if (boost::regex_search(line, posVec, re_sig)) {
      if (posVec.size() == 2) {
        std::string num_str = posVec[1];
        Poco::NumberParser::tryParseFloat(num_str, m_moderator_position);
        m_moderator_position = -m_moderator_position / 1000.0;
      }
    }
  }
}

/// Read the source slit sizes from a config file string
/// @param line :: line (string) from the config file
void EQSANSLoad::readSourceSlitSize(const std::string &line) {
  boost::regex re_key("wheel", boost::regex::icase);
  if (boost::regex_search(line, re_key)) {
    boost::regex re_sig("([1-8]) wheel[ ]*([1-3])[ \\t]*=[ \\t]*(\\w+)");
    boost::smatch posVec;
    if (boost::regex_search(line, posVec, re_sig)) {
      if (posVec.size() == 4) {
        std::string num_str = posVec[1];
        int slit_number = 0;
        Poco::NumberParser::tryParse(num_str, slit_number);
        slit_number--;

        num_str = posVec[2];
        int wheel_number = 0;
        Poco::NumberParser::tryParse(num_str, wheel_number);
        wheel_number--;

        num_str = posVec[3];
        boost::regex re_size("\\w*?([0-9]+)mm");
        int slit_size = 0;
        if (boost::regex_search(num_str, posVec, re_size)) {
          if (posVec.size() == 2) {
            num_str = posVec[1];
            Poco::NumberParser::tryParse(num_str, slit_size);
          }
        }
        m_slit_positions[wheel_number][slit_number] = slit_size;
      }
    }
  }
}

/// Get the source slit size from the slit information of the run properties
void EQSANSLoad::getSourceSlitSize() {
  if (!dataWS->run().hasProperty("vBeamSlit")) {
    m_output_message += "   Could not determine source aperture diameter: ";
    m_output_message += "slit parameters were not found in the run log\n";
    return;
  }

  const std::string slit1Name = "vBeamSlit";
  Mantid::Kernel::Property *prop = dataWS->run().getProperty(slit1Name);
  Mantid::Kernel::TimeSeriesProperty<double> *dp =
      dynamic_cast<Mantid::Kernel::TimeSeriesProperty<double> *>(prop);
  if (!dp) {
    throw std::runtime_error("Could not cast (interpret) the property " +
                             slit1Name + " as a time series property with "
                             "floating point values.");
  }
  int slit1 = (int)dp->getStatistics().mean;

  const std::string slit2Name = "vBeamSlit2";
  prop = dataWS->run().getProperty(slit2Name);
  dp = dynamic_cast<Mantid::Kernel::TimeSeriesProperty<double> *>(prop);
  if (!dp) {
    throw std::runtime_error("Could not cast (interpret) the property " +
                             slit2Name + " as a time series property with "
                             "floating point values.");
  }
  int slit2 = (int)dp->getStatistics().mean;

  const std::string slit3Name = "vBeamSlit3";
  prop = dataWS->run().getProperty(slit3Name);
  dp = dynamic_cast<Mantid::Kernel::TimeSeriesProperty<double> *>(prop);
  if (!dp) {
    throw std::runtime_error("Could not cast (interpret) the property " +
                             slit3Name + " as a time series property with "
                             "floating point values.");
  }
  int slit3 = (int)dp->getStatistics().mean;

  if (slit1 < 0 && slit2 < 0 && slit3 < 0) {
    m_output_message += "   Could not determine source aperture diameter\n";
    return;
  }

  // Default slit size
  double S1 = 20.0;
  double L1 = -1.0;
  const double ssd =
      fabs(dataWS->getInstrument()->getSource()->getPos().Z()) * 1000.0;
  int slits[3] = {slit1, slit2, slit3};
  for (int i = 0; i < 3; i++) {
    int m = slits[i] - 1;
    if (m >= 0 && m < 6) {
      double x = m_slit_positions[i][m];
      double y = ssd - m_slit_to_source[i];
      if (L1 < 0 || x / y < S1 / L1) {
        L1 = y;
        S1 = x;
      }
    }
  }
  dataWS->mutableRun().addProperty("source-aperture-diameter", S1, "mm", true);
  m_output_message += "   Source aperture diameter: ";
  Poco::NumberFormatter::append(m_output_message, S1, 1);
  m_output_message += " mm\n";
}

/// Move the detector according to the beam center
void EQSANSLoad::moveToBeamCenter() {
  // Check that we have a beam center defined, otherwise set the
  // default beam center
  if (isEmpty(m_center_x) || isEmpty(m_center_y)) {
    EQSANSInstrument::getDefaultBeamCenter(dataWS, m_center_x, m_center_y);
    g_log.information() << "Setting beam center to ["
                        << Poco::NumberFormatter::format(m_center_x, 1) << ", "
                        << Poco::NumberFormatter::format(m_center_y, 1) << "]"
                        << std::endl;
    return;
  }

  // Check that the center of the detector really is at (0,0)
  int nx_pixels = (int)(dataWS->getInstrument()->getNumberParameter(
      "number-of-x-pixels")[0]);
  int ny_pixels = (int)(dataWS->getInstrument()->getNumberParameter(
      "number-of-y-pixels")[0]);
  V3D pixel_first = dataWS->getInstrument()->getDetector(0)->getPos();
  int detIDx = EQSANSInstrument::getDetectorFromPixel(nx_pixels - 1, 0, dataWS);
  int detIDy = EQSANSInstrument::getDetectorFromPixel(0, ny_pixels - 1, dataWS);

  V3D pixel_last_x = dataWS->getInstrument()->getDetector(detIDx)->getPos();
  V3D pixel_last_y = dataWS->getInstrument()->getDetector(detIDy)->getPos();
  double x_offset = (pixel_first.X() + pixel_last_x.X()) / 2.0;
  double y_offset = (pixel_first.Y() + pixel_last_y.Y()) / 2.0;
  double beam_ctr_x = 0.0;
  double beam_ctr_y = 0.0;
  EQSANSInstrument::getCoordinateFromPixel(m_center_x, m_center_y, dataWS,
                                           beam_ctr_x, beam_ctr_y);

  IAlgorithm_sptr mvAlg =
      createChildAlgorithm("MoveInstrumentComponent", 0.5, 0.50);
  mvAlg->setProperty<MatrixWorkspace_sptr>("Workspace", dataWS);
  mvAlg->setProperty("ComponentName", "detector1");
  mvAlg->setProperty("X", -x_offset - beam_ctr_x);
  mvAlg->setProperty("Y", -y_offset - beam_ctr_y);
  mvAlg->setProperty("RelativePosition", true);
  mvAlg->executeAsChildAlg();
  m_output_message += "   Beam center offset: " +
                      Poco::NumberFormatter::format(x_offset) + ", " +
                      Poco::NumberFormatter::format(y_offset) + " m\n";
  // m_output_message += "   Beam center in real-space: " +
  // Poco::NumberFormatter::format(-x_offset-beam_ctr_x)
  //    + ", " + Poco::NumberFormatter::format(-y_offset-beam_ctr_y) + " m\n";
  g_log.information() << "Moving beam center to " << m_center_x << " "
                      << m_center_y << std::endl;

  dataWS->mutableRun().addProperty("beam_center_x", m_center_x, "pixel", true);
  dataWS->mutableRun().addProperty("beam_center_y", m_center_y, "pixel", true);
  m_output_message += "   Beam center: " +
                      Poco::NumberFormatter::format(m_center_x, 1) + ", " +
                      Poco::NumberFormatter::format(m_center_y, 1) + "\n";
}

/// Read a config file
/// @param filePath :: path of the config file to read
void EQSANSLoad::readConfigFile(const std::string &filePath) {
  // Initialize parameters
  m_mask_as_string = "";
  m_moderator_position = 0;

  // The following should be properties
  bool use_config_mask = getProperty("UseConfigMask");
  bool use_config_cutoff = getProperty("UseConfigTOFCuts");
  bool use_config_center = getProperty("UseConfigBeam");

  std::ifstream file(filePath.c_str());
  if (!file) {
    g_log.error() << "Unable to open file: " << filePath << std::endl;
    throw Exception::FileError("Unable to open file: ", filePath);
  }
  g_log.information() << "Using config file: " << filePath << std::endl;
  m_output_message += "   Using configuration file: " + filePath + "\n";

  std::string line;
  while (getline(file, line)) {
    boost::trim(line);
    std::string comment = line.substr(0, 1);
    if (Poco::icompare(comment, "#") == 0)
      continue;
    if (use_config_mask)
      readRectangularMasks(line);
    if (use_config_cutoff)
      readTOFcuts(line);
    if (use_config_center)
      readBeamCenter(line);
    readModeratorPosition(line);
    readSourceSlitSize(line);
  }

  if (use_config_mask) {
    dataWS->mutableRun().addProperty("rectangular_masks", m_mask_as_string,
                                     "pixels", true);
  }

  dataWS->mutableRun().addProperty("low_tof_cut", m_low_TOF_cut, "microsecond",
                                   true);
  dataWS->mutableRun().addProperty("high_tof_cut", m_high_TOF_cut,
                                   "microsecond", true);
  m_output_message +=
      "   Discarding lower " + Poco::NumberFormatter::format(m_low_TOF_cut, 1) +
      " and upper " + Poco::NumberFormatter::format(m_high_TOF_cut, 1) +
      " microsec\n";

  if (m_moderator_position != 0) {
    dataWS->mutableRun().addProperty("moderator_position", m_moderator_position,
                                     "mm", true);
  }
}

void EQSANSLoad::exec() {
  // Verify the validity of the inputs
  // TODO: this should be done by the new data management algorithm used for
  // live data reduction (when it's implemented...)
  const std::string fileName = getPropertyValue("Filename");
  EventWorkspace_sptr inputEventWS = getProperty("InputWorkspace");
  if (fileName.size() == 0 && !inputEventWS) {
    g_log.error() << "EQSANSLoad input error: Either a valid file path or an "
                     "input workspace must be provided" << std::endl;
    throw std::runtime_error("EQSANSLoad input error: Either a valid file path "
                             "or an input workspace must be provided");
  } else if (fileName.size() > 0 && inputEventWS) {
    g_log.error() << "EQSANSLoad input error: Either a valid file path or an "
                     "input workspace must be provided, but not both"
                  << std::endl;
    throw std::runtime_error("EQSANSLoad input error: Either a valid file path "
                             "or an input workspace must be provided, but not "
                             "both");
  }

  // Read in default TOF cuts
  const bool skipTOFCorrection = getProperty("SkipTOFCorrection");
  m_low_TOF_cut = getProperty("LowTOFCut");
  m_high_TOF_cut = getProperty("HighTOFCut");

  // Read in default beam center
  m_center_x = getProperty("BeamCenterX");
  m_center_y = getProperty("BeamCenterY");
  const bool noBeamCenter = getProperty("NoBeamCenter");

  // Reduction property manager
  const std::string reductionManagerName = getProperty("ReductionProperties");
  boost::shared_ptr<PropertyManager> reductionManager;
  if (PropertyManagerDataService::Instance().doesExist(reductionManagerName)) {
    reductionManager =
        PropertyManagerDataService::Instance().retrieve(reductionManagerName);
  } else {
    reductionManager = boost::make_shared<PropertyManager>();
    PropertyManagerDataService::Instance().addOrReplace(reductionManagerName,
                                                        reductionManager);
  }

  if (!reductionManager->existsProperty("LoadAlgorithm")) {
    AlgorithmProperty *loadProp = new AlgorithmProperty("LoadAlgorithm");
    setPropertyValue("InputWorkspace", "");
    setProperty("NoBeamCenter", false);
    loadProp->setValue(toString());
    reductionManager->declareProperty(loadProp);
  }

  if (!reductionManager->existsProperty("InstrumentName")) {
    reductionManager->declareProperty(
        new PropertyWithValue<std::string>("InstrumentName", "EQSANS"));
  }

  // Output log
  m_output_message = "";

  // Check whether we need to load the data
  if (!inputEventWS) {
    const bool loadMonitors = getProperty("LoadMonitors");
    IAlgorithm_sptr loadAlg = createChildAlgorithm("LoadEventNexus", 0, 0.2);
    loadAlg->setProperty("LoadMonitors", loadMonitors);
    loadAlg->setProperty("MonitorsAsEvents", false);
    loadAlg->setProperty("Filename", fileName);
    if (skipTOFCorrection) {
      if (m_low_TOF_cut > 0.0)
        loadAlg->setProperty("FilterByTofMin", m_low_TOF_cut);
      if (m_high_TOF_cut > 0.0)
        loadAlg->setProperty("FilterByTofMax", m_high_TOF_cut);
    }
    loadAlg->execute();
    IEventWorkspace_sptr dataWS_asWks = loadAlg->getProperty("OutputWorkspace");
    dataWS = boost::dynamic_pointer_cast<MatrixWorkspace>(dataWS_asWks);

    // Get monitor workspace as necessary
    std::string mon_wsname = getPropertyValue("OutputWorkspace") + "_monitors";
    if (loadMonitors && loadAlg->existsProperty("MonitorWorkspace")) {
      MatrixWorkspace_sptr monWS = loadAlg->getProperty("MonitorWorkspace");
      declareProperty(new WorkspaceProperty<>("MonitorWorkspace", mon_wsname,
                                              Direction::Output),
                      "Monitors from the Event NeXus file");
      setProperty("MonitorWorkspace", monWS);
    }
  } else {
    MatrixWorkspace_sptr outputWS = getProperty("OutputWorkspace");
    EventWorkspace_sptr outputEventWS =
        boost::dynamic_pointer_cast<EventWorkspace>(outputWS);
    if (inputEventWS != outputEventWS) {
      IAlgorithm_sptr copyAlg = createChildAlgorithm("CloneWorkspace", 0, 0.2);
      copyAlg->setProperty("InputWorkspace", inputEventWS);
      copyAlg->executeAsChildAlg();
      Workspace_sptr dataWS_asWks = copyAlg->getProperty("OutputWorkspace");
      dataWS = boost::dynamic_pointer_cast<MatrixWorkspace>(dataWS_asWks);
    } else {
      dataWS = boost::dynamic_pointer_cast<MatrixWorkspace>(inputEventWS);
    }
  }

  // Get the sample-detector distance
  double sdd = 0.0;
  const double sample_det_dist = getProperty("SampleDetectorDistance");
  if (!isEmpty(sample_det_dist)) {
    sdd = sample_det_dist;
  } else {
    if (!dataWS->run().hasProperty("detectorZ")) {
      g_log.error() << "Could not determine Z position: the "
                       "SampleDetectorDistance property was not set "
                       "and the run logs do not contain the detectorZ property"
                    << std::endl;
      throw std::invalid_argument(
          "Could not determine Z position: stopping execution");
    }

    const std::string dzName = "detectorZ";
    Mantid::Kernel::Property *prop = dataWS->run().getProperty(dzName);
    Mantid::Kernel::TimeSeriesProperty<double> *dp =
        dynamic_cast<Mantid::Kernel::TimeSeriesProperty<double> *>(prop);
    if (!dp) {
      throw std::runtime_error("Could not cast (interpret) the property " +
                               dzName + " as a time series property value.");
    } 
    sdd = dp->getStatistics().mean;

    // Modify SDD according to offset if given
    const double sample_det_offset =
        getProperty("SampleDetectorDistanceOffset");
    if (!isEmpty(sample_det_offset)) {
      sdd += sample_det_offset;
    }
  }
  dataWS->mutableRun().addProperty("sample_detector_distance", sdd, "mm", true);

  // Move the detector to its correct position
  IAlgorithm_sptr mvAlg =
      createChildAlgorithm("MoveInstrumentComponent", 0.2, 0.4);
  mvAlg->setProperty<MatrixWorkspace_sptr>("Workspace", dataWS);
  mvAlg->setProperty("ComponentName", "detector1");
  mvAlg->setProperty("Z", sdd / 1000.0);
  mvAlg->setProperty("RelativePosition", false);
  mvAlg->executeAsChildAlg();
  g_log.information() << "Moving detector to " << sdd / 1000.0 << " meters"
                      << std::endl;
  m_output_message += "   Detector position: " +
                      Poco::NumberFormatter::format(sdd / 1000.0, 3) + " m\n";

  // Get the run number so we can find the proper config file
  int run_number = 0;
  std::string config_file = "";
  if (dataWS->run().hasProperty("run_number")) {
    Mantid::Kernel::Property *prop = dataWS->run().getProperty("run_number");
    Mantid::Kernel::PropertyWithValue<std::string> *dp =
        dynamic_cast<Mantid::Kernel::PropertyWithValue<std::string> *>(prop);
    const std::string run_str = *dp;
    Poco::NumberParser::tryParse(run_str, run_number);
    // Find a proper config file
    config_file = findConfigFile(run_number);
  } else {
    g_log.error() << "Could not find run number for workspace "
                  << getPropertyValue("OutputWorkspace") << std::endl;
    m_output_message += "   Could not find run number for data file\n";
  }

  // Process the config file
  bool use_config = getProperty("UseConfig");
  if (use_config && config_file.size() > 0) {
    // Special case to force reading the beam center from the config file
    // We're adding this to be compatible with the original EQSANS load
    // written in python
    if (m_center_x == 0.0 && m_center_y == 0.0) {
      setProperty("UseConfigBeam", true);
    }

    readConfigFile(config_file);
  } else if (use_config) {
    use_config = false;
    g_log.error() << "Cound not find config file for workspace "
                  << getPropertyValue("OutputWorkspace") << std::endl;
    m_output_message += "   Could not find configuration file for run " +
                        Poco::NumberFormatter::format(run_number) + "\n";
  }

  // If we use the config file, move the moderator position
  if (use_config) {
    if (m_moderator_position > -13.0)
      g_log.error()
          << "Moderator position seems close to the sample, please check"
          << std::endl;
    g_log.information() << "Moving moderator to " << m_moderator_position
                        << std::endl;
    m_output_message += "   Moderator position: " +
                        Poco::NumberFormatter::format(m_moderator_position, 3) +
                        " m\n";
    mvAlg = createChildAlgorithm("MoveInstrumentComponent", 0.4, 0.45);
    mvAlg->setProperty<MatrixWorkspace_sptr>("Workspace", dataWS);
    mvAlg->setProperty("ComponentName", "moderator");
    mvAlg->setProperty("Z", m_moderator_position);
    mvAlg->setProperty("RelativePosition", false);
    mvAlg->executeAsChildAlg();
  }

  // Get source aperture radius
  getSourceSlitSize();

  // Move the beam center to its proper position
  if (!noBeamCenter) {
    if (isEmpty(m_center_x) || isEmpty(m_center_y)) {
      if (reductionManager->existsProperty("LatestBeamCenterX") &&
          reductionManager->existsProperty("LatestBeamCenterY")) {
        m_center_x = reductionManager->getProperty("LatestBeamCenterX");
        m_center_y = reductionManager->getProperty("LatestBeamCenterY");
      }
    }
    moveToBeamCenter();

    // Add beam center to reduction properties, as the last beam center position
    // that was used.
    // This will give us our default position next time.
    if (!reductionManager->existsProperty("LatestBeamCenterX"))
      reductionManager->declareProperty(
          new PropertyWithValue<double>("LatestBeamCenterX", m_center_x));
    else
      reductionManager->setProperty("LatestBeamCenterX", m_center_x);
    if (!reductionManager->existsProperty("LatestBeamCenterY"))
      reductionManager->declareProperty(
          new PropertyWithValue<double>("LatestBeamCenterY", m_center_y));
    else
      reductionManager->setProperty("LatestBeamCenterY", m_center_y);
  }

  // Modify TOF
  const bool correct_for_flight_path = getProperty("CorrectForFlightPath");
  double wl_min = 0.0;
  double wl_max = 0.0;
  double wl_combined_max = 0.0;
  if (skipTOFCorrection) {
    m_output_message +=
        "    Skipping EQSANS TOF correction: assuming a single frame\n";
    dataWS->mutableRun().addProperty("is_frame_skipping", 0, true);
    if (correct_for_flight_path) {
      g_log.error() << "CorrectForFlightPath and SkipTOFCorrection can't be "
                       "set to true at the same time" << std::endl;
      m_output_message += "    Skipped flight path correction: see error log\n";
    }
  } else {
    m_output_message += "   Flight path correction ";
    if (!correct_for_flight_path)
      m_output_message += "NOT ";
    m_output_message += "applied\n";
    DataObjects::EventWorkspace_sptr dataWS_evt =
        boost::dynamic_pointer_cast<EventWorkspace>(dataWS);
    IAlgorithm_sptr tofAlg =
        createChildAlgorithm("EQSANSTofStructure", 0.5, 0.7);
    tofAlg->setProperty<EventWorkspace_sptr>("InputWorkspace", dataWS_evt);
    tofAlg->setProperty("LowTOFCut", m_low_TOF_cut);
    tofAlg->setProperty("HighTOFCut", m_high_TOF_cut);
    tofAlg->setProperty("FlightPathCorrection", correct_for_flight_path);
    tofAlg->executeAsChildAlg();
    wl_min = tofAlg->getProperty("WavelengthMin");
    wl_max = tofAlg->getProperty("WavelengthMax");
    if (wl_min != wl_min || wl_max != wl_max) {
      g_log.error() << "Bad wavelength range" << std::endl;
      g_log.error() << m_output_message << std::endl;
    }

    const bool frame_skipping = tofAlg->getProperty("FrameSkipping");
    dataWS->mutableRun().addProperty("wavelength_min", wl_min, "Angstrom",
                                     true);
    dataWS->mutableRun().addProperty("wavelength_max", wl_max, "Angstrom",
                                     true);
    dataWS->mutableRun().addProperty("is_frame_skipping", int(frame_skipping),
                                     true);
    wl_combined_max = wl_max;
    m_output_message += "   Wavelength range: " +
                        Poco::NumberFormatter::format(wl_min, 1) + " - " +
                        Poco::NumberFormatter::format(wl_max, 1);
    if (frame_skipping) {
      const double wl_min2 = tofAlg->getProperty("WavelengthMinFrame2");
      const double wl_max2 = tofAlg->getProperty("WavelengthMaxFrame2");
      wl_combined_max = wl_max2;
      dataWS->mutableRun().addProperty("wavelength_min_frame2", wl_min2,
                                       "Angstrom", true);
      dataWS->mutableRun().addProperty("wavelength_max_frame2", wl_max2,
                                       "Angstrom", true);
      m_output_message += " and " + Poco::NumberFormatter::format(wl_min2, 1) +
                          " - " + Poco::NumberFormatter::format(wl_max2, 1) +
                          " Angstrom\n";
    } else
      m_output_message += " Angstrom\n";
  }

  // Convert to wavelength
  const double ssd =
      fabs(dataWS->getInstrument()->getSource()->getPos().Z()) * 1000.0;
  const double conversion_factor = 3.9560346 / (sdd + ssd);
  m_output_message += "   TOF to wavelength conversion factor: " +
                      Poco::NumberFormatter::format(conversion_factor) + "\n";

  if (skipTOFCorrection) {
    DataObjects::EventWorkspace_sptr dataWS_evt =
        boost::dynamic_pointer_cast<EventWorkspace>(dataWS);
    if (dataWS_evt->getNumberEvents() == 0)
      throw std::invalid_argument("No event to process: check your TOF cuts");
    wl_min = dataWS_evt->getTofMin() * conversion_factor;
    wl_max = dataWS_evt->getTofMax() * conversion_factor;
    wl_combined_max = wl_max;
    g_log.information() << "Wavelength range: " << wl_min << " to " << wl_max
                        << std::endl;
    dataWS->mutableRun().addProperty("wavelength_min", wl_min, "Angstrom",
                                     true);
    dataWS->mutableRun().addProperty("wavelength_max", wl_max, "Angstrom",
                                     true);
  }

  IAlgorithm_sptr scAlg = createChildAlgorithm("ScaleX", 0.7, 0.71);
  scAlg->setProperty<MatrixWorkspace_sptr>("InputWorkspace", dataWS);
  scAlg->setProperty<MatrixWorkspace_sptr>("OutputWorkspace", dataWS);
  scAlg->setProperty("Factor", conversion_factor);
  scAlg->executeAsChildAlg();
  dataWS->getAxis(0)->setUnit("Wavelength");

  // Rebin so all the wavelength bins are aligned
  const bool preserveEvents = getProperty("PreserveEvents");
  const double wl_step = getProperty("WavelengthStep");
  std::string params = Poco::NumberFormatter::format(wl_min, 2) + "," +
                       Poco::NumberFormatter::format(wl_step, 2) + "," +
                       Poco::NumberFormatter::format(wl_combined_max, 2);
  IAlgorithm_sptr rebinAlg = createChildAlgorithm("Rebin", 0.71, 0.72);
  rebinAlg->setProperty<MatrixWorkspace_sptr>("InputWorkspace", dataWS);
  if (preserveEvents)
    rebinAlg->setProperty<MatrixWorkspace_sptr>("OutputWorkspace", dataWS);
  rebinAlg->setPropertyValue("Params", params);
  rebinAlg->setProperty("PreserveEvents", preserveEvents);
  rebinAlg->executeAsChildAlg();

  if (!preserveEvents)
    dataWS = rebinAlg->getProperty("OutputWorkspace");

  dataWS->mutableRun().addProperty("event_ws",
                                   getPropertyValue("OutputWorkspace"), true);
  setProperty<MatrixWorkspace_sptr>(
      "OutputWorkspace", boost::dynamic_pointer_cast<MatrixWorkspace>(dataWS));
  // m_output_message = "Loaded " + fileName + '\n' + m_output_message;
  setPropertyValue("OutputMessage", m_output_message);
}

} // namespace WorkflowAlgorithms
} // namespace Mantid
