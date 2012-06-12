/*WIKI* 
Create a PropertyManager object setting the reduction options for EQSANS.
The property manager object is then added to the PropertyManagerDataService.

See [http://www.mantidproject.org/Reduction_for_HFIR_SANS SANS Reduction] documentation for details.

*WIKI*/
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidWorkflowAlgorithms/SetupEQSANSReduction.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/EnabledWhenProperty.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/AlgorithmProperty.h"
#include "MantidAPI/PropertyManagerDataService.h"
#include "MantidKernel/PropertyManager.h"
#include "Poco/NumberFormatter.h"

namespace Mantid
{
namespace WorkflowAlgorithms
{

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(SetupEQSANSReduction)

/// Sets documentation strings for this algorithm
void SetupEQSANSReduction::initDocs()
{
  this->setWikiSummary("Set up EQSANS SANS reduction options.");
  this->setOptionalMessage("Set up EQSANS SANS reduction options.");
}

using namespace Kernel;
using namespace API;
using namespace Geometry;

void SetupEQSANSReduction::init()
{
  // Load options
  std::string load_grp = "Load Options";
  declareProperty("UseConfigTOFCuts", false, "If true, the edges of the TOF distribution will be cut according to the configuration file");
  declareProperty("LowTOFCut", 0.0, "TOF value below which events will not be loaded into the workspace at load-time");
  declareProperty("HighTOFCut", 0.0, "TOF value above which events will not be loaded into the workspace at load-time");
  declareProperty("WavelengthStep", 0.1, "Wavelength steps to be used when rebinning the data before performing the reduction");
  declareProperty("UseConfigMask", false, "If true, the masking information found in the configuration file will be used");
  declareProperty("UseConfig", true, "If true, the best configuration file found will be used");
  declareProperty("CorrectForFlightPath", false, "If true, the TOF will be modified for the true flight path from the sample to the detector pixel");

  declareProperty("SkipTOFCorrection", false, "IF true, the EQSANS TOF correction will be skipped");
  declareProperty("PreserveEvents", true, "If true, the output workspace will be an event workspace");
  declareProperty("LoadMonitors", false, "If true, the monitor workspace will be loaded");

  declareProperty("SolidAngleCorrection", true, "If true, the solide angle correction will be applied to the data");

  setPropertyGroup("UseConfigTOFCuts", load_grp);
  setPropertyGroup("LowTOFCut", load_grp);
  setPropertyGroup("HighTOFCut", load_grp);

  setPropertyGroup("WavelengthStep", load_grp);
  setPropertyGroup("UseConfigMask", load_grp);
  setPropertyGroup("UseConfig", load_grp);
  setPropertyGroup("CorrectForFlightPath", load_grp);

  setPropertyGroup("SkipTOFCorrection", load_grp);
  setPropertyGroup("PreserveEvents", load_grp);
  setPropertyGroup("LoadMonitors", load_grp);

  setPropertyGroup("SolidAngleCorrection", load_grp);

  declareProperty("SampleDetectorDistance", EMPTY_DBL(), "Sample to detector distance to use (overrides meta data), in mm");
  declareProperty("SampleDetectorDistanceOffset", EMPTY_DBL(), "Offset to the sample to detector distance (use only when using the distance found in the meta data), in mm");

  // Beam center
  std::string center_grp = "Beam Center";
  declareProperty("FindBeamCenter", false, "If True, the beam center will be calculated");
  declareProperty("UseConfigBeam", false, "If True, the beam center will be taken from the config file");

  //    Option 1: Set beam center by hand
  declareProperty("BeamCenterX", EMPTY_DBL(), "Position of the beam center, in pixel");
  declareProperty("BeamCenterY", EMPTY_DBL(), "Position of the beam center, in pixel");

  //    Option 2: Find it (expose properties from FindCenterOfMass)
  declareProperty(new API::FileProperty("BeamCenterFile", "", API::FileProperty::OptionalLoad, "_event.nxs"),
      "The name of the input event Nexus file to load");
  declareProperty("Tolerance", EMPTY_DBL(), "Tolerance on the center of mass position between each iteration [m]. Default: 0.00125");
  auto positiveDouble = boost::make_shared<BoundedValidator<double> >();
  positiveDouble->setLower(0);
  declareProperty("UseDirectBeamMethod", true, "If true, the direct beam method will be used");
  declareProperty("BeamRadius", EMPTY_DBL(),
      "Radius of the beam area used the exclude the beam when calculating "
      "the center of mass of the scattering pattern [pixels]. Default=3.0");

  setPropertyGroup("FindBeamCenter", center_grp);
  setPropertyGroup("UseConfigBeam", center_grp);
  setPropertyGroup("BeamCenterX", center_grp);
  setPropertyGroup("BeamCenterY", center_grp);
  setPropertyGroup("BeamCenterFile", center_grp);
  setPropertyGroup("Tolerance", center_grp);
  setPropertyGroup("UseDirectBeamMethod", center_grp);
  setPropertyGroup("BeamRadius", center_grp);

  // Dark current
  declareProperty(new API::FileProperty("DarkCurrentFile", "", API::FileProperty::OptionalLoad, "_event.nxs"),
      "The name of the input event Nexus file to load as dark current.");

  // Sensitivity
  std::string eff_grp = "Sensitivity";
  declareProperty(new API::FileProperty("SensitivityFile", "", API::FileProperty::OptionalLoad, "_event.nxs"),
      "Flood field or sensitivity file.");
  declareProperty("MinEfficiency", EMPTY_DBL(), positiveDouble,
      "Minimum efficiency for a pixel to be considered (default: no minimum).");
  declareProperty("MaxEfficiency", EMPTY_DBL(), positiveDouble,
      "Maximum efficiency for a pixel to be considered (default: no maximum).");
  declareProperty("UseDefaultDC", true, "If true, the dark current subtracted from the sample data will also be subtracted from the flood field.");
  declareProperty(new API::FileProperty("SensitivityDarkCurrentFile", "", API::FileProperty::OptionalLoad, "_event.nxs"),
      "The name of the input file to load as dark current.");
  declareProperty("SensitivityBeamCenterX", EMPTY_DBL(), "Position of the beam center for the sensitivity data, in pixel");
  declareProperty("SensitivityBeamCenterY", EMPTY_DBL(), "Position of the beam center for the sensitivity data, in pixel");
  declareProperty(new WorkspaceProperty<MatrixWorkspace>("OutputSensitivityWorkspace","", Direction::Output, PropertyMode::Optional));

  setPropertyGroup("SensitivityFile", eff_grp);
  setPropertyGroup("MinEfficiency", eff_grp);
  setPropertyGroup("MaxEfficiency", eff_grp);
  setPropertyGroup("UseDefaultDC", eff_grp);
  setPropertyGroup("SensitivityDarkCurrentFile", eff_grp);
  setPropertyGroup("SensitivityBeamCenterX", eff_grp);
  setPropertyGroup("SensitivityBeamCenterY", eff_grp);
  setPropertyGroup("OutputSensitivityWorkspace", eff_grp);

  declareProperty("SetupReducer",false, "If true, a Reducer object will be created");
  declareProperty("TransmissionValue", EMPTY_DBL(), "If set, this value will be used as the transmission");
  declareProperty(new API::FileProperty("TransmissionDirectBeam", "",
      API::FileProperty::OptionalLoad, "_event.nxs"),
      "Direct beam data file used to compute transmission");

  declareProperty(new API::FileProperty("TransmissionEmptyBeam", "",
      API::FileProperty::OptionalLoad, "_event.nxs"),
      "Empty beam data file used to compute transmission");
  setPropertySettings("TransmissionValue", new EnabledWhenProperty("SetupReducer", IS_EQUAL_TO, "1") );
  setPropertySettings("TransmissionDirectBeam", new EnabledWhenProperty("SetupReducer", IS_EQUAL_TO, "1") );
  setPropertySettings("TransmissionEmptyBeam", new EnabledWhenProperty("SetupReducer", IS_EQUAL_TO, "1") );


  // Outputs
  declareProperty("OutputMessage","",Direction::Output);
  declareProperty("ReductionProperties","__sans_reduction_properties", Direction::Input);
}

void SetupEQSANSReduction::exec()
{
  // Reduction property manager
  const std::string reductionManagerName = getProperty("ReductionProperties");
  if (reductionManagerName.size()==0)
  {
    g_log.error() << "ERROR: Reduction Property Manager name is empty" << std::endl;
    return;
  }
  boost::shared_ptr<PropertyManager> reductionManager = boost::make_shared<PropertyManager>();
  PropertyManagerDataService::Instance().addOrReplace(reductionManagerName, reductionManager);

  // Store name of the instrument
  reductionManager->declareProperty(new PropertyWithValue<std::string>("InstrumentName", "EQSANS") );

  // Load algorithm
  IAlgorithm_sptr loadAlg = createSubAlgorithm("EQSANSLoad");
  const bool useConfigBeam = getProperty("UseConfigBeam");
  loadAlg->setProperty("UseConfigBeam", useConfigBeam);
  const bool useConfigTOFCuts = getProperty("UseConfigTOFCuts");
  loadAlg->setProperty("UseConfigTOFCuts", useConfigTOFCuts);
  if (!useConfigTOFCuts)
  {
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
  const bool loadMonitors = getProperty("LoadMonitors");
  loadAlg->setProperty("LoadMonitors", loadMonitors);

  const double sdd = getProperty("SampleDetectorDistance");
  loadAlg->setProperty("SampleDetectorDistance", sdd);
  const double sddOffset = getProperty("SampleDetectorDistanceOffset");
  loadAlg->setProperty("SampleDetectorDistanceOffset", sddOffset);
  const double wlStep = getProperty("WavelengthStep");
  loadAlg->setProperty("WavelengthStep", wlStep);

  const bool useConfig = getProperty("UseConfig");
  loadAlg->setProperty("UseConfig", useConfig);
  const bool useConfigMask = getProperty("UseConfigMask");
  loadAlg->setProperty("UseConfigMask", useConfigMask);
  reductionManager->declareProperty(new AlgorithmProperty("LoadAlgorithm"));
  reductionManager->setProperty("LoadAlgorithm", loadAlg);

  // Store dark current algorithm
  const std::string darkCurrentFile = getPropertyValue("DarkCurrentFile");
  if (darkCurrentFile.size() > 0)
  {
    IAlgorithm_sptr darkAlg = createSubAlgorithm("EQSANSDarkCurrentSubtraction");
    darkAlg->setProperty("Filename", darkCurrentFile);
    darkAlg->setProperty("OutputDarkCurrentWorkspace", "");
    darkAlg->setPropertyValue("ReductionProperties", reductionManagerName);
    reductionManager->declareProperty(new AlgorithmProperty("DarkCurrentAlgorithm"));
    reductionManager->setProperty("DarkCurrentAlgorithm", darkAlg);
  }

  // Store default dark current algorithm
  IAlgorithm_sptr darkDefaultAlg = createSubAlgorithm("EQSANSDarkCurrentSubtraction");
  darkDefaultAlg->setProperty("OutputDarkCurrentWorkspace", "");
  darkDefaultAlg->setPropertyValue("ReductionProperties", reductionManagerName);
  reductionManager->declareProperty(new AlgorithmProperty("DefaultDarkCurrentAlgorithm"));
  reductionManager->setProperty("DefaultDarkCurrentAlgorithm", darkDefaultAlg);

  // Solid angle correction
  const bool solidAngleCorrection = getProperty("SolidAngleCorrection");
  if (solidAngleCorrection)
  {
    IAlgorithm_sptr solidAlg = createSubAlgorithm("SANSSolidAngleCorrection");
    reductionManager->declareProperty(new AlgorithmProperty("SANSSolidAngleCorrection"));
    reductionManager->setProperty("SANSSolidAngleCorrection", solidAlg);
  }

  // Beam center
  const double beamCenterX = getProperty("BeamCenterX");
  const double beamCenterY = getProperty("BeamCenterY");
  const bool calcBeamCenter = getProperty("FindBeamCenter");
  if (calcBeamCenter)
  {
    const std::string beamCenterFile = getProperty("BeamCenterFile");
    const bool useDirectBeamMethod = getProperty("UseDirectBeamMethod");
    const double beamRadius = getProperty("BeamRadius");

    IAlgorithm_sptr ctrAlg = createSubAlgorithm("SANSBeamFinder");
    ctrAlg->setProperty("Filename", beamCenterFile);
    ctrAlg->setProperty("UseDirectBeamMethod", useDirectBeamMethod);
    if (!isEmpty(beamRadius)) ctrAlg->setProperty("BeamRadius", beamRadius);
    ctrAlg->setPropertyValue("ReductionProperties", reductionManagerName);

    reductionManager->declareProperty(new AlgorithmProperty("SANSBeamFinderAlgorithm"));
    reductionManager->setProperty("SANSBeamFinderAlgorithm", ctrAlg);
  } else if (!isEmpty(beamCenterX) && !isEmpty(beamCenterY))
  {
    reductionManager->declareProperty(new PropertyWithValue<double>("LatestBeamCenterX", beamCenterX) );
    reductionManager->declareProperty(new PropertyWithValue<double>("LatestBeamCenterY", beamCenterY) );
  }

  // Sensitivity correction
  const std::string sensitivityFile = getPropertyValue("SensitivityFile");
  if (sensitivityFile.size() > 0)
  {
    const bool useSampleDC = getProperty("UseDefaultDC");
    const std::string sensitivityDarkCurrentFile = getPropertyValue("SensitivityDarkCurrentFile");
    const std::string outputSensitivityWS = getPropertyValue("OutputSensitivityWorkspace");
    const double minEff = getProperty("MinEfficiency");
    const double maxEff = getProperty("MaxEfficiency");
    const double sensitivityBeamCenterX = getProperty("SensitivityBeamCenterX");
    const double sensitivityBeamCenterY = getProperty("SensitivityBeamCenterY");

    IAlgorithm_sptr effAlg = createSubAlgorithm("SANSSensitivityCorrection");
    effAlg->setProperty("Filename", sensitivityFile);
    effAlg->setProperty("UseSampleDC", useSampleDC);
    effAlg->setProperty("DarkCurrentFile", sensitivityDarkCurrentFile);
    effAlg->setProperty("MinEfficiency", minEff);
    effAlg->setProperty("MaxEfficiency", maxEff);
    if (!isEmpty(sensitivityBeamCenterX)) effAlg->setProperty("BeamCenterX", sensitivityBeamCenterX);
    if (!isEmpty(sensitivityBeamCenterY)) effAlg->setProperty("BeamCenterY", sensitivityBeamCenterY);
    effAlg->setProperty("OutputSensitivityWorkspace", outputSensitivityWS);
    effAlg->setPropertyValue("ReductionProperties", reductionManagerName);
    reductionManager->declareProperty(new AlgorithmProperty("SensitivityAlgorithm"));
    reductionManager->setProperty("SensitivityAlgorithm", effAlg);
  }

  setPropertyValue("OutputMessage", "EQSANS reduction options set");

  // Create a python reduction singleton as needed
  const bool setupReducer = getProperty("SetupReducer");
  if (setupReducer) initializeReduction(reductionManager);
}

/*
 * For backward compatibility, we have the option of creating a
 * python ReductionSingleton object.
 */
void SetupEQSANSReduction::initializeReduction(boost::shared_ptr<PropertyManager> reductionManager)
{
  // Write the Reducer python script to be executed
  std::string script = "import reduction.instruments.sans.sns_command_interface as cmd\n";


  //  - beam center
  double center_x = 0.0;
  double center_y = 0.0;
  if (reductionManager->existsProperty("LatestBeamCenterX")
      && reductionManager->existsProperty("LatestBeamCenterY"))
  {
    center_x = reductionManager->getProperty("LatestBeamCenterX");
    center_y = reductionManager->getProperty("LatestBeamCenterY");
  }
  else
    throw std::runtime_error("EQSANSReduce not yet compatible with beam finder: enter beam center coordinates");

  script += "cmd.EQSANS()\n";
  script += "cmd.SetBeamCenter(" + Poco::NumberFormatter::format(center_x, 2)
    + ", " + Poco::NumberFormatter::format(center_y, 2) + ")\n";

  //  - sensitivity file
  if (reductionManager->existsProperty("SensitivityAlgorithm"))
  {
    IAlgorithm_sptr effAlg = reductionManager->getProperty("SensitivityAlgorithm");
    const std::string fileName = effAlg->getPropertyValue("Filename");
    if (fileName.size()>0)
      script += "cmd.SensitivityCorrection(\"" + fileName + "\")\n";
  }

  //  - load options
  if (reductionManager->existsProperty("LoadAlgorithm"))
  {
    IAlgorithm_sptr loadAlg = reductionManager->getProperty("LoadAlgorithm");

    // Correct for flight path?
    const bool tofCorr = loadAlg->getProperty("CorrectForFlightPath");
    if (tofCorr)
      script += "cmd.PerformFlightPathCorrection(True)\n";
    else
      script += "cmd.PerformFlightPathCorrection(False)\n";

    // Use TOF cut from config file?
    const bool confTOF = loadAlg->getProperty("UseConfigTOFCuts");
    if (confTOF)
      script += "cmd.UseConfigTOFTailsCutoff(use_config=True)\n";
    else
    {
      script += "cmd.UseConfigTOFTailsCutoff(use_config=False)\n";
      // Manual TOF cut
      const double lowTOF = loadAlg->getProperty("LowTOFCut");
      const double highTOF = loadAlg->getProperty("HighTOFCut");
      script += "cmd.SetTOFTailsCutoff(low_cut=" + Poco::NumberFormatter::format(lowTOF, 2)
          + ", high_cut=" + Poco::NumberFormatter::format(highTOF, 2) + ")\n";
    }

    // Use config mask?
    const bool confMask = loadAlg->getProperty("UseConfigMask");
    if (confMask)
      script += "cmd.UseConfigMask(use_config=True)\n";
    else
      script += "cmd.UseConfigMask(use_config=False)\n";
  }

  //  - Transmission
  const double trans = getProperty("TransmissionValue");
  if (isEmpty(trans))
  {
    const std::string directBeam = getPropertyValue("TransmissionDirectBeam");
    const std::string emptyBeam = getPropertyValue("TransmissionEmptyBeam");

    script += "cmd.DirectBeamTransmission(\"" + directBeam
        + "\", \"" + emptyBeam + "\")\n";
    script += "cmd.ThetaDependentTransmission(False)\n";
  }
  else
  {
    script += "cmd.SetTransmission(" + Poco::NumberFormatter::format(trans, 2) + ", 0.0)\n";
  }
  script += "cmd.ReductionSingleton().set_azimuthal_averager(None)\n";

  g_log.information() << "Reducer script:\n" << script << std::endl;

  // Run a snippet of python
  IAlgorithm_sptr alg = this->createSubAlgorithm("RunPythonScript");
  alg->setLogging(false);
  alg->setPropertyValue("Code", script);
  alg->execute();
}

} // namespace WorkflowAlgorithms
} // namespace Mantid

