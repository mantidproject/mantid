//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/Qxy.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidAPI/NumericAxis.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/VectorHelper.h"

namespace Mantid
{
namespace Algorithms
{

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(Qxy)

/// Sets documentation strings for this algorithm
void Qxy::initDocs()
{
  this->setWikiSummary("Performs the final part of a SANS (LOQ/SANS2D) two dimensional (in Q) data reduction. ");
  this->setOptionalMessage("Performs the final part of a SANS (LOQ/SANS2D) two dimensional (in Q) data reduction.");
}


using namespace Kernel;
using namespace API;
using namespace Geometry;


void Qxy::init()
{
  CompositeValidator<> *wsValidator = new CompositeValidator<>;
  wsValidator->add(new WorkspaceUnitValidator<>("Wavelength"));
  wsValidator->add(new HistogramValidator<>);
  wsValidator->add(new InstrumentValidator<>);

  declareProperty(new WorkspaceProperty<>("InputWorkspace","",Direction::Input,wsValidator));
  declareProperty(new WorkspaceProperty<>("OutputWorkspace","",Direction::Output));
  
  BoundedValidator<double> *mustBePositive = new BoundedValidator<double>();
  mustBePositive->setLower(1.0e-12);
  
  declareProperty("MaxQxy",-1.0,mustBePositive);
  declareProperty("DeltaQ",-1.0,mustBePositive->clone());
  declareProperty("AccountForGravity",false,Direction::Input);
}

void Qxy::exec()
{
  MatrixWorkspace_const_sptr inputWorkspace = getProperty("InputWorkspace");

  const bool doGravity = getProperty("AccountForGravity");

  // Create the output Qx-Qy grid
  MatrixWorkspace_sptr outputWorkspace = this->setUpOutputWorkspace();

  // Copy over the instrument name and the workspace title
  outputWorkspace->getBaseInstrument()->setName(inputWorkspace->getInstrument()->getName());
  outputWorkspace->setTitle(inputWorkspace->getTitle());
  // Will also need an identically-sized workspace to hold the solid angle/time bin masked weight
  MatrixWorkspace_sptr weights = WorkspaceFactory::Instance().create(outputWorkspace);
  // Copy the X values from the output workspace to the solidAngles one
  cow_ptr<MantidVec> axis;
  axis.access() = outputWorkspace->readX(0);
  for ( int i = 0; i < weights->getNumberHistograms(); ++i ) weights->setX(i,axis);
  
  const int numSpec = inputWorkspace->getNumberHistograms();
  const int numBins = inputWorkspace->blocksize();
  
  // the samplePos is often not (0, 0, 0) because the instruments components are moved to account for the beam centre
  const V3D samplePos = inputWorkspace->getInstrument()->getSample()->getPos();
  
  // Set the progress bar (1 update for every one percent increase in progress)
  Progress prog(this, 0.05, 1.0, numSpec);

  PARALLEL_FOR2(inputWorkspace,outputWorkspace)
  for (int i = 0; i < numSpec; ++i)
  {
    PARALLEL_START_INTERUPT_REGION
    // Get the pixel relating to this spectrum
    IDetector_const_sptr det;
    try {
      det = inputWorkspace->getDetector(i);
    } catch (Exception::NotFoundError&) {
      g_log.warning() << "Spectrum index " << i << " has no detector assigned to it - discarding" << std::endl;
      continue;
    }
    // If this is a monitor, then skip
    if ( det->isMonitor() ) continue;
    if ( det->isMasked() ) continue;
    
    V3D detPos = det->getPos()-samplePos;
      
    // these will be re-calculated if gravity is on but without gravity there is no need
    double phi = atan2(detPos.Y(),detPos.X());
    double a = cos(phi);
    double b = sin(phi);
    double sinTheta = sin( inputWorkspace->detectorTwoTheta(det)/2.0 );

    // Get references to the data for this spectrum
    const MantidVec& X = inputWorkspace->readX(i);
    const MantidVec& Y = inputWorkspace->readY(i);
    const MantidVec& E = inputWorkspace->readE(i);
 
    const MantidVec& axis = outputWorkspace->readX(0);

    // the solid angle of the detector as seen by the sample is used for normalisation later on
    double angle = det->solidAngle(samplePos);

    // some bins are masked completely or partially, the following vector will contain the fractions
    MantidVec fractions;
    if ( inputWorkspace->hasMaskedBins(i) )
    {
      // go through the set and convert it to a vector
      const MatrixWorkspace::MaskList& mask = inputWorkspace->maskedBins(i);
      fractions.resize(numBins, 1.0);
      MatrixWorkspace::MaskList::const_iterator it, itEnd(mask.end());
      for (it = mask.begin(); it != itEnd; ++it)
      {
        // The weight for this masked bin is 1 minus the degree to which this bin is masked
        fractions[it->first] -= it->second;
      }
    }
    double fraction(1);
    
    // this object is not used if gravity correction is off, but it is only constructed once per spectrum
    GravitySANSHelper grav;
    if (doGravity)
    {
      grav = GravitySANSHelper(inputWorkspace, det);
    }

    for (int j = numBins-1; j >= 0; --j)
    {
      const double binWidth = X[j+1]-X[j];
      // Calculate the wavelength at the mid-point of this bin
      const double wavLength = X[j]+(binWidth)/2.0;
      
      if (doGravity)
      {
        // SANS instruments must have their y-axis pointing up, show the detector position as where the neutron would be without gravity
        sinTheta = grav.calcComponents(wavLength, a, b);
      }

      // Calculate |Q| for this bin
      const double Q = 4.0*M_PI*sinTheta/wavLength;

      // Now get the x & y components of Q.
      const double Qx = a*Q;
      // Test whether they're in range, if not go to next spectrum.
      if ( Qx < axis.front() || Qx >= axis.back() ) break;
      const double Qy = b*Q;
      if ( Qy < axis.front() || Qy >= axis.back() ) break;
      // Find the indices pointing to the place in the 2D array where this bin's contents should go
      const MantidVec::difference_type xIndex = std::upper_bound(axis.begin(),axis.end(),Qx) - axis.begin() - 1;
      const int yIndex = static_cast<int>(
        std::upper_bound(axis.begin(),axis.end(),Qy) - axis.begin() - 1);
      PARALLEL_CRITICAL(qxy)    /* Write to shared memory - must protect */
      {
        // the data will be copied to this bin in the output array
        double & outputBinY = outputWorkspace->dataY(yIndex)[xIndex];
        double & outputBinE = outputWorkspace->dataE(yIndex)[xIndex];
        // all bins start out at Nan and hence pass the odd conditional below
        if ( outputBinY != outputBinY )
        {
          outputBinY = outputBinE = 0;
        }
        // Add the contents of the current bin to the 2D array.
        outputBinY += Y[j];
        // add the errors in quadranture
        outputBinE = std::sqrt( (outputBinE*outputBinE) + (E[j]*E[j]) );
        
        // account for masked bins
        if ( ! fractions.empty() )
        {
          fraction = fractions[j];
        }
        // add the total weight for this bin in the weights workspace, in an equivelant bin to where the data was stored
        weights->dataY(yIndex)[xIndex] += fraction*angle;      

      }
    } // loop over single spectrum
    
    prog.report("Calculating Q");

    PARALLEL_END_INTERUPT_REGION
  } // loop over all spectra
  PARALLEL_CHECK_INTERUPT_REGION

  // Divide the output data by the solid angles
  outputWorkspace /= weights;
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
    for (int j=0; j < bins-j; ++j)
    {
      outputWorkspace->dataY(i)[j] = std::numeric_limits<double>::quiet_NaN();
      outputWorkspace->dataE(i)[j] = std::numeric_limits<double>::quiet_NaN();
    }
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
