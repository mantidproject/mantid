//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidWorkflowAlgorithms/SetupILLSANSReduction.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/RebinParamsValidator.h"
#include "MantidKernel/EnabledWhenProperty.h"
#include "MantidKernel/VisibleWhenProperty.h"
#include "MantidAPI/FileProperty.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidAPI/AlgorithmProperty.h"
#include "MantidKernel/PropertyManagerDataService.h"
#include "MantidKernel/PropertyManager.h"
#include "Poco/NumberFormatter.h"

namespace Mantid {
namespace WorkflowAlgorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(SetupILLSANSReduction)

using namespace Kernel;
using namespace API;
using namespace Geometry;

void SetupILLSANSReduction::init() {

  // Beam center
  const std::string center_grp = "Beam Center";
  std::vector<std::string> centerOptions{"None", "Value", "DirectBeam",
                                         "Scattering"};
  declareProperty("BeamCenterMethod", "None",
                  boost::make_shared<StringListValidator>(centerOptions),
                  "Method for determining the data beam center");

  //    Option 1: Set beam center by hand
  declareProperty("BeamCenterX", EMPTY_DBL(),
                  "Position of the beam center, in pixel");
  declareProperty("BeamCenterY", EMPTY_DBL(),
                  "Position of the beam center, in pixel");
  setPropertySettings("BeamCenterX",
                      make_unique<VisibleWhenProperty>("BeamCenterMethod",
                                                       IS_EQUAL_TO, "Value"));
  setPropertySettings("BeamCenterY",
                      make_unique<VisibleWhenProperty>("BeamCenterMethod",
                                                       IS_EQUAL_TO, "Value"));

  //    Option 2: Find it (expose properties from FindCenterOfMass)
  declareProperty(
      make_unique<API::FileProperty>(
          "BeamCenterFile", "", API::FileProperty::OptionalLoad, ".nxs"),
      "The name of the beam center file to load");
  setPropertySettings("BeamCenterFile",
                      make_unique<VisibleWhenProperty>(
                          "BeamCenterMethod", IS_NOT_EQUAL_TO, "None"));

  auto positiveDouble = boost::make_shared<BoundedValidator<double>>();
  positiveDouble->setLower(0);
  declareProperty(
      "BeamRadius", EMPTY_DBL(),
      "Radius of the beam area used the exclude the beam when calculating "
      "the center of mass of the scattering pattern [pixels]. Default=3.0");
  setPropertySettings("BeamRadius",
                      make_unique<VisibleWhenProperty>(
                          "BeamCenterMethod", IS_EQUAL_TO, "Scattering"));

  // -- Define group --
  setPropertyGroup("BeamCenterMethod", center_grp);
  setPropertyGroup("BeamCenterX", center_grp);
  setPropertyGroup("BeamCenterY", center_grp);
  setPropertyGroup("BeamCenterFile", center_grp);
  setPropertyGroup("BeamRadius", center_grp);

  // Dark current
  const std::string dark_grp = "Dark Current (Cd/B4C)";
  declareProperty(
      make_unique<API::FileProperty>(
          "DarkCurrentFile", "", API::FileProperty::OptionalLoad, ".nxs"),
      "The name of the input event Nexus file to load as dark current.");
  setPropertyGroup("DarkCurrentFile", dark_grp);

  // Normalisation
  const std::string norm_grp = "Normalisation";
  std::vector<std::string> incidentBeamNormOptions;
  incidentBeamNormOptions.emplace_back("None");
  // The data will be normalised to the monitor counts
  incidentBeamNormOptions.emplace_back("Monitor");
  // The data will be normalised to the total charge only (no beam profile)
  incidentBeamNormOptions.emplace_back("Timer");
  this->declareProperty(
      "Normalisation", "None",
      boost::make_shared<StringListValidator>(incidentBeamNormOptions),
      "Options for data normalisation");
  setPropertyGroup("Normalisation", norm_grp);

  // Masking
  const std::string mask_grp = "Mask";
  declareProperty(make_unique<WorkspaceProperty<MatrixWorkspace>>(
      "MaskedWorkspace", "", Direction::Input, PropertyMode::Optional));
  declareProperty(
      make_unique<ArrayProperty<int>>("MaskedEdges"),
      "Number of pixels to mask on the edges: X-low, X-high, Y-low, Y-high");
  declareProperty("MaskedComponent","","Component names to mask the edges of.");
  declareProperty(make_unique<ArrayProperty<int>>("MaskedDetectorList"),
                  "List of detector IDs to be masked");
  declareProperty("MaskedFullComponent", "", "Component name to mask entirely.");
  setPropertyGroup("MaskedWorkspace", mask_grp);
  setPropertyGroup("MaskedEdges", mask_grp);
  setPropertyGroup("MaskedComponent", mask_grp);
  setPropertyGroup("MaskedFullComponent", mask_grp);
  setPropertyGroup("MaskedDetectorList", mask_grp);

  // Solid angle correction
  const std::string load_grp = "Solid Angle Correction";
  declareProperty(
      "SolidAngleCorrection", true,
      "If true, the solide angle correction will be applied to the data");
  declareProperty(
      "DetectorTubes", false,
      "If true, the solid angle correction for tube detectors will be applied");
  setPropertyGroup("SolidAngleCorrection", load_grp);
  setPropertyGroup("DetectorTubes", load_grp);

  // Sensitivity
  const std::string eff_grp = "Sensitivity";
  declareProperty(
      make_unique<API::FileProperty>(
          "SensitivityFile", "", API::FileProperty::OptionalLoad, ".nxs"),
      "Flood field or sensitivity file.");
  declareProperty(
      "MinEfficiency", EMPTY_DBL(), positiveDouble,
      "Minimum efficiency for a pixel to be considered (default: no minimum).");
  declareProperty(
      "MaxEfficiency", EMPTY_DBL(), positiveDouble,
      "Maximum efficiency for a pixel to be considered (default: no maximum).");
  declareProperty("UseDefaultDC", true, "If true, the dark current subtracted "
                                        "from the sample data will also be "
                                        "subtracted from the flood field.");
  declareProperty(make_unique<API::FileProperty>(
                      "SensitivityDarkCurrentFile", "",
                      API::FileProperty::OptionalLoad, ".nxs"),
                  "The name of the input file to load as dark current.");
  // - sensitivity beam center
  declareProperty("SensitivityBeamCenterMethod", "None",
                  boost::make_shared<StringListValidator>(centerOptions),
                  "Method for determining the sensitivity data beam center");

  //    Option 1: Set beam center by hand
  declareProperty("SensitivityBeamCenterX", EMPTY_DBL(),
                  "Sensitivity beam center location in X [pixels]");
  setPropertySettings("SensitivityBeamCenterX",
                      make_unique<VisibleWhenProperty>(
                          "SensitivityBeamCenterMethod", IS_EQUAL_TO, "Value"));

  declareProperty("SensitivityBeamCenterY", EMPTY_DBL(),
                  "Sensitivity beam center location in Y [pixels]");
  setPropertySettings("SensitivityBeamCenterY",
                      make_unique<VisibleWhenProperty>(
                          "SensitivityBeamCenterMethod", IS_EQUAL_TO, "Value"));

  //    Option 2: Find it (expose properties from FindCenterOfMass)
  declareProperty(
      make_unique<API::FileProperty>("SensitivityBeamCenterFile", "",
                                     API::FileProperty::OptionalLoad, ".xml"),
      "The name of the input data file to load");
  setPropertySettings(
      "SensitivityBeamCenterFile",
      make_unique<VisibleWhenProperty>("SensitivityBeamCenterMethod",
                                       IS_NOT_EQUAL_TO, "None"));

  declareProperty(
      "SensitivityBeamCenterRadius", EMPTY_DBL(),
      "Radius of the beam area used the exclude the beam when calculating "
      "the center of mass of the scattering pattern [pixels]. Default=3.0");
  setPropertySettings("SensitivityBeamCenterRadius",
                      make_unique<VisibleWhenProperty>(
                          "BeamCenterMethod", IS_EQUAL_TO, "Scattering"));

  declareProperty("OutputSensitivityWorkspace", "",
                  "Name to give the sensitivity workspace");

  // -- Define group --
  setPropertyGroup("SensitivityFile", eff_grp);
  setPropertyGroup("MinEfficiency", eff_grp);
  setPropertyGroup("MaxEfficiency", eff_grp);
  setPropertyGroup("UseDefaultDC", eff_grp);
  setPropertyGroup("SensitivityDarkCurrentFile", eff_grp);
  setPropertyGroup("SensitivityBeamCenterMethod", eff_grp);
  setPropertyGroup("SensitivityBeamCenterX", eff_grp);
  setPropertyGroup("SensitivityBeamCenterY", eff_grp);
  setPropertyGroup("SensitivityBeamCenterFile", eff_grp);
  setPropertyGroup("SensitivityBeamCenterRadius", eff_grp);
  setPropertyGroup("OutputSensitivityWorkspace", eff_grp);

  // Transmission
  const std::string trans_grp = "Transmission";
  std::vector<std::string> transOptions{"Value", "DirectBeam"};
  declareProperty("TransmissionMethod", "Value",
                  boost::make_shared<StringListValidator>(transOptions),
                  "Transmission determination method");

  // - Transmission value entered by hand
  declareProperty("TransmissionValue", EMPTY_DBL(), positiveDouble,
                  "Transmission value.");
  setPropertySettings("TransmissionValue",
                      make_unique<VisibleWhenProperty>("TransmissionMethod",
                                                       IS_EQUAL_TO, "Value"));
  declareProperty("TransmissionError", EMPTY_DBL(), positiveDouble,
                  "Transmission error.");
  setPropertySettings("TransmissionError",
                      make_unique<VisibleWhenProperty>("TransmissionMethod",
                                                       IS_EQUAL_TO, "Value"));

  // - Direct beam method transmission calculation
  declareProperty(
      "TransmissionBeamRadius", 3.0,
      "Radius of the beam area used to compute the transmission [pixels]");
  setPropertySettings("TransmissionBeamRadius",
                      make_unique<VisibleWhenProperty>(
                          "TransmissionMethod", IS_EQUAL_TO, "DirectBeam"));
  declareProperty(
      make_unique<API::FileProperty>("TransmissionSampleDataFile", "",
                                     API::FileProperty::OptionalLoad, ".xml"),
      "Sample data file for transmission calculation");
  setPropertySettings("TransmissionSampleDataFile",
                      make_unique<VisibleWhenProperty>(
                          "TransmissionMethod", IS_EQUAL_TO, "DirectBeam"));
  declareProperty(
      make_unique<API::FileProperty>("TransmissionEmptyDataFile", "",
                                     API::FileProperty::OptionalLoad, ".xml"),
      "Empty data file for transmission calculation");
  setPropertySettings("TransmissionEmptyDataFile",
                      make_unique<VisibleWhenProperty>(
                          "TransmissionMethod", IS_EQUAL_TO, "DirectBeam"));

  // - transmission beam center
  declareProperty("TransmissionBeamCenterMethod", "None",
                  boost::make_shared<StringListValidator>(centerOptions),
                  "Method for determining the transmission data beam center");
  setPropertySettings("TransmissionBeamCenterMethod",
                      make_unique<VisibleWhenProperty>(
                          "TransmissionMethod", IS_EQUAL_TO, "DirectBeam"));

  //    Option 1: Set beam center by hand
  declareProperty("TransmissionBeamCenterX", EMPTY_DBL(),
                  "Transmission beam center location in X [pixels]");
  setPropertySettings("TransmissionBeamCenterX",
                      make_unique<VisibleWhenProperty>(
                          "TransmissionMethod", IS_EQUAL_TO, "DirectBeam"));
  declareProperty("TransmissionBeamCenterY", EMPTY_DBL(),
                  "Transmission beam center location in Y [pixels]");
  setPropertySettings("TransmissionBeamCenterY",
                      make_unique<VisibleWhenProperty>(
                          "TransmissionMethod", IS_EQUAL_TO, "DirectBeam"));

  //    Option 2: Find it (expose properties from FindCenterOfMass)
  declareProperty(
      make_unique<API::FileProperty>("TransmissionBeamCenterFile", "",
                                     API::FileProperty::OptionalLoad, ".xml"),
      "The name of the input data file to load");
  setPropertySettings("TransmissionBeamCenterFile",
                      make_unique<VisibleWhenProperty>(
                          "TransmissionMethod", IS_EQUAL_TO, "DirectBeam"));

  declareProperty(
      make_unique<API::FileProperty>("TransmissionDarkCurrentFile", "",
                                     API::FileProperty::OptionalLoad, ".xml"),
      "The name of the input data file to load as transmission dark current.");
  setPropertySettings("TransmissionDarkCurrentFile",
                      make_unique<VisibleWhenProperty>(
                          "TransmissionMethod", IS_NOT_EQUAL_TO, "Value"));

  declareProperty(
      "TransmissionUseSampleDC", true,
      "If true, the sample dark current will be used IF a dark current file is"
      "not set.");
  setPropertySettings("TransmissionUseSampleDC",
                      make_unique<VisibleWhenProperty>(
                          "TransmissionMethod", IS_NOT_EQUAL_TO, "Value"));

  declareProperty(
      "ThetaDependentTransmission", true,
      "If true, a theta-dependent transmission correction will be applied.");

  // -- Define group --
  setPropertyGroup("TransmissionMethod", trans_grp);
  setPropertyGroup("TransmissionValue", trans_grp);
  setPropertyGroup("TransmissionError", trans_grp);
  setPropertyGroup("TransmissionBeamRadius", trans_grp);
  setPropertyGroup("TransmissionSampleDataFile", trans_grp);
  setPropertyGroup("TransmissionEmptyDataFile", trans_grp);
  setPropertyGroup("TransmissionBeamCenterMethod", trans_grp);
  setPropertyGroup("TransmissionBeamCenterX", trans_grp);
  setPropertyGroup("TransmissionBeamCenterY", trans_grp);
  setPropertyGroup("TransmissionBeamCenterFile", trans_grp);
  setPropertyGroup("TransmissionDarkCurrentFile", trans_grp);
  setPropertyGroup("TransmissionUseSampleDC", trans_grp);
  setPropertyGroup("ThetaDependentTransmission", trans_grp);

  // Background options
  const std::string bck_grp = "Background";
  declareProperty("BackgroundFiles", "", "Background data files");
  declareProperty("BckTransmissionMethod", "Value",
                  boost::make_shared<StringListValidator>(transOptions),
                  "Transmission determination method");

  // - Transmission value entered by hand
  declareProperty("BckTransmissionValue", EMPTY_DBL(), positiveDouble,
                  "Transmission value.");
  setPropertySettings("BckTransmissionValue",
                      make_unique<VisibleWhenProperty>("BckTransmissionMethod",
                                                       IS_EQUAL_TO, "Value"));

  declareProperty("BckTransmissionError", EMPTY_DBL(), positiveDouble,
                  "Transmission error.");
  setPropertySettings("BckTransmissionError",
                      make_unique<VisibleWhenProperty>("BckTransmissionMethod",
                                                       IS_EQUAL_TO, "Value"));

  // - Direct beam method transmission calculation
  declareProperty(
      "BckTransmissionBeamRadius", 3.0,
      "Radius of the beam area used to compute the transmission [pixels]");
  setPropertySettings("BckTransmissionBeamRadius",
                      make_unique<VisibleWhenProperty>(
                          "BckTransmissionMethod", IS_EQUAL_TO, "DirectBeam"));
  declareProperty(
      make_unique<API::FileProperty>("BckTransmissionSampleDataFile", "",
                                     API::FileProperty::OptionalLoad, ".xml"),
      "Sample data file for transmission calculation");
  setPropertySettings("BckTransmissionSampleDataFile",
                      make_unique<VisibleWhenProperty>(
                          "BckTransmissionMethod", IS_EQUAL_TO, "DirectBeam"));
  declareProperty(
      make_unique<API::FileProperty>("BckTransmissionEmptyDataFile", "",
                                     API::FileProperty::OptionalLoad, ".xml"),
      "Empty data file for transmission calculation");
  setPropertySettings("BckTransmissionEmptyDataFile",
                      make_unique<VisibleWhenProperty>(
                          "BckTransmissionMethod", IS_EQUAL_TO, "DirectBeam"));

  // - transmission beam center
  declareProperty("BckTransmissionBeamCenterMethod", "None",
                  boost::make_shared<StringListValidator>(centerOptions),
                  "Method for determining the transmission data beam center");
  setPropertySettings("BckTransmissionBeamCenterMethod",
                      make_unique<VisibleWhenProperty>(
                          "BckTransmissionMethod", IS_EQUAL_TO, "DirectBeam"));
  //    Option 1: Set beam center by hand
  declareProperty("BckTransmissionBeamCenterX", EMPTY_DBL(),
                  "Transmission beam center location in X [pixels]");
  setPropertySettings("BckTransmissionBeamCenterX",
                      make_unique<VisibleWhenProperty>(
                          "BckTransmissionMethod", IS_EQUAL_TO, "DirectBeam"));
  declareProperty("BckTransmissionBeamCenterY", EMPTY_DBL(),
                  "Transmission beam center location in Y [pixels]");
  //    Option 2: Find it (expose properties from FindCenterOfMass)
  setPropertySettings("BckTransmissionBeamCenterY",
                      make_unique<VisibleWhenProperty>(
                          "BckTransmissionMethod", IS_EQUAL_TO, "DirectBeam"));
  declareProperty(
      make_unique<API::FileProperty>("BckTransmissionBeamCenterFile", "",
                                     API::FileProperty::OptionalLoad, ".xml"),
      "The name of the input data file to load");
  setPropertySettings("BckTransmissionBeamCenterFile",
                      make_unique<VisibleWhenProperty>(
                          "BckTransmissionMethod", IS_EQUAL_TO, "DirectBeam"));

  declareProperty(
      make_unique<API::FileProperty>("BckTransmissionDarkCurrentFile", "",
                                     API::FileProperty::OptionalLoad, ".xml"),
      "The name of the input data file to load as background "
      "transmission dark current.");
  setPropertySettings("BckTransmissionDarkCurrentFile",
                      make_unique<VisibleWhenProperty>("BckTransmissionMethod",
                                                       IS_EQUAL_TO,
                                                       "BeamSpreader"));
  declareProperty(
      "BckThetaDependentTransmission", true,
      "If true, a theta-dependent transmission correction will be applied.");

  setPropertyGroup("BackgroundFiles", bck_grp);
  setPropertyGroup("BckTransmissionMethod", bck_grp);
  setPropertyGroup("BckTransmissionValue", bck_grp);
  setPropertyGroup("BckTransmissionError", bck_grp);
  setPropertyGroup("BckTransmissionBeamRadius", bck_grp);
  setPropertyGroup("BckTransmissionSampleDataFile", bck_grp);
  setPropertyGroup("BckTransmissionEmptyDataFile", bck_grp);
  setPropertyGroup("BckTransmissionBeamCenterMethod", bck_grp);
  setPropertyGroup("BckTransmissionBeamCenterX", bck_grp);
  setPropertyGroup("BckTransmissionBeamCenterY", bck_grp);
  setPropertyGroup("BckTransmissionBeamCenterFile", bck_grp);
  setPropertyGroup("BckTransmissionDarkCurrentFile", bck_grp);
  setPropertyGroup("BckThetaDependentTransmission", bck_grp);

  // Absolute scale
  const std::string abs_scale_grp = "Absolute Scale";
  std::vector<std::string> scaleOptions;
  scaleOptions.emplace_back("None");
  scaleOptions.emplace_back("Value");
  scaleOptions.emplace_back("ReferenceData");
  declareProperty("AbsoluteScaleMethod", "None",
                  boost::make_shared<StringListValidator>(scaleOptions),
                  "Absolute scale correction method");
  declareProperty("AbsoluteScalingFactor", 1.0, "Absolute scaling factor");
  setPropertySettings("AbsoluteScalingFactor",
                      make_unique<VisibleWhenProperty>("AbsoluteScaleMethod",
                                                       IS_EQUAL_TO, "Value"));

  declareProperty(
      make_unique<API::FileProperty>("AbsoluteScalingReferenceFilename", "",
                                     API::FileProperty::OptionalLoad, ".xml"));
  setPropertySettings("AbsoluteScalingReferenceFilename",
                      make_unique<VisibleWhenProperty>(
                          "AbsoluteScaleMethod", IS_EQUAL_TO, "ReferenceData"));
  declareProperty(
      "AbsoluteScalingBeamDiameter", 0.0,
      "Beamstop diameter for computing the absolute scale factor [mm]. "
      "Read from file if not supplied.");
  setPropertySettings("AbsoluteScalingBeamDiameter",
                      make_unique<VisibleWhenProperty>(
                          "AbsoluteScaleMethod", IS_EQUAL_TO, "ReferenceData"));
  declareProperty(
      "AbsoluteScalingAttenuatorTrans", 1.0,
      "Attenuator transmission value for computing the absolute scale factor");
  setPropertySettings("AbsoluteScalingAttenuatorTrans",
                      make_unique<VisibleWhenProperty>(
                          "AbsoluteScaleMethod", IS_EQUAL_TO, "ReferenceData"));
  declareProperty("AbsoluteScalingApplySensitivity", false,
                  "Apply sensitivity correction to the reference data "
                  "when computing the absolute scale factor");
  setPropertySettings("AbsoluteScalingApplySensitivity",
                      make_unique<VisibleWhenProperty>(
                          "AbsoluteScaleMethod", IS_EQUAL_TO, "ReferenceData"));

  setPropertyGroup("AbsoluteScaleMethod", abs_scale_grp);
  setPropertyGroup("AbsoluteScalingFactor", abs_scale_grp);
  setPropertyGroup("AbsoluteScalingReferenceFilename", abs_scale_grp);
  setPropertyGroup("AbsoluteScalingBeamDiameter", abs_scale_grp);
  setPropertyGroup("AbsoluteScalingAttenuatorTrans", abs_scale_grp);
  setPropertyGroup("AbsoluteScalingApplySensitivity", abs_scale_grp);

  // Sample thickness
  const std::string sample_group = "Sample Thickness";
  declareProperty("SampleThickness", EMPTY_DBL(), "Sample thickness [cm]");
  setPropertyGroup("SampleThickness", sample_group);

  // I(Q) calculation
  const std::string iq1d_grp = "I(q) Transformation";
  declareProperty("DoAzimuthalAverage", true);
  auto positiveInt = boost::make_shared<BoundedValidator<int>>();
  positiveInt->setLower(0);
  declareProperty("IQNumberOfBins", 100, positiveInt,
                  "Number of I(q) bins when binning is not specified");
  declareProperty("IQLogBinning", false,
                  "I(q) log binning when binning is not specified");
  declareProperty("ComputeResolution", true,
                  "If true the Q resolution will be computed");

  declareProperty("Do2DReduction", false);
  declareProperty("IQ2DNumberOfBins", 100, positiveInt,
                  "Number of I(qx,qy) bins.");

  // -- Define group --
  setPropertyGroup("DoAzimuthalAverage", iq1d_grp);
  setPropertyGroup("IQNumberOfBins", iq1d_grp);
  setPropertyGroup("IQLogBinning", iq1d_grp);
  setPropertyGroup("ComputeResolution", iq1d_grp);
  setPropertyGroup("Do2DReduction", iq1d_grp);
  setPropertyGroup("IQ2DNumberOfBins", iq1d_grp);

  // Outputs
  const std::string out_grp = "Output";
  declareProperty("ProcessInfo", "", "Additional process information");
  declareProperty("OutputDirectory", "",
                  "Directory to put the output files in");
  declareProperty("OutputMessage", "", Direction::Output);
  declareProperty("ReductionProperties", "__sans_reduction_properties",
                  Direction::Input);
  setPropertyGroup("ProcessInfo", out_grp);
  setPropertyGroup("OutputMessage", out_grp);
  setPropertyGroup("OutputDirectory", out_grp);
  setPropertyGroup("ReductionProperties", out_grp);
}

void SetupILLSANSReduction::exec() {
  // Reduction property manager
  const std::string reductionManagerName = getProperty("ReductionProperties");
  if (reductionManagerName.empty()) {
    g_log.error() << "ERROR: Reduction Property Manager name is empty\n";
    return;
  }
  boost::shared_ptr<PropertyManager> reductionManager =
      boost::make_shared<PropertyManager>();
  PropertyManagerDataService::Instance().addOrReplace(reductionManagerName,
                                                      reductionManager);

  // Store name of the instrument
  reductionManager->declareProperty(
      make_unique<PropertyWithValue<std::string>>("InstrumentName", "D33"));

  // Store additional (and optional) process information
  const std::string processInfo = getProperty("ProcessInfo");
  reductionManager->declareProperty(
      Kernel::make_unique<PropertyWithValue<std::string>>("ProcessInfo",
                                                          processInfo));

  // Store the output directory
  const std::string outputDirectory = getProperty("OutputDirectory");
  reductionManager->declareProperty(
      Kernel::make_unique<PropertyWithValue<std::string>>("OutputDirectory",
                                                          outputDirectory));

  // Store normalization algorithm
  const std::string normalization = getProperty("Normalisation");

  if (!boost::contains(normalization, "None")) {
    // If we normalize to monitor, force the loading of monitor data
    IAlgorithm_sptr normAlg = createChildAlgorithm("HFIRSANSNormalise");
    normAlg->setProperty("NormalisationType", normalization);
    // normAlg->setPropertyValue("ReductionProperties", reductionManagerName);
    auto algProp = make_unique<AlgorithmProperty>("NormaliseAlgorithm");
    algProp->setValue(normAlg->toString());
    reductionManager->declareProperty(std::move(algProp));
  }

  // Load algorithm
  IAlgorithm_sptr loadAlg = createChildAlgorithm("LoadILLSANS");
  auto algProp = make_unique<AlgorithmProperty>("LoadAlgorithm");
  algProp->setValue(loadAlg->toString());
  reductionManager->declareProperty(std::move(algProp));

  // Store dark current algorithm
  const std::string darkCurrentFile = getPropertyValue("DarkCurrentFile");
  if (!darkCurrentFile.empty()) {
    IAlgorithm_sptr darkAlg =
        createChildAlgorithm("EQSANSDarkCurrentSubtraction");
    darkAlg->setProperty("Filename", darkCurrentFile);
    darkAlg->setProperty("OutputDarkCurrentWorkspace", "");
    darkAlg->setPropertyValue("ReductionProperties", reductionManagerName);
    auto dcalgProp = make_unique<AlgorithmProperty>("DarkCurrentAlgorithm");
    dcalgProp->setValue(darkAlg->toString());
    reductionManager->declareProperty(std::move(dcalgProp));
  }

  // Store default dark current algorithm
  IAlgorithm_sptr darkDefaultAlg =
      createChildAlgorithm("EQSANSDarkCurrentSubtraction");
  darkDefaultAlg->setProperty("OutputDarkCurrentWorkspace", "");
  darkDefaultAlg->setPropertyValue("ReductionProperties", reductionManagerName);
  auto ddcalgProp =
      make_unique<AlgorithmProperty>("DefaultDarkCurrentAlgorithm");
  ddcalgProp->setValue(darkDefaultAlg->toString());
  reductionManager->declareProperty(std::move(ddcalgProp));

  // Solid angle correction
  const bool solidAngleCorrection = getProperty("SolidAngleCorrection");
  if (solidAngleCorrection) {
    const bool detectorTubes = getProperty("DetectorTubes");
    IAlgorithm_sptr solidAlg = createChildAlgorithm("SANSSolidAngleCorrection");
    solidAlg->setProperty("DetectorTubes", detectorTubes);
    auto ssaalgProp =
        make_unique<AlgorithmProperty>("SANSSolidAngleCorrection");
    ssaalgProp->setValue(solidAlg->toString());
    reductionManager->declareProperty(std::move(ssaalgProp));
  }

  // Beam center
  const double beamCenterX = getProperty("BeamCenterX");
  const double beamCenterY = getProperty("BeamCenterY");
  const std::string centerMethod = getPropertyValue("BeamCenterMethod");

  // Beam center option for transmission data
  if (boost::iequals(centerMethod, "Value")) {
    if (!isEmpty(beamCenterX) && !isEmpty(beamCenterY)) {
      reductionManager->declareProperty(make_unique<PropertyWithValue<double>>(
          "LatestBeamCenterX", beamCenterX));
      reductionManager->declareProperty(make_unique<PropertyWithValue<double>>(
          "LatestBeamCenterY", beamCenterY));
    }
  } else if (!boost::iequals(centerMethod, "None")) {
    bool useDirectBeamMethod = true;
    if (!boost::iequals(centerMethod, "DirectBeam"))
      useDirectBeamMethod = false;
    const std::string beamCenterFile = getProperty("BeamCenterFile");
    if (!beamCenterFile.empty()) {
      const double beamRadius = getProperty("BeamRadius");

      IAlgorithm_sptr ctrAlg = createChildAlgorithm("SANSBeamFinder");
      ctrAlg->setProperty("Filename", beamCenterFile);
      ctrAlg->setProperty("UseDirectBeamMethod", useDirectBeamMethod);
      if (!isEmpty(beamRadius))
        ctrAlg->setProperty("BeamRadius", beamRadius);
      ctrAlg->setPropertyValue("ReductionProperties", reductionManagerName);

      auto algProp = make_unique<AlgorithmProperty>("SANSBeamFinderAlgorithm");
      algProp->setValue(ctrAlg->toString());
      reductionManager->declareProperty(std::move(algProp));
    } else {
      g_log.error() << "ERROR: Beam center determination was required"
                       " but no file was provided\n";
    }
  }

  // Sensitivity correction, transmission and background
  setupSensitivity(reductionManager);
  setupTransmission(reductionManager);
  setupBackground(reductionManager);

  // Geometry correction
  const double thickness = getProperty("SampleThickness");
  if (!isEmpty(thickness)) {
    IAlgorithm_sptr thickAlg = createChildAlgorithm("NormaliseByThickness");
    thickAlg->setProperty("SampleThickness", thickness);

    auto geomalgProp = make_unique<AlgorithmProperty>("GeometryAlgorithm");
    geomalgProp->setValue(thickAlg->toString());
    reductionManager->declareProperty(std::move(geomalgProp));
  }

  // Mask
  IAlgorithm_sptr maskAlg = createChildAlgorithm("SANSMask");
  const std::string maskedDetList = getPropertyValue("MaskedDetectorList");
  const std::string maskedEdges = getPropertyValue("MaskedEdges");
  const std::string maskedComponent = getPropertyValue("MaskedComponent");
  const std::string maskedFullComponent = getPropertyValue("MaskedFullComponent");
  API::MatrixWorkspace_sptr maskedWorkspace = getProperty("MaskedWorkspace");  
  maskAlg->setPropertyValue("Facility", "SNS");
  maskAlg->setPropertyValue("MaskedDetectorList", maskedDetList);
  maskAlg->setPropertyValue("MaskedEdges", maskedEdges);
  maskAlg->setPropertyValue("MaskedComponent", maskedComponent);
  maskAlg->setPropertyValue("MaskedFullComponent", maskedFullComponent);
  maskAlg->setProperty("MaskedWorkspace", maskedWorkspace);
  auto maskalgProp = make_unique<AlgorithmProperty>("MaskAlgorithm");
  maskalgProp->setValue(maskAlg->toString());
  reductionManager->declareProperty(std::move(maskalgProp));

  // Absolute scaling
  const std::string absScaleMethod = getProperty("AbsoluteScaleMethod");
  if (boost::iequals(absScaleMethod, "Value")) {
    const double absScaleFactor = getProperty("AbsoluteScalingFactor");

    IAlgorithm_sptr absAlg = createChildAlgorithm("SANSAbsoluteScale");
    absAlg->setProperty("Method", absScaleMethod);
    absAlg->setProperty("ScalingFactor", absScaleFactor);
    absAlg->setPropertyValue("ReductionProperties", reductionManagerName);
    auto absScaleAlgProp =
        make_unique<AlgorithmProperty>("AbsoluteScaleAlgorithm");
    absScaleAlgProp->setValue(absAlg->toString());
    reductionManager->declareProperty(std::move(absScaleAlgProp));
  } else if (boost::iequals(absScaleMethod, "ReferenceData")) {
    const std::string absRefFile =
        getPropertyValue("AbsoluteScalingReferenceFilename");
    const double beamDiam = getProperty("AbsoluteScalingBeamDiameter");
    const double attTrans = getProperty("AbsoluteScalingAttenuatorTrans");
    const bool applySensitivity =
        getProperty("AbsoluteScalingApplySensitivity");

    IAlgorithm_sptr absAlg = createChildAlgorithm("SANSAbsoluteScale");
    absAlg->setProperty("Method", absScaleMethod);
    absAlg->setProperty("ReferenceDataFilename", absRefFile);
    absAlg->setProperty("BeamstopDiameter", beamDiam);
    absAlg->setProperty("AttenuatorTransmission", attTrans);
    absAlg->setProperty("ApplySensitivity", applySensitivity);
    absAlg->setPropertyValue("ReductionProperties", reductionManagerName);
    auto refscalealgProp =
        make_unique<AlgorithmProperty>("AbsoluteScaleAlgorithm");
    refscalealgProp->setValue(absAlg->toString());
    reductionManager->declareProperty(std::move(refscalealgProp));
  }

  // Azimuthal averaging
  const bool doAveraging = getProperty("DoAzimuthalAverage");
  if (doAveraging) {
    const std::string nBins = getPropertyValue("IQNumberOfBins");
    const bool logBinning = getProperty("IQLogBinning");
    const bool computeResolution = getProperty("ComputeResolution");

    IAlgorithm_sptr iqAlg = createChildAlgorithm("SANSAzimuthalAverage1D");
    iqAlg->setPropertyValue("NumberOfBins", nBins);
    iqAlg->setProperty("LogBinning", logBinning);
    iqAlg->setProperty("ComputeResolution", computeResolution);
    iqAlg->setPropertyValue("NumberOfWedges", "0");
    iqAlg->setPropertyValue("ReductionProperties", reductionManagerName);

    auto iqalgProp = make_unique<AlgorithmProperty>("IQAlgorithm");
    iqalgProp->setValue(iqAlg->toString());
    reductionManager->declareProperty(std::move(iqalgProp));
  }

  // 2D reduction
  const bool do2DReduction = getProperty("Do2DReduction");
  if (do2DReduction) {
    const std::string n_bins = getPropertyValue("IQ2DNumberOfBins");
    IAlgorithm_sptr iqAlg = createChildAlgorithm("EQSANSQ2D");
    iqAlg->setPropertyValue("NumberOfBins", n_bins);
    auto iqxyalgProp = make_unique<AlgorithmProperty>("IQXYAlgorithm");
    iqxyalgProp->setValue(iqAlg->toString());
    reductionManager->declareProperty(std::move(iqxyalgProp));
  }
  setPropertyValue("OutputMessage", "EQSANS reduction options set");
}

void SetupILLSANSReduction::setupSensitivity(
    boost::shared_ptr<PropertyManager> reductionManager) {
  const std::string reductionManagerName = getProperty("ReductionProperties");

  const std::string sensitivityFile = getPropertyValue("SensitivityFile");
  if (!sensitivityFile.empty()) {
    const bool useSampleDC = getProperty("UseDefaultDC");
    const std::string sensitivityDarkCurrentFile =
        getPropertyValue("SensitivityDarkCurrentFile");
    const std::string outputSensitivityWS =
        getPropertyValue("OutputSensitivityWorkspace");
    const double minEff = getProperty("MinEfficiency");
    const double maxEff = getProperty("MaxEfficiency");
    const double sensitivityBeamCenterX = getProperty("SensitivityBeamCenterX");
    const double sensitivityBeamCenterY = getProperty("SensitivityBeamCenterY");

    IAlgorithm_sptr effAlg = createChildAlgorithm("SANSSensitivityCorrection");
    effAlg->setProperty("Filename", sensitivityFile);
    effAlg->setProperty("UseSampleDC", useSampleDC);
    effAlg->setProperty("DarkCurrentFile", sensitivityDarkCurrentFile);
    effAlg->setProperty("MinEfficiency", minEff);
    effAlg->setProperty("MaxEfficiency", maxEff);

    // Beam center option for sensitivity data
    const std::string centerMethod =
        getPropertyValue("SensitivityBeamCenterMethod");
    if (boost::iequals(centerMethod, "Value")) {
      if (!isEmpty(sensitivityBeamCenterX) &&
          !isEmpty(sensitivityBeamCenterY)) {
        effAlg->setProperty("BeamCenterX", sensitivityBeamCenterX);
        effAlg->setProperty("BeamCenterY", sensitivityBeamCenterY);
      }
    } else if (boost::iequals(centerMethod, "DirectBeam") ||
               boost::iequals(centerMethod, "Scattering")) {
      const std::string beamCenterFile =
          getProperty("SensitivityBeamCenterFile");
      const double sensitivityBeamRadius =
          getProperty("SensitivityBeamCenterRadius");
      bool useDirectBeam = boost::iequals(centerMethod, "DirectBeam");
      if (!beamCenterFile.empty()) {
        IAlgorithm_sptr ctrAlg = createChildAlgorithm("SANSBeamFinder");
        ctrAlg->setProperty("Filename", beamCenterFile);
        ctrAlg->setProperty("UseDirectBeamMethod", useDirectBeam);
        ctrAlg->setProperty("PersistentCorrection", false);
        if (useDirectBeam && !isEmpty(sensitivityBeamRadius))
          ctrAlg->setProperty("BeamRadius", sensitivityBeamRadius);
        ctrAlg->setPropertyValue("ReductionProperties", reductionManagerName);

        auto sbcalgProp =
            make_unique<AlgorithmProperty>("SensitivityBeamCenterAlgorithm");
        sbcalgProp->setValue(ctrAlg->toString());
        reductionManager->declareProperty(std::move(sbcalgProp));
      } else {
        g_log.error()
            << "ERROR: Sensitivity beam center determination was required"
               " but no file was provided\n";
      }
    }

    effAlg->setPropertyValue("OutputSensitivityWorkspace", outputSensitivityWS);
    effAlg->setPropertyValue("ReductionProperties", reductionManagerName);

    auto sensalgProp = make_unique<AlgorithmProperty>("SensitivityAlgorithm");
    sensalgProp->setValue(effAlg->toString());
    reductionManager->declareProperty(std::move(sensalgProp));
  }
}
void SetupILLSANSReduction::setupTransmission(
    boost::shared_ptr<PropertyManager> reductionManager) {
  const std::string reductionManagerName = getProperty("ReductionProperties");
  // Transmission options
  const bool thetaDependentTrans = getProperty("ThetaDependentTransmission");
  const std::string transMethod = getProperty("TransmissionMethod");
  const std::string darkCurrent =
      getPropertyValue("TransmissionDarkCurrentFile");
  const bool useSampleDC = getProperty("TransmissionUseSampleDC");

  // Transmission is entered by hand
  if (boost::iequals(transMethod, "Value")) {
    const double transValue = getProperty("TransmissionValue");
    const double transError = getProperty("TransmissionError");
    if (!isEmpty(transValue) && !isEmpty(transError)) {
      IAlgorithm_sptr transAlg =
          createChildAlgorithm("ApplyTransmissionCorrection");
      transAlg->setProperty("TransmissionValue", transValue);
      transAlg->setProperty("TransmissionError", transError);
      transAlg->setProperty("ThetaDependent", thetaDependentTrans);

      auto transalgProp =
          make_unique<AlgorithmProperty>("TransmissionAlgorithm");
      transalgProp->setValue(transAlg->toString());
      reductionManager->declareProperty(std::move(transalgProp));
    } else {
      g_log.information(
          "SetupILLSANSReduction [TransmissionAlgorithm]:"
          "expected transmission/error values and got empty values");
    }
  }
  // Direct beam method for transmission determination
  else if (boost::iequals(transMethod, "DirectBeam")) {
    const std::string sampleFilename =
        getPropertyValue("TransmissionSampleDataFile");
    const std::string emptyFilename =
        getPropertyValue("TransmissionEmptyDataFile");
    const double beamRadius = getProperty("TransmissionBeamRadius");
    const double beamX = getProperty("TransmissionBeamCenterX");
    const double beamY = getProperty("TransmissionBeamCenterY");
    const std::string centerMethod =
        getPropertyValue("TransmissionBeamCenterMethod");

    IAlgorithm_sptr transAlg =
        createChildAlgorithm("SANSDirectBeamTransmission");
    transAlg->setProperty("SampleDataFilename", sampleFilename);
    transAlg->setProperty("EmptyDataFilename", emptyFilename);
    transAlg->setProperty("BeamRadius", beamRadius);
    transAlg->setProperty("DarkCurrentFilename", darkCurrent);
    transAlg->setProperty("UseSampleDarkCurrent", useSampleDC);

    // Beam center option for transmission data
    if (boost::iequals(centerMethod, "Value") && !isEmpty(beamX) &&
        !isEmpty(beamY)) {
      transAlg->setProperty("BeamCenterX", beamX);
      transAlg->setProperty("BeamCenterY", beamY);
    } else if (boost::iequals(centerMethod, "DirectBeam")) {
      const std::string beamCenterFile =
          getProperty("TransmissionBeamCenterFile");
      if (!beamCenterFile.empty()) {
        IAlgorithm_sptr ctrAlg = createChildAlgorithm("SANSBeamFinder");
        ctrAlg->setProperty("Filename", beamCenterFile);
        ctrAlg->setProperty("UseDirectBeamMethod", true);
        ctrAlg->setProperty("PersistentCorrection", false);
        ctrAlg->setPropertyValue("ReductionProperties", reductionManagerName);

        auto tbcalgProp =
            make_unique<AlgorithmProperty>("TransmissionBeamCenterAlgorithm");
        tbcalgProp->setValue(ctrAlg->toString());
        reductionManager->declareProperty(std::move(tbcalgProp));
      } else {
        g_log.error()
            << "ERROR: Transmission beam center determination was required"
               " but no file was provided\n";
      }
    }
    transAlg->setProperty("ThetaDependent", thetaDependentTrans);
    auto transmissionalgProp =
        make_unique<AlgorithmProperty>("TransmissionAlgorithm");
    transmissionalgProp->setValue(transAlg->toString());
    reductionManager->declareProperty(std::move(transmissionalgProp));
  }
}

void SetupILLSANSReduction::setupBackground(
    boost::shared_ptr<PropertyManager> reductionManager) {
  const std::string reductionManagerName = getProperty("ReductionProperties");
  // Background
  const std::string backgroundFile = getPropertyValue("BackgroundFiles");
  if (!backgroundFile.empty())
    reductionManager->declareProperty(
        Kernel::make_unique<PropertyWithValue<std::string>>("BackgroundFiles",
                                                            backgroundFile));
  else
    return;

  const std::string darkCurrent =
      getPropertyValue("BckTransmissionDarkCurrentFile");
  const bool bckThetaDependentTrans =
      getProperty("BckThetaDependentTransmission");
  const std::string bckTransMethod = getProperty("BckTransmissionMethod");
  if (boost::iequals(bckTransMethod, "Value")) {
    const double transValue = getProperty("BckTransmissionValue");
    const double transError = getProperty("BckTransmissionError");
    if (!isEmpty(transValue) && !isEmpty(transError)) {
      IAlgorithm_sptr transAlg =
          createChildAlgorithm("ApplyTransmissionCorrection");
      transAlg->setProperty("TransmissionValue", transValue);
      transAlg->setProperty("TransmissionError", transError);
      transAlg->setProperty("ThetaDependent", bckThetaDependentTrans);

      auto bckTransAlgProp =
          make_unique<AlgorithmProperty>("BckTransmissionAlgorithm");
      bckTransAlgProp->setValue(transAlg->toString());
      reductionManager->declareProperty(std::move(bckTransAlgProp));
    } else {
      g_log.information(
          "SetupILLSANSReduction [BckTransmissionAlgorithm]: "
          "expected transmission/error values and got empty values");
    }
  } else if (boost::iequals(bckTransMethod, "DirectBeam")) {
    const std::string sampleFilename =
        getPropertyValue("BckTransmissionSampleDataFile");
    const std::string emptyFilename =
        getPropertyValue("BckTransmissionEmptyDataFile");
    const double beamRadius = getProperty("BckTransmissionBeamRadius");
    const double beamX = getProperty("BckTransmissionBeamCenterX");
    const double beamY = getProperty("BckTransmissionBeamCenterY");
    const bool thetaDependentTrans =
        getProperty("BckThetaDependentTransmission");
    const bool useSampleDC = getProperty("TransmissionUseSampleDC");

    IAlgorithm_sptr transAlg =
        createChildAlgorithm("SANSDirectBeamTransmission");
    transAlg->setProperty("SampleDataFilename", sampleFilename);
    transAlg->setProperty("EmptyDataFilename", emptyFilename);
    transAlg->setProperty("BeamRadius", beamRadius);
    transAlg->setProperty("DarkCurrentFilename", darkCurrent);
    transAlg->setProperty("UseSampleDarkCurrent", useSampleDC);

    // Beam center option for transmission data
    const std::string centerMethod =
        getPropertyValue("BckTransmissionBeamCenterMethod");
    if (boost::iequals(centerMethod, "Value") && !isEmpty(beamX) &&
        !isEmpty(beamY)) {
      transAlg->setProperty("BeamCenterX", beamX);
      transAlg->setProperty("BeamCenterY", beamY);
    } else if (boost::iequals(centerMethod, "DirectBeam")) {
      const std::string beamCenterFile =
          getProperty("BckTransmissionBeamCenterFile");
      if (!beamCenterFile.empty()) {
        IAlgorithm_sptr ctrAlg = createChildAlgorithm("SANSBeamFinder");
        ctrAlg->setProperty("Filename", beamCenterFile);
        ctrAlg->setProperty("UseDirectBeamMethod", true);
        ctrAlg->setProperty("PersistentCorrection", false);
        ctrAlg->setPropertyValue("ReductionProperties", reductionManagerName);

        auto btbcAlgProp = make_unique<AlgorithmProperty>(
            "BckTransmissionBeamCenterAlgorithm");
        btbcAlgProp->setValue(ctrAlg->toString());
        reductionManager->declareProperty(std::move(btbcAlgProp));
      } else {
        g_log.error() << "ERROR: Beam center determination was required"
                         " but no file was provided\n";
      }
    }
    transAlg->setProperty("DarkCurrentFilename", darkCurrent);
    transAlg->setProperty("ThetaDependent", thetaDependentTrans);
    auto btransAlgProp =
        make_unique<AlgorithmProperty>("BckTransmissionAlgorithm");
    btransAlgProp->setValue(transAlg->toString());
    reductionManager->declareProperty(std::move(btransAlgProp));
  }
}
} // namespace WorkflowAlgorithms
} // namespace Mantid
