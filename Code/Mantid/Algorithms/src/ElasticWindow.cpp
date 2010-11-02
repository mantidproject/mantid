//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/ElasticWindow.h"
#include "MantidAPI/WorkspaceValidators.h"

namespace Mantid
{
namespace Algorithms
{

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(ElasticWindow)

using namespace Kernel;
using namespace API;

void ElasticWindow::init()
{
  declareProperty(new WorkspaceProperty<>("InputWorkspace","",Direction::Input, new WorkspaceUnitValidator<>("DeltaE")));
  declareProperty(new WorkspaceProperty<>("OutputInQ","",Direction::Output));
  declareProperty(new WorkspaceProperty<>("OutputInQSquared","",Direction::Output));
  declareProperty("Range1Start", EMPTY_DBL(), new MandatoryValidator<double>());
  declareProperty("Range1End", EMPTY_DBL(), new MandatoryValidator<double>());
  declareProperty("Range2Start", EMPTY_DBL(), Direction::Input);
  declareProperty("Range2End", EMPTY_DBL(), Direction::Input);
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

  // Determine if we need to use the second time range...
  if ( ! ( ( enR2S == enR2E ) && ( enR2S == EMPTY_DBL() ) ) )
  {
    // ... FlatBackground, Minus, Integration...
    IAlgorithm_sptr flatBG = createSubAlgorithm("FlatBackground");
    flatBG->setProperty<MatrixWorkspace_sptr>("InputWorkspace", inputWorkspace);
    flatBG->setProperty<double>("StartX", enR2S);
    flatBG->setProperty<double>("EndX", enR2E);
    flatBG->setPropertyValue("Mode", "Mean");
    flatBG->setPropertyValue("OutputWorkspace", "flatBG");
    flatBG->execute();

    MatrixWorkspace_sptr flatBGws = flatBG->getProperty("OutputWorkspace");

    IAlgorithm_sptr integ = createSubAlgorithm("Integration");
    integ->setProperty<MatrixWorkspace_sptr>("InputWorkspace", flatBGws);
    integ->setProperty<double>("RangeLower", enR1S);
    integ->setProperty<double>("RangeUpper", enR1E);
    integ->setPropertyValue("OutputWorkspace", "integ");
    integ->execute();

    integWS = integ->getProperty("OutputWorkspace");
  }
  else
  {
    // ... Just Integration ...
    IAlgorithm_sptr integ = createSubAlgorithm("Integration");
    integ->setProperty<MatrixWorkspace_sptr>("InputWorkspace", inputWorkspace);
    integ->setProperty<double>("RangeLower", enR1S);
    integ->setProperty<double>("RangeUpper", enR1E);
    integ->setPropertyValue("OutputWorkspace", "integ");
    integ->execute();

    integWS = integ->getProperty("OutputWorkspace");
  }

  // ... ConvertSpectrumAxis (MomentumTransfer) ...
  IAlgorithm_sptr csaQ = createSubAlgorithm("ConvertSpectrumAxis");
  csaQ->setProperty<MatrixWorkspace_sptr>("InputWorkspace", integWS);
  csaQ->setPropertyValue("Target", "MomentumTransfer");
  csaQ->setPropertyValue("EMode", "Indirect");
  csaQ->setPropertyValue("OutputWorkspace", "csaQ");
  csaQ->execute();
  MatrixWorkspace_sptr csaQws = csaQ->getProperty("OutputWorkspace");
  // ... ConvertSpectrumAxis (Q2) ...
  IAlgorithm_sptr csaQ2 = createSubAlgorithm("ConvertSpectrumAxis");
  csaQ2->setProperty<MatrixWorkspace_sptr>("InputWorkspace", integWS);
  csaQ2->setPropertyValue("Target", "QSquared");
  csaQ2->setPropertyValue("EMode", "Indirect");
  csaQ2->setPropertyValue("OutputWorkspace", "csaQ2");
  csaQ2->execute();
  MatrixWorkspace_sptr csaQ2ws = csaQ2->getProperty("OutputWorkspace");

  // ... Transpose A ...
  IAlgorithm_sptr tranQ = createSubAlgorithm("Transpose");
  tranQ->setProperty<MatrixWorkspace_sptr>("InputWorkspace",csaQws);
  tranQ->setPropertyValue("OutputWorkspace", "outQ");
  tranQ->execute();
  outputQ = tranQ->getProperty("OutputWorkspace");
  // ... Transpose B ...
  IAlgorithm_sptr tranQ2 = createSubAlgorithm("Transpose");
  tranQ2->setProperty<MatrixWorkspace_sptr>("InputWorkspace",csaQ2ws);
  tranQ2->setPropertyValue("OutputWorkspace", "outQSquared");
  tranQ2->execute();
  outputQSquared = tranQ2->getProperty("OutputWorkspace");


  setProperty("OutputInQ", outputQ);
  setProperty("OutputInQSquared", outputQSquared);
}

} // namespace Algorithms
} // namespace Mantid

