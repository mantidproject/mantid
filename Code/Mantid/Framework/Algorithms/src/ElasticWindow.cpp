/*WIKI* 


This algorithm Integrates over the range specified, converts the spectrum axis into units of Q and Q^2 and Transposes the result workspaces.

There are two output workspaces.


*WIKI*/
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/ElasticWindow.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/MandatoryValidator.h"

namespace Mantid
{
namespace Algorithms
{

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(ElasticWindow)

/// Sets documentation strings for this algorithm
void ElasticWindow::initDocs()
{
  this->setWikiSummary("This algorithm performs an integration over an energy range, with the option to subtract a background over a second range, then transposes the result into a single-spectrum workspace with units in Q and Q^2. ");
  this->setOptionalMessage("This algorithm performs an integration over an energy range, with the option to subtract a background over a second range, then transposes the result into a single-spectrum workspace with units in Q and Q^2.");
}


using namespace Kernel;
using namespace API;

void ElasticWindow::init()
{
  declareProperty(new WorkspaceProperty<>("InputWorkspace","",Direction::Input, boost::make_shared<WorkspaceUnitValidator>("DeltaE")),
    "The input workspace.");
  declareProperty(new WorkspaceProperty<>("OutputInQ","",Direction::Output),
    "The name for output workspace with the X axis in units of Q");
  declareProperty(new WorkspaceProperty<>("OutputInQSquared","",Direction::Output),
    "The name for output workspace with the X axis in units of Q^2.");
  declareProperty("Range1Start", EMPTY_DBL(), boost::make_shared<MandatoryValidator<double> >(),
    "Start Point of Range 1");
  declareProperty("Range1End", EMPTY_DBL(), boost::make_shared<MandatoryValidator<double> >(),
    "End Point of Range 1");
  declareProperty("Range2Start", EMPTY_DBL(),"Start Point of Range 2", Direction::Input);
  declareProperty("Range2End", EMPTY_DBL(), "End Point of Range 2.", Direction::Input);
}

void ElasticWindow::exec()
{
  MatrixWorkspace_sptr inputWorkspace = getProperty("InputWorkspace");

  double enR1S = getProperty("Range1Start");
  double enR1E = getProperty("Range1End");
  double enR2S = getProperty("Range2Start");
  double enR2E = getProperty("Range2End");

  // Create the output workspaces
  MatrixWorkspace_sptr integWS;

  MatrixWorkspace_sptr outputQ;
  MatrixWorkspace_sptr outputQSquared;
  
  const bool childAlgLogging(true);
  double startProgress(0.0), stepProgress(0.0), endProgress(0.0);
  // Determine if we need to use the second time range...
  if ( ! ( ( enR2S == enR2E ) && ( enR2S == EMPTY_DBL() ) ) )
  {
    stepProgress = 1.0/6.0;
    
    // ... CalculateFlatBackground, Minus, Integration...
    IAlgorithm_sptr flatBG = createChildAlgorithm("CalculateFlatBackground",startProgress, endProgress,childAlgLogging);
    flatBG->setProperty<MatrixWorkspace_sptr>("InputWorkspace", inputWorkspace);
    flatBG->setProperty<double>("StartX", enR2S);
    flatBG->setProperty<double>("EndX", enR2E);
    flatBG->setPropertyValue("Mode", "Mean");
    flatBG->setPropertyValue("OutputWorkspace", "flatBG");
    flatBG->execute();
    startProgress += stepProgress;
    endProgress += stepProgress;

    MatrixWorkspace_sptr flatBGws = flatBG->getProperty("OutputWorkspace");

    IAlgorithm_sptr integ = createChildAlgorithm("Integration",startProgress, endProgress,childAlgLogging);
    integ->setProperty<MatrixWorkspace_sptr>("InputWorkspace", flatBGws);
    integ->setProperty<double>("RangeLower", enR1S);
    integ->setProperty<double>("RangeUpper", enR1E);
    integ->setPropertyValue("OutputWorkspace", "integ");
    integ->execute();

    integWS = integ->getProperty("OutputWorkspace");
  }
  else
  {
    stepProgress = 1.0/5.0;

    // ... Just Integration ...
    IAlgorithm_sptr integ = createChildAlgorithm("Integration",startProgress, endProgress,childAlgLogging);
    integ->setProperty<MatrixWorkspace_sptr>("InputWorkspace", inputWorkspace);
    integ->setProperty<double>("RangeLower", enR1S);
    integ->setProperty<double>("RangeUpper", enR1E);
    integ->setPropertyValue("OutputWorkspace", "integ");
    integ->execute();

    integWS = integ->getProperty("OutputWorkspace");
  }
  startProgress += stepProgress;
  endProgress += stepProgress;

  // ... ConvertSpectrumAxis (ElasticQ). Version 2 to give the correct number
  const int version = 2;
  IAlgorithm_sptr csaQ = createChildAlgorithm("ConvertSpectrumAxis",startProgress,endProgress,childAlgLogging,version);
  csaQ->setProperty<MatrixWorkspace_sptr>("InputWorkspace", integWS);
  csaQ->setPropertyValue("Target", "ElasticQ");
  csaQ->setPropertyValue("EMode", "Indirect");
  csaQ->setPropertyValue("OutputWorkspace", "csaQ");
  csaQ->execute();
  MatrixWorkspace_sptr csaQws = csaQ->getProperty("OutputWorkspace");
  startProgress += stepProgress;
  endProgress += stepProgress;
  
  // ... ConvertSpectrumAxis (Q2) ...
  IAlgorithm_sptr csaQ2 = createChildAlgorithm("ConvertSpectrumAxis",startProgress,endProgress,childAlgLogging,version);
  csaQ2->setProperty<MatrixWorkspace_sptr>("InputWorkspace", integWS);
  csaQ2->setPropertyValue("Target", "ElasticQSquared");
  csaQ2->setPropertyValue("EMode", "Indirect");
  csaQ2->setPropertyValue("OutputWorkspace", "csaQ2");
  csaQ2->execute();
  MatrixWorkspace_sptr csaQ2ws = csaQ2->getProperty("OutputWorkspace");
  startProgress += stepProgress;
  endProgress += stepProgress;


  // ... Transpose A ...
  IAlgorithm_sptr tranQ = createChildAlgorithm("Transpose",startProgress,endProgress,childAlgLogging);
  tranQ->setProperty<MatrixWorkspace_sptr>("InputWorkspace",csaQws);
  tranQ->setPropertyValue("OutputWorkspace", "outQ");
  tranQ->execute();
  outputQ = tranQ->getProperty("OutputWorkspace");
  startProgress += stepProgress;
  endProgress += stepProgress;

  // ... Transpose B ...
  IAlgorithm_sptr tranQ2 = createChildAlgorithm("Transpose", startProgress,endProgress,childAlgLogging);
  tranQ2->setProperty<MatrixWorkspace_sptr>("InputWorkspace",csaQ2ws);
  tranQ2->setPropertyValue("OutputWorkspace", "outQSquared");
  tranQ2->execute();
  outputQSquared = tranQ2->getProperty("OutputWorkspace");
  startProgress += stepProgress;
  endProgress += stepProgress;


  setProperty("OutputInQ", outputQ);
  setProperty("OutputInQSquared", outputQSquared);
}

} // namespace Algorithms
} // namespace Mantid

