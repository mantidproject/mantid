//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/ApplyTransmissionCorrection.h"
#include "MantidAPI/WorkspaceValidators.h"

namespace Mantid
{
namespace Algorithms
{

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(ApplyTransmissionCorrection)

using namespace Kernel;
using namespace API;
using namespace Geometry;

void ApplyTransmissionCorrection::init()
{
  CompositeValidator<> *wsValidator = new CompositeValidator<>;
  wsValidator->add(new WorkspaceUnitValidator<>("Wavelength"));
  wsValidator->add(new HistogramValidator<>);
  declareProperty(new WorkspaceProperty<>("InputWorkspace","",Direction::Input,wsValidator));
  declareProperty(new WorkspaceProperty<>("TransmissionWorkspace","",Direction::Input,wsValidator->clone()));
  declareProperty(new WorkspaceProperty<>("OutputWorkspace","",Direction::Output));
}

void ApplyTransmissionCorrection::exec()
{
  MatrixWorkspace_const_sptr inputWS = getProperty("InputWorkspace");
  MatrixWorkspace_const_sptr transWS = getProperty("TransmissionWorkspace");

  // Now create the output workspace
  MatrixWorkspace_sptr outputWS = getProperty("OutputWorkspace");
  if ( outputWS != inputWS )
  {
    outputWS = WorkspaceFactory::Instance().create(inputWS);
    setProperty("OutputWorkspace",outputWS);
  }

  const int numHists = inputWS->getNumberHistograms();

  Progress progress(this,0.0,1.0,numHists);

  // Check that the two input workspaces are consistent (same number of X bins)
  if ( transWS->readY(0).size() != inputWS->readY(0).size() )
  {
    g_log.error() << "Input and transmission workspaces have a different number of wavelength bins" << std::endl;
    throw std::invalid_argument("Input and transmission workspaces have a different number of wavelength bins");
  }

  // Access the transmission data
  const MantidVec& TrIn  = transWS->readY(0);
  const MantidVec& ETrIn = transWS->readE(0);

  // Loop through the spectra and apply correction
  for (int i = 0; i < numHists; ++i)
  {
    IDetector_const_sptr det;
    try {
      det = inputWS->getDetector(i);
    } catch (Exception::NotFoundError&) {
      g_log.warning() << "Spectrum index " << i << " has no detector assigned to it - discarding" << std::endl;
      continue;
    }

    // Skip if we have a monitor or if the detector is masked.
    if ( det->isMonitor() || det->isMasked() ) continue;

    const MantidVec& YIn = inputWS->readY(i);
    const MantidVec& EIn = inputWS->readE(i);

    MantidVec& YOut = outputWS->dataY(i);
    MantidVec& EOut = outputWS->dataE(i);

    // Compute transmission exponent
    const double exp_term = (1.0/cos( inputWS->detectorTwoTheta(det) ) + 1.0)/2.0;

    // Correct data for all X bins
    for (int j = 0; j < (int)inputWS->readY(0).size(); j++)
    {
      const double t_term = pow(TrIn[j], exp_term);
      YOut[j] = YIn[j]/t_term;
      const double d1 = EIn[j]/t_term;
      const double d2 = ETrIn[j]*YIn[j]*exp_term/pow(TrIn[j], exp_term+1.0);
      EOut[j] = sqrt( d1*d1 + d2*d2 );
    }
    progress.report();
  }
}

} // namespace Algorithms
} // namespace Mantid

