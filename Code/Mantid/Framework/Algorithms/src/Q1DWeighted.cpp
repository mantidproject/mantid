//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/Q1DWeighted.h"
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
DECLARE_ALGORITHM(Q1DWeighted)

/// Sets documentation strings for this algorithm
void Q1DWeighted::initDocs()
{
  this->setWikiSummary("Performs azimuthal averaging on a 2D SANS data to produce I(Q). ");
  this->setOptionalMessage("Performs azimuthal averaging on a 2D SANS data to produce I(Q).");
}


using namespace Kernel;
using namespace API;
using namespace Geometry;
using namespace DataObjects;

void Q1DWeighted::init()
{
  CompositeValidator<> *wsValidator = new CompositeValidator<>;
  wsValidator->add(new WorkspaceUnitValidator<>("Wavelength"));
  wsValidator->add(new HistogramValidator<>);
  wsValidator->add(new InstrumentValidator<>);
  declareProperty(new WorkspaceProperty<>("InputWorkspace","",Direction::Input,wsValidator));
  declareProperty(new WorkspaceProperty<>("OutputWorkspace","",Direction::Output));
  declareProperty(new ArrayProperty<double>("OutputBinning", new RebinParamsValidator));

  BoundedValidator<int> *positiveInt = new BoundedValidator<int>();
  positiveInt->setLower(0);
  declareProperty("NPixelDivision", 1, positiveInt,
      "Number of sub-pixels used for each detector pixel in each direction."
      "The total number of sub-pixels will be NPixelDivision*NPixelDivision.");

  BoundedValidator<double> *positiveDouble = new BoundedValidator<double>();
  positiveDouble->setLower(0);
  declareProperty("PixelSizeX", 0.00515, positiveDouble,
      "Pixel size in the X direction (m).");
  declareProperty("PixelSizeY", 0.00515, positiveDouble->clone(),
      "Pixel size in the Y direction (m).");
  declareProperty("ErrorWeighting", false,
      "Choose whether each pixel contribution will be weighted by 1/error^2.");
}

