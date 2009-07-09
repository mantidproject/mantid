//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/FFT.h"
#include "MantidDataObjects/Workspace2D.h"

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
      declareProperty(new WorkspaceProperty<DataObjects::Workspace2D>("InputWorkspace",
        "",Direction::Input), "The name of the input workspace.");
      declareProperty(new WorkspaceProperty<DataObjects::Workspace2D>("OutputWorkspace",
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
    DataObjects::Workspace2D_sptr inWS = getProperty("InputWorkspace");

    int iReal = getProperty("Real");
    int iImag = getProperty("Imaginary");
    bool isComplex = iImag != EMPTY_INT();

    const std::vector<double>& X = inWS->readX(iReal);
    int ySize = inWS->blocksize();
    int xSize = X.size();

    if (iReal >= ySize) throw std::invalid_argument("Property Real is out of range");
    if (isComplex && iImag >= ySize) throw std::invalid_argument("Property Imaginary is out of range");

    //Check that the x values are evenly spaced
    double dx = (X.back() - X.front()) / (X.size() - 1);
    for(size_t i=0;i<X.size()-2;i++)
        if (abs(dx - X[i+1] + X[i])/dx > 1e-7) throw std::invalid_argument("X axis must be linear (all bins have same width)");

    gsl_fft_complex_wavetable * wavetable = gsl_fft_complex_wavetable_alloc(ySize);
    gsl_fft_complex_workspace * workspace = gsl_fft_complex_workspace_alloc(ySize);

    boost::shared_array<double> data(new double[2*ySize]);
    std::string transform = getProperty("Transform");

    DataObjects::Workspace2D_sptr outWS = boost::dynamic_pointer_cast<DataObjects::Workspace2D>
        (WorkspaceFactory::Instance().create("Workspace2D",2,xSize,ySize));

    double df = 1.0 / (dx * (xSize - 1));

    if (transform == "Forward")
    {
        for(int i=0;i<ySize;i++)
        {
            data[2*i] = inWS->dataY(iReal)[i];
            data[2*i+1] = isComplex? inWS->dataY(iImag)[i] : 0.;
        }

        gsl_fft_complex_forward (data.get(), 1, ySize, wavetable, workspace);
        for(int i=0;i<ySize;i++)
        {
            int j = (ySize/2 + i) % ySize;
            outWS->dataX(0)[i] = df*(-ySize/2 + i);
            outWS->dataY(0)[i] = data[2*j]; // real part
            outWS->dataY(1)[i] = data[2*j+1]; // imaginary part
        }
        if (xSize == ySize + 1) outWS->dataX(0)[ySize] = outWS->dataX(0)[ySize - 1] + df;
    }
    else
    {
        for(int i=0;i<ySize;i++)
        {
            int j = (ySize/2 + i) % ySize;
            data[2*i] = inWS->dataY(iReal)[j];
            data[2*i+1] = isComplex? inWS->dataY(iImag)[j] : 0.;
        }
        gsl_fft_complex_backward(data.get(), 1, ySize, wavetable, workspace);
        for(int i=0;i<ySize;i++)
        {
            outWS->dataX(0)[i] = df*i;
            outWS->dataY(0)[i] = data[2*i]; // real part
            outWS->dataY(1)[i] = data[2*i+1]; // imaginary part
        }
    }

    gsl_fft_complex_wavetable_free (wavetable);
    gsl_fft_complex_workspace_free (workspace);
        
        

    outWS->dataX(1) = outWS->dataX(0);

    setProperty("OutputWorkspace",outWS);

}

} // namespace Algorithm
} // namespace Mantid
