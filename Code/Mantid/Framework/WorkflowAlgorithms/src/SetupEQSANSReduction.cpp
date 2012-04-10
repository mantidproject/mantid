/*WIKI* 


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
  declareProperty("LowTOFCut", 0.0, Direction::Input);
  declareProperty("HighTOFCut", 0.0, Direction::Input);
  declareProperty("UseConfigMask", false, "If true, the masking information found in the configuration file will be used");
  declareProperty("CorrectForFlightPath", false, "If true, the TOF will be modified for the true flight path from the sample to the detector pixel");
  declareProperty("SolidAngleCorrection", true, Direction::Input);

  setPropertyGroup("UseConfigTOFCuts", load_grp);
  setPropertyGroup("LowTOFCut", load_grp);
  setPropertyGroup("HighTOFCut", load_grp);
  setPropertyGroup("UseConfigMask", load_grp);
  setPropertyGroup("CorrectForFlightPath", load_grp);
  setPropertyGroup("SolidAngleCorrection", load_grp);

  // Beam center
  std::string center_grp = "Beam Center";
  declareProperty("FindBeamCenter", false, "If True, the beam center will be calculated");

  //    Option 1: Set beam center by hand
  declareProperty("BeamCenterX", EMPTY_DBL(), "Position of the beam center, in pixel");
  declareProperty("BeamCenterY", EMPTY_DBL(), "Position of the beam center, in pixel");

  //    Option 2: Find it (expose properties from FindCenterOfMass)
  declareProperty(new API::FileProperty("BeamCenterFile", "", API::FileProperty::OptionalLoad, ".nxs"),
      "The name of the input event Nexus file to load");
  declareProperty("Tolerance", EMPTY_DBL(), "Tolerance on the center of mass position between each iteration [m]. Default: 0.00125");
  auto positiveDouble = boost::make_shared<BoundedValidator<double> >();
  positiveDouble->setLower(0);
  declareProperty("BeamRadius", EMPTY_DBL(),
      "Radius of the beam area used the exclude the beam when calculating "
      "the center of mass of the scattering pattern [pixels]. Default=3.0");

  setPropertyGroup("FindBeamCenter", center_grp);
  setPropertyGroup("BeamCenterX", center_grp);
  setPropertyGroup("BeamCenterY", center_grp);
  setPropertyGroup("BeamCenterFile", center_grp);
  setPropertyGroup("Tolerance", center_grp);
  setPropertyGroup("BeamRadius", center_grp);

  // Dark current
  declareProperty(new API::FileProperty("DarkCurrentFile", "", API::FileProperty::OptionalLoad, ".nxs"),
      "The name of the input event Nexus file to load as dark current.");

  // Sensitivity
  std::string eff_grp = "Sensitivity";
  declareProperty(new API::FileProperty("SensitivityFile", "", API::FileProperty::OptionalLoad, ".nxs"),
      "Flood field or sensitivity file.");
  declareProperty("MinEfficiency", EMPTY_DBL(), positiveDouble,
      "Minimum efficiency for a pixel to be considered (default: no minimum).");
  declareProperty("MaxEfficiency", EMPTY_DBL(), positiveDouble,
      "Maximum efficiency for a pixel to be considered (default: no maximum).");
  declareProperty("UseDefaultDC", true, "If true, the dark current subtracted from the sample data will also be subtracted from the flood field.");
  declareProperty(new API::FileProperty("SensitivityDarkCurrentFile", "", API::FileProperty::OptionalLoad, ".nxs"),
      "The name of the input file to load as dark current.");
  declareProperty("SensitivityBeamCenterX", EMPTY_DBL(), "Position of the beam center for the sensitivity data, in pixel");
  declareProperty("SensitivityBeamCenterY", EMPTY_DBL(), "Position of the beam center for the sensitivity data, in pixel");
  declareProperty(new WorkspaceProperty<MatrixWorkspace>("OutputSensitivityWorkspace","", Direction::Output, PropertyMode::Optional));

  setPropertyGroup("SensitivityFile", eff_grp);
  setPropertyGroup("MinEfficiency", eff_grp);
  setPropertyGroup("MaxEfficiency", eff_grp);
  setPropertyGroup("UseDefaultDC", eff_grp);
  setPropertyGroup("SensitivityDarkCurrentFile", eff_grp);
  setPropertyGroup("OutputSensitivityWorkspace", eff_grp);

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
  loadAlg->setProperty("UseConfigBeam", false);
  const bool useConfigTOFCuts = getProperty("UseConfigTOFCuts");
  loadAlg->setProperty("UseConfigTOFCuts", useConfigTOFCuts);
  if (!useConfigTOFCuts)
  {
    const double lowTOFCut = getProperty("LowTOFCut");
    const double highTOFCut = getProperty("HighTOFCut");
    loadAlg->setProperty("LowTOFCut", lowTOFCut);
    loadAlg->setProperty("HighTOFCut", highTOFCut);
  }
  const bool useConfigMask = getProperty("UseConfigMask");
  loadAlg->setProperty("UseConfigMask", useConfigMask);
  const bool correctForFlightPath = getProperty("CorrectForFlightPath");
  loadAlg->setProperty("CorrectForFlightPath", correctForFlightPath);
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
    const double tolerance = getProperty("Tolerance");
    const double beamRadius = getProperty("BeamRadius");
    IAlgorithm_sptr ctrAlg = createSubAlgorithm("FindCenterOfMassPosition");
    if (!isEmpty(tolerance)) ctrAlg->setProperty("Tolerance", tolerance);
    if (!isEmpty(beamRadius)) ctrAlg->setProperty("BeamRadius", beamRadius);
    reductionManager->declareProperty(new AlgorithmProperty("BeamCenterAlgorithm"));
    reductionManager->setProperty("BeamCenterAlgorithm", ctrAlg);
    reductionManager->declareProperty(new PropertyWithValue<std::string>("BeamCenterFile", beamCenterFile) );
  } else {
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
}

} // namespace WorkflowAlgorithms
} // namespace Mantid

