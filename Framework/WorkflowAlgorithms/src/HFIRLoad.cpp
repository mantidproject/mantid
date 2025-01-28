// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include <string>

#include "MantidAPI/AlgorithmProperty.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/Run.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/PropertyManager.h"
#include "MantidKernel/PropertyManagerDataService.h"
#include "MantidWorkflowAlgorithms/HFIRInstrument.h"
#include "MantidWorkflowAlgorithms/HFIRLoad.h"
#include "Poco/NumberFormatter.h"

namespace Mantid::WorkflowAlgorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(HFIRLoad)

using namespace Kernel;
using namespace API;
using namespace Geometry;
using namespace DataObjects;

void HFIRLoad::init() {
  declareProperty(std::make_unique<API::FileProperty>("Filename", "", API::FileProperty::Load, ".xml"),
                  "The name of the input file to load");
  declareProperty(std::make_unique<WorkspaceProperty<>>("OutputWorkspace", "", Direction::Output),
                  "Then name of the output workspace");
  declareProperty("NoBeamCenter", false, "If true, the detector will not be moved according to the beam center");
  declareProperty("BeamCenterX", EMPTY_DBL(), "Beam position in X pixel coordinates");
  declareProperty("BeamCenterY", EMPTY_DBL(), "Beam position in Y pixel coordinates");
  declareProperty("SampleDetectorDistance", EMPTY_DBL(),
                  "Sample to detector distance to use (overrides meta data), in mm");
  declareProperty("SampleDetectorDistanceOffset", EMPTY_DBL(),
                  "Offset to the sample to detector distance (use only when "
                  "using the distance found in the meta data), in mm."
                  "Not used when SampleDetectorDistance is provided.");

  // Optionally, we can specify the wavelength and wavelength spread and
  // overwrite
  // the value in the data file (used when the data file is not populated)
  auto mustBePositive = std::make_shared<Kernel::BoundedValidator<double>>();
  mustBePositive->setLower(0.0);
  declareProperty("Wavelength", EMPTY_DBL(), mustBePositive,
                  "Wavelength value to use when loading the data file (Angstrom).");
  declareProperty("WavelengthSpread", 0.1, mustBePositive,
                  "Wavelength spread to use when loading the data file (default 0.0)");

  declareProperty("OutputMessage", "", Direction::Output);
  declareProperty("ReductionProperties", "__sans_reduction_properties", Direction::Input);
}

/// Move the detector according to the beam center
void HFIRLoad::moveToBeamCenter(API::MatrixWorkspace_sptr &dataWS, double &center_x, double &center_y) {
  double default_ctr_x_pix = 0.0;
  double default_ctr_y_pix = 0.0;
  double default_ctr_x = 0.0;
  double default_ctr_y = 0.0;
  HFIRInstrument::getDefaultBeamCenter(dataWS, default_ctr_x_pix, default_ctr_y_pix);
  HFIRInstrument::getCoordinateFromPixel(default_ctr_x_pix, default_ctr_y_pix, dataWS, default_ctr_x, default_ctr_y);

  // Check that we have a beam center defined, otherwise set the
  // default beam center
  if (isEmpty(center_x) || isEmpty(center_y)) {
    center_x = default_ctr_x_pix;
    center_y = default_ctr_y_pix;
    g_log.information() << "Setting beam center to [" << Poco::NumberFormatter::format(center_x, 1) << ", "
                        << Poco::NumberFormatter::format(center_y, 1) << "]\n";
    return;
  }

  double beam_ctr_x = 0.0;
  double beam_ctr_y = 0.0;
  HFIRInstrument::getCoordinateFromPixel(center_x, center_y, dataWS, beam_ctr_x, beam_ctr_y);

  auto mvAlg = createChildAlgorithm("MoveInstrumentComponent", 0.5, 0.50);
  mvAlg->setProperty<MatrixWorkspace_sptr>("Workspace", dataWS);
  mvAlg->setProperty("ComponentName", "detector1");
  mvAlg->setProperty("X", default_ctr_x - beam_ctr_x);
  mvAlg->setProperty("Y", default_ctr_y - beam_ctr_y);
  mvAlg->setProperty("RelativePosition", true);
  mvAlg->executeAsChildAlg();
  g_log.information() << "Moving beam center to " << center_x << " " << center_y << '\n';
}

/**
 * Here the property "sample_detector_distance" is set.
 * This is the Sample - center of detector distance that all legacy algorithms
 * use
 * This was done by Mathieu before BioSANS had the wing detector
 */
