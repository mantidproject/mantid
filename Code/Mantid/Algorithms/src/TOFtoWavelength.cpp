//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/TOFtoWavelength.h"
#include "MantidAlgorithms/PhysicalConstants.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidAPI/Instrument.h"
#include "MantidAPI/WorkspaceProperty.h"

namespace Mantid
{
namespace Algorithms
{

// Register the class into the algorithm factory
DECLARE_ALGORITHM(TOFtoWavelength)

using namespace Kernel;
using API::WorkspaceFactory;
using API::WorkspaceProperty;
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
 */
void TOFtoWavelength::init()
{
  declareProperty(new WorkspaceProperty<Workspace2D>("InputWorkspace","",Direction::Input));
  declareProperty(new WorkspaceProperty<Workspace2D>("OutputWorkspace","",Direction::Output));  
}

/** Executes the algorithm
 * 
 *  @throw runtime_error Thrown if algorithm cannot execute
 */
void TOFtoWavelength::exec()
{
  // Get the input workspace
//  Property *p = getProperty("InputWorkspace");
//  WorkspaceProperty<Workspace2D> *wp = dynamic_cast< WorkspaceProperty<Workspace2D>* >(p);
  Workspace2D *inputWS = getProperty("InputWorkspace");

  // Get the number of histograms in the input 2D workspace
  const int numberOfSpectra = inputWS->getHistogramNumber();
  
  // Create the 2D workspace for the output
  // Get a pointer to the workspace factory (later will be shared)
  WorkspaceFactory *factory = WorkspaceFactory::Instance();
  Workspace2D *localWorkspace = static_cast<Workspace2D*>(factory->create("Workspace2D"));

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
	throw std::runtime_error("Unable to calculate source-sample distance");
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
  
  // Assign the result to the output workspace property
  setProperty("OutputWorkspace",localWorkspace);
  
  return;
}

/** Finalisation method. Does nothing at present.
 *
 */
void TOFtoWavelength::final()
{  
}

} // namespace Algorithm
} // namespace Mantid
