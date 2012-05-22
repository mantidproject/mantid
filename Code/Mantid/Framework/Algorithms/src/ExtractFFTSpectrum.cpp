/*WIKI* 



*WIKI*/
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/ExtractFFTSpectrum.h"
#include "MantidKernel/MultiThreaded.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/BoundedValidator.h"

namespace Mantid
{
namespace Algorithms
{

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(ExtractFFTSpectrum)

/// Sets documentation strings for this algorithm
void ExtractFFTSpectrum::initDocs()
{
  this->setWikiSummary("This algorithm performs a [[FFT|Fast Fourier Transform]] on each spectrum in a workspace, and from the result takes the indicated spectrum and places it into the OutputWorkspace, so that you end up with one result spectrum for each input spectrum in the same workspace. ");
  this->setOptionalMessage("This algorithm performs a Fast Fourier Transform on each spectrum in a workspace, and from the result takes the indicated spectrum and places it into the OutputWorkspace, so that you end up with one result spectrum for each input spectrum in the same workspace.");
}


using namespace Kernel;
using namespace API;

void ExtractFFTSpectrum::init()
{
  declareProperty(new WorkspaceProperty<>("InputWorkspace","",Direction::Input),"The input workspace.");
  //if desired, provide the imaginary part in a separate workspace.
  declareProperty(new WorkspaceProperty<>("InputImagWorkspace","",Direction::Input,PropertyMode::Optional),"The optional input workspace for the imaginary part.");
  declareProperty("FFTPart", 2, boost::make_shared<BoundedValidator<int>>(0,5));
  declareProperty(new WorkspaceProperty<>("OutputWorkspace","",Direction::Output), "The output workspace.");
}

void ExtractFFTSpectrum::exec()
{
  MatrixWorkspace_sptr inputWS = getProperty("InputWorkspace");
  MatrixWorkspace_sptr inputImagWS = getProperty("InputImagWorkspace");
  const int fftPart = getProperty("FFTPart");
  const int numHists = static_cast<int>(inputWS->getNumberHistograms());
  MatrixWorkspace_sptr outputWS = WorkspaceFactory::Instance().create(inputWS);

  Progress prog(this, 0.0, 1.0, numHists);

  PARALLEL_FOR1(outputWS)
  for ( int i = 0; i < numHists; i++ )
  {
    PARALLEL_START_INTERUPT_REGION

    IAlgorithm_sptr childFFT = createSubAlgorithm("FFT");
    childFFT->setProperty<MatrixWorkspace_sptr>("InputWorkspace", inputWS);
    childFFT->setProperty<int>("Real", i);
    if( inputImagWS )
    {
      childFFT->setProperty<MatrixWorkspace_sptr>("InputImagWorkspace", inputImagWS);
      childFFT->setProperty<int>("Imaginary", i);
    }
    childFFT->execute();
    MatrixWorkspace_const_sptr fftTemp = childFFT->getProperty("OutputWorkspace");

    outputWS->dataE(i) = fftTemp->readE(fftPart);
    outputWS->dataY(i) = fftTemp->readY(fftPart);
    outputWS->dataX(i) = fftTemp->readX(fftPart);

    prog.report();

    PARALLEL_END_INTERUPT_REGION
  }
  PARALLEL_CHECK_INTERUPT_REGION

  boost::shared_ptr<Kernel::Units::Label> lblUnit = boost::dynamic_pointer_cast<Kernel::Units::Label>(UnitFactory::Instance().create("Label"));
  lblUnit->setLabel("Time", "ns");
  outputWS->getAxis(0)->unit() = lblUnit;

  setProperty("OutputWorkspace", outputWS);
}
} // namespace Algorithms
} // namespace Mantid
