//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/RealFFT.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidKernel/Exception.h"
#include "MantidAPI/TextAxis.h"

#include <boost/shared_array.hpp>
#include <gsl/gsl_errno.h>
#include <gsl/gsl_fft_real.h>
#include <gsl/gsl_fft_halfcomplex.h>

#define REAL(z,i) ((z)[2*(i)])
#define IMAG(z,i) ((z)[2*(i)+1])

#include <sstream>
#include <numeric>
#include <algorithm>
#include <functional>
#include <cmath>

#include <iostream>

namespace Mantid
{
namespace Algorithms
{

// Register the class into the algorithm factory
DECLARE_ALGORITHM(RealFFT)

/// Sets documentation strings for this algorithm
void RealFFT::initDocs()
{
  this->setWikiSummary("Performs real Fast Fourier Transform ");
  this->setOptionalMessage("Performs real Fast Fourier Transform");
}


using namespace Kernel;
using namespace API;

/// Initialisation method. Declares properties to be used in algorithm.
void RealFFT::init()
{
      declareProperty(new WorkspaceProperty<API::MatrixWorkspace>("InputWorkspace",
        "",Direction::Input), "The name of the input workspace.");
      declareProperty(new WorkspaceProperty<API::MatrixWorkspace>("OutputWorkspace",
        "",Direction::Output), "The name of the output workspace.");

      BoundedValidator<int> *mustBePositive = new BoundedValidator<int>();
      mustBePositive->setLower(0);
      declareProperty("WorkspaceIndex",0,mustBePositive,"Spectrum to transform");

      std::vector<std::string> fft_dir;
      fft_dir.push_back("Forward");
      fft_dir.push_back("Backward");
      declareProperty("Transform","Forward",new ListValidator(fft_dir),"Direction of the transform: forward or backward");

      declareProperty("IgnoreXBins",false,
          "Ignores the requirement that X bins be linear and of the same size.\n"
          "Set this to true if you are using log binning.\n"
          "FFT result will not be valid for the X axis, and should be ignored.");
}

/** Executes the algorithm
 *
 *  @throw runtime_error Thrown if
 */
void RealFFT::exec()
{
    API::MatrixWorkspace_sptr inWS = getProperty("InputWorkspace");
    std::string transform = getProperty("Transform");
    IgnoreXBins = getProperty("IgnoreXBins");

    int spec = (transform == "Forward") ? getProperty("WorkspaceIndex") : 0;

    const MantidVec& X = inWS->readX(spec);
    int ySize = static_cast<int>(inWS->blocksize());

    if (spec >= ySize) throw std::invalid_argument("Property WorkspaceIndex is out of range");

    //Check that the x values are evenly spaced
    double dx = (X.back() - X.front()) / static_cast<double>(X.size() - 1);
    if (!IgnoreXBins)
    {
      for(size_t i=0;i<X.size()-2;i++)
          if (std::abs(dx - X[i+1] + X[i])/dx > 1e-7)
            throw std::invalid_argument("X axis must be linear (all bins have same width). This can be ignored if IgnoreXBins is set to true.");
    }

    API::MatrixWorkspace_sptr outWS;

    double df = 1.0 / (dx * ySize);

    if (transform == "Forward")
    {
        int yOutSize = ySize / 2 + 1;
        int xOutSize = inWS->isHistogramData() ? yOutSize + 1 : yOutSize;
        bool odd = ySize % 2 != 0;

        outWS = WorkspaceFactory::Instance().create(inWS,3,xOutSize,yOutSize);
        API::TextAxis* tAxis = new API::TextAxis(3);
        tAxis->setLabel(0,"Real");
        tAxis->setLabel(1,"Imag");
        tAxis->setLabel(2,"Modulus");
        outWS->replaceAxis(1,tAxis);

        gsl_fft_real_workspace * workspace = gsl_fft_real_workspace_alloc(ySize);
        boost::shared_array<double> data(new double[2*ySize]);

        for(int i=0;i<ySize;i++)
        {
            data[i] = inWS->dataY(spec)[i];
        }

        gsl_fft_real_wavetable * wavetable = gsl_fft_real_wavetable_alloc(ySize);
        gsl_fft_real_transform (data.get(), 1, ySize, wavetable, workspace);
        gsl_fft_real_wavetable_free (wavetable);
        gsl_fft_real_workspace_free (workspace);

        for(int i=0;i<yOutSize;i++)
        {
            int j = i * 2;
            outWS->dataX(0)[i] = df * i;
            double re = i!=0 ? data[j-1] : data[0];
            double im = (i!=0 && (odd || i!=yOutSize-1)) ? data[j] : 0;
            outWS->dataY(0)[i] = re*dx; // real part
            outWS->dataY(1)[i] = im*dx; // imaginary part
            outWS->dataY(2)[i] = dx*sqrt(re*re+im*im); // modulus
        }
        if (inWS->isHistogramData()) 
        {
            outWS->dataX(0)[yOutSize] = outWS->dataX(0)[yOutSize - 1] + df;
        }
        outWS->dataX(1) = outWS->dataX(0);
        outWS->dataX(2) = outWS->dataX(0);
        //outWS->getAxis(1)->spectraNo(0)=inWS->getAxis(1)->spectraNo(spec);
        //outWS->getAxis(1)->spectraNo(1)=inWS->getAxis(1)->spectraNo(spec);
    }
    else // Backward
    {

      if (inWS->getNumberHistograms() < 2)
        throw std::runtime_error("The input workspace must have at least 2 spectra.");

      int yOutSize = (ySize - 1)* 2;
      if (inWS->readY(1).back() != 0.0) yOutSize++;
      int xOutSize = inWS->isHistogramData() ? yOutSize + 1 : yOutSize;
      bool odd = yOutSize % 2 != 0;

      df = 1.0 / (dx * (yOutSize));

      outWS = WorkspaceFactory::Instance().create(inWS,1,xOutSize,yOutSize);
      API::TextAxis* tAxis = new API::TextAxis(1);
      tAxis->setLabel(0,"Real");
      outWS->replaceAxis(1,tAxis);

      gsl_fft_real_workspace * workspace = gsl_fft_real_workspace_alloc(yOutSize);
      boost::shared_array<double> data(new double[yOutSize]);

      for(int i=0;i<ySize;i++)
      {
        int j = i * 2;
        outWS->dataX(0)[i] = df * i;
        if (i!=0)
        {
          data[j-1] = inWS->dataY(0)[i];
          if (odd || i!=ySize-1)
          {
            data[j] = inWS->dataY(1)[i];
          }
        }
        else
        {
          data[0] = inWS->dataY(0)[0];
        }
      }

      gsl_fft_halfcomplex_wavetable * wavetable = gsl_fft_halfcomplex_wavetable_alloc(yOutSize);
      gsl_fft_halfcomplex_inverse(data.get(), 1, yOutSize, wavetable, workspace);
      gsl_fft_halfcomplex_wavetable_free (wavetable);
      gsl_fft_real_workspace_free (workspace);
  
      for(int i=0;i<yOutSize;i++)
      {
        double x = df*i;
        outWS->dataX(0)[i] = x;
        outWS->dataY(0)[i] = data[i]/df; 
      }
      if (outWS->isHistogramData()) outWS->dataX(0)[yOutSize] = outWS->dataX(0)[yOutSize - 1] + df;
      //outWS->getAxis(1)->spectraNo(0)=inWS->getAxis(1)->spectraNo(spec);
    }

    setProperty("OutputWorkspace",outWS);

}

} // namespace Algorithm
} // namespace Mantid
