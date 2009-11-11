//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/Q1D.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/RebinParamsValidator.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/VectorHelper.h"

namespace Mantid
{
namespace Algorithms
{

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(Q1D)

using namespace Kernel;
using namespace API;
using namespace Geometry;

void Q1D::init()
{
  CompositeValidator<> *wsValidator = new CompositeValidator<>;
  wsValidator->add(new WorkspaceUnitValidator<>("Wavelength"));
  wsValidator->add(new HistogramValidator<>);
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
  DataObjects::Histogram1D::RCtype XOut;
  const int sizeOut = VectorHelper::createAxisFromRebinParams(binParams,XOut.access());

  // Now create the output workspace
  MatrixWorkspace_sptr outputWS = WorkspaceFactory::Instance().create("Workspace2D",1,sizeOut,sizeOut-1);
  outputWS->getAxis(0)->unit() = UnitFactory::Instance().create("MomentumTransfer");
  outputWS->setYUnit("I(Q)");
  setProperty("OutputWorkspace",outputWS);
  // Set the X vector for the output workspace
  boost::dynamic_pointer_cast<DataObjects::Workspace2D>(outputWS)->setX(0,XOut);
  MantidVec& YOut = outputWS->dataY(0);
  // Don't care about errors here - create locals
  MantidVec EIn(inputWS->readE(0).size());
  MantidVec EOutDummy(sizeOut-1);
  MantidVec emptyVec(1);
  // Create temporary vectors for the summation & rebinning of the 'errors' workspace
  MantidVec errY(sizeOut-1),errE(sizeOut-1);
  // And one for the solid angles
  MantidVec anglesSum(sizeOut-1);

  const int numSpec = inputWS->getNumberHistograms();
  // Set up the progress reporting object
  Progress progress(this,0.0,1.0,numSpec);

  const V3D samplePos = inputWS->getInstrument()->getSample()->getPos();

  const bool doGravity = getProperty("AccountForGravity");

  // A temporary vector to store intermediate Q values
  const int xLength = inputWS->readX(0).size();
  MantidVec Qx(xLength);
  std::vector<double> widths(xLength);

  for (int i = 0; i < numSpec; ++i)
  {
    // Get the pixel relating to this spectrum
    IDetector_const_sptr det;
    try {
      det = inputWS->getDetector(i);
    } catch (Exception::NotFoundError) {
      g_log.warning() << "Spectrum index " << i << " has no detector assigned to it - discarding" << std::endl;
      continue;
    }
    // If this detector is masked, skip onto the next one
    if ( det->isMasked() ) continue;

    // Get the current spectrum for both input workspaces - not references, have to reverse below
    const MantidVec& XIn = inputWS->readX(i);
    MantidVec YIn = inputWS->readY(i);
    MantidVec errYIn = errorsWS->readY(i);
    MantidVec errEIn = errorsWS->readE(i);

    if ( doGravity )
    {
      g_log.debug("Correcting for gravity");
      // Get the vector to get at the 3 components of the pixel position
      V3D detPos = det->getPos();
      // This is the square of the horizontal distance from sample to pixel
      const double L2 = std::pow(detPos.Z()-samplePos.Z(),2);
      for ( int j = 0; j < xLength; ++j)
      {
	// Lots more to do in this loop than for the non-gravity case
	// since we have a number of calculations to do for each bin boundary

	// Calculate the drop (I'm fairly confident that Y is up!)
	// Using approx. constant prefix - will fix next week
	const double drop = 3.13e-4 * XIn[j] * XIn[j] * L2;
	detPos[1] += drop;
	// Calculate new 2theta in light of this
	const V3D sampleDetVec = detPos - samplePos;
	// Do beamline vector more rigorously later
	const double twoTheta = sampleDetVec.angle(V3D(0,0,1));
	const double sinTheta = sin( 0.5 * twoTheta );

	// Now we're ready to go to Q
	Qx[xLength-j-1] = 4.0*M_PI*sinTheta/XIn[j];
      }
    }
    else
    {
      // Calculate the Q values for the current spectrum
      const double sinTheta = sin( inputWS->detectorTwoTheta(det)/2.0 );
      const double factor = 4.0*M_PI*sinTheta;
      for ( int j = 0; j < xLength; ++j)
      {
        Qx[xLength-j-1] = factor/XIn[j];
      }
    }

    // Unfortunately, have to reverse data vectors for rebin functions to work
    std::reverse(YIn.begin(),YIn.end());
    std::reverse(errYIn.begin(),errYIn.end());
    std::reverse(errEIn.begin(),errEIn.end());
    // Now the fudge that acts as though we've turned this into a distribution at some point
    std::adjacent_difference(Qx.begin(),Qx.end(),widths.begin());
    std::transform(YIn.begin(),YIn.end(),widths.begin()+1,YIn.begin(),std::multiplies<double>());

    VectorHelper::rebinHistogram(Qx,YIn,EIn,*XOut,YOut,EOutDummy,true);
    VectorHelper::rebinHistogram(Qx,errYIn,errEIn,*XOut,errY,errE,true);

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
      
      // Create a temporary vector for the rebinned angles
      MantidVec anglesTemp(anglesSum.size(),0.0);
      // Create a zero vector for the errors because we don't care about them here
      const MantidVec zeroes(solidAngleVec.size(),0.0);
      // Rebin the solid angles - note that this is a distribution
      VectorHelper::rebin(included_bins,solidAngleVec,zeroes,*XOut,anglesTemp,EOutDummy,true);
      // Add solid angles for this spectrum to the output solid angles vector
      std::transform(anglesSum.begin(),anglesSum.end(),anglesTemp.begin(),anglesSum.begin(),std::plus<double>());
    }
    else // No masked bins
    {
      // Two element vector to define the range
      MantidVec xRange(2);
      xRange[0] = Qx.front();
      xRange[1] = Qx.back();
      // Single element vector containing the solid angle
      MantidVec solidAngleVec(1, solidAngle);

      VectorHelper::rebinNonDispersive(xRange,solidAngleVec,emptyVec,*XOut,anglesSum,EOutDummy,true);
    }

    progress.report();
  }

  // Now need to loop over resulting vectors dividing by solid angle and bin width
  // and setting fractional error to match that in the 'errors' workspace
  MantidVec& EOut = outputWS->dataE(0);
  for (int k = 0; k < sizeOut-1; ++k)
  {
    YOut[k] /= anglesSum[k]*((*XOut)[k+1]-(*XOut)[k]);
    const double fractional = errY[k] ? std::sqrt(errE[k])/errY[k] : 0.0;
    EOut[k] = fractional*YOut[k];
  }

}

} // namespace Algorithms
} // namespace Mantid

