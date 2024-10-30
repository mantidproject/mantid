// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidWorkflowAlgorithms/SetupEQSANSReduction.h"
#include "MantidAPI/AlgorithmProperty.h"
#include "MantidAPI/FileProperty.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/EnabledWhenProperty.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/PropertyManager.h"
#include "MantidKernel/PropertyManagerDataService.h"
#include "MantidKernel/RebinParamsValidator.h"
#include "MantidKernel/VisibleWhenProperty.h"
#include "Poco/NumberFormatter.h"

#include <boost/algorithm/string/predicate.hpp>

namespace Mantid::WorkflowAlgorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(SetupEQSANSReduction)

using namespace Kernel;
using namespace API;
using namespace Geometry;

void SetupEQSANSReduction::init() {
  // Load options
  std::string load_grp = "Load Options";
  declareProperty("UseConfigTOFCuts", false,
                  "If true, the edges of the TOF distribution will be cut "
                  "according to the configuration file");
  declareProperty("LowTOFCut", 0.0,
                  "TOF value below which events will not be "
                  "loaded into the workspace at load-time");
  declareProperty("HighTOFCut", 0.0,
                  "TOF value above which events will not be "
                  "loaded into the workspace at load-time");
  declareProperty("WavelengthStep", 0.1,
                  "Wavelength steps to be used when "
                  "rebinning the data before performing "
                  "the reduction");
  declareProperty("UseConfigMask", false,
                  "If true, the masking information "
                  "found in the configuration file "
                  "will be used");
  declareProperty("UseConfig", true, "If true, the best configuration file found will be used");
  declareProperty("CorrectForFlightPath", false,
                  "If true, the TOF will be modified for the true flight path "
                  "from the sample to the detector pixel");

  declareProperty("SkipTOFCorrection", false, "If true, the EQSANS TOF correction will be skipped");
  declareProperty("PreserveEvents", false, "If true, the output workspace will be an event workspace");

  declareProperty("SampleDetectorDistance", EMPTY_DBL(),
                  "Sample to detector distance to use (overrides meta data), in mm");

  declareProperty("SampleDetectorDistanceOffset", EMPTY_DBL(),
                  "Offset to the sample to detector distance (use only when "
                  "using the detector distance found in the meta data), in mm");
  declareProperty("SampleOffset", EMPTY_DBL(),
                  "Offset applies to the sample position (use only when "
                  "using the detector distance found in the meta data), in mm");
  declareProperty("DetectorOffset", EMPTY_DBL(),
                  "Offset applies to the detector position (use only when "
                  "using the distance found in the meta data), in mm");

  declareProperty("SolidAngleCorrection", true, "If true, the solid angle correction will be applied to the data");
  declareProperty("DetectorTubes", false, "If true, the solid angle correction for tube detectors will be applied");

  declareProperty("LoadNexusInstrumentXML", true,
                  "Reads the embedded Instrument XML from the NeXus file "
                  "(optional, default True). ");

  // -- Define group --
  setPropertyGroup("UseConfigTOFCuts", load_grp);
  setPropertyGroup("LowTOFCut", load_grp);
  setPropertyGroup("HighTOFCut", load_grp);

  setPropertyGroup("WavelengthStep", load_grp);
  setPropertyGroup("UseConfigMask", load_grp);
  setPropertyGroup("UseConfig", load_grp);
  setPropertyGroup("CorrectForFlightPath", load_grp);

  setPropertyGroup("SkipTOFCorrection", load_grp);
  setPropertyGroup("PreserveEvents", load_grp);

  setPropertyGroup("SampleDetectorDistance", load_grp);
  setPropertyGroup("SampleDetectorDistanceOffset", load_grp);

  setPropertyGroup("SampleOffset", load_grp);
  setPropertyGroup("DetectorOffset", load_grp);
  setPropertyGroup("SolidAngleCorrection", load_grp);
  setPropertyGroup("DetectorTubes", load_grp);
  setPropertyGroup("LoadNexusInstrumentXML", load_grp);

  // Beam center
  std::string center_grp = "Beam Center";
  std::vector<std::string> centerOptions{"None", "Value", "DirectBeam", "Scattering"};

  declareProperty("BeamCenterMethod", "None", std::make_shared<StringListValidator>(centerOptions),
                  "Method for determining the data beam center");

  // declareProperty("FindBeamCenter", false, "If True, the beam center will be
  // calculated");
  declareProperty("UseConfigBeam", false, "If True, the beam center will be taken from the config file");

  //    Option 1: Set beam center by hand
  declareProperty("BeamCenterX", EMPTY_DBL(), "Position of the beam center, in pixel");
  declareProperty("BeamCenterY", EMPTY_DBL(), "Position of the beam center, in pixel");
  setPropertySettings("BeamCenterX", std::make_unique<VisibleWhenProperty>("BeamCenterMethod", IS_EQUAL_TO, "Value"));
  setPropertySettings("BeamCenterY", std::make_unique<VisibleWhenProperty>("BeamCenterMethod", IS_EQUAL_TO, "Value"));

  //    Option 2: Find it (expose properties from FindCenterOfMass)
  declareProperty(
      std::make_unique<API::FileProperty>("BeamCenterFile", "", API::FileProperty::OptionalLoad, "_event.nxs"),
      "The name of the input event Nexus file to load");
  setPropertySettings("BeamCenterFile",
                      std::make_unique<VisibleWhenProperty>("BeamCenterMethod", IS_NOT_EQUAL_TO, "None"));

  // declareProperty("Tolerance", EMPTY_DBL(), "Tolerance on the center of mass
  // position between each iteration [m]. Default: 0.00125");
  auto positiveDouble = std::make_shared<BoundedValidator<double>>();
  positiveDouble->setLower(0);
  // declareProperty("UseDirectBeamMethod", true, "If true, the direct beam
  // method will be used");
  declareProperty("BeamRadius", EMPTY_DBL(),
                  "Radius of the beam area used the exclude the beam when calculating "
                  "the center of mass of the scattering pattern [pixels]. Default=3.0");
  setPropertySettings("BeamRadius",
                      std::make_unique<VisibleWhenProperty>("BeamCenterMethod", IS_EQUAL_TO, "Scattering"));

  // -- Define group --
  setPropertyGroup("BeamCenterMethod", center_grp);
  setPropertyGroup("UseConfigBeam", center_grp);
  setPropertyGroup("BeamCenterX", center_grp);
  setPropertyGroup("BeamCenterY", center_grp);
  setPropertyGroup("BeamCenterFile", center_grp);
  // setPropertyGroup("Tolerance", center_grp);
  // setPropertyGroup("UseDirectBeamMethod", center_grp);
  setPropertyGroup("BeamRadius", center_grp);

  // Normalisation
  std::string norm_grp = "Normalisation";
  std::vector<std::string> incidentBeamNormOptions;
  incidentBeamNormOptions.emplace_back("None");
  // The data will be normalised to the monitor counts
  incidentBeamNormOptions.emplace_back("Monitor");
  // The data will be normalised to the total charge and divided by the beam
  // profile
  incidentBeamNormOptions.emplace_back("BeamProfileAndCharge");
  // The data will be normalised to the total charge only (no beam profile)
  incidentBeamNormOptions.emplace_back("Charge");
  this->declareProperty("Normalisation", "BeamProfileAndCharge",
                        std::make_shared<StringListValidator>(incidentBeamNormOptions),
                        "Options for data normalisation");

  declareProperty("LoadMonitors", false, "If true, the monitor workspace will be loaded");
  // declareProperty("NormaliseToBeam", true, "If true, the data will be
  // normalised to the total charge and divided by the beam profile");
  // declareProperty("NormaliseToMonitor", false, "If true, the data will be
  // normalised to the monitor, otherwise the total charge will be used");
  declareProperty(
      std::make_unique<API::FileProperty>("MonitorReferenceFile", "", API::FileProperty::OptionalLoad, "_event.nxs"),
      "The name of the beam monitor reference file used for normalisation");

  setPropertyGroup("Normalisation", norm_grp);
  setPropertyGroup("LoadMonitors", norm_grp);
  setPropertyGroup("MonitorReferenceFile", norm_grp);

  // Dark current
  declareProperty(
      std::make_unique<API::FileProperty>("DarkCurrentFile", "", API::FileProperty::OptionalLoad, "_event.nxs"),
      "The name of the input event Nexus file to load as dark current.");

  // Sensitivity
  std::string eff_grp = "Sensitivity";
  declareProperty(
      std::make_unique<API::FileProperty>("SensitivityFile", "", API::FileProperty::OptionalLoad, "_event.nxs"),
      "Flood field or sensitivity file.");
  declareProperty("MinEfficiency", EMPTY_DBL(), positiveDouble,
                  "Minimum efficiency for a pixel to be considered (default: no minimum).");
  declareProperty("MaxEfficiency", EMPTY_DBL(), positiveDouble,
                  "Maximum efficiency for a pixel to be considered (default: no maximum).");
  declareProperty("UseDefaultDC", true,
                  "If true, the dark current subtracted "
                  "from the sample data will also be "
                  "subtracted from the flood field.");
  declareProperty(std::make_unique<API::FileProperty>("SensitivityDarkCurrentFile", "", API::FileProperty::OptionalLoad,
                                                      "_event.nxs"),
                  "The name of the input file to load as dark current.");
  // - sensitivity beam center
  declareProperty("SensitivityBeamCenterMethod", "None", std::make_shared<StringListValidator>(centerOptions),
                  "Method for determining the sensitivity data beam center");

  //    Option 1: Set beam center by hand
  declareProperty("SensitivityBeamCenterX", EMPTY_DBL(), "Sensitivity beam center location in X [pixels]");
  setPropertySettings("SensitivityBeamCenterX",
                      std::make_unique<VisibleWhenProperty>("SensitivityBeamCenterMethod", IS_EQUAL_TO, "Value"));

  declareProperty("SensitivityBeamCenterY", EMPTY_DBL(), "Sensitivity beam center location in Y [pixels]");
  setPropertySettings("SensitivityBeamCenterY",
                      std::make_unique<VisibleWhenProperty>("SensitivityBeamCenterMethod", IS_EQUAL_TO, "Value"));

  //    Option 2: Find it (expose properties from FindCenterOfMass)
  declareProperty(
      std::make_unique<API::FileProperty>("SensitivityBeamCenterFile", "", API::FileProperty::OptionalLoad, ".xml"),
      "The name of the input data file to load");
  setPropertySettings("SensitivityBeamCenterFile",
                      std::make_unique<VisibleWhenProperty>("SensitivityBeamCenterMethod", IS_NOT_EQUAL_TO, "None"));

  declareProperty("SensitivityBeamCenterRadius", EMPTY_DBL(),
                  "Radius of the beam area used the exclude the beam when calculating "
                  "the center of mass of the scattering pattern [pixels]. Default=3.0");
  setPropertySettings("SensitivityBeamCenterRadius",
                      std::make_unique<VisibleWhenProperty>("BeamCenterMethod", IS_EQUAL_TO, "Scattering"));

  declareProperty("OutputSensitivityWorkspace", "", "Name to give the sensitivity workspace");

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
  std::string trans_grp = "Transmission";
  std::vector<std::string> transOptions{"Value", "DirectBeam"};
  declareProperty("TransmissionMethod", "Value", std::make_shared<StringListValidator>(transOptions),
                  "Transmission determination method");

  // - Transmission value entered by hand
  declareProperty("TransmissionValue", EMPTY_DBL(), positiveDouble, "Transmission value.");
  setPropertySettings("TransmissionValue",
                      std::make_unique<VisibleWhenProperty>("TransmissionMethod", IS_EQUAL_TO, "Value"));
  declareProperty("TransmissionError", EMPTY_DBL(), positiveDouble, "Transmission error.");
  setPropertySettings("TransmissionError",
                      std::make_unique<VisibleWhenProperty>("TransmissionMethod", IS_EQUAL_TO, "Value"));

  // - Direct beam method transmission calculation
  declareProperty("TransmissionBeamRadius", 3.0, "Radius of the beam area used to compute the transmission [pixels]");
  setPropertySettings("TransmissionBeamRadius",
                      std::make_unique<VisibleWhenProperty>("TransmissionMethod", IS_EQUAL_TO, "DirectBeam"));
  declareProperty(
      std::make_unique<API::FileProperty>("TransmissionSampleDataFile", "", API::FileProperty::OptionalLoad, ".xml"),
      "Sample data file for transmission calculation");
  setPropertySettings("TransmissionSampleDataFile",
                      std::make_unique<VisibleWhenProperty>("TransmissionMethod", IS_EQUAL_TO, "DirectBeam"));
  declareProperty(
      std::make_unique<API::FileProperty>("TransmissionEmptyDataFile", "", API::FileProperty::OptionalLoad, ".xml"),
      "Empty data file for transmission calculation");
  setPropertySettings("TransmissionEmptyDataFile",
                      std::make_unique<VisibleWhenProperty>("TransmissionMethod", IS_EQUAL_TO, "DirectBeam"));
  declareProperty("FitFramesTogether", false, "If true, the two frames will be fit together");
  setPropertySettings("FitFramesTogether",
                      std::make_unique<VisibleWhenProperty>("TransmissionMethod", IS_EQUAL_TO, "DirectBeam"));

  // - transmission beam center
  declareProperty("TransmissionBeamCenterMethod", "None", std::make_shared<StringListValidator>(centerOptions),
                  "Method for determining the transmission data beam center");
  setPropertySettings("TransmissionBeamCenterMethod",
                      std::make_unique<VisibleWhenProperty>("TransmissionMethod", IS_EQUAL_TO, "DirectBeam"));

  //    Option 1: Set beam center by hand
  declareProperty("TransmissionBeamCenterX", EMPTY_DBL(), "Transmission beam center location in X [pixels]");
  setPropertySettings("TransmissionBeamCenterX",
                      std::make_unique<VisibleWhenProperty>("TransmissionMethod", IS_EQUAL_TO, "DirectBeam"));
  declareProperty("TransmissionBeamCenterY", EMPTY_DBL(), "Transmission beam center location in Y [pixels]");
  setPropertySettings("TransmissionBeamCenterY",
                      std::make_unique<VisibleWhenProperty>("TransmissionMethod", IS_EQUAL_TO, "DirectBeam"));

  //    Option 2: Find it (expose properties from FindCenterOfMass)
  declareProperty(
      std::make_unique<API::FileProperty>("TransmissionBeamCenterFile", "", API::FileProperty::OptionalLoad, ".xml"),
      "The name of the input data file to load");
  setPropertySettings("TransmissionBeamCenterFile",
                      std::make_unique<VisibleWhenProperty>("TransmissionMethod", IS_EQUAL_TO, "DirectBeam"));

  declareProperty(
      std::make_unique<API::FileProperty>("TransmissionDarkCurrentFile", "", API::FileProperty::OptionalLoad, ".xml"),
      "The name of the input data file to load as transmission dark current.");
  setPropertySettings("TransmissionDarkCurrentFile",
                      std::make_unique<VisibleWhenProperty>("TransmissionMethod", IS_NOT_EQUAL_TO, "Value"));

  declareProperty("TransmissionUseSampleDC", true,
                  "If true, the sample dark current will be used IF a dark current file is"
                  "not set.");
  setPropertySettings("TransmissionUseSampleDC",
                      std::make_unique<VisibleWhenProperty>("TransmissionMethod", IS_NOT_EQUAL_TO, "Value"));

  declareProperty("ThetaDependentTransmission", true,
                  "If true, a theta-dependent transmission correction will be applied.");

  // -- Define group --
  setPropertyGroup("TransmissionMethod", trans_grp);
  setPropertyGroup("TransmissionValue", trans_grp);
  setPropertyGroup("TransmissionError", trans_grp);
  setPropertyGroup("TransmissionBeamRadius", trans_grp);
  setPropertyGroup("TransmissionSampleDataFile", trans_grp);
  setPropertyGroup("TransmissionEmptyDataFile", trans_grp);
  setPropertyGroup("FitFramesTogether", trans_grp);
  setPropertyGroup("TransmissionBeamCenterMethod", trans_grp);
  setPropertyGroup("TransmissionBeamCenterX", trans_grp);
  setPropertyGroup("TransmissionBeamCenterY", trans_grp);
  setPropertyGroup("TransmissionBeamCenterFile", trans_grp);

  setPropertyGroup("TransmissionDarkCurrentFile", trans_grp);
  setPropertyGroup("TransmissionUseSampleDC", trans_grp);
  setPropertyGroup("ThetaDependentTransmission", trans_grp);

  // Background options
  std::string bck_grp = "Background";
  declareProperty("BackgroundFiles", "", "Background data files");
  declareProperty("BckTransmissionMethod", "Value", std::make_shared<StringListValidator>(transOptions),
                  "Transmission determination method");

  // - Transmission value entered by hand
  declareProperty("BckTransmissionValue", EMPTY_DBL(), positiveDouble, "Transmission value.");
  setPropertySettings("BckTransmissionValue",
                      std::make_unique<VisibleWhenProperty>("BckTransmissionMethod", IS_EQUAL_TO, "Value"));

  declareProperty("BckTransmissionError", EMPTY_DBL(), positiveDouble, "Transmission error.");
  setPropertySettings("BckTransmissionError",
                      std::make_unique<VisibleWhenProperty>("BckTransmissionMethod", IS_EQUAL_TO, "Value"));

  // - Direct beam method transmission calculation
  declareProperty("BckTransmissionBeamRadius", 3.0,
                  "Radius of the beam area used to compute the transmission [pixels]");
  setPropertySettings("BckTransmissionBeamRadius",
                      std::make_unique<VisibleWhenProperty>("BckTransmissionMethod", IS_EQUAL_TO, "DirectBeam"));
  declareProperty(
      std::make_unique<API::FileProperty>("BckTransmissionSampleDataFile", "", API::FileProperty::OptionalLoad, ".xml"),
      "Sample data file for transmission calculation");
  setPropertySettings("BckTransmissionSampleDataFile",
                      std::make_unique<VisibleWhenProperty>("BckTransmissionMethod", IS_EQUAL_TO, "DirectBeam"));
  declareProperty(
      std::make_unique<API::FileProperty>("BckTransmissionEmptyDataFile", "", API::FileProperty::OptionalLoad, ".xml"),
      "Empty data file for transmission calculation");
  setPropertySettings("BckTransmissionEmptyDataFile",
                      std::make_unique<VisibleWhenProperty>("BckTransmissionMethod", IS_EQUAL_TO, "DirectBeam"));
  declareProperty("BckFitFramesTogether", false, "If true, the two frames will be fit together");
  setPropertySettings("BckFitFramesTogether",
                      std::make_unique<VisibleWhenProperty>("BckTransmissionMethod", IS_EQUAL_TO, "DirectBeam"));

  // - transmission beam center
  declareProperty("BckTransmissionBeamCenterMethod", "None", std::make_shared<StringListValidator>(centerOptions),
                  "Method for determining the transmission data beam center");
  setPropertySettings("BckTransmissionBeamCenterMethod",
                      std::make_unique<VisibleWhenProperty>("BckTransmissionMethod", IS_EQUAL_TO, "DirectBeam"));
  //    Option 1: Set beam center by hand
  declareProperty("BckTransmissionBeamCenterX", EMPTY_DBL(), "Transmission beam center location in X [pixels]");
  setPropertySettings("BckTransmissionBeamCenterX",
                      std::make_unique<VisibleWhenProperty>("BckTransmissionMethod", IS_EQUAL_TO, "DirectBeam"));
  declareProperty("BckTransmissionBeamCenterY", EMPTY_DBL(), "Transmission beam center location in Y [pixels]");
  //    Option 2: Find it (expose properties from FindCenterOfMass)
  setPropertySettings("BckTransmissionBeamCenterY",
                      std::make_unique<VisibleWhenProperty>("BckTransmissionMethod", IS_EQUAL_TO, "DirectBeam"));
  declareProperty(
      std::make_unique<API::FileProperty>("BckTransmissionBeamCenterFile", "", API::FileProperty::OptionalLoad, ".xml"),
      "The name of the input data file to load");
  setPropertySettings("BckTransmissionBeamCenterFile",
                      std::make_unique<VisibleWhenProperty>("BckTransmissionMethod", IS_EQUAL_TO, "DirectBeam"));

  declareProperty(std::make_unique<API::FileProperty>("BckTransmissionDarkCurrentFile", "",
                                                      API::FileProperty::OptionalLoad, ".xml"),
                  "The name of the input data file to load as background "
                  "transmission dark current.");
  setPropertySettings("BckTransmissionDarkCurrentFile",
                      std::make_unique<VisibleWhenProperty>("BckTransmissionMethod", IS_EQUAL_TO, "BeamSpreader"));

  declareProperty("BckThetaDependentTransmission", true,
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

  // Geometry correction
  declareProperty("SampleThickness", EMPTY_DBL(), "Sample thickness [cm]");

  // Masking
  std::string mask_grp = "Mask";
  declareProperty(std::make_unique<ArrayProperty<int>>("MaskedDetectorList"), "List of detector IDs to be masked");
  declareProperty(std::make_unique<ArrayProperty<int>>("MaskedEdges"),
                  "Number of pixels to mask on the edges: X-low, X-high, Y-low, Y-high");
  std::vector<std::string> maskOptions{"None", "Front", "Back"};
  declareProperty("MaskedSide", "None", std::make_shared<StringListValidator>(maskOptions),
                  "Mask one side of the detector");

  setPropertyGroup("MaskedDetectorList", mask_grp);
  setPropertyGroup("MaskedEdges", mask_grp);
  setPropertyGroup("MaskedSide", mask_grp);

  // Absolute scale
  std::string abs_scale_grp = "Absolute Scale";
  std::vector<std::string> scaleOptions;
  scaleOptions.emplace_back("None");
  scaleOptions.emplace_back("Value");
  scaleOptions.emplace_back("ReferenceData");
  declareProperty("AbsoluteScaleMethod", "None", std::make_shared<StringListValidator>(scaleOptions),
                  "Absolute scale correction method");
  declareProperty("AbsoluteScalingFactor", 1.0, "Absolute scaling factor");
  setPropertySettings("AbsoluteScalingFactor",
                      std::make_unique<VisibleWhenProperty>("AbsoluteScaleMethod", IS_EQUAL_TO, "Value"));

  declareProperty(std::make_unique<API::FileProperty>("AbsoluteScalingReferenceFilename", "",
                                                      API::FileProperty::OptionalLoad, ".xml"));
  setPropertySettings("AbsoluteScalingReferenceFilename",
                      std::make_unique<VisibleWhenProperty>("AbsoluteScaleMethod", IS_EQUAL_TO, "ReferenceData"));
  declareProperty("AbsoluteScalingBeamDiameter", 0.0,
                  "Beamstop diameter for computing the absolute scale factor [mm]. "
                  "Read from file if not supplied.");
  setPropertySettings("AbsoluteScalingBeamDiameter",
                      std::make_unique<VisibleWhenProperty>("AbsoluteScaleMethod", IS_EQUAL_TO, "ReferenceData"));
  declareProperty("AbsoluteScalingAttenuatorTrans", 1.0,
                  "Attenuator transmission value for computing the absolute scale factor");
  setPropertySettings("AbsoluteScalingAttenuatorTrans",
                      std::make_unique<VisibleWhenProperty>("AbsoluteScaleMethod", IS_EQUAL_TO, "ReferenceData"));
  declareProperty("AbsoluteScalingApplySensitivity", false,
                  "Apply sensitivity correction to the reference data "
                  "when computing the absolute scale factor");
  setPropertySettings("AbsoluteScalingApplySensitivity",
                      std::make_unique<VisibleWhenProperty>("AbsoluteScaleMethod", IS_EQUAL_TO, "ReferenceData"));

  setPropertyGroup("AbsoluteScaleMethod", abs_scale_grp);
  setPropertyGroup("AbsoluteScalingFactor", abs_scale_grp);
  setPropertyGroup("AbsoluteScalingReferenceFilename", abs_scale_grp);
  setPropertyGroup("AbsoluteScalingBeamDiameter", abs_scale_grp);
  setPropertyGroup("AbsoluteScalingAttenuatorTrans", abs_scale_grp);
  setPropertyGroup("AbsoluteScalingApplySensitivity", abs_scale_grp);

  // I(Q) calculation
  std::string iq1d_grp = "I(q) Calculation";
  declareProperty("DoAzimuthalAverage", true);
  auto positiveInt = std::make_shared<BoundedValidator<int>>();
  positiveInt->setLower(0);
  declareProperty("IQNumberOfBins", 100, positiveInt, "Number of I(q) bins when binning is not specified");
  declareProperty("IQLogBinning", false, "I(q) log binning when binning is not specified");
  declareProperty("IQIndependentBinning", true,
                  "If true and frame skipping is "
                  "used, each frame will have "
                  "its own binning");
  declareProperty("IQScaleResults", true, "If true and frame skipping is used, frame 1 will be scaled to frame 2");
  declareProperty("ComputeResolution", false, "If true the Q resolution will be computed");
  declareProperty("SampleApertureDiameter", 10.0, "Sample aperture diameter [mm]");

  declareProperty("Do2DReduction", true);
  declareProperty("IQ2DNumberOfBins", 100, positiveInt, "Number of I(qx,qy) bins.");

  // -- Define group --
  setPropertyGroup("DoAzimuthalAverage", iq1d_grp);
  setPropertyGroup("IQNumberOfBins", iq1d_grp);
  setPropertyGroup("IQLogBinning", iq1d_grp);
  setPropertyGroup("IQIndependentBinning", iq1d_grp);
  setPropertyGroup("IQScaleResults", iq1d_grp);
  setPropertyGroup("ComputeResolution", iq1d_grp);
  setPropertyGroup("SampleApertureDiameter", iq1d_grp);
  setPropertyGroup("Do2DReduction", iq1d_grp);
  setPropertyGroup("IQ2DNumberOfBins", iq1d_grp);

  // Outputs
  declareProperty("ProcessInfo", "", "Additional process information");
  declareProperty("OutputDirectory", "", "Directory to put the output files in");
  declareProperty("OutputMessage", "", Direction::Output);
  declareProperty("ReductionProperties", "__sans_reduction_properties", Direction::Input);
}

void SetupEQSANSReduction::exec() {
  // Reduction property manager
  const std::string reductionManagerName = getProperty("ReductionProperties");
  if (reductionManagerName.empty()) {
    g_log.error() << "ERROR: Reduction Property Manager name is empty\n";
    return;
  }
  std::shared_ptr<PropertyManager> reductionManager = std::make_shared<PropertyManager>();
  PropertyManagerDataService::Instance().addOrReplace(reductionManagerName, reductionManager);

  // Store name of the instrument
  reductionManager->declareProperty(std::make_unique<PropertyWithValue<std::string>>("InstrumentName", "EQSANS"));

  // Store additional (and optional) process information
  const std::string processInfo = getProperty("ProcessInfo");
  reductionManager->declareProperty(std::make_unique<PropertyWithValue<std::string>>("ProcessInfo", processInfo));

  // Store the output directory
  const std::string outputDirectory = getProperty("OutputDirectory");
  reductionManager->declareProperty(
      std::make_unique<PropertyWithValue<std::string>>("OutputDirectory", outputDirectory));

  // Store normalization algorithm
  const std::string normalization = getProperty("Normalisation");
  bool loadMonitors = getProperty("LoadMonitors");
  const std::string monitorRefFile = getPropertyValue("MonitorReferenceFile");
  // If we normalize to monitor, force the loading of monitor data
  auto normAlg = createChildAlgorithm("EQSANSNormalise");

  if (boost::contains(normalization, "BeamProfileAndCharge")) {
    normAlg->setProperty("NormaliseToBeam", true);
    normAlg->setProperty("BeamSpectrumFile", monitorRefFile);
  } else if (boost::contains(normalization, "Charge")) {
    normAlg->setProperty("NormaliseToBeam", false);
  } else if (boost::contains(normalization, "Monitor")) {
    if (monitorRefFile.empty()) {
      g_log.error() << "ERROR: normalize-to-monitor was turned ON but no "
                       "reference data was selected\n";
    } else {
      loadMonitors = true;
    }
    normAlg->setProperty("NormaliseToMonitor", true);
    normAlg->setProperty("BeamSpectrumFile", monitorRefFile);
  }
  normAlg->setPropertyValue("ReductionProperties", reductionManagerName);
  auto normAlgProp = std::make_unique<AlgorithmProperty>("NormaliseAlgorithm");
  normAlgProp->setValue(normAlg->toString());
  reductionManager->declareProperty(std::move(normAlgProp));

  // Load algorithm
  auto loadAlg = createChildAlgorithm("EQSANSLoad");
  const bool useConfigBeam = getProperty("UseConfigBeam");
  loadAlg->setProperty("UseConfigBeam", useConfigBeam);
  const bool useConfigTOFCuts = getProperty("UseConfigTOFCuts");
  loadAlg->setProperty("UseConfigTOFCuts", useConfigTOFCuts);
  if (!useConfigTOFCuts) {
    const double lowTOFCut = getProperty("LowTOFCut");
    const double highTOFCut = getProperty("HighTOFCut");
    loadAlg->setProperty("LowTOFCut", lowTOFCut);
    loadAlg->setProperty("HighTOFCut", highTOFCut);
  }

  const bool skipTOFCorrection = getProperty("SkipTOFCorrection");
  loadAlg->setProperty("SkipTOFCorrection", skipTOFCorrection);

  const bool correctForFlightPath = getProperty("CorrectForFlightPath");
  loadAlg->setProperty("CorrectForFlightPath", correctForFlightPath);

  const bool preserveEvents = getProperty("PreserveEvents");
  loadAlg->setProperty("PreserveEvents", preserveEvents);
  // load from nexus if they aren't in the reference file
  loadAlg->setProperty("LoadMonitors", (loadMonitors && monitorRefFile.empty()));

  const double sdd = getProperty("SampleDetectorDistance");
  loadAlg->setProperty("SampleDetectorDistance", sdd);
  const double sddOffset = getProperty("SampleDetectorDistanceOffset");
  loadAlg->setProperty("SampleDetectorDistanceOffset", sddOffset);
  const double dOffset = getProperty("DetectorOffset");
  loadAlg->setProperty("DetectorOffset", dOffset);
  const double sOffset = getProperty("SampleOffset");
  loadAlg->setProperty("SampleOffset", sOffset);
  const double wlStep = getProperty("WavelengthStep");
  loadAlg->setProperty("WavelengthStep", wlStep);

  const bool useConfig = getProperty("UseConfig");
  loadAlg->setProperty("UseConfig", useConfig);
  const bool useConfigMask = getProperty("UseConfigMask");
  loadAlg->setProperty("UseConfigMask", useConfigMask);
  const bool loadNexusInstrumentXML = getProperty("LoadNexusInstrumentXML");
  loadAlg->setProperty("LoadNexusInstrumentXML", loadNexusInstrumentXML);
  auto loadAlgProp = std::make_unique<AlgorithmProperty>("LoadAlgorithm");
  loadAlgProp->setValue(loadAlg->toString());
  reductionManager->declareProperty(std::move(loadAlgProp));

  // Store dark current algorithm
  const std::string darkCurrentFile = getPropertyValue("DarkCurrentFile");
  if (!darkCurrentFile.empty()) {
    auto darkAlg = createChildAlgorithm("EQSANSDarkCurrentSubtraction");
    darkAlg->setProperty("Filename", darkCurrentFile);
    darkAlg->setProperty("OutputDarkCurrentWorkspace", "");
    darkAlg->setPropertyValue("ReductionProperties", reductionManagerName);
    auto dcAlgProp = std::make_unique<AlgorithmProperty>("DarkCurrentAlgorithm");
    dcAlgProp->setValue(darkAlg->toString());
    reductionManager->declareProperty(std::move(dcAlgProp));
  }

  // Store default dark current algorithm
  auto darkDefaultAlg = createChildAlgorithm("EQSANSDarkCurrentSubtraction");
  darkDefaultAlg->setProperty("OutputDarkCurrentWorkspace", "");
  darkDefaultAlg->setPropertyValue("ReductionProperties", reductionManagerName);
  auto ddcAlgProp = std::make_unique<AlgorithmProperty>("DefaultDarkCurrentAlgorithm");
  ddcAlgProp->setValue(darkDefaultAlg->toString());
  reductionManager->declareProperty(std::move(ddcAlgProp));

  // Solid angle correction
  const bool solidAngleCorrection = getProperty("SolidAngleCorrection");
  if (solidAngleCorrection) {
    const bool detectorTubes = getProperty("DetectorTubes");
    auto solidAlg = createChildAlgorithm("SANSSolidAngleCorrection");
    solidAlg->setProperty("DetectorTubes", detectorTubes);
    auto ssaAlgProp = std::make_unique<AlgorithmProperty>("SANSSolidAngleCorrection");
    ssaAlgProp->setValue(solidAlg->toString());
    reductionManager->declareProperty(std::move(ssaAlgProp));
  }

  // Beam center
  const double beamCenterX = getProperty("BeamCenterX");
  const double beamCenterY = getProperty("BeamCenterY");
  const std::string centerMethod = getPropertyValue("BeamCenterMethod");

  // Beam center option for transmission data
  if (boost::iequals(centerMethod, "Value")) {
    if (!isEmpty(beamCenterX) && !isEmpty(beamCenterY)) {
      reductionManager->declareProperty(std::make_unique<PropertyWithValue<double>>("LatestBeamCenterX", beamCenterX));
      reductionManager->declareProperty(std::make_unique<PropertyWithValue<double>>("LatestBeamCenterY", beamCenterY));
    }
  } else if (!boost::iequals(centerMethod, "None")) {
    bool useDirectBeamMethod = true;
    if (!boost::iequals(centerMethod, "DirectBeam"))
      useDirectBeamMethod = false;
    const std::string beamCenterFile = getProperty("BeamCenterFile");
    if (!beamCenterFile.empty()) {
      const double beamRadius = getProperty("BeamRadius");

      auto ctrAlg = createChildAlgorithm("SANSBeamFinder");
      ctrAlg->setProperty("Filename", beamCenterFile);
      ctrAlg->setProperty("UseDirectBeamMethod", useDirectBeamMethod);
      if (!isEmpty(beamRadius))
        ctrAlg->setProperty("BeamRadius", beamRadius);
      ctrAlg->setPropertyValue("ReductionProperties", reductionManagerName);

      auto ctrAlgProp = std::make_unique<AlgorithmProperty>("SANSBeamFinderAlgorithm");
      ctrAlgProp->setValue(ctrAlg->toString());
      reductionManager->declareProperty(std::move(ctrAlgProp));
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
    auto thickAlg = createChildAlgorithm("NormaliseByThickness");
    thickAlg->setProperty("SampleThickness", thickness);

    auto geomAlgProp = std::make_unique<AlgorithmProperty>("GeometryAlgorithm");
    geomAlgProp->setValue(thickAlg->toString());
    reductionManager->declareProperty(std::move(geomAlgProp));
  }

  // Mask
  const std::string maskDetList = getPropertyValue("MaskedDetectorList");
  const std::string maskEdges = getPropertyValue("MaskedEdges");
  const std::string maskSide = getProperty("MaskedSide");

  auto maskAlg = createChildAlgorithm("SANSMask");
  // The following is broken, try PropertyValue
  maskAlg->setPropertyValue("Facility", "SNS");
  maskAlg->setPropertyValue("MaskedDetectorList", maskDetList);
  maskAlg->setPropertyValue("MaskedEdges", maskEdges);
  maskAlg->setProperty("MaskedSide", maskSide);
  auto maskAlgProp = std::make_unique<AlgorithmProperty>("MaskAlgorithm");
  maskAlgProp->setValue(maskAlg->toString());
  reductionManager->declareProperty(std::move(maskAlgProp));

  // Absolute scaling
  const std::string absScaleMethod = getProperty("AbsoluteScaleMethod");
  if (boost::iequals(absScaleMethod, "Value")) {
    const double absScaleFactor = getProperty("AbsoluteScalingFactor");

    auto absAlg = createChildAlgorithm("SANSAbsoluteScale");
    absAlg->setProperty("Method", absScaleMethod);
    absAlg->setProperty("ScalingFactor", absScaleFactor);
    absAlg->setPropertyValue("ReductionProperties", reductionManagerName);
    auto absAlgProp = std::make_unique<AlgorithmProperty>("AbsoluteScaleAlgorithm");
    absAlgProp->setValue(absAlg->toString());
    reductionManager->declareProperty(std::move(absAlgProp));
  } else if (boost::iequals(absScaleMethod, "ReferenceData")) {
    const std::string absRefFile = getPropertyValue("AbsoluteScalingReferenceFilename");
    const double beamDiam = getProperty("AbsoluteScalingBeamDiameter");
    const double attTrans = getProperty("AbsoluteScalingAttenuatorTrans");
    const bool applySensitivity = getProperty("AbsoluteScalingApplySensitivity");

    auto absAlg = createChildAlgorithm("SANSAbsoluteScale");
    absAlg->setProperty("Method", absScaleMethod);
    absAlg->setProperty("ReferenceDataFilename", absRefFile);
    absAlg->setProperty("BeamstopDiameter", beamDiam);
    absAlg->setProperty("AttenuatorTransmission", attTrans);
    absAlg->setProperty("ApplySensitivity", applySensitivity);
    absAlg->setPropertyValue("ReductionProperties", reductionManagerName);
    auto scaleAlgProp = std::make_unique<AlgorithmProperty>("AbsoluteScaleAlgorithm");
    scaleAlgProp->setValue(absAlg->toString());
    reductionManager->declareProperty(std::move(scaleAlgProp));
  }

  // Azimuthal averaging
  const bool doAveraging = getProperty("DoAzimuthalAverage");
  if (doAveraging) {
    const std::string nBins = getPropertyValue("IQNumberOfBins");
    const bool logBinning = getProperty("IQLogBinning");
    const double sampleApert = getProperty("SampleApertureDiameter");
    const bool computeResolution = getProperty("ComputeResolution");
    const bool indepBinning = getProperty("IQIndependentBinning");
    const bool scaleResults = getProperty("IQScaleResults");

    auto iqAlg = createChildAlgorithm("EQSANSAzimuthalAverage1D");
    iqAlg->setPropertyValue("NumberOfBins", nBins);
    iqAlg->setProperty("LogBinning", logBinning);
    iqAlg->setProperty("ScaleResults", scaleResults);
    iqAlg->setProperty("ComputeResolution", computeResolution);
    iqAlg->setProperty("IndependentBinning", indepBinning);
    iqAlg->setProperty("SampleApertureDiameter", sampleApert);
    iqAlg->setPropertyValue("ReductionProperties", reductionManagerName);

    auto iqalgProp = std::make_unique<AlgorithmProperty>("IQAlgorithm");
    iqalgProp->setValue(iqAlg->toString());
    reductionManager->declareProperty(std::move(iqalgProp));
  }

  // 2D reduction
  const bool do2DReduction = getProperty("Do2DReduction");
  if (do2DReduction) {
    const std::string n_bins = getPropertyValue("IQ2DNumberOfBins");
    auto iqAlg = createChildAlgorithm("EQSANSQ2D");
    iqAlg->setPropertyValue("NumberOfBins", n_bins);
    auto xyalgProp = std::make_unique<AlgorithmProperty>("IQXYAlgorithm");
    xyalgProp->setValue(iqAlg->toString());
    reductionManager->declareProperty(std::move(xyalgProp));
  }
  setPropertyValue("OutputMessage", "EQSANS reduction options set");

  // Save a string representation of this algorithm
  auto setupAlgProp = std::make_unique<AlgorithmProperty>("SetupAlgorithm");
  setupAlgProp->setValue(toString());
  reductionManager->declareProperty(std::move(setupAlgProp));
}

void SetupEQSANSReduction::setupSensitivity(const std::shared_ptr<PropertyManager> &reductionManager) {
  const std::string reductionManagerName = getProperty("ReductionProperties");

  const std::string sensitivityFile = getPropertyValue("SensitivityFile");
  if (!sensitivityFile.empty()) {
    const bool useSampleDC = getProperty("UseDefaultDC");
    const std::string sensitivityDarkCurrentFile = getPropertyValue("SensitivityDarkCurrentFile");
    const std::string outputSensitivityWS = getPropertyValue("OutputSensitivityWorkspace");
    const double minEff = getProperty("MinEfficiency");
    const double maxEff = getProperty("MaxEfficiency");
    const double sensitivityBeamCenterX = getProperty("SensitivityBeamCenterX");
    const double sensitivityBeamCenterY = getProperty("SensitivityBeamCenterY");

    auto effAlg = createChildAlgorithm("SANSSensitivityCorrection");
    effAlg->setProperty("Filename", sensitivityFile);
    effAlg->setProperty("UseSampleDC", useSampleDC);
    effAlg->setProperty("DarkCurrentFile", sensitivityDarkCurrentFile);
    effAlg->setProperty("MinEfficiency", minEff);
    effAlg->setProperty("MaxEfficiency", maxEff);

    // Beam center option for sensitivity data
    const std::string centerMethod = getPropertyValue("SensitivityBeamCenterMethod");
    if (boost::iequals(centerMethod, "Value")) {
      if (!isEmpty(sensitivityBeamCenterX) && !isEmpty(sensitivityBeamCenterY)) {
        effAlg->setProperty("BeamCenterX", sensitivityBeamCenterX);
        effAlg->setProperty("BeamCenterY", sensitivityBeamCenterY);
      }
    } else if (boost::iequals(centerMethod, "DirectBeam") || boost::iequals(centerMethod, "Scattering")) {
      const std::string beamCenterFile = getProperty("SensitivityBeamCenterFile");
      const double sensitivityBeamRadius = getProperty("SensitivityBeamCenterRadius");
      bool useDirectBeam = boost::iequals(centerMethod, "DirectBeam");
      if (!beamCenterFile.empty()) {
        auto ctrAlg = createChildAlgorithm("SANSBeamFinder");
        ctrAlg->setProperty("Filename", beamCenterFile);
        ctrAlg->setProperty("UseDirectBeamMethod", useDirectBeam);
        ctrAlg->setProperty("PersistentCorrection", false);
        if (!isEmpty(sensitivityBeamRadius))
          ctrAlg->setProperty("BeamRadius", sensitivityBeamRadius);
        ctrAlg->setPropertyValue("ReductionProperties", reductionManagerName);

        auto sensAlgProp = std::make_unique<AlgorithmProperty>("SensitivityBeamCenterAlgorithm");
        sensAlgProp->setValue(ctrAlg->toString());
        reductionManager->declareProperty(std::move(sensAlgProp));
      } else {
        g_log.error() << "ERROR: Sensitivity beam center determination was required"
                         " but no file was provided\n";
      }
    }

    effAlg->setPropertyValue("OutputSensitivityWorkspace", outputSensitivityWS);
    effAlg->setPropertyValue("ReductionProperties", reductionManagerName);

    auto algProp = std::make_unique<AlgorithmProperty>("SensitivityAlgorithm");
    algProp->setValue(effAlg->toString());
    reductionManager->declareProperty(std::move(algProp));
  }
}
void SetupEQSANSReduction::setupTransmission(const std::shared_ptr<PropertyManager> &reductionManager) {
  const std::string reductionManagerName = getProperty("ReductionProperties");
  // Transmission options
  const bool thetaDependentTrans = getProperty("ThetaDependentTransmission");
  const std::string transMethod = getProperty("TransmissionMethod");
  const std::string darkCurrent = getPropertyValue("TransmissionDarkCurrentFile");
  const bool useSampleDC = getProperty("TransmissionUseSampleDC");

  // Transmission is entered by hand
  if (boost::iequals(transMethod, "Value")) {
    const double transValue = getProperty("TransmissionValue");
    const double transError = getProperty("TransmissionError");
    if (!isEmpty(transValue) && !isEmpty(transError)) {
      auto transAlg = createChildAlgorithm("ApplyTransmissionCorrection");
      transAlg->setProperty("TransmissionValue", transValue);
      transAlg->setProperty("TransmissionError", transError);
      transAlg->setProperty("ThetaDependent", thetaDependentTrans);

      auto algProp = std::make_unique<AlgorithmProperty>("TransmissionAlgorithm");
      algProp->setValue(transAlg->toString());
      reductionManager->declareProperty(std::move(algProp));
    } else {
      g_log.information("SetupEQSANSReduction [TransmissionAlgorithm]:"
                        "expected transmission/error values and got empty values");
    }
  }
  // Direct beam method for transmission determination
  else if (boost::iequals(transMethod, "DirectBeam")) {
    const std::string sampleFilename = getPropertyValue("TransmissionSampleDataFile");
    const std::string emptyFilename = getPropertyValue("TransmissionEmptyDataFile");
    const double beamRadius = getProperty("TransmissionBeamRadius");
    const bool fitFramesTogether = getProperty("FitFramesTogether");
    const double beamX = getProperty("TransmissionBeamCenterX");
    const double beamY = getProperty("TransmissionBeamCenterY");
    const std::string centerMethod = getPropertyValue("TransmissionBeamCenterMethod");

    auto transAlg = createChildAlgorithm("EQSANSDirectBeamTransmission");
    transAlg->setProperty("FitFramesTogether", fitFramesTogether);
    transAlg->setProperty("SampleDataFilename", sampleFilename);
    transAlg->setProperty("EmptyDataFilename", emptyFilename);
    transAlg->setProperty("BeamRadius", beamRadius);
    transAlg->setProperty("DarkCurrentFilename", darkCurrent);
    transAlg->setProperty("UseSampleDarkCurrent", useSampleDC);

    // Beam center option for transmission data
    if (boost::iequals(centerMethod, "Value") && !isEmpty(beamX) && !isEmpty(beamY)) {
      transAlg->setProperty("BeamCenterX", beamX);
      transAlg->setProperty("BeamCenterY", beamY);
    } else if (boost::iequals(centerMethod, "DirectBeam")) {
      const std::string beamCenterFile = getProperty("TransmissionBeamCenterFile");
      if (!beamCenterFile.empty()) {
        auto ctrAlg = createChildAlgorithm("SANSBeamFinder");
        ctrAlg->setProperty("Filename", beamCenterFile);
        ctrAlg->setProperty("UseDirectBeamMethod", true);
        ctrAlg->setProperty("PersistentCorrection", false);
        ctrAlg->setPropertyValue("ReductionProperties", reductionManagerName);

        auto algProp = std::make_unique<AlgorithmProperty>("TransmissionBeamCenterAlgorithm");
        algProp->setValue(ctrAlg->toString());
        reductionManager->declareProperty(std::move(algProp));
      } else {
        g_log.error() << "ERROR: Transmission beam center determination was required"
                         " but no file was provided\n";
      }
    }
    transAlg->setProperty("ThetaDependent", thetaDependentTrans);
    auto algProp = std::make_unique<AlgorithmProperty>("TransmissionAlgorithm");
    algProp->setValue(transAlg->toString());
    reductionManager->declareProperty(std::move(algProp));
  }
}

void SetupEQSANSReduction::setupBackground(const std::shared_ptr<PropertyManager> &reductionManager) {
  const std::string reductionManagerName = getProperty("ReductionProperties");
  // Background
  const std::string backgroundFile = getPropertyValue("BackgroundFiles");
  if (!backgroundFile.empty())
    reductionManager->declareProperty(
        std::make_unique<PropertyWithValue<std::string>>("BackgroundFiles", backgroundFile));
  else
    return;

  const std::string darkCurrent = getPropertyValue("BckTransmissionDarkCurrentFile");
  const bool bckThetaDependentTrans = getProperty("BckThetaDependentTransmission");
  const std::string bckTransMethod = getProperty("BckTransmissionMethod");
  if (boost::iequals(bckTransMethod, "Value")) {
    const double transValue = getProperty("BckTransmissionValue");
    const double transError = getProperty("BckTransmissionError");
    if (!isEmpty(transValue) && !isEmpty(transError)) {
      auto transAlg = createChildAlgorithm("ApplyTransmissionCorrection");
      transAlg->setProperty("TransmissionValue", transValue);
      transAlg->setProperty("TransmissionError", transError);
      transAlg->setProperty("ThetaDependent", bckThetaDependentTrans);

      auto algProp = std::make_unique<AlgorithmProperty>("BckTransmissionAlgorithm");
      algProp->setValue(transAlg->toString());
      reductionManager->declareProperty(std::move(algProp));
    } else {
      g_log.information("SetupEQSANSReduction [BckTransmissionAlgorithm]: "
                        "expected transmission/error values and got empty values");
    }
  } else if (boost::iequals(bckTransMethod, "DirectBeam")) {
    const std::string sampleFilename = getPropertyValue("BckTransmissionSampleDataFile");
    const std::string emptyFilename = getPropertyValue("BckTransmissionEmptyDataFile");
    const double beamRadius = getProperty("BckTransmissionBeamRadius");
    const double beamX = getProperty("BckTransmissionBeamCenterX");
    const double beamY = getProperty("BckTransmissionBeamCenterY");
    const bool thetaDependentTrans = getProperty("BckThetaDependentTransmission");
    const bool useSampleDC = getProperty("TransmissionUseSampleDC");
    const bool fitFramesTogether = getProperty("BckFitFramesTogether");

    auto transAlg = createChildAlgorithm("EQSANSDirectBeamTransmission");
    transAlg->setProperty("FitFramesTogether", fitFramesTogether);
    transAlg->setProperty("SampleDataFilename", sampleFilename);
    transAlg->setProperty("EmptyDataFilename", emptyFilename);
    transAlg->setProperty("BeamRadius", beamRadius);
    transAlg->setProperty("DarkCurrentFilename", darkCurrent);
    transAlg->setProperty("UseSampleDarkCurrent", useSampleDC);

    // Beam center option for transmission data
    const std::string centerMethod = getPropertyValue("BckTransmissionBeamCenterMethod");
    if (boost::iequals(centerMethod, "Value") && !isEmpty(beamX) && !isEmpty(beamY)) {
      transAlg->setProperty("BeamCenterX", beamX);
      transAlg->setProperty("BeamCenterY", beamY);
    } else if (boost::iequals(centerMethod, "DirectBeam")) {
      const std::string beamCenterFile = getProperty("BckTransmissionBeamCenterFile");
      if (!beamCenterFile.empty()) {
        auto ctrAlg = createChildAlgorithm("SANSBeamFinder");
        ctrAlg->setProperty("Filename", beamCenterFile);
        ctrAlg->setProperty("UseDirectBeamMethod", true);
        ctrAlg->setProperty("PersistentCorrection", false);
        ctrAlg->setPropertyValue("ReductionProperties", reductionManagerName);

        auto algProp = std::make_unique<AlgorithmProperty>("BckTransmissionBeamCenterAlgorithm");
        algProp->setValue(ctrAlg->toString());
        reductionManager->declareProperty(std::move(algProp));
      } else {
        g_log.error() << "ERROR: Beam center determination was required"
                         " but no file was provided\n";
      }
    }
    transAlg->setProperty("DarkCurrentFilename", darkCurrent);
    transAlg->setProperty("ThetaDependent", thetaDependentTrans);
    auto algProp = std::make_unique<AlgorithmProperty>("BckTransmissionAlgorithm");
    algProp->setValue(transAlg->toString());
    reductionManager->declareProperty(std::move(algProp));
  }
}
} // namespace Mantid::WorkflowAlgorithms
