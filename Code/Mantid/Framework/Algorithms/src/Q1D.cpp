//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/Q1D.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/RebinParamsValidator.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/PhysicalConstants.h"
#include "MantidKernel/VectorHelper.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidAPI/SpectraDetectorMap.h"
#include "MantidDataObjects/Histogram1D.h"

namespace Mantid
{
namespace Algorithms
{

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(Q1D)

/// Sets documentation strings for this algorithm
void Q1D::initDocs()
{
  this->setWikiSummary("Part of the 1D data reduction chain for SANS instruments. ");
  this->setOptionalMessage("Part of the 1D data reduction chain for SANS instruments.");
}


using namespace Kernel;
using namespace API;
using namespace Geometry;

void Q1D::init()
{
  CompositeValidator<> *wsValidator = new CompositeValidator<>;
  wsValidator->add(new WorkspaceUnitValidator<>("Wavelength"));
  wsValidator->add(new HistogramValidator<>);
  wsValidator->add(new InstrumentValidator<>);
  declareProperty(new WorkspaceProperty<>("InputWorkspace","",Direction::Input,wsValidator));
  declareProperty(new WorkspaceProperty<>("InputForErrors","",Direction::Input));
  declareProperty(new WorkspaceProperty<>("OutputWorkspace","",Direction::Output));

  declareProperty(new ArrayProperty<double>("OutputBinning", new RebinParamsValidator));

  declareProperty("AccountForGravity",false);
}

void Q1D::exec()
{
  MatrixWorkspace_const_sptr inputWS = getProperty("InputWorkspace");
  MatrixWorkspace_const_sptr errorsWS = getProperty("InputForErrors");

  // Check that the input workspaces match up
  if ( ! WorkspaceHelpers::matchingBins(inputWS,errorsWS) )
  {
    g_log.error("The input workspaces must have matching bins");
    throw std::runtime_error("The input workspaces must have matching bins");
  }

  // Calculate the output binning
  const std::vector<double> binParams = getProperty("OutputBinning");
  MantidVecPtr XOut;
  const int sizeOut = VectorHelper::createAxisFromRebinParams(binParams,XOut.access());

  // Now create the output workspace
  MatrixWorkspace_sptr outputWS = WorkspaceFactory::Instance().create(inputWS,1,sizeOut,sizeOut-1);
  outputWS->getAxis(0)->unit() = UnitFactory::Instance().create("MomentumTransfer");
  outputWS->setYUnitLabel("1/cm");
  setProperty("OutputWorkspace",outputWS);
  // Set the X vector for the output workspace
  outputWS->setX(0,XOut);
  MantidVec& YOut = outputWS->dataY(0);
  // Don't care about errors here - create locals
  MantidVec EIn(inputWS->readE(0).size());
  MantidVec EOutDummy(sizeOut-1);
  MantidVec emptyVec(1);
  // Create temporary vectors for the summation & rebinning of the 'errors' workspace
  MantidVec errY(sizeOut-1),errE(sizeOut-1);
  // And one for the solid angles
  MantidVec anglesSum(sizeOut-1);

  const int numSpec = static_cast<int>(inputWS->getNumberHistograms());

  // Get a reference to the spectra-detector map
  SpectraDetectorMap& specMap = outputWS->mutableSpectraMap();
  // Clear the map, this will be faster than remap
  specMap.clear();
  //
  const SpectraDetectorMap& inSpecMap = inputWS->spectraMap();

  const Axis* const spectraAxis = inputWS->getAxis(1);
  int newSpectrumNo = -1;

  // Set the progress bar (1 update for every one percent increase in progress)
  Progress progress(this, 0.0, 1.0, numSpec);

  const V3D samplePos = inputWS->getInstrument()->getSample()->getPos();

  const bool doGravity = getProperty("AccountForGravity");


  const size_t xLength = inputWS->readX(0).size();
  const double fmp=4.0*M_PI;

  PARALLEL_FOR3(inputWS,outputWS,errorsWS)
  for (int i = 0; i < numSpec; ++i)
  {
    PARALLEL_START_INTERUPT_REGION
    // Get the pixel relating to this spectrum
    IDetector_const_sptr det;
    try {
      det = inputWS->getDetector(i);
    } catch (Exception::NotFoundError&) {
      g_log.warning() << "Spectrum index " << i << " has no detector assigned to it - discarding" << std::endl;
      // Catch if no detector. Next line tests whether this happened - test placed
      // outside here because Mac Intel compiler doesn't like 'continue' in a catch
      // in an openmp block.
    }
    // If no detector found or if detector is masked, skip onto the next spectrum
    if ( !det || det->isMasked() ) continue;

    // Map all the detectors onto the spectrum of the output
    if (spectraAxis->isSpectra()) 
    {
      if (newSpectrumNo == -1) 
      {
        PARALLEL_CRITICAL(q1d_a)
        {
          if( newSpectrumNo == -1 )
          { 
            newSpectrumNo = outputWS->getAxis(1)->spectraNo(0) = spectraAxis->spectraNo(i);
          }
         }
      }
      PARALLEL_CRITICAL(q1d_b)
      {/* Write to shared memory - must protect */
        specMap.addSpectrumEntries(newSpectrumNo,inSpecMap.getDetectors(spectraAxis->spectraNo(i)));
      }
    }

    // Get the current spectrum for both input workspaces - not references, have to reverse below
    const MantidVec& XIn = inputWS->readX(i);
    MantidVec YIn = inputWS->readY(i);
    MantidVec errYIn = errorsWS->readY(i);
    MantidVec errEIn = errorsWS->readE(i);
    // A temporary vector to store intermediate Q values
    MantidVec Qx(xLength);

    if (doGravity)
    {
      // Get the vector to get at the 3 components of the pixel position
      GravitySANSHelper grav(inputWS, det);
      for ( int j = 0; j < xLength; ++j)
      {
        // as the fall under gravity is wavelength dependent sin theta is now different for each bin with each detector 
        const double sinTheta = grav.calcSinTheta(XIn[j]);
        // Now we're ready to go to Q
        Qx[xLength-j-1] = fmp*sinTheta/XIn[j];
      }
    }
    else
    {
      // Calculate the Q values for the current spectrum
      const double sinTheta = sin( inputWS->detectorTwoTheta(det)/2.0 );
      const double factor = fmp*sinTheta;
      for ( int j = 0; j < xLength; ++j)
      {
        Qx[xLength-j-1] = factor/XIn[j];
      }
    }

    // Unfortunately, have to reverse data vectors for rebin functions to work
    std::reverse(YIn.begin(),YIn.end());
    std::reverse(errYIn.begin(),errYIn.end());
    std::reverse(errEIn.begin(),errEIn.end());
    // Pass to rebin, flagging as a distribution. Need to protect from writing by more that one thread at once.
    PARALLEL_CRITICAL(q1d_c)
    {
      VectorHelper::rebin(Qx,YIn,EIn,*XOut,YOut,EOutDummy,true,true);
      VectorHelper::rebin(Qx,errYIn,errEIn,*XOut,errY,errE,true,true);
    }

    // Now summing the solid angle across the appropriate range
    const double solidAngle = det->solidAngle(samplePos);
    // At this point we check for masked bins in the input workspace and modify accordingly
    if ( inputWS->hasMaskedBins(i) )
    {
      MantidVec included_bins,solidAngleVec;
      included_bins.push_back(Qx.front());
      // Get a reference to the list of masked bins
      const MatrixWorkspace::MaskList& mask = inputWS->maskedBins(i);
      // Now iterate over the list, adjusting the weights for the affected bins
      // Since we've reversed Qx, we need to reverse the list too
      MatrixWorkspace::MaskList::const_reverse_iterator it;
      for (it = mask.rbegin(); it != mask.rend(); ++it)
      {
        const double currentX = Qx[xLength-2-(*it).first];
        // Add an intermediate bin with full weight if masked bins aren't consecutive
        if (included_bins.back() != currentX) 
        {
          solidAngleVec.push_back(solidAngle);
          included_bins.push_back(currentX);
        }
        // The weight for this masked bin is 1 - the degree to which this bin is masked
        solidAngleVec.push_back( solidAngle * (1.0-(*it).second) );
        included_bins.push_back( Qx[xLength-1-(*it).first]);
      }
      // Add on a final bin with full weight if masking doesn't go up to the end
      if (included_bins.back() != Qx.back()) 
      {
        solidAngleVec.push_back(solidAngle);
        included_bins.push_back(Qx.back());
      }
      
      // Create a zero vector for the errors because we don't care about them here
      const MantidVec zeroes(solidAngleVec.size(),0.0);
      PARALLEL_CRITICAL(q1d_d)
      {
        // Rebin the solid angles - note that this is a distribution
        VectorHelper::rebin(included_bins,solidAngleVec,zeroes,*XOut,anglesSum,EOutDummy,true,true);
      }
    }
    else // No masked bins
    {
      // Two element vector to define the range
      MantidVec xRange(2);
      xRange[0] = Qx.front();
      xRange[1] = Qx.back();
      // Single element vector containing the solid angle
      MantidVec solidAngleVec(1, solidAngle);
      PARALLEL_CRITICAL(q1d_e)
     {
        // Rebin the solid angles - note that this is a distribution
        VectorHelper::rebin(xRange,solidAngleVec,emptyVec,*XOut,anglesSum,EOutDummy,true,true);
      }
    }

    progress.report();
    PARALLEL_END_INTERUPT_REGION
  }
  PARALLEL_CHECK_INTERUPT_REGION

  // Now need to loop over resulting vectors dividing by solid angle
  // and setting fractional error to match that in the 'errors' workspace
  MantidVec& EOut = outputWS->dataE(0);
  for (int k = 0; k < sizeOut-1; ++k)
  {
    YOut[k] /= anglesSum[k];
    const double fractional = errY[k] ? std::sqrt(errE[k])/errY[k] : 0.0;
    EOut[k] = fractional*YOut[k];
  }

  outputWS->isDistribution(true);
}

} // namespace Algorithms
} // namespace Mantid

