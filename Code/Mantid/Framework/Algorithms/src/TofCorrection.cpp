#include "MantidAlgorithms/TofCorrection.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidKernel/MultiThreaded.h"

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
  wsVal->add(new InstrumentValidator<>);
  declareProperty(new WorkspaceProperty<>("InputWorkspace", "", Direction::Input,wsVal));
  declareProperty(new WorkspaceProperty<>("OutputWorkspace", "", Direction::Output));
}

void TofCorrection::exec()
{
  // Get the input workspace
  MatrixWorkspace_const_sptr inputWS = getProperty("InputWorkspace");
  const int nHist = inputWS->getNumberHistograms();

  // Create the output workspace to be a duplicate of the input one
  MatrixWorkspace_sptr outputWS;
  outputWS = WorkspaceFactory::Instance().create(inputWS);
  // Get the sample object
  IObjComponent_const_sptr sample = inputWS->getInstrument()->getSample();

  PARALLEL_FOR2(inputWS, outputWS)
  for ( int i = 0; i < nHist; i++ )
  {
    // Start multi-threading
    PARALLEL_START_INTERUPT_REGION

    outputWS->dataY(i) = inputWS->readY(i);
    outputWS->dataE(i) = inputWS->readE(i);
    IDetector_sptr detector;
    try
    {
      detector = inputWS->getDetector(i);
    }
    catch ( Mantid::Kernel::Exception::NotFoundError& )
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
    // 2286.2873574 = sqrt(0.5 * m_neutron) with units of microsec meV^0.5 m^-1
    double adjustment = ( l2 / sqrt(efixed) ) * 2286.2873574;

    std::transform(inputWS->readX(i).begin(), inputWS->readX(i).end(), outputWS->dataX(i).begin(), std::bind2nd(std::minus<double>(), adjustment));

    // end multi-threading
    PARALLEL_END_INTERUPT_REGION
  }
  PARALLEL_CHECK_INTERUPT_REGION
  // Finally, set the output property to be the workspace created.
  setProperty("OutputWorkspace", outputWS);
}
} // namespace Algorithms
} // namespace Mantid
