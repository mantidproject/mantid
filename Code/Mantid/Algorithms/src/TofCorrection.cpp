#include "MantidAlgorithms/TofCorrection.h"

#include "MantidAPI/WorkspaceValidators.h"

namespace Mantid
{
namespace Algorithms
{
using namespace Kernel;
using namespace API;
using namespace Geometry;

DECLARE_ALGORITHM(TofCorrection)

void TofCorrection::init()
{
  CompositeValidator<> *wsVal = new CompositeValidator<>;
  wsVal->add(new WorkspaceUnitValidator<>("TOF"));
  wsVal->add(new HistogramValidator<>);
  wsVal->add(new SpectraAxisValidator<>);
  declareProperty(new WorkspaceProperty<>("InputWorkspace", "", Direction::Input,wsVal));
  declareProperty(new WorkspaceProperty<>("OutputWorkspace", "", Direction::Output));
}

void TofCorrection::exec()
{
  // Get the input workspace
  MatrixWorkspace_const_sptr inputWS = getProperty("InputWorkspace");
  const int nHist = inputWS->getNumberHistograms();
  const int nBins = inputWS->blocksize();
  // Create the output workspace to be a duplicate of the input one
  MatrixWorkspace_sptr outputWS;
  outputWS = WorkspaceFactory::Instance().create(inputWS);
  // Get the sample object
  IObjComponent_const_sptr sample = inputWS->getInstrument()->getSample();
  for ( int i = 0; i < nHist; i++ )
  {
    outputWS->dataY(i) = inputWS->readY(i);
    outputWS->dataE(i) = inputWS->readE(i);
    IDetector_sptr detector;
    try
    {
      detector = inputWS->getDetector(i);
    } catch ( Mantid::Kernel::Exception::NotFoundError )
    {
      g_log.warning() << "Unable to retrieve detector information for spectra " << i <<  ", data has been copied verbatim." << std::endl;
      outputWS->dataX(i) = inputWS->readX(i);
      continue;
    }
    double l2 = detector->getDistance(*sample);
    double efixed;
    if ( detector->hasParameter("Efixed") )
    {
      efixed = detector->getNumberParameter("Efixed").at(0);
    }
    else
    {
      g_log.warning() << "No Efixed set for detector " << i << ", data has been copied verbatim." << std::endl;
      outputWS->dataX(i) = inputWS->readX(i);
      continue;
    }
    double adjustment = ( l2 / sqrt(efixed) ) * 2286.2873574; // 2286.2873574 - what is this value?
                                                              // require clarification from T Ramirez-Cuesta
    for ( int j = 0; j < (nBins+1); j++ )
    {
      outputWS->dataX(i)[j] = inputWS->readX(i)[j] - adjustment;
    }
  }
  // Finally, set the output property to be the workspace created.
  setProperty("OutputWorkspace", outputWS);
}
} // namespace Algorithms
} // namespace Mantid