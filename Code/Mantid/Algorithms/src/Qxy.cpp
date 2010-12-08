//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/Qxy.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidAPI/NumericAxis.h"
#include "MantidKernel/UnitFactory.h"

namespace Mantid
{
namespace Algorithms
{

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(Qxy)

using namespace Kernel;
using namespace API;
using namespace Geometry;

void Qxy::init()
{
  CompositeValidator<> *wsValidator = new CompositeValidator<>;
  wsValidator->add(new WorkspaceUnitValidator<>("Wavelength"));
  wsValidator->add(new HistogramValidator<>);
  declareProperty(new WorkspaceProperty<>("InputWorkspace","",Direction::Input,wsValidator));
  declareProperty(new WorkspaceProperty<>("OutputWorkspace","",Direction::Output));
  
  BoundedValidator<double> *mustBePositive = new BoundedValidator<double>();
  mustBePositive->setLower(1.0e-12);
  
  declareProperty("MaxQxy",-1.0,mustBePositive);
  declareProperty("DeltaQ",-1.0,mustBePositive->clone());
}

void Qxy::exec()
{
  MatrixWorkspace_const_sptr inputWorkspace = getProperty("InputWorkspace");

  // Create the output Qx-Qy grid
  MatrixWorkspace_sptr outputWorkspace = this->setUpOutputWorkspace();
  // Copy over the instrument name and the workspace title
  outputWorkspace->getBaseInstrument()->setName(inputWorkspace->getInstrument()->getName());
  outputWorkspace->setTitle(inputWorkspace->getTitle());
  // Will also need an identically-sized workspace to hold the solid angles
  MatrixWorkspace_sptr solidAngles = WorkspaceFactory::Instance().create(outputWorkspace);
  // Copy the X values from the output workspace to the solidAngles one
  cow_ptr<MantidVec> axis;
  axis.access() = outputWorkspace->readX(0);
  for ( int i = 0; i < solidAngles->getNumberHistograms(); ++i ) solidAngles->setX(i,axis);
  
  const int numSpec = inputWorkspace->getNumberHistograms();
  const int numBins = inputWorkspace->blocksize();
  
  const V3D samplePos = inputWorkspace->getInstrument()->getSample()->getPos();
  // Set up the progress reporting object
  Progress progress(this,0.0,1.0,numSpec);
  
  PARALLEL_FOR2(inputWorkspace,outputWorkspace)
  for (int i = 0; i < numSpec; ++i)
  {
    PARALLEL_START_INTERUPT_REGION
    // Get the pixel relating to this spectrum
    IDetector_sptr det;
    try {
      det = inputWorkspace->getDetector(i);
    } catch (Exception::NotFoundError) {
      g_log.warning() << "Spectrum index " << i << " has no detector assigned to it - discarding" << std::endl;
      continue;
    }
    // If this is a monitor, then skip
    if ( det->isMonitor() ) continue;
    
    const V3D detPos = det->getPos();
    const double phi = atan2(detPos.Y(),detPos.X());
    const double a = cos(phi);
    const double b = sin(phi);
    
    const double sinTheta = sin( inputWorkspace->detectorTwoTheta(det)/2.0 );
    
    const double solidAngle = det->solidAngle(samplePos);
    
    // Get references to the data for this spectrum
    const MantidVec& X = inputWorkspace->readX(i);
    const MantidVec& Y = inputWorkspace->readY(i);
 
    const MantidVec& axis = outputWorkspace->readX(0);
    
    for (int j = numBins-1; j >= 0; --j)
    {
      // Calculate the wavelength at the mid-point of this bin (note this is 1/lambda)
      const double oneOverLambda = 2.0/(X[j]+X[j+1]);
      // Calculate |Q| for this bin
      const double Q = 4.0*M_PI*sinTheta*oneOverLambda;
      // Now get the x & y components of Q.
      const double Qx = a*Q;
      // Test whether they're in range, if not go to next spectrum.
      if ( Qx < axis.front() || Qx >= axis.back() ) break;
      const double Qy = b*Q;
      if ( Qy < axis.front() || Qy >= axis.back() ) break;
      // Find the indices pointing to the place in the 2D array where this bin's contents should go
      const MantidVec::difference_type xIndex = std::upper_bound(axis.begin(),axis.end(),Qx) - axis.begin() - 1;
      const MantidVec::difference_type yIndex = std::upper_bound(axis.begin(),axis.end(),Qy) - axis.begin() - 1;
      PARALLEL_CRITICAL(qxy)    /* Write to shared memory - must protect */
      {
        // Add the contents of the current bin to the 2D array.
        outputWorkspace->dataY(yIndex)[xIndex] += Y[j];
        // And the current solid angle number to same location.
        solidAngles->dataY(yIndex)[xIndex] += solidAngle;
      }
    } // loop over single spectrum
    
    progress.report();
    PARALLEL_END_INTERUPT_REGION
  } // loop over all spectra
  PARALLEL_CHECK_INTERUPT_REGION

  // Divide the output data by the solid angles
  outputWorkspace /= solidAngles;
  outputWorkspace->isDistribution(true);
  
  // Count of the number of empty cells
  MatrixWorkspace::const_iterator wsIt(*outputWorkspace);
  int emptyBins = 0;
  for (;wsIt != wsIt.end(); ++wsIt)
  {
      if (wsIt->Y() < 1.0e-12) ++emptyBins;
  }
  // Log the number of empty bins
  g_log.notice() << "There are a total of " << emptyBins << " (" 
                 << (100*emptyBins)/(outputWorkspace->size()) << "%) empty Q bins.\n"; 
}

/** Creates the output workspace, setting the X vector to the bins boundaries in Qx.
 *  @return A pointer to the newly-created workspace
 */
API::MatrixWorkspace_sptr Qxy::setUpOutputWorkspace()
{
  const double max = getProperty("MaxQxy");
  const double delta = getProperty("DeltaQ");
  
  int bins = static_cast<int>(max/delta);
  if ( bins*delta != max ) ++bins; // Stop at first boundary past MaxQxy if max is not a multiple of delta
  const double startVal = -1.0*delta*bins;
  bins *= 2; // go from -max to +max
  bins += 1; // Add 1 - this is a histogram
  
  // Create the output workspace
  MatrixWorkspace_sptr outputWorkspace = WorkspaceFactory::Instance().create("Workspace2D",bins-1,bins,bins-1);

  // Create a numeric axis to replace the vertical one
  Axis* verticalAxis = new NumericAxis(bins);
  outputWorkspace->replaceAxis(1,verticalAxis);

  // Build up the X values
  Kernel::cow_ptr<MantidVec> axis;
  MantidVec& horizontalAxisRef = axis.access();
  horizontalAxisRef.resize(bins);
  for (int i = 0; i < bins; ++i)
  {
    const double currentVal = startVal + i*delta;
    // Set the X value
    horizontalAxisRef[i] = currentVal;
    // Set the Y value on the axis
    verticalAxis->setValue(i,currentVal);
  }
  
  // Fill the X vectors in the output workspace
  for (int i=0; i < bins-1; ++i)
  {
    outputWorkspace->setX(i,axis);
  }

  // Set the axis units
  outputWorkspace->getAxis(1)->unit() = outputWorkspace->getAxis(0)->unit() = UnitFactory::Instance().create("MomentumTransfer");
  // Set the 'Y' unit (gets confusing here...this is probably a Z axis in this case)
  outputWorkspace->setYUnitLabel("Cross Section (1/cm)");

  setProperty("OutputWorkspace",outputWorkspace);
  return outputWorkspace;
}

} // namespace Algorithms
} // namespace Mantid
