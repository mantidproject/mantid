/*WIKI* 


*WIKI*/
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidWorkflowAlgorithms/SetupEQSANSReduction.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidWorkflowAlgorithms/ReductionTableHandler.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/EnabledWhenProperty.h"
#include "MantidAPI/FileProperty.h"

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
using namespace DataObjects;

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
  declareProperty(new WorkspaceProperty<TableWorkspace>("ReductionTableWorkspace","", Direction::Output, PropertyMode::Optional));
}

void SetupEQSANSReduction::exec()
{
  TableWorkspace_sptr reductionTable = getProperty("ReductionTableWorkspace");
  const std::string reductionTableName = getPropertyValue("ReductionTableWorkspace");

  ReductionTableHandler reductionHandler(reductionTable);
  if (!reductionTable && reductionTableName.size()>0)
    setProperty("ReductionTableWorkspace", reductionHandler.getTable());

  // Store name of the instrument
  reductionHandler.addEntry("InstrumentName", "EQSANS", true);

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
  reductionHandler.addEntry("LoadAlgorithm", loadAlg->toString());

  // Store dark current algorithm algorithm
  const std::string darkCurrentFile = getPropertyValue("DarkCurrentFile");
  if (darkCurrentFile.size() > 0)
  {
    IAlgorithm_sptr darkAlg = createSubAlgorithm("EQSANSDarkCurrentSubtraction");
    darkAlg->setProperty("Filename", darkCurrentFile);
    darkAlg->setProperty("OutputDarkCurrentWorkspace", "");
    darkAlg->setPropertyValue("ReductionTableWorkspace", reductionTableName);
    reductionHandler.addEntry("DarkCurrentAlgorithm", darkAlg->toString());
  }

  // Solid angle correction
  const bool solidAngleCorrection = getProperty("SolidAngleCorrection");
  if (solidAngleCorrection)
  {
    IAlgorithm_sptr solidAlg = createSubAlgorithm("SANSSolidAngleCorrection");
    reductionHandler.addEntry("SolidAngleAlgorithm", solidAlg->toString());
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
    reductionHandler.addEntry("BeamCenterAlgorithm", ctrAlg->toString());
    reductionHandler.addEntry("BeamCenterFile", beamCenterFile);
  } else {
    reductionHandler.addEntry("LatestBeamCenterX", beamCenterX);
    reductionHandler.addEntry("LatestBeamCenterY", beamCenterY);
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
    effAlg->setPropertyValue("ReductionTableWorkspace", reductionTableName);
    reductionHandler.addEntry("SensitivityAlgorithm", effAlg->toString());
  }

  setPropertyValue("OutputMessage", "EQSANS reduction options set");
}

} // namespace WorkflowAlgorithms
} // namespace Mantid

