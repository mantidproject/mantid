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

/// Sets documentation strings for this algorithm
void ApplyTransmissionCorrection::initDocs()
{
  this->setWikiSummary("Apply a transmission correction to 2D SANS data. ");
  this->setOptionalMessage("Apply a transmission correction to 2D SANS data.");
}


using namespace Kernel;
using namespace API;
using namespace Geometry;

void ApplyTransmissionCorrection::init()
{
  CompositeValidator<> *wsValidator = new CompositeValidator<>;
  wsValidator->add(new WorkspaceUnitValidator<>("Wavelength"));
  wsValidator->add(new HistogramValidator<>);
  declareProperty(new WorkspaceProperty<>("InputWorkspace","",Direction::Input,wsValidator),
      "Workspace to apply the transmission correction to");
  declareProperty("TransmissionWorkspace", "", "Workspace containing the transmission values");
  declareProperty(new WorkspaceProperty<>("OutputWorkspace","",Direction::Output),
      "Workspace to store the corrected data in");

  // Alternatively, the user can specify a transmission that will ba applied to all wavelength bins
  declareProperty("TransmissionValue", EMPTY_DBL(),
      "Transmission value to apply to all wavelengths. If specified, "
      "TransmissionWorkspace will not be used.");
  BoundedValidator<double> *mustBePositive = new BoundedValidator<double>();
  mustBePositive->setLower(0.0);
  declareProperty("TransmissionError", 0.0, mustBePositive,
    "The error on the transmission value (default 0.0)" );

}

void ApplyTransmissionCorrection::exec()
{
  MatrixWorkspace_const_sptr inputWS = getProperty("InputWorkspace");
  const double trans_value = getProperty("TransmissionValue");
  const double trans_error = getProperty("TransmissionError");

  // Now create the output workspace
  MatrixWorkspace_sptr outputWS = getProperty("OutputWorkspace");
  if ( outputWS != inputWS )
  {
    outputWS = WorkspaceFactory::Instance().create(inputWS);
    setProperty("OutputWorkspace",outputWS);
  }

  const int numHists = inputWS->getNumberHistograms();

  Progress progress(this,0.0,1.0,numHists);

  // If a transmission value was given, use it instead of the transmission workspace
  MantidVec trans(inputWS->readY(0).size(), trans_value);
  MantidVec dtrans(inputWS->readY(0).size(),trans_error);
  MantidVec& TrIn = trans;
  MantidVec& ETrIn = dtrans;

  if ( isEmpty(trans_value) ) {
    // Get the transmission workspace
    MatrixWorkspace_const_sptr transWS = boost::dynamic_pointer_cast<MatrixWorkspace>
          (AnalysisDataService::Instance().retrieve(getPropertyValue("TransmissionWorkspace")));

    // Check that the two input workspaces are consistent (same number of X bins)
    if ( transWS->readY(0).size() != inputWS->readY(0).size() )
    {
      g_log.error() << "Input and transmission workspaces have a different number of wavelength bins" << std::endl;
      throw std::invalid_argument("Input and transmission workspaces have a different number of wavelength bins");
    }

    TrIn  = transWS->readY(0);
    ETrIn = transWS->readE(0);
  }
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

    // Copy over the X data
    outputWS->dataX(i) = inputWS->readX(i);

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
      const double d1 = EIn[j]/t_term;
      const double d2 = ETrIn[j]*YIn[j]*exp_term/pow(TrIn[j], exp_term+1.0);
      EOut[j] = sqrt( d1*d1 + d2*d2 );
      YOut[j] = YIn[j]/t_term;
    }
    progress.report();
  }
}

} // namespace Algorithms
} // namespace Mantid

