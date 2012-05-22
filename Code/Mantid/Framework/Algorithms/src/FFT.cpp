/*WIKI* 


The FFT algorithm performs discrete Fourier transform of complex data using the Fast Fourier Transform algorithm. It uses the GSL Fourier transform functions to do the calculations. Due to the nature of the fast fourier transform the input spectra must have linear x axes. If the imaginary part is not set the data is considered real. The "Transform" property defines the direction of the transform: direct ("Forward") or inverse ("Backward").

Note that the input data is shifted before the transform along the x axis to place the origin in the middle of the x-value range. It means that for the data defined on an interval [A,B] the output <math>F(\xi_k)</math> must be multiplied by <math> e^{-2\pi ix_0\xi_k} </math>, where <math>x_0=\tfrac{1}{2}(A+B)</math>, <math>\xi_k</math> is the frequency.

==Details==

The Fourier transform of a complex function <math>f(x)</math> is defined by equation:

:<math>F(\xi)=\int_{-\infty}^\infty f(x)e^{-2\pi ix\xi} dx</math>

For discrete data with equally spaced <math>x_n</math> and only non-zero on an interval <math>[A,B]</math> the integral can be approximated by a sum:

:<math>F(\xi)=\Delta x\sum_{n=0}^{N-1}f(x_n)e^{-2\pi ix_n\xi}</math>

Here <math>\Delta x</math> is the bin width, <math>x_n=A+\Delta xn</math>. If we are looking for values of the transformed function <math>F</math> at discrete frequencies <math>\xi_k=\Delta\xi k</math> with 

:<math>\Delta\xi=\frac{1}{B-A}=\frac{1}{L}=\frac{1}{N\Delta x}</math>

the equation can be rewritten:

:<math>F_k=e^{-2\pi iA\xi_k}\Delta x\sum_{n=0}^{N-1}f_ne^{-\tfrac{2\pi i}{N}nk}</math>

Here <math>f_n=f(x_n)</math> and <math>F_k=F(\xi_k)</math>. The fromula 

:<math>\tilde{F}_k=\Delta x\sum_{n=0}^{N-1}f_ne^{-\tfrac{2\pi i}{N}nk}</math>

is the Discrete Fourier Transform (DFT) and can be efficiently evaluated using the Fast Fourier Transform algorithm. The DFT formula calculates the Fourier transform of data on the interval <math>[0,L]</math>. It should be noted that it is periodic in <math>k</math> with period <math>N</math>. If we also assume that <math>f_n</math> is aslo periodic with period <math>N</math> the DFT can be used to transform data on the interval <math>[-L/2,L/2]</math>. To do this we swap the two halves of the data array <math>f_n</math>. If we denote the modified array as <math>\bar{f}_n</math>, its transform will be

:<math>\bar{F}_k=\Delta x\sum_{n=0}^{N-1}\bar{f}_ne^{-\tfrac{2\pi i}{N}nk}</math>

The Mantid FFT algorithm returns the complex array <math>\bar{F}_K</math> as Y values of two spectra in the output workspace, one for the real and the other for the imaginary part of the transform. The X values are set to the transform frequencies and have the range approximately equal to <math>[-N/L,N/L]</math>. The actual limits depend sllightly on whether <math>N</math> is even or odd and whether the input spectra are histograms or point data. The variations are of the order of <math>\Delta\xi</math>. The zero frequency is always in the bin with index <math>k=N/2</math>.

===Example 1===

In this example the input data were calculated using function <math>\exp(-(x-1)^2)</math>.

[[Image:FFTGaussian1.png|Gaussian]]

[[Image:FFTGaussian1FFT.png|FFT of a Gaussian]]

Because the <math>x=0</math> is in the middle of the data array the transform shown is the exact DFT of the input data.

===Example 2===

In this example the input data were calculated using function <math>\exp(-x^2)</math>.

[[Image:FFTGaussian2.png|Gaussian]]

[[Image:FFTGaussian1FFT.png|FFT of a Gaussian]]

Because the <math>x=0</math> is not in the middle of the data array the transform shown includes a shifting factor of <math>\exp(2\pi i\xi)</math>. To remove it the output must be mulitplied by <math>\exp(-2\pi i\xi)</math>. The corrected transform will be:

[[Image:FFTGaussian2FFT.png|FFT of a Gaussian]]

It should be noted that in a case like this, i.e. when the input is a real positive even function, the correction can be done by finding the transform's modulus <math>(Re^2+Im^2)^{1/2}</math>. The output workspace includes the modulus of the transform.

==Output==

The output workspace for a direct ("Forward") transform contains six spectra. The actual transfor is written to spectra with indeces 3 and 4 which are the real and imaginary parts correspondingly. The last spectrum (index 5) has the modulus of the transform: <math> \sqrt(Re^2+Im^2) </math>. The spectra from 0 to 2 repeat these results for positive frequencies only. This corresponds to Fourier transform of real data and is only meaningful if the input data are real. 

{| border="1" cellpadding="5" cellspacing="0"
!Workspace index
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

The output workspace for an inverse ("Backward") transform has 3 spectra for the real (0), imaginary (1) parts, and the modulus (2).

{| border="1" cellpadding="5" cellspacing="0"
!Workspace index
!Description
|-
|0
|Real part
|-
|1
|Imaginary part
|-
|2
|Modulus
|}


*WIKI*/
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/FFT.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidAPI/TextAxis.h"

