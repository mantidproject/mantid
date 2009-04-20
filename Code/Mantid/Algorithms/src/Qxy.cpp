//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/Qxy.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/UnitFactory.h"
#include "Poco/Timestamp.h"
#include "Poco/DateTimeFormatter.h"

#include <fstream>
#include <iomanip>

namespace Mantid
{
namespace Algorithms
{

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(Qxy)

using namespace Kernel;
using namespace API;
using namespace Geometry;

// Get a reference to the logger. It is used to print out information, warning and error messages
Logger& Qxy::g_log = Logger::get("AnisotropicQ");

void Qxy::init()
{
  CompositeValidator<> *wsValidator = new CompositeValidator<>;
  wsValidator->add(new WorkspaceUnitValidator<>("Wavelength"));
  wsValidator->add(new HistogramValidator<>);
  declareProperty(new WorkspaceProperty<>("InputWorkspace","",Direction::Input,wsValidator));
  declareProperty(new WorkspaceProperty<DataObjects::Workspace2D>("OutputWorkspace","",Direction::Output));
  declareProperty("OutputFilename","",new MandatoryValidator<std::string>);
  
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
  // Will also need an identically-sized workspace to hold the solid angles
  MatrixWorkspace_sptr solidAngles = WorkspaceFactory::Instance().create(outputWorkspace);

  const int numSpec = inputWorkspace->getNumberHistograms();
  const int numBins = inputWorkspace->blocksize();
  
  const V3D samplePos = inputWorkspace->getInstrument()->getSample()->getPos();
  // Set up the progress reporting object
  Progress progress(this,0.0,1.0,numSpec,100);
  
  for (int i = 0; i < numSpec; ++i)
  {
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
      //if (Y[j]==0) continue;
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
      // Add the contents of the current bin to the 2D array
      outputWorkspace->dataY(yIndex)[xIndex] += Y[j];
      // And the current solid angle number to same location
      solidAngles->dataY(yIndex)[xIndex] += solidAngle;
    } // loop over single spectrum
    
    progress.report();
  } // loop over all spectra

  // Divide the output data by the solid angles
  outputWorkspace /= solidAngles;
  
  this->writeResult(outputWorkspace);
  
  // Need to log a count of the number of empty cells
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
  
  // Create the output workspace
  MatrixWorkspace_sptr outputWorkspace = WorkspaceFactory::Instance().create("Workspace2D",bins,bins+1,bins);
  
  // Create a numeric axis to replace the vertical one
  Axis* verticalAxis = new Axis(AxisType::Numeric,bins);
  outputWorkspace->replaceAxis(1,verticalAxis);

  // Build up the X values
  Kernel::cow_ptr<MantidVec> axis;
  MantidVec& horizontalAxisRef = axis.access();
  horizontalAxisRef.resize(bins+1);
  for (int i = 0; i < bins; ++i)
  {
    const double currentVal = startVal + i*delta;
    // Set the X value
    horizontalAxisRef[i] = currentVal;
    // Set the value on the new axis to the mid-point of the bin
    // Later may want vertical axis to properly model bins, but since this is presently
    // just for display purposes, it doesn't really matter
    verticalAxis->setValue(i,currentVal+(delta/2.0));
  }
  // One extra value for the X vector
  horizontalAxisRef[bins] = startVal + bins*delta;
  outputWorkspace->setX(axis);

  // Set the axis units
  outputWorkspace->getAxis(1)->unit() = outputWorkspace->getAxis(0)->unit() = UnitFactory::Instance().create("MomentumTransfer");
  // Set the 'Y' unit (gets confusing here...this is probably a Z axis in this case)
  outputWorkspace->setYUnit("Cross Section");

  setProperty("OutputWorkspace",boost::dynamic_pointer_cast<DataObjects::Workspace2D>(outputWorkspace));
  return outputWorkspace;
}

// Later, should be spun out into a separate algorithm
/** Writes a FISH file containing the 2D data
 *  @param result The workspace to write out
 */
void Qxy::writeResult(API::MatrixWorkspace_const_sptr result)
{
  const std::string filename = getProperty("OutputFilename");
  std::ofstream outRKH(filename.c_str());

  if( !outRKH )
  {
    g_log.error() << "An error occurred while attempting to open the output file" << std::endl;;
    throw std::runtime_error("An error occurred while trying to open the output file for writing");
  }

  outRKH <<  " LOQ ";
  Poco::Timestamp timestamp;
  //The sample file has the format of the data/time as in this example Thu 28-OCT-2004 12:23
  outRKH << Poco::DateTimeFormatter::format(timestamp, std::string("%w")) << " " << Poco::DateTimeFormatter::format(timestamp, std::string("%d")) 
         << "-";
  std::string month = Poco::DateTimeFormatter::format(timestamp, std::string("%b"));
  std::transform(month.begin(), month.end(), month.begin(), toupper);
  outRKH << month << "-" << Poco::DateTimeFormatter::format(timestamp, std::string("%Y %H:%M")) << "\n";
  // The units that the data is in
  outRKH << "  6 Q (\A\u-1\d)\n";
  outRKH << "  6 Q (\A\u-1\d)\n";
  outRKH << "  0 Cross section (cm\u-1\d)\n";
  outRKH << "  0\n";
  // Now the axis values
  const MantidVec& X = result->readX(0);
  const size_t bins = X.size(); 
  outRKH << "  " << bins << std::endl;
  for (size_t i = 0; i < bins; ++i) outRKH << std::scientific << std::setprecision(6) << X[i] << " ";
  outRKH << std::endl;
  outRKH << "  " << bins << std::endl;
  for (size_t i = 0; i < bins; ++i) outRKH << std::scientific << std::setprecision(6) << X[i] << " ";
  outRKH << std::endl;
  const int xSize = result->blocksize();
  const int ySize = result->getNumberHistograms();
  outRKH << "   " << xSize << "   " << ySize << "  " 
         << std::scientific << std::setprecision(12) << 1.0 << std::endl;
  const int iflag = 3;
  outRKH << iflag << "(8E12.4)" << std::endl;
  // Question over whether I have X & Y swapped over compared to what they're expecting
  int emptyBins = 0;
  for (int i = 0; i < ySize; ++i)
  {
    const MantidVec& Y = result->readY(i);
    for (int j = 0; j < xSize; ++j) 
    {
      outRKH << std::scientific << std::setprecision(4) << Y[j] << " ";
      // Count the empty bins for logging
      if (Y[j] < 1.0e-12) ++emptyBins;
    }
    outRKH << std::endl;
  }
  // Log the number of empty bins
  g_log.information() << "There are a total of " << emptyBins << " (" 
                                                 << (100*emptyBins)/(xSize*ySize) << "%) empty Q bins." << std::endl; 
  for (int i = 0; i < ySize; ++i)
  {
    const MantidVec& E = result->readE(i);
    for (int j = 0; j < xSize; ++j) outRKH << std::scientific << std::setprecision(4) << E[j] << " ";
    outRKH << std::endl;
  }
  
  outRKH.close();
}

} // namespace Algorithms
} // namespace Mantid

