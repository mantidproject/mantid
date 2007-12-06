//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidDataHandling/LoadRaw.h"
#include "MantidDataObjects/Workspace2D.h"

#include <cmath>
#include <boost/shared_ptr.hpp>

#include "LoadRaw/isisraw.h"

namespace Mantid
{
namespace DataHandling
{

  // Register the algorithm into the algorithm factory
  DECLARE_ALGORITHM(LoadRaw)

  using namespace Kernel;
  using DataObjects::Workspace2D;

  Logger& LoadRaw::g_log = Logger::get("LoadRaw");

  /// Empty default constructor
  LoadRaw::LoadRaw() { }

  /** Initialisation method.
   * 
   *  @return A StatusCode object indicating whether the operation was successful
   */
  StatusCode LoadRaw::init()
  {
    declareProperty("Filename","",new MandatoryValidator);
    
    return StatusCode::SUCCESS;
  }
  
  /** Executes the algorithm. Reading in the file and creating and populating
   *  the output workspace
   * 
   *  @return A StatusCode object indicating whether the operation was successful
   */
  StatusCode LoadRaw::exec()
  {
    // Retrieve the filename from the properties
    m_filename = getPropertyValue("Filename");
    
    ISISRAW iraw(NULL);
    if (iraw.readFromFile(m_filename.c_str()) != 0)
    {
      g_log.error("Unable to open file " + m_filename);
      return StatusCode::FAILURE;
    }
    
    // Read the number of time channels from the RAW file (calling FORTRAN)
    int channelsPerSpectrum, lengthIn, lengthOut;
    lengthIn = lengthOut = 1;
    channelsPerSpectrum = iraw.t_ntc1;

    // Read in the number of spectra in the RAW file (calling FORTRAN)
    int numberOfSpectra = iraw.t_nsp1;
    
    // Read in the time bin boundaries (calling FORTRAN)
    lengthIn = channelsPerSpectrum + 1;    
    float* timeChannels = new float[lengthIn];
    // Put the read in array into a vector (inside a shared pointer)
    boost::shared_ptr<std::vector<double> > timeChannelsVec
                     (new std::vector<double>(timeChannels, timeChannels + lengthIn));

    // Create the 2D workspace for the output
    // Get a pointer to the workspace factory (later will be shared)
    API::WorkspaceFactory *factory = API::WorkspaceFactory::Instance();
    m_outputWorkspace = factory->create("Workspace2D");
    Workspace2D *localWorkspace = dynamic_cast<Workspace2D*>(m_outputWorkspace);

    // Set number of histograms in 2D workspace
    localWorkspace->setHistogramNumber(numberOfSpectra);

    int* spectrum = new int[lengthIn];
    // Loop over the spectra. Zeroth spectrum is garbage, so loop runs from 1 to NSP1
    for (int i = 1; i <= numberOfSpectra; i++)
    {
      // Read in a spectrum via the FORTRAN routine
      memcpy(spectrum, iraw.dat1 + i * lengthIn, lengthIn * sizeof(int));
//      getdat_(m_filename.c_str(), i, 1, spectrum, lengthIn, errorCode, strlen( m_filename.c_str() ));
      // Put it into a vector, discarding the 1st entry, which is rubbish
      // But note that the last (overflow) bin is kept
      std::vector<double> v(spectrum + 1, spectrum + lengthIn);
      // Create and fill another vector for the errors, containing sqrt(count)
      std::vector<double> e(lengthIn-1);
      std::transform(v.begin(), v.end(), e.begin(), dblSqrt);
      // Populate the workspace. Loop starts from 1, hence i-1
//      localWorkspace->setX(i-1, timeChannelsVec);
      localWorkspace->setData(i-1, v, e);
      // NOTE: Raw numbers go straight into the workspace 
      //     - no account taken of bin widths/units etc.
    }
    
    // Clean up
    delete[] timeChannels;
    delete[] spectrum;
    return StatusCode::SUCCESS;
  }


  /** Finalisation method. Does nothing at present.
   *
   *  @return A StatusCode object indicating whether the operation was successful
   */
  StatusCode LoadRaw::final()
  {
    return StatusCode::SUCCESS;
  }

  double LoadRaw::dblSqrt(double in)
  {
	return sqrt(in);
  }
  
} // namespace DataHandling
} // namespace Mantid