#include <boost/shared_array.hpp>
#include <gsl/gsl_errno.h>
#include <gsl/gsl_fft_complex.h>

#define REAL(z,i) ((z)[2*(i)])
#define IMAG(z,i) ((z)[2*(i)+1])

#include <sstream>
#include <numeric>
#include <algorithm>
#include <functional>
#include <cmath>
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/ListValidator.h"

namespace Mantid
{
namespace Algorithms
{

// Register the class into the algorithm factory
DECLARE_ALGORITHM(FFT)

/// Sets documentation strings for this algorithm
void FFT::initDocs()
{
  this->setWikiSummary("Performs complex Fast Fourier Transform ");
  this->setOptionalMessage("Performs complex Fast Fourier Transform");
}


using namespace Kernel;
using namespace API;

/// Initialisation method. Declares properties to be used in algorithm.
void FFT::init()
{
  declareProperty(new WorkspaceProperty<>("InputWorkspace","",Direction::Input),"The name of the input workspace.");
  declareProperty(new WorkspaceProperty<>("OutputWorkspace","",Direction::Output), "The name of the output workspace.");
  //if desired, provide the imaginary part in a separate workspace.
  declareProperty(new WorkspaceProperty<>("InputImagWorkspace","",Direction::Input,PropertyMode::Optional), "The name of the input workspace for the imaginary part. Leave blank if same as InputWorkspace");

  auto mustBePositive = boost::make_shared<BoundedValidator<int> >();
  mustBePositive->setLower(0);
  declareProperty("Real",0,mustBePositive,"Spectrum number to use as real part for transform");
  declareProperty("Imaginary",EMPTY_INT(),mustBePositive,"Spectrum number to use as imaginary part for transform");

  std::vector<std::string> fft_dir;
  fft_dir.push_back("Forward");
  fft_dir.push_back("Backward");
  declareProperty("Transform","Forward",boost::make_shared<StringListValidator>(fft_dir),"Direction of the transform: forward or backward");
}

/** Executes the algorithm
 *
 *  @throw std::invalid_argument if the input properties are invalid 
                                 or the bins of the spectrum being transformed do not have constant width
 */
void FFT::exec()
{
  MatrixWorkspace_const_sptr inWS = getProperty("InputWorkspace");
  MatrixWorkspace_const_sptr inImagWS = getProperty("InputImagWorkspace");
  if( !inImagWS ) inImagWS = inWS; //workspaces are one and the same

  const int iReal = getProperty("Real");
  const int iImag = getProperty("Imaginary");
  const bool isComplex = iImag != EMPTY_INT();

  const MantidVec& X = inWS->readX(iReal);
  const int ySize = static_cast<int>(inWS->blocksize());
  const int xSize = static_cast<int>(X.size());

  if (iReal >= ySize) throw std::invalid_argument("Property Real is out of range");
  if (isComplex && iImag >= ySize) throw std::invalid_argument("Property Imaginary is out of range");

  //Check that the x values are evenly spaced
  const double dx = (X.back() - X.front()) / (static_cast<int>(X.size()) - 1);
  for(size_t i=0;i<X.size()-2;i++)
    if (std::abs(dx - X[i+1] + X[i])/dx > 1e-7) throw std::invalid_argument("X axis must be linear (all bins have same width)");

  gsl_fft_complex_wavetable * wavetable = gsl_fft_complex_wavetable_alloc(ySize);
  gsl_fft_complex_workspace * workspace = gsl_fft_complex_workspace_alloc(ySize);

  boost::shared_array<double> data(new double[2*ySize]);
  const std::string transform = getProperty("Transform");

  // The number of spectra in the output workspace
  int nOut = 3;
  bool addPositiveOnly = false;
  // If the input is real add 3 more spectra with positive "frequencies" only
  if (!isComplex && transform == "Forward") 
  {
    nOut += 3;
    addPositiveOnly = true;
  }

  MatrixWorkspace_sptr outWS = WorkspaceFactory::Instance().create(inWS,nOut,xSize,ySize);

  bool isEnergyMeV = false;
  if (inWS->getAxis(0)->unit() && 
    (inWS->getAxis(0)->unit()->caption() == "Energy" ||
    inWS->getAxis(0)->unit()->caption() == "Energy transfer")&&
    inWS->getAxis(0)->unit()->label() == "meV")
  {
    boost::shared_ptr<Kernel::Units::Label> lblUnit = 
      boost::dynamic_pointer_cast<Kernel::Units::Label>(UnitFactory::Instance().create("Label"));
    if (lblUnit)
    {
      lblUnit->setLabel("Time","ns");
      outWS->getAxis(0)->unit() = lblUnit;
    }
    isEnergyMeV = true;
  }
  else
    outWS->getAxis(0)->unit() = UnitFactory::Instance().create("Label");

  double df = 1.0 / (dx * ySize);
  if (isEnergyMeV) df /= 2.418e2;

  // shift == true means that the zero on the x axis is assumed to be in the data centre 
  // at point with index i = ySize/2. If shift == false the zero is at i = 0
  bool shift = true;

  API::TextAxis* tAxis = new API::TextAxis(nOut);
  int iRe = 0;
  int iIm = 1;
  int iAbs = 2;
  if (addPositiveOnly)
  {
    iRe = 3;
    iIm = 4;
    iAbs = 5;
    tAxis->setLabel(0,"Real Positive");
    tAxis->setLabel(1,"Imag Positive");
    tAxis->setLabel(2,"Modulus Positive");
  }
  tAxis->setLabel(iRe,"Real");
  tAxis->setLabel(iIm,"Imag");
  tAxis->setLabel(iAbs,"Modulus");
  outWS->replaceAxis(1,tAxis);

  const int dys = ySize % 2;
  if (transform == "Forward")
  {
    for(int i=0;i<ySize;i++)
    {
      int j = shift? (ySize/2 + i) % ySize : i; 
      data[2*i] = inWS->dataY(iReal)[j];
      data[2*i+1] = isComplex? inImagWS->dataY(iImag)[j] : 0.;
    }

    gsl_fft_complex_forward (data.get(), 1, ySize, wavetable, workspace);
    for(int i=0;i<ySize;i++)
    {
      int j = (ySize/2 + i + dys) % ySize;
      outWS->dataX(iRe)[i] = df*(-ySize/2 + i);
      double re = data[2*j]*dx;
      double im = data[2*j+1]*dx;
      outWS->dataY(iRe)[i] = re; // real part
      outWS->dataY(iIm)[i] = im; // imaginary part
      outWS->dataY(iAbs)[i] = sqrt(re*re + im*im); // modulus
      if (addPositiveOnly)
      {
        outWS->dataX(0)[i] = df*i;
        if (j < ySize/2)
        {
          outWS->dataY(0)[j] = re; // real part
          outWS->dataY(1)[j] = im; // imaginary part
          outWS->dataY(2)[j] = sqrt(re*re + im*im); // modulus
        }
        else
        {
          outWS->dataY(0)[j] = 0.; // real part
          outWS->dataY(1)[j] = 0.; // imaginary part
          outWS->dataY(2)[j] = 0.; // modulus
        }
      }
    }
    if (xSize == ySize + 1) 
    {
      outWS->dataX(0)[ySize] = outWS->dataX(0)[ySize - 1] + df;
      if (addPositiveOnly)
        outWS->dataX(iRe)[ySize] = outWS->dataX(iRe)[ySize - 1] + df;
    }
  }
  else // Backward
  {
    for(int i=0;i<ySize;i++)
    {
      int j = (ySize/2 + i) % ySize;
      data[2*i] = inWS->dataY(iReal)[j];
      data[2*i+1] = isComplex? inImagWS->dataY(iImag)[j] : 0.;
    }
    gsl_fft_complex_inverse(data.get(), 1, ySize, wavetable, workspace);
    for(int i=0;i<ySize;i++)
    {
      double x = df*i;
      if (shift) x -= df*(ySize/2);
      outWS->dataX(0)[i] = x;
      int j = shift? (ySize/2 + i + dys) % ySize : i; 
      double re = data[2*j]/df;
      double im = data[2*j+1]/df;
      outWS->dataY(0)[i] = re;                  // real part
      outWS->dataY(1)[i] = im;                  // imaginary part
      outWS->dataY(2)[i] = sqrt(re*re + im*im); // modulus
    }
    if (xSize == ySize + 1) outWS->dataX(0)[ySize] = outWS->dataX(0)[ySize - 1] + df;
  }

  gsl_fft_complex_wavetable_free (wavetable);
  gsl_fft_complex_workspace_free (workspace);

  outWS->dataX(1) = outWS->dataX(0);
  outWS->dataX(2) = outWS->dataX(0);

  if (addPositiveOnly)
  {
    outWS->dataX(iIm) = outWS->dataX(iRe);
    outWS->dataX(iAbs) = outWS->dataX(iRe);
  }

  setProperty("OutputWorkspace",outWS);

}

} // namespace Algorithm
} // namespace Mantid
