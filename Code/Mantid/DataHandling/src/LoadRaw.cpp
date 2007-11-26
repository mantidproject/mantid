//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidDataHandling/LoadRaw.h"
#include "MantidDataObjects/Workspace2D.h"

#include <cmath>
#include <boost/shared_ptr.hpp>

// Declaration of the FORTRAN functions used to access the RAW file
extern "C" void open_file__(const char* fname, int* found, unsigned fname_len);
extern "C" void getpari_(const char* fname, const char* item, int* val, 
    int* len_in, int* len_out, int* errcode, unsigned len_fname, unsigned len_item);
extern "C" void getparr_(const char* fname, const char* item, float* val, 
    int* len_in, int* len_out, int* errcode, unsigned len_fname, unsigned len_item);
extern "C" void getdat_(const char* fname, const int& spec_no, const int& nspec, 
    int* idata, int& length, int& errcode, unsigned len_fname);
extern "C" void close_data_file__();

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
    declareProperty("Filename",".");
    
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
    try {
      m_filename = getPropertyValue("Filename");
    } catch (Exception::NotFoundError e) {
      g_log.error("Filename property has not been set.");
      return StatusCode::FAILURE;      
    }
    
    int found = 0;  
    // Call the FORTRAN function to open the RAW file
    open_file__( m_filename.c_str(), &found, strlen( m_filename.c_str() ) );
    if ( ! found )
    {
      // Unable to open file
      g_log.error("Unable to open file " + m_filename);
      return StatusCode::FAILURE;
    }
    
    // Read the number of time channels from the RAW file (calling FORTRAN)
    int channelsPerSpectrum, lengthIn, lengthOut, errorCode;
    lengthIn = lengthOut = 1;
    getpari_(m_filename.c_str(), "NTC1", &channelsPerSpectrum, &lengthIn, &lengthOut,
       &errorCode, strlen( m_filename.c_str() ), strlen("NTC1"));
    if (errorCode) return StatusCode::FAILURE;

    // Read in the number of spectra in the RAW file (calling FORTRAN)
    int numberOfSpectra;
    getpari_(m_filename.c_str(), "NSP1", &numberOfSpectra, &lengthIn, &lengthOut,
       &errorCode, strlen( m_filename.c_str() ), strlen("NSP1"));
    if (errorCode) return StatusCode::FAILURE;
    
    // Read in the time bin boundaries (calling FORTRAN)
    lengthIn = channelsPerSpectrum + 1;    
    float* timeChannels = new float[lengthIn];
    getparr_(m_filename.c_str(), "TIM1", timeChannels, &lengthIn, &lengthOut,
                  &errorCode, strlen( m_filename.c_str() ), strlen("TIM1"));
    if (errorCode) return StatusCode::FAILURE;
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
      getdat_(m_filename.c_str(), i, 1, spectrum, lengthIn, errorCode, strlen( m_filename.c_str() ));
      // Put it into a vector, discarding the 1st entry, which is rubbish
      // But note that the last (overflow) bin is kept
      std::vector<double> v(spectrum + 1, spectrum + lengthIn);
      // Create and fill another vector for the errors, containing sqrt(count)
      std::vector<double> e(lengthIn-1);
      std::transform(v.begin(), v.end(), e.begin(), dblSqrt);
      // Populate the workspace. Loop starts from 1, hence i-1
      localWorkspace->setX(i-1, timeChannelsVec);
      localWorkspace->setData(i-1, v, e);
      // NOTE: Raw numbers go straight into the workspace 
      //     - no account taken of bin widths/units etc.
    }
    
    // Close the input data file
    close_data_file__();
    
    // Clean up
    delete[] timeChannels;
    delete[] spectrum;
    return StatusCode::SUCCESS;
  }

  /** Personal wrapper for sqrt to allow msvs to compile.
   *
   *  @param in Some number
   *  @return The square-root of input parameter
   */
	double LoadRaw::dblSqrt(double in)
	{
		return sqrt(in);
	}

  /** Finalisation method. Does nothing at present.
   *
   *  @return A StatusCode object indicating whether the operation was successful
   */
	StatusCode LoadRaw::final()
  {
    return StatusCode::SUCCESS;
  }
  
} // namespace DataHandling
} // namespace Mantid