void HFIRLoad::exec() {
  // Reduction property manager
  const std::string reductionManagerName = getProperty("ReductionProperties");
  std::shared_ptr<PropertyManager> reductionManager;
  if (PropertyManagerDataService::Instance().doesExist(reductionManagerName)) {
    reductionManager = PropertyManagerDataService::Instance().retrieve(reductionManagerName);
  } else {
    reductionManager = std::make_shared<PropertyManager>();
    PropertyManagerDataService::Instance().addOrReplace(reductionManagerName, reductionManager);
  }

  Progress progress(this, 0.0, 1.0, 5);

  progress.report();

  // If the load algorithm isn't in the reduction properties, add it
  if (!reductionManager->existsProperty("LoadAlgorithm")) {
    auto algProp = std::make_unique<AlgorithmProperty>("LoadAlgorithm");
    algProp->setValue(toString());
    reductionManager->declareProperty(std::move(algProp));
  }

  const std::string fileName = getPropertyValue("Filename");

  // Output log
  std::string output_message;
  const double wavelength_input = getProperty("Wavelength");
  const double wavelength_spread_input = getProperty("WavelengthSpread");

  progress.report("LoadSpice2D...");

  auto loadAlg = createChildAlgorithm("LoadSpice2D", 0, 0.2);
  loadAlg->setProperty("Filename", fileName);
  loadAlg->setPropertyValue("OutputWorkspace", getPropertyValue("OutputWorkspace"));
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
                       "a Nexus file.\n";
    loadAlg = createChildAlgorithm("Load", 0, 0.2);
    loadAlg->setProperty("Filename", fileName);
    loadAlg->executeAsChildAlg();
    Workspace_sptr dataWS_tmp = loadAlg->getProperty("OutputWorkspace");
    MatrixWorkspace_sptr dataWS = std::dynamic_pointer_cast<MatrixWorkspace>(dataWS_tmp);
    dataWS->mutableRun().addProperty("is_sensitivity", 1, "", true);
    setProperty<MatrixWorkspace_sptr>("OutputWorkspace", dataWS);
    g_log.notice() << "Successfully loaded " << fileName << " and setting sensitivity flag to True\n";
    return;
  }
  Workspace_sptr dataWS_tmp = loadAlg->getProperty("OutputWorkspace");
  AnalysisDataService::Instance().addOrReplace(getPropertyValue("OutputWorkspace"), dataWS_tmp);
  g_log.debug() << "Calling LoadSpice2D Done. OutputWorkspace name = " << dataWS_tmp->getName() << '\n';
  API::MatrixWorkspace_sptr dataWS = std::dynamic_pointer_cast<MatrixWorkspace>(dataWS_tmp);

  // Get the sample-detector distance
  // If SampleDetectorDistance is provided, use it!
  // Otherwise get's "sample-detector-distance" from the data file
  // And uses SampleDetectorDistanceOffset if given!
  double sdd = 0.0;
  const double sample_det_dist = getProperty("SampleDetectorDistance");
  if (!isEmpty(sample_det_dist)) {
    g_log.debug() << "Getting the SampleDetectorDistance = " << sample_det_dist
                  << " from the Algorithm input property.\n";
    sdd = sample_det_dist;
  } else {
    const std::string sddName = "total-sample-detector-distance";
    Mantid::Kernel::Property *prop = dataWS->run().getProperty(sddName);
    const auto *dp = dynamic_cast<Mantid::Kernel::PropertyWithValue<double> *>(prop);
    if (!dp) {
      throw std::runtime_error("Could not cast (interpret) the property " + sddName +
                               " as a floating point numeric value.");
    }
    sdd = *dp;

    // Modify SDD according to offset if given
    const double sample_det_offset = getProperty("SampleDetectorDistanceOffset");
    if (!isEmpty(sample_det_offset)) {
      sdd += sample_det_offset;
    }
  }
  dataWS->mutableRun().addProperty("sample_detector_distance", sdd, "mm", true);
  g_log.debug() << "FINAL: Using Total Sample Detector Distance = " << sdd << "\n";

  progress.report("MoveInstrumentComponent...");

  // Move the detector to its correct position
  auto mvAlg = createChildAlgorithm("MoveInstrumentComponent", 0.2, 0.4);
  mvAlg->setProperty<MatrixWorkspace_sptr>("Workspace", dataWS);
  mvAlg->setProperty("ComponentName", "detector1");
  mvAlg->setProperty("Z", sdd / 1000.0);
  mvAlg->setProperty("RelativePosition", false);
  mvAlg->executeAsChildAlg();
  g_log.information() << "Moving detector to " << sdd / 1000.0 << '\n';
  output_message += "   Detector position: " + Poco::NumberFormatter::format(sdd / 1000.0, 3) + " m\n";

  // Compute beam diameter at the detector
  double src_to_sample = 0.0;

  try {
    src_to_sample = HFIRInstrument::getSourceToSampleDistance(dataWS);
    dataWS->mutableRun().addProperty("source-sample-distance", src_to_sample, "mm", true);
    output_message +=
        "   Computed SSD from number of guides: " + Poco::NumberFormatter::format(src_to_sample / 1000.0, 3) + " \n";
  } catch (...) {
    Mantid::Kernel::Property *prop = dataWS->run().getProperty("source-sample-distance");
    const auto *dp = dynamic_cast<Mantid::Kernel::PropertyWithValue<double> *>(prop);
    src_to_sample = *dp;
    output_message += "   Could not compute SSD from number of guides, taking: " +
                      Poco::NumberFormatter::format(src_to_sample / 1000.0, 3) + " \n";
  }

  const std::string sampleADName = "sample-aperture-diameter";
  Mantid::Kernel::Property *prop = dataWS->run().getProperty(sampleADName);
  auto *dp = dynamic_cast<Mantid::Kernel::PropertyWithValue<double> *>(prop);
  if (!dp) {
    throw std::runtime_error("Could not cast (interpret) the property " + sampleADName +
                             " as a floating point numeric value.");
  }
  double sample_apert = *dp;

  const std::string sourceADName = "source-aperture-diameter";
  prop = dataWS->run().getProperty(sourceADName);
  dp = dynamic_cast<Mantid::Kernel::PropertyWithValue<double> *>(prop);
  if (!dp) {
    throw std::runtime_error("Could not cast (interpret) the property " + sourceADName +
                             " as a floating point numeric value.");
  }
  double source_apert = *dp;

  const double beam_diameter = sdd / src_to_sample * (source_apert + sample_apert) + sample_apert;
  dataWS->mutableRun().addProperty("beam-diameter", beam_diameter, "mm", true);

  progress.report("Move to center beam...");

  double center_x = 0;
  double center_y = 0;

  // Move the beam center to its proper position
  const bool noBeamCenter = getProperty("NoBeamCenter");
  if (!noBeamCenter) {
    center_x = getProperty("BeamCenterX");
    center_y = getProperty("BeamCenterY");
    if (isEmpty(center_x) && isEmpty(center_y)) {
      if (reductionManager->existsProperty("LatestBeamCenterX") &&
          reductionManager->existsProperty("LatestBeamCenterY")) {
        center_x = reductionManager->getProperty("LatestBeamCenterX");
        center_y = reductionManager->getProperty("LatestBeamCenterY");
      }
    }

    moveToBeamCenter(dataWS, center_x, center_y);

    progress.report();

    // Add beam center to reduction properties, as the last beam center position
    // that was used.
    // This will give us our default position next time.
    if (!reductionManager->existsProperty("LatestBeamCenterX"))
      reductionManager->declareProperty(std::make_unique<PropertyWithValue<double>>("LatestBeamCenterX", center_x));
    else
      reductionManager->setProperty("LatestBeamCenterX", center_x);
    if (!reductionManager->existsProperty("LatestBeamCenterY"))
      reductionManager->declareProperty(std::make_unique<PropertyWithValue<double>>("LatestBeamCenterY", center_y));
    else
      reductionManager->setProperty("LatestBeamCenterY", center_y);

    dataWS->mutableRun().addProperty("beam_center_x", center_x, "pixel", true);
    dataWS->mutableRun().addProperty("beam_center_y", center_y, "pixel", true);
    output_message += "   Beam center: " + Poco::NumberFormatter::format(center_x, 1) + ", " +
                      Poco::NumberFormatter::format(center_y, 1) + "\n";
  } else {
    HFIRInstrument::getDefaultBeamCenter(dataWS, center_x, center_y);

    dataWS->mutableRun().addProperty("beam_center_x", center_x, "pixel", true);
    dataWS->mutableRun().addProperty("beam_center_y", center_y, "pixel", true);
    output_message += "   Default beam center: " + Poco::NumberFormatter::format(center_x, 1) + ", " +
                      Poco::NumberFormatter::format(center_y, 1) + "\n";
  }

  setProperty<MatrixWorkspace_sptr>("OutputWorkspace", std::dynamic_pointer_cast<MatrixWorkspace>(dataWS));
  setPropertyValue("OutputMessage", output_message);
}

} // namespace Mantid::WorkflowAlgorithms
