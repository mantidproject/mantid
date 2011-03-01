//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/SANSSolidAngleCorrection.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/RebinParamsValidator.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/PhysicalConstants.h"
#include "MantidKernel/VectorHelper.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidAPI/SpectraDetectorMap.h"
#include "MantidDataObjects/Histogram1D.h"
#include <iostream>
#include <vector>

namespace Mantid
{
namespace Algorithms
{

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(SANSSolidAngleCorrection)

using namespace Kernel;
using namespace API;
using namespace Geometry;
using namespace DataObjects;

void SANSSolidAngleCorrection::init()
{
  //this->setWikiSummary("Performs solid angle correction on SANS 2D data.");
  //this->setOptionalMessage("Performs solid angle correction on SANS 2D data.");

  CompositeValidator<> *wsValidator = new CompositeValidator<>;
  wsValidator->add(new WorkspaceUnitValidator<>("Wavelength"));
  wsValidator->add(new HistogramValidator<>);
  declareProperty(new WorkspaceProperty<>("InputWorkspace","",Direction::Input,wsValidator));
  declareProperty(new WorkspaceProperty<>("OutputWorkspace","",Direction::Output));
}

void SANSSolidAngleCorrection::exec()
{
  MatrixWorkspace_const_sptr inputWS = getProperty("InputWorkspace");

  // Now create the output workspace
  MatrixWorkspace_sptr outputWS = getProperty("OutputWorkspace");
  if ( outputWS != inputWS )
  {
    outputWS = WorkspaceFactory::Instance().create(inputWS);
    setProperty("OutputWorkspace",outputWS);
  }

  const int numHists = inputWS->getNumberHistograms();
  Progress progress(this,0.0,1.0,numHists);

  // Number of X bins
  const int xLength = inputWS->readY(0).size();

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

    // Compute solid angle correction factor
    const double tanTheta = tan( inputWS->detectorTwoTheta(det) );
    const double term = sqrt(tanTheta*tanTheta + 1.0);
    const double corr = term*term*term;

    // Correct data for all X bins
    for (int j = 0; j < xLength; j++)
    {
      YOut[j] = YIn[j]*corr;
      EOut[j] = fabs(EIn[j]*corr);
    }
    progress.report();
  }

}

} // namespace Algorithms
} // namespace Mantid

