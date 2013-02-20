/*WIKI*

This algorithm iterates the [[FFT]] algorithm on each spectrum of InputWorkspace, computing the Fourier Transform and storing the transformed spectrum in OutputWorkspace. If InputImagWorkspace is also passed, then the pair spectrum ''i'' of InputWorkspace (real) and spectrum ''i'' of InputImagWorkspace (real) are taken together as spectrum ''i'' of a complex workspace, on which [[FFT]] is applied.

The FFTPart parameter specifies which transform is selected from the output of the [[FFT]] algorithm:

For the case of input containing real and imaginary workspaces:
{| border="1" cellpadding="5" cellspacing="0"
!FFTPart
!Description
|-
|0
|Complete real part
|-
|1
|Complete imaginary part
|-
|2
|Complete transform modulus
|}

For the case of input containing no imaginary workspace:
{| border="1" cellpadding="5" cellspacing="0"
!FFTPart
!Description
|-
|0
|Real part, positive frequencies
|-
|1
|Imaginary part, positive frequencies
|-
|2
|Modulus, positive frequencies
|-
|3
|Complete real part
|-
|4
|Complete imaginary part
|-
|5
|Complete transform modulus
|}

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
  declareProperty("FFTPart", 2, boost::make_shared<BoundedValidator<int>>(0,5),"Spectrum index, one of the six possible spectra output by the FFT algorithm");
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

    IAlgorithm_sptr childFFT = createChildAlgorithm("FFT");
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