void Q1DWeighted::exec()
{
  MatrixWorkspace_const_sptr inputWS = getProperty("InputWorkspace");

  // Calculate the output binning
  const std::vector<double> binParams = getProperty("OutputBinning");
  // XOut defines the output histogram, so its length is equal to the number of bins + 1
  MantidVecPtr XOut;
  const int sizeOut = VectorHelper::createAxisFromRebinParams(binParams,XOut.access());

  // Get pixel size and pixel sub-division
  double pixelSizeX = getProperty("PixelSizeX");
  double pixelSizeY = getProperty("PixelSizeY");
  int nSubPixels = getProperty("NPixelDivision");

  // Get weighting option
  const bool errorWeighting = getProperty("ErrorWeighting");

  // Now create the output workspace
  MatrixWorkspace_sptr outputWS = WorkspaceFactory::Instance().create(inputWS,1,sizeOut,sizeOut-1);
  outputWS->getAxis(0)->unit() = UnitFactory::Instance().create("MomentumTransfer");
  outputWS->setYUnitLabel("1/cm");
  setProperty("OutputWorkspace",outputWS);

  // Set the X vector for the output workspace
  outputWS->setX(0,XOut);
  MantidVec& YOut = outputWS->dataY(0);
  MantidVec& EOut = outputWS->dataE(0);

  const int numSpec = inputWS->getNumberHistograms();

  // Set up the progress reporting object
  Progress progress(this,0.0,1.0,numSpec);

  const V3D sourcePos = inputWS->getInstrument()->getSource()->getPos();
  const V3D samplePos = inputWS->getInstrument()->getSample()->getPos();

  const int xLength = inputWS->readX(0).size();
  const double fmp=4.0*M_PI;

  // Count histogram for normalization
  std::vector<double> XNorm(sizeOut-1, 0.0) ;

  // Beam line axis, to compute scattering angle
  V3D beamLine = samplePos - sourcePos;

  PARALLEL_FOR2(inputWS,outputWS)
  for (int i = 0; i < numSpec; i++)
  {
    PARALLEL_START_INTERUPT_REGION
    // Get the pixel relating to this spectrum
    IDetector_const_sptr det;
    try {
      det = inputWS->getDetector(i);
    } catch (Exception::NotFoundError&) {
      g_log.warning() << "Spectrum index " << i << " has no detector assigned to it - discarding" << std::endl;
      continue;
    }
    // If this detector is masked, skip to the next one
    if ( det->isMasked() ) continue;
    // If this detector is a monitor, skip to the next one
    if ( det->isMonitor() ) continue;

    // Get the current spectrum for both input workspaces
    const MantidVec& XIn = inputWS->readX(i);
    const MantidVec& YIn = inputWS->readY(i);
    const MantidVec& EIn = inputWS->readE(i);


    // Calculate the Q values for the current spectrum
    // Each pixel is sub-divided in the number of pixels given as input parameter (NPixelDivision)
    for ( int isub=0; isub<nSubPixels*nSubPixels; isub++ )
    {

      // Find the position offset for this sub-pixel in real space
      double sub_y = pixelSizeY * ((isub%nSubPixels) - (nSubPixels-1.0)/2.0) / nSubPixels;
      double sub_x = pixelSizeX * (floor((double)isub/nSubPixels) - (nSubPixels-1.0)/2.0) / nSubPixels;

      // Find the position of this sub-pixel in real space and compute Q
      // For reference - in the case where we don't use sub-pixels, simply use:
      //     double sinTheta = sin( inputWS->detectorTwoTheta(det)/2.0 );
      V3D pos = det->getPos() - V3D(sub_x, sub_y, 0.0);
      double sinTheta = sin( pos.angle(beamLine)/2.0 );
      double factor = fmp*sinTheta;

      // Loop over all xLength-1 detector channels
      // Note: xLength -1, because X is a histogram and has a number of boundaries
      // equal to the number of detector channels + 1.
      for ( int j = 0; j < xLength-1; j++)
      {
        double q = factor*2.0/(XIn[j]+XIn[j+1]);
        int iq = 0;

        // Bin assignment depends on whether we have log or linear bins
        if(binParams[1]>0.0)
        {
          iq = (int)floor( (q-binParams[0])/ binParams[1] );
        } else {
          iq = (int)floor(log(q/binParams[0])/log(1.0-binParams[1]));
        }

        if (iq>=0 && iq < sizeOut-1)
        {
          double w = 1.0;
          if ( errorWeighting )
          {
            // When using the error as weight we have:
            //    w_i = 1/s_i^2   where s_i is the uncertainty on the ith pixel.
            //
            //    I(q_i) = (sum over i of I_i * w_i) / (sum over i of w_i)
            //       where all pixels i contribute to the q_i bin, and I_i is the intensity in the ith pixel.
            //
            //    delta I(q_i) = 1/sqrt( (sum over i of w_i) )  using simple error propagation.
            double err = 1.0;
            if (EIn[j] > 0) err = EIn[j];
            w = 1.0/(nSubPixels*nSubPixels*err*err);
          }

          PARALLEL_CRITICAL(iq)    /* Write to shared memory - must protect */
          {
            YOut[iq] += YIn[j]*w;
            EOut[iq] += w*w*EIn[j]*EIn[j];
            XNorm[iq] += w;
          }
        }
      }
      progress.report("Computing I(Q)");
    }
    PARALLEL_END_INTERUPT_REGION
  }
  PARALLEL_CHECK_INTERUPT_REGION

  // Normalize according to the chosen weighting scheme
  for ( int i = 0; i<sizeOut-1; i++ )
  {
    YOut[i] /= XNorm[i];
    EOut[i] = sqrt(EOut[i])/XNorm[i];
  }

}

} // namespace Algorithms
} // namespace Mantid
