#include "MantidWorkflowAlgorithms/SWANSLoad.h"
#include "MantidWorkflowAlgorithms/EQSANSInstrument.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AlgorithmProperty.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FileFinder.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/PropertyManagerDataService.h"
#include "MantidAPI/WorkspaceUnitValidator.h"
#include "MantidGeometry/Instrument.h"
#include "MantidKernel/PropertyManager.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidAPI/NumericAxis.h"

#include <boost/algorithm/string.hpp>
#include <boost/tokenizer.hpp>
#include <boost/regex.hpp>
#include <boost/math/constants/constants.hpp> //pi

#include "Poco/DirectoryIterator.h"
#include "Poco/NumberParser.h"
#include "Poco/NumberFormatter.h"
#include "Poco/String.h"

#include <iostream>
#include <fstream>
#include <istream>
#include <cmath>       /* sqrt */

namespace Mantid {
namespace WorkflowAlgorithms {

using Mantid::Kernel::Direction;
using Mantid::API::WorkspaceProperty;
using namespace Kernel;
using namespace API;
using namespace Geometry;
using namespace DataObjects;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(SWANSLoad)

//----------------------------------------------------------------------------------------------

/// Algorithms name for identification. @see Algorithm::name
const std::string SWANSLoad::name() const { return "SWANSLoad"; }

/// Algorithm's version for identification. @see Algorithm::version
int SWANSLoad::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string SWANSLoad::category() const { return "Workflow\\SANS"; }

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string SWANSLoad::summary() const { return "Load SWANS data."; }

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void SWANSLoad::init() {
  declareProperty(make_unique<API::FileProperty>(
                      "Filename", "", API::FileProperty::OptionalLoad, ".dat"),
                  "The name of the input event file to load");

  auto wsValidator = boost::make_shared<WorkspaceUnitValidator>("TOF");
  declareProperty(make_unique<WorkspaceProperty<EventWorkspace>>(
                      "InputWorkspace", "", Direction::Input,
                      PropertyMode::Optional, wsValidator),
                  "Input event workspace. Assumed to be unmodified events "
                  "straight from LoadSwans");

  declareProperty(make_unique<WorkspaceProperty<>>("OutputWorkspace", "",
                                                   Direction::Output),
                  "Then name of the output EventWorkspace");

  declareProperty(
      "NoBeamCenter", true,
      "If true, the detector will not be moved according to the beam center");

  declareProperty("BeamCenterX", EMPTY_DBL(), "Beam position in X pixel "
                                              "coordinates (used only if "
                                              "UseConfigBeam is false)");
  declareProperty("BeamCenterY", EMPTY_DBL(), "Beam position in Y pixel "
                                              "coordinates (used only if "
                                              "UseConfigBeam is false)");

  declareProperty("WavelengthStep", 0.1, "Wavelength steps to be used when "
                                         "rebinning the data before performing "
                                         "the reduction");

  declareProperty("PreserveEvents", true,
                  "If true, the output workspace will be an event workspace");
  declareProperty(
      "SampleDetectorDistance", EMPTY_DBL(),
      "Sample to detector distance to use (overrides meta data), in mm");

  declareProperty("OutputMessage", "", Direction::Output);
  declareProperty("ReductionProperties", "__sans_reduction_properties",
                  Direction::Input);
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void SWANSLoad::exec() {

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

  g_log.debug() << "Set LoadAlgorithm if it's in the Properties." << std::endl;

  if (!reductionManager->existsProperty("LoadAlgorithm")) {
    auto loadProp = make_unique<AlgorithmProperty>("LoadAlgorithm");
    setPropertyValue("InputWorkspace", "");
    loadProp->setValue(toString());
    reductionManager->declareProperty(std::move(loadProp));
  }

  g_log.debug() << "Set instrument name." << std::endl;
  if (!reductionManager->existsProperty("InstrumentName")) {
    reductionManager->declareProperty(
        make_unique<PropertyWithValue<std::string>>("InstrumentName", "SWANS"));
  }

  // Output log
  m_output_message = "";

  // Check whether we need to load the data
  if (!inputEventWS) {
    g_log.debug() << "Loading data..." << std::endl;
    IAlgorithm_sptr loadAlg = createChildAlgorithm("LoadSwans", 0, 0.2);
    loadAlg->setProperty("FilenameData", fileName);
    loadAlg->execute();
    Workspace_sptr dataWS_asWks = loadAlg->getProperty("OutputWorkspace");
    dataWS = boost::dynamic_pointer_cast<MatrixWorkspace>(dataWS_asWks);
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

  // Get the sample-detector distance and move detector if necessary
  const double sdd = getProperty("SampleDetectorDistance");
  if (!isEmpty(sdd)) {
    g_log.debug() << "Set sample detector distance" << std::endl;

    dataWS->mutableRun().addProperty("sample_detector_distance", sdd, "mm",
                                     true);

    // Move the detector to its correct position
    std::string componentName =
        dataWS->getInstrument()->getStringParameter("detector-name")[0];

    IAlgorithm_sptr mvAlg =
        createChildAlgorithm("MoveInstrumentComponent", 0.2, 0.4);
    mvAlg->setProperty<MatrixWorkspace_sptr>("Workspace", dataWS);
    mvAlg->setProperty("ComponentName", componentName);
    mvAlg->setProperty("Z", sdd / 1000.0);
    mvAlg->setProperty("RelativePosition", false);
    mvAlg->executeAsChildAlg();
    g_log.information() << "Moving detector to " << sdd / 1000.0 << " meters"
                        << std::endl;
    m_output_message += "   Detector position: " +
                        Poco::NumberFormatter::format(sdd / 1000.0, 3) + " m\n";
  }

  //
  //
  //

  // Read in default beam center
  g_log.debug() << "Read in default beam center" << std::endl;
  m_center_x = getProperty("BeamCenterX");
  m_center_y = getProperty("BeamCenterY");
  const bool noBeamCenter = getProperty("NoBeamCenter");

  g_log.debug() << "Move beam center" << std::endl;
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
    // Add beam center to reduction properties, as the last beam center
    // position that was used.
    // This will give us our default position next time.
    if (!reductionManager->existsProperty("LatestBeamCenterX"))
      reductionManager->declareProperty(make_unique<PropertyWithValue<double>>(
          "LatestBeamCenterX", m_center_x));
    else
      reductionManager->setProperty("LatestBeamCenterX", m_center_x);
    if (!reductionManager->existsProperty("LatestBeamCenterY"))
      reductionManager->declareProperty(make_unique<PropertyWithValue<double>>(
          "LatestBeamCenterY", m_center_y));
    else
      reductionManager->setProperty("LatestBeamCenterY", m_center_y);
  }

  //
  // Convert to wavelength
  //
  g_log.debug() << "Converting to wavelength..." << std::endl;
  IAlgorithm_sptr convertUnits =
      createChildAlgorithm("ConvertUnits", 0.7, 0.71);
  convertUnits->setProperty<MatrixWorkspace_sptr>("InputWorkspace", dataWS);
  convertUnits->setProperty<MatrixWorkspace_sptr>("OutputWorkspace", dataWS);
  convertUnits->setProperty("Target", "Wavelength");
  convertUnits->executeAsChildAlg();

  //
  //
  // slits:
  setSourceSlitSize();

  //
  // Rebin so all the wavelength bins are aligned
  //

  const bool preserveEvents = getProperty("PreserveEvents");
  const double wl_step = getProperty("WavelengthStep");

  std::string params =
      Poco::NumberFormatter::format(dataWS->readX(0).front(), 2) + "," +
      Poco::NumberFormatter::format(wl_step) + "," +
      Poco::NumberFormatter::format(dataWS->readX(0).back(), 2);
  g_log.information() << "Rebin parameters: " << params << std::endl;
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

  g_log.information() << m_output_message << std::endl;
}

/// Move the detector according to the beam center
void SWANSLoad::moveToBeamCenter() {
  // Check that we have a beam center defined, otherwise set the
  // default beam center
  if (isEmpty(m_center_x) || isEmpty(m_center_y)) {
    EQSANSInstrument::getDefaultBeamCenter(dataWS, m_center_x, m_center_y);
    g_log.information() << "Setting beam center to [" << m_center_x << ", "
                        << m_center_y << "]" << std::endl;
    return;
  }

  // Check that the center of the detector really is at (0,0)
  int nx_pixels = static_cast<int>(
      dataWS->getInstrument()->getNumberParameter("number-of-x-pixels")[0]);
  int ny_pixels = static_cast<int>(
      dataWS->getInstrument()->getNumberParameter("number-of-y-pixels")[0]);
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
                      Poco::NumberFormatter::format(m_center_x) + ", " +
                      Poco::NumberFormatter::format(m_center_y) + "\n";
}

/// Get the source slit size from the slit information of the run properties
void SWANSLoad::setSourceSlitSize() {

  auto run = dataWS->run();
  const double pi = boost::math::constants::pi<double>();

  if (!run.hasProperty("IVA") && !run.hasProperty("IHA")) {
    m_output_message += "   Could not determine source aperture diameter: ";
    m_output_message += "slit parameters were not found in the run log\n";
    g_log.information() << "Slit parameters were not found in the run log..." << std::endl;
    return;
  }
  auto iva  = run.getPropertyValueAsType<double>("IVA");
  auto iha  = run.getPropertyValueAsType<double>("IHA");
  // aproximate square to circle and get the diameter
  auto slitsDiameter = 2 * std::sqrt(iva*iha / pi);

  dataWS->mutableRun().addProperty("source-aperture-diameter", slitsDiameter, "mm", true);
  m_output_message += "   Source aperture diameter: ";
  Poco::NumberFormatter::append(m_output_message, slitsDiameter, 1);
  m_output_message += " mm\n";
}

} // namespace WorkflowAlgorithms
} // namespace Mantid
