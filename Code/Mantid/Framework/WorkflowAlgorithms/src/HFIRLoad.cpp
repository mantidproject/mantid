//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidWorkflowAlgorithms/HFIRLoad.h"
#include "MantidWorkflowAlgorithms/HFIRInstrument.h"
#include "MantidAPI/WorkspaceValidators.h"
#include <MantidAPI/FileProperty.h>
#include "Poco/NumberFormatter.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidAPI/AlgorithmProperty.h"
#include "MantidAPI/PropertyManagerDataService.h"
#include "MantidKernel/PropertyManager.h"

namespace Mantid {
namespace WorkflowAlgorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(HFIRLoad)

using namespace Kernel;
using namespace API;
using namespace Geometry;
using namespace DataObjects;

void HFIRLoad::init() {
  declareProperty(
      new API::FileProperty("Filename", "", API::FileProperty::Load, ".xml"),
      "The name of the input file to load");
  declareProperty(
      new WorkspaceProperty<>("OutputWorkspace", "", Direction::Output),
      "Then name of the output workspace");
  declareProperty(
      "NoBeamCenter", false,
      "If true, the detector will not be moved according to the beam center");
  declareProperty("BeamCenterX", EMPTY_DBL(),
                  "Beam position in X pixel coordinates");
  declareProperty("BeamCenterY", EMPTY_DBL(),
                  "Beam position in Y pixel coordinates");
  declareProperty(
      "SampleDetectorDistance", EMPTY_DBL(),
      "Sample to detector distance to use (overrides meta data), in mm");
  declareProperty("SampleDetectorDistanceOffset", EMPTY_DBL(),
                  "Offset to the sample to detector distance (use only when "
                  "using the distance found in the meta data), in mm");

  // Optionally, we can specify the wavelength and wavelength spread and
  // overwrite
  // the value in the data file (used when the data file is not populated)
  auto mustBePositive = boost::make_shared<Kernel::BoundedValidator<double>>();
  mustBePositive->setLower(0.0);
  declareProperty(
      "Wavelength", EMPTY_DBL(), mustBePositive,
      "Wavelength value to use when loading the data file (Angstrom).");
  declareProperty(
      "WavelengthSpread", 0.1, mustBePositive,
      "Wavelength spread to use when loading the data file (default 0.0)");

  declareProperty("OutputMessage", "", Direction::Output);
  declareProperty("ReductionProperties", "__sans_reduction_properties",
                  Direction::Input);
}

/// Move the detector according to the beam center
void HFIRLoad::moveToBeamCenter() {
  double default_ctr_x_pix = 0.0;
  double default_ctr_y_pix = 0.0;
  double default_ctr_x = 0.0;
  double default_ctr_y = 0.0;
  HFIRInstrument::getDefaultBeamCenter(dataWS, default_ctr_x_pix,
                                       default_ctr_y_pix);
  HFIRInstrument::getCoordinateFromPixel(default_ctr_x_pix, default_ctr_y_pix,
                                         dataWS, default_ctr_x, default_ctr_y);

  // Check that we have a beam center defined, otherwise set the
  // default beam center
  if (isEmpty(m_center_x) || isEmpty(m_center_y)) {
    m_center_x = default_ctr_x_pix;
    m_center_y = default_ctr_y_pix;
    g_log.information() << "Setting beam center to ["
                        << Poco::NumberFormatter::format(m_center_x, 1) << ", "
                        << Poco::NumberFormatter::format(m_center_y, 1) << "]"
                        << std::endl;
    return;
  }

  double beam_ctr_x = 0.0;
  double beam_ctr_y = 0.0;
  HFIRInstrument::getCoordinateFromPixel(m_center_x, m_center_y, dataWS,
                                         beam_ctr_x, beam_ctr_y);

  IAlgorithm_sptr mvAlg =
      createChildAlgorithm("MoveInstrumentComponent", 0.5, 0.50);
  mvAlg->setProperty<MatrixWorkspace_sptr>("Workspace", dataWS);
  mvAlg->setProperty("ComponentName", "detector1");
  mvAlg->setProperty("X", default_ctr_x - beam_ctr_x);
  mvAlg->setProperty("Y", default_ctr_y - beam_ctr_y);
  mvAlg->setProperty("RelativePosition", true);
  mvAlg->executeAsChildAlg();
  g_log.information() << "Moving beam center to " << m_center_x << " "
                      << m_center_y << std::endl;
}

void HFIRLoad::exec() {
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

  // If the load algorithm isn't in the reduction properties, add it
  if (!reductionManager->existsProperty("LoadAlgorithm")) {
    AlgorithmProperty *algProp = new AlgorithmProperty("LoadAlgorithm");
    algProp->setValue(toString());
    reductionManager->declareProperty(algProp);
  }

  const std::string fileName = getPropertyValue("Filename");

  // Output log
  m_output_message = "";
  const double wavelength_input = getProperty("Wavelength");
  const double wavelength_spread_input = getProperty("WavelengthSpread");

  IAlgorithm_sptr loadAlg = createChildAlgorithm("LoadSpice2D", 0, 0.2);
  loadAlg->setProperty("Filename", fileName);
  if (!isEmpty(wavelength_input)) {
    loadAlg->setProperty("Wavelength", wavelength_input);
    loadAlg->setProperty("WavelengthSpread", wavelength_spread_input);
  }
  try {
    loadAlg->executeAsChildAlg();
  } catch (...) {
    // The only way HFIR SANS can load Nexus files is if it's loading data that
    // has already
    // been processed. This will only happen with sensitivity data.
    // So if we make it here and are still unable to load the file, assume it's
    // a sensitivity file.
    // This will cover the special case where the instrument scientist uses a
    // reduced data set
    // as a sensitivity data set.
    g_log.warning() << "Unable to load file as a SPICE file. Trying to load as "
                       "a Nexus file." << std::endl;
    loadAlg = createChildAlgorithm("Load", 0, 0.2);
    loadAlg->setProperty("Filename", fileName);
    loadAlg->executeAsChildAlg();
    Workspace_sptr dataWS_tmp = loadAlg->getProperty("OutputWorkspace");
    MatrixWorkspace_sptr dataWS =
        boost::dynamic_pointer_cast<MatrixWorkspace>(dataWS_tmp);
    dataWS->mutableRun().addProperty("is_sensitivity", 1, "", true);
    setProperty<MatrixWorkspace_sptr>("OutputWorkspace", dataWS);
    g_log.notice() << "Successfully loaded " << fileName
                   << " and setting sensitivity flag to True" << std::endl;
    return;
  }
  Workspace_sptr dataWS_tmp = loadAlg->getProperty("OutputWorkspace");
  dataWS = boost::dynamic_pointer_cast<MatrixWorkspace>(dataWS_tmp);

  // Get the sample-detector distance
  double sdd = 0.0;
  const double sample_det_dist = getProperty("SampleDetectorDistance");
  if (!isEmpty(sample_det_dist)) {
    sdd = sample_det_dist;
  } else {
    Mantid::Kernel::Property *prop =
        dataWS->run().getProperty("sample-detector-distance");
    Mantid::Kernel::PropertyWithValue<double> *dp =
        dynamic_cast<Mantid::Kernel::PropertyWithValue<double> *>(prop);
    sdd = *dp;

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
  g_log.information() << "Moving detector to " << sdd / 1000.0 << std::endl;
  m_output_message += "   Detector position: " +
                      Poco::NumberFormatter::format(sdd / 1000.0, 3) + " m\n";

  // Compute beam diameter at the detector
  double src_to_sample = 0.0;

  try {
    src_to_sample = HFIRInstrument::getSourceToSampleDistance(dataWS);
    dataWS->mutableRun().addProperty("source-sample-distance", src_to_sample,
                                     "mm", true);
    m_output_message +=
        "   Computed SSD from number of guides: " +
        Poco::NumberFormatter::format(src_to_sample / 1000.0, 3) + " \n";
  } catch (...) {
    Mantid::Kernel::Property *prop =
        dataWS->run().getProperty("source-sample-distance");
    Mantid::Kernel::PropertyWithValue<double> *dp =
        dynamic_cast<Mantid::Kernel::PropertyWithValue<double> *>(prop);
    src_to_sample = *dp;
    m_output_message +=
        "   Could not compute SSD from number of guides, taking: " +
        Poco::NumberFormatter::format(src_to_sample / 1000.0, 3) + " \n";
  };

  Mantid::Kernel::Property *prop =
      dataWS->run().getProperty("sample-aperture-diameter");
  Mantid::Kernel::PropertyWithValue<double> *dp =
      dynamic_cast<Mantid::Kernel::PropertyWithValue<double> *>(prop);
  double sample_apert = *dp;

  prop = dataWS->run().getProperty("source-aperture-diameter");
  dp = dynamic_cast<Mantid::Kernel::PropertyWithValue<double> *>(prop);
  double source_apert = *dp;

  const double beam_diameter =
      sdd / src_to_sample * (source_apert + sample_apert) + sample_apert;
  dataWS->mutableRun().addProperty("beam-diameter", beam_diameter, "mm", true);

  // Move the beam center to its proper position
  const bool noBeamCenter = getProperty("NoBeamCenter");
  if (!noBeamCenter) {
    m_center_x = getProperty("BeamCenterX");
    m_center_y = getProperty("BeamCenterY");
    if (isEmpty(m_center_x) && isEmpty(m_center_y)) {
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

    dataWS->mutableRun().addProperty("beam_center_x", m_center_x, "pixel",
                                     true);
    dataWS->mutableRun().addProperty("beam_center_y", m_center_y, "pixel",
                                     true);
    m_output_message += "   Beam center: " +
                        Poco::NumberFormatter::format(m_center_x, 1) + ", " +
                        Poco::NumberFormatter::format(m_center_y, 1) + "\n";
  } else {
    HFIRInstrument::getDefaultBeamCenter(dataWS, m_center_x, m_center_y);

    dataWS->mutableRun().addProperty("beam_center_x", m_center_x, "pixel",
                                     true);
    dataWS->mutableRun().addProperty("beam_center_y", m_center_y, "pixel",
                                     true);
    m_output_message += "   Default beam center: " +
                        Poco::NumberFormatter::format(m_center_x, 1) + ", " +
                        Poco::NumberFormatter::format(m_center_y, 1) + "\n";
  }

  setProperty<MatrixWorkspace_sptr>(
      "OutputWorkspace", boost::dynamic_pointer_cast<MatrixWorkspace>(dataWS));
  setPropertyValue("OutputMessage", m_output_message);
}

} // namespace WorkflowAlgorithms
} // namespace Mantid
