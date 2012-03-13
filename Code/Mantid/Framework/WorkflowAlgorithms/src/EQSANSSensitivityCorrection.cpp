/*WIKI* 





*WIKI*/
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidWorkflowAlgorithms/EQSANSSensitivityCorrection.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidAPI/WorkspaceOpOverloads.h"
#include "MantidGeometry/IDetector.h"

namespace Mantid
{
namespace WorkflowAlgorithms
{

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(EQSANSSensitivityCorrection)

/// Sets documentation strings for this algorithm
void EQSANSSensitivityCorrection::initDocs()
{
  this->setWikiSummary("Perform EQSANS sensitivity correction.");
  this->setOptionalMessage("Perform EQSANS sensitivity correction.");
}

using namespace Kernel;
using namespace API;
using namespace Geometry;

void EQSANSSensitivityCorrection::init()
{
  auto wsValidator = boost::make_shared<CompositeValidator>();
  wsValidator->add<WorkspaceUnitValidator>("Wavelength");
  wsValidator->add<HistogramValidator>();
  wsValidator->add(boost::make_shared<CommonBinsValidator>());
  declareProperty(new WorkspaceProperty<>("InputWorkspace","",Direction::Input,wsValidator));

  auto histoValidator = boost::make_shared<HistogramValidator>();
  declareProperty(new WorkspaceProperty<>("EfficiencyWorkspace","",Direction::Input, histoValidator));

  declareProperty(new WorkspaceProperty<>("OutputWorkspace","",Direction::Output));
  declareProperty(new WorkspaceProperty<>("OutputEfficiencyWorkspace","",Direction::Output));

  declareProperty("OutputMessage","",Direction::Output);

  declareProperty("Factor", 1.0, "Exponential factor for the wavelength dependence of the efficiency.");
  declareProperty("Error", 0.0, "Error on the expornential factor.");
}

void EQSANSSensitivityCorrection::exec()
{
  MatrixWorkspace_sptr inputWS = getProperty("InputWorkspace");
  MatrixWorkspace_sptr effWS = getProperty("EfficiencyWorkspace");
  const double factor = getProperty("Factor");
  const double error = getProperty("Error");

  Progress progress(this,0.0,1.0,5);

  // Now create a workspace to put in the wavelength-dependence
  MatrixWorkspace_sptr lambdaWS = WorkspaceFactory::Instance().create(inputWS);

  // Number of X bins
  const int xLength = static_cast<int>(inputWS->readY(0).size());
  // Number of detector pixels
  const int numHists = static_cast<int>(inputWS->getNumberHistograms());
  const MantidVec& XIn = inputWS->dataX(0);
  MantidVec& YOut = lambdaWS->dataY(0);
  MantidVec& EOut = lambdaWS->dataE(0);

  progress.report("Computing detector efficiency");
  for (int i = 0; i < xLength; i++)
  {
    double wl = (XIn[i]+XIn[i+1])/2.0;
    YOut[i] = 1.0-std::exp(-factor*wl);
    EOut[i] = std::fabs(factor)*std::exp(-factor*wl)*error;
  }

  progress.report("Computing detector efficiency");
  for (int i = 0; i < numHists; i++)
  {
    MantidVec& YOut_i = lambdaWS->dataY(i);
    MantidVec& EOut_i = lambdaWS->dataE(i);
    MantidVec& XOut_i = lambdaWS->dataX(i);
    YOut_i = YOut;
    EOut_i = EOut;
    XOut_i = XIn;
  }

  lambdaWS = lambdaWS * effWS;
  MatrixWorkspace_sptr outputWS = inputWS / lambdaWS;
  setProperty("OutputWorkspace", outputWS);
  setProperty("OutputEfficiencyWorkspace", lambdaWS);

  setProperty("OutputMessage", "Applied wavelength-dependent sensitivity correction");
}

} // namespace WorkflowAlgorithms
} // namespace Mantid

