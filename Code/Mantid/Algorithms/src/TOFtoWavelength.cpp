//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/TOFtoWavelength.h"
#include "MantidAlgorithms/PhysicalConstants.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidAPI/Instrument.h"

// Register the class into the algorithm factory
DECLARE_NAMESPACED_ALGORITHM(Mantid::Algorithms, TOFtoWavelength)

namespace Mantid
{
namespace Algorithms
{

using namespace Kernel;
using API::WorkspaceFactory;
using DataObjects::Workspace2D;

// Get a reference to the logger
Logger& TOFtoWavelength::g_log = Logger::get("TOFtoWavelength");

/// Default constructor
TOFtoWavelength::TOFtoWavelength()
{
}

/// Destructor
TOFtoWavelength::~TOFtoWavelength()
{
}

/** Initialisation method. Does nothing at present.
 * 
 *  @return A StatusCode object indicating whether the operation was successful
 */
StatusCode TOFtoWavelength::init()
{
  // No extra properties besides the base class ones
  return StatusCode::SUCCESS;
}

/** Executes the algorithm
 * 
 *  @return A StatusCode object indicating whether the operation was successful
 */
StatusCode TOFtoWavelength::exec()
{
  // Cast the input workspace to a Workspace2D
  Workspace2D *inputWS = dynamic_cast<Workspace2D*>(m_inputWorkspace);
  if (!inputWS)
  {
    g_log.error("Input workspace is of incorrect type");
    return StatusCode::FAILURE;
  }
  
  // Get the number of histograms in the input 2D workspace
  const int numberOfSpectra = inputWS->getHistogramNumber();
  
  // Create the 2D workspace for the output
  // Get a pointer to the workspace factory (later will be shared)
  WorkspaceFactory *factory = WorkspaceFactory::Instance();
  m_outputWorkspace = factory->create("Workspace2D");
  Workspace2D *localWorkspace = dynamic_cast<Workspace2D*>(m_outputWorkspace);

  // Set number of histograms in 2D workspace
  localWorkspace->setHistogramNumber(numberOfSpectra);
  
  // Get a reference to the instrument contained in the workspace
  API::Instrument &instrument = inputWS->getInstrument();
  Geometry::ObjComponent &samplePos = *instrument.getSamplePos();

  // Get the distance between the source and the sample (assume in metres)
  double deltaSourceSample;
  try {
    deltaSourceSample = instrument.getSource()->getDistance(samplePos);
    g_log.debug() << "Source-sample distance: " << deltaSourceSample << std::endl;
  } catch (Exception::NotFoundError e) {
    g_log.error("Unable to calculate source-sample distance");
    return StatusCode::FAILURE;
  }
  
  // Loop over the histograms (detector spectra)
  for (int i = 0; i < numberOfSpectra; ++i) {
    
    // Get the x data
    std::vector<double> XBins = inputWS->getX(i);
    // Get the histogram bin contents and errors for copying to the output workspace (they don't change)
    std::vector<double> YData = inputWS->getY(i);
    std::vector<double> errors = inputWS->getE(i);
    
    // Get the sample-detector distance for this detector (assume in metres)
    double deltaSampleDetector;
    try {
      deltaSampleDetector = instrument.getDetector(i)->getDistance(samplePos);
      g_log.debug() << "Sample-detector[" << i << "] distance: " << deltaSampleDetector << std::endl; 
      
      // Factors to get the units right
      const double TOFisinMicroseconds = 1e-6;
      const double toAngstroms = 1e10;
    
      // This is the core of the algorithm
      double factor = (toAngstroms * TOFisinMicroseconds * PhysicalConstants::h) / 
          ( PhysicalConstants::NeutronMass * ( deltaSourceSample + deltaSampleDetector) );
      std::transform( XBins.begin(), XBins.end(), XBins.begin(),
                      std::bind2nd(std::multiplies<double>(), factor) );

    } catch (Exception::NotFoundError e) {
      // Get to here if exception thrown when calculating distance to detector
      g_log.information() << "Unable to calculate sample-detector[" << i << "] distance. Zeroing spectrum." << std::endl;
      XBins.assign(XBins.size(),0.0);
      YData.assign(YData.size(),0.0);
      errors.assign(errors.size(),0.0);
    }

    // Store the result into the output workspace
    localWorkspace->setX(i, XBins);
    localWorkspace->setData(i, YData, errors);
  }
  
  return StatusCode::SUCCESS;
}

/** Finalisation method. Does nothing at present.
 *
 *  @return A StatusCode object indicating whether the operation was successful
 */
StatusCode TOFtoWavelength::final()
{
  return StatusCode::SUCCESS;
}

} // namespace Algorithm
} // namespace Mantid
