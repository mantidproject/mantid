/*WIKI* 
Calculate the EQSANS detector sensitivity.
*WIKI*/
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidWorkflowAlgorithms/ComputeSensitivity.h"
#include "MantidWorkflowAlgorithms/ReductionTableHandler.h"
#include "MantidAPI/FileProperty.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidWorkflowAlgorithms/EQSANSInstrument.h"

namespace Mantid
{
namespace WorkflowAlgorithms
{

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(ComputeSensitivity)

/// Sets documentation strings for this algorithm
void ComputeSensitivity::initDocs()
{
  this->setWikiSummary("Calculate sensitivity correction.");
  this->setOptionalMessage("Calculate sensitivity correction.");
}

using namespace Kernel;
using namespace API;
using namespace Geometry;
using namespace DataObjects;

void ComputeSensitivity::init()
{
  declareProperty(new API::FileProperty("Filename", "", API::FileProperty::Load, ".nxs"),
      "Flood field or sensitivity file.");
  declareProperty(new WorkspaceProperty<TableWorkspace>("ReductionTableWorkspace","", Direction::Output));
  declareProperty(new WorkspaceProperty<>("OutputWorkspace","",Direction::Output));
  declareProperty("OutputMessage","",Direction::Output);
}

void ComputeSensitivity::exec()
{
  std::string outputMessage = "";
  TableWorkspace_sptr reductionTable = getProperty("ReductionTableWorkspace");
  const std::string reductionTableName = getPropertyValue("ReductionTableWorkspace");
  const std::string outputWS = getPropertyValue("OutputWorkspace");
  const std::string fileName = getPropertyValue("Filename");

  ReductionTableHandler reductionHandler(reductionTable);
  if (!reductionTable && reductionTableName.size()>0)
    setProperty("ReductionTableWorkspace", reductionHandler.getTable());

  // Find load algorithm
  const std::string loader = reductionHandler.findStringEntry("LoadAlgorithm");
  if (loader.size()==0) g_log.error() << "WARNING! No data loader found!" << std::endl;

  // Find beam center
  const std::string beamCenter = reductionHandler.findStringEntry("BeamCenterAlgorithm");
  const std::string beamCenterFile = reductionHandler.findStringEntry("BeamCenterFile");
  double center_x = reductionHandler.findDoubleEntry("LatestBeamCenterX");
  double center_y = reductionHandler.findDoubleEntry("LatestBeamCenterY");

  if (beamCenter.size()>0 && beamCenterFile.size()>0)
  {
    // Load direct beam file
    IAlgorithm_sptr loadAlg = Algorithm::fromString(loader);
    loadAlg->setChild(true);
    loadAlg->setProperty("Filename", beamCenterFile);
    loadAlg->execute();
    MatrixWorkspace_sptr beamCenterWS = loadAlg->getProperty("OutputWorkspace");
    const std::string outMsg = loadAlg->getPropertyValue("OutputMessage");
    outputMessage += outMsg;

    IAlgorithm_sptr centerAlg = Algorithm::fromString(beamCenter);
    centerAlg->setChild(true);
    centerAlg->setProperty("InputWorkspace", beamCenterWS);
    centerAlg->execute();
    std::vector<double> centerOfMass = centerAlg->getProperty("CenterOfMass");
    EQSANSInstrument::getPixelFromCoordinate(centerOfMass[0], centerOfMass[1], beamCenterWS, center_x, center_y);

  }
  else if (isEmpty(center_x) || isEmpty(center_y))
  {
    g_log.notice() << "WARNING! No beam center information found!" << std::endl;
  }

  std::string eff = reductionHandler.findStringEntry("SensitivityAlgorithm");
  if (eff.size()==0) g_log.error() << "Could not find sensitivity algorithm" << std::endl;
  IAlgorithm_sptr effAlg = Algorithm::fromString(eff);
  effAlg->setChild(true);
  effAlg->setProperty("Filename", fileName);
  effAlg->setProperty("BeamCenterX", center_x);
  effAlg->setProperty("BeamCenterY", center_y);
  effAlg->setPropertyValue("OutputSensitivityWorkspace", outputWS);
  effAlg->execute();
  MatrixWorkspace_sptr effWS = effAlg->getProperty("OutputSensitivityWorkspace");
  std::string outMsg2 = effAlg->getPropertyValue("OutputMessage");
  setProperty("OutputWorkspace", effWS);
  outputMessage += outMsg2;
  setProperty("OutputMessage", outputMessage);

}

} // namespace WorkflowAlgorithms
} // namespace Mantid

