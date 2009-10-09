//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/FFT.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidKernel/Exception.h"

#include <boost/shared_array.hpp>
#include <gsl/gsl_errno.h>
#include <gsl/gsl_fft_complex.h>

#define REAL(z,i) ((z)[2*(i)])
#define IMAG(z,i) ((z)[2*(i)+1])

#include <sstream>
#include <numeric>
#include <algorithm>
#include <functional>
#include <math.h>

#include <iostream>

namespace Mantid
{
namespace Algorithms
{

// Register the class into the algorithm factory
DECLARE_ALGORITHM(FFT)

using namespace Kernel;
using namespace API;


/// Initialisation method. Declares properties to be used in algorithm.
void FFT::init()
{
      declareProperty(new WorkspaceProperty<API::MatrixWorkspace>("InputWorkspace",
        "",Direction::Input), "The name of the input workspace.");
      declareProperty(new WorkspaceProperty<API::MatrixWorkspace>("OutputWorkspace",
        "",Direction::Output), "The name of the output workspace.");

      BoundedValidator<int> *mustBePositive = new BoundedValidator<int>();
      mustBePositive->setLower(0);
      declareProperty("Real",0,mustBePositive,"Spectrum number to use as real part for transform");
      declareProperty("Imaginary",EMPTY_INT(),mustBePositive->clone(),"Spectrum number to use as imaginary part for transform");

      std::vector<std::string> fft_dir;
      fft_dir.push_back("Forward");
      fft_dir.push_back("Backward");
      declareProperty("Transform","Forward",new ListValidator(fft_dir),"Direction of the transform: forward or backward");

}

/** Executes the algorithm
 *
 *  @throw runtime_error Thrown if
 */
void FFT::exec()
{
    API::MatrixWorkspace_sptr inWS = getProperty("InputWorkspace");

    int iReal = getProperty("Real");
    int iImag = getProperty("Imaginary");
    bool isComplex = iImag != EMPTY_INT();

    const MantidVec& X = inWS->readX(iReal);
    int ySize = inWS->blocksize();
    int xSize = X.size();

    if (iReal >= ySize) throw std::invalid_argument("Property Real is out of range");
    if (isComplex && iImag >= ySize) throw std::invalid_argument("Property Imaginary is out of range");

    //Check that the x values are evenly spaced
    double dx = (X.back() - X.front()) / (X.size() - 1);
    for(size_t i=0;i<X.size()-2;i++)
        if (std::abs(dx - X[i+1] + X[i])/dx > 1e-7) throw std::invalid_argument("X axis must be linear (all bins have same width)");

    gsl_fft_complex_wavetable * wavetable = gsl_fft_complex_wavetable_alloc(ySize);
    gsl_fft_complex_workspace * workspace = gsl_fft_complex_workspace_alloc(ySize);

    boost::shared_array<double> data(new double[2*ySize]);
    std::string transform = getProperty("Transform");

    // The number of spectra in the output workspace
    int nOut = 3;
    bool addPositiveOnly = false;
    // If the input is real add 3 more spectra with positive "frequencies" only
    if (!isComplex && transform == "Forward") 
    {
        nOut += 3;
        addPositiveOnly = true;
    }

    API::MatrixWorkspace_sptr outWS = WorkspaceFactory::Instance().create("Workspace2D",nOut,xSize,ySize);

    bool isEnergyMeV = false;
    if (inWS->getAxis(0)->unit() && 
        (inWS->getAxis(0)->unit()->caption() == "Energy" ||
         inWS->getAxis(0)->unit()->caption() == "Energy transfer")&&
        inWS->getAxis(0)->unit()->label() == "meV")
    {
        outWS->getAxis(0)->unit() = boost::shared_ptr<Kernel::Unit>(new LabelUnit("Time","ns"));
        isEnergyMeV = true;
    }
    else
        outWS->getAxis(0)->unit() = boost::shared_ptr<Kernel::Unit>(new LabelUnit());

    double df = 1.0 / (dx * ySize);
    if (isEnergyMeV) df /= 2.418e2;

    // shift == true means that the zero on the x axis is assumed to be in the data centre 
    // at point with index i = ySize/2. If shift == false the zero is at i = 0
    bool shift = true;

    int iRe = 0;
    int iIm = 1;
    int iAbs = 2;
    if (addPositiveOnly)
    {
        iRe = 3;
        iIm = 4;
        iAbs = 5;
    }

    int dys = ySize % 2;
    if (transform == "Forward")
    {
        for(int i=0;i<ySize;i++)
        {
            int j = shift? (ySize/2 + i) % ySize : i; 
            data[2*i] = inWS->dataY(iReal)[j];
            data[2*i+1] = isComplex? inWS->dataY(iImag)[j] : 0.;
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
            data[2*i+1] = isComplex? inWS->dataY(iImag)[j] : 0.;
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
