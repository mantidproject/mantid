//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidDataHandling/LoadRaw.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidAPI/WorkspaceProperty.h"

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
  using namespace API;
  using namespace DataObjects;

  Logger& LoadRaw::g_log = Logger::get("LoadRaw");

  /// Empty default constructor
  LoadRaw::LoadRaw()
  {
  }

  /** Initialisation method.
   * 
   */
  void LoadRaw::init()
  {
    declareProperty("Filename","",new MandatoryValidator);
    declareProperty(new WorkspaceProperty<Workspace2D>("OutputWorkspace","",Direction::Output));
  }
  
  /** Executes the algorithm. Reading in the file and creating and populating
   *  the output workspace
   * 
   *  @throw runtime_error Thrown if algorithm cannot execute
   */
  void LoadRaw::exec()
  {
    // Retrieve the filename from the properties
    m_filename = getPropertyValue("Filename");
    
    ISISRAW iraw(NULL);
    if (iraw.readFromFile(m_filename.c_str()) != 0)
    {
      g_log.error("Unable to open file " + m_filename);
      throw Exception::FileError("Unable to open File:" , m_filename);	  
    }
    
    // Read the number of time channels from the RAW file 
    int channelsPerSpectrum, lengthIn, lengthOut;
    lengthIn = lengthOut = 1;
    channelsPerSpectrum = iraw.t_ntc1;

    // Read in the number of spectra in the RAW file
    int numberOfSpectra = iraw.t_nsp1;
    
    // Read in the time bin boundaries 
    lengthIn = channelsPerSpectrum + 1;    
    float* timeChannels = new float[lengthIn];
    iraw.getTimeChannels(timeChannels, lengthIn);
    // Put the read in array into a vector (inside a shared pointer)
    boost::shared_ptr<std::vector<double> > timeChannelsVec
                     (new std::vector<double>(timeChannels, timeChannels + lengthIn));

    // Create the 2D workspace for the output
    // Get a pointer to the workspace factory (later will be shared)
    API::WorkspaceFactory *factory = API::WorkspaceFactory::Instance();
    m_localWorkspace = boost::dynamic_pointer_cast<Workspace2D>(factory->create("Workspace2D"));

    // Set number of histograms in 2D workspace
    m_localWorkspace->setHistogramNumber(numberOfSpectra);

    int* spectrum = new int[lengthIn];
    // Loop over the spectra. Zeroth spectrum is garbage, so loop runs from 1 to NSP1
    for (int i = 1; i <= numberOfSpectra; i++)
    {
      // Read in a spectrum
      memcpy(spectrum, iraw.dat1 + i * lengthIn, lengthIn * sizeof(int));
      // Put it into a vector, discarding the 1st entry, which is rubbish
      // But note that the last (overflow) bin is kept
      std::vector<double> v(spectrum + 1, spectrum + lengthIn);
      // Create and fill another vector for the errors, containing sqrt(count)
      std::vector<double> e(lengthIn-1);
      std::transform(v.begin(), v.end(), e.begin(), dblSqrt);
      // Populate the workspace. Loop starts from 1, hence i-1
      m_localWorkspace->setData(i-1, v, e);
      m_localWorkspace->setX(i-1, timeChannelsVec);
      // NOTE: Raw numbers go straight into the workspace 
      //     - no account taken of bin widths/units etc.
    }
    
    // Assign the result to the output workspace property
    setProperty("OutputWorkspace",m_localWorkspace);
    
    // Clean up
    delete[] timeChannels;
    delete[] spectrum;
    
    // Run the sub-algorithms (LoadInstrument & LoadRaw)
    runSubAlgorithms();
    
    return;
  }


  /// Run the sub-algorithms (LoadInstrument & LoadLog)
  void LoadRaw::runSubAlgorithms()
  {
    // First deal with LoadInstruments
    Algorithm_sptr loadInst = createSubAlgorithm("LoadInstrument");
    // Hardcoded filename for now...this will certainly change
    loadInst->setPropertyValue("Filename","../../../../Test/Instrument/HET_Definition.txt");
    // Set the workspace property to be the same one filled above
    loadInst->setProperty<Workspace_sptr>("Workspace",m_localWorkspace);
    
    // Now execute the sub-algorithm. Catch and log any error, but don't stop.
    try
    {
      loadInst->execute();
    }
    catch (std::runtime_error& err)
    {
      g_log.error("Unable to successfully run LoadInstrument sub-algorithm");
    }
    
    if ( ! loadInst->isExecuted() )
    {
      g_log.error("Unable to successfully run LoadInstrument sub-algorithm");
    }

    // Now do LoadLog
    Algorithm_sptr loadLog = createSubAlgorithm("LoadLog");
    // Pass through the same input filename
    loadLog->setPropertyValue("Filename",m_filename);
    // Set the workspace property to be the same one filled above
    loadLog->setProperty<Workspace_sptr>("Workspace",m_localWorkspace);
    
    // Now execute the sub-algorithm. Catch and log any error, but don't stop.
    try
    {
      loadLog->execute();
    }
    catch (std::runtime_error& err)
    {
      g_log.error("Unable to successfully run LoadLog sub-algorithm");
    }
    
    if ( ! loadLog->isExecuted() )
    {
      g_log.error("Unable to successfully run LoadLog sub-algorithm");
    }
    
  }
  
  double LoadRaw::dblSqrt(double in)
  {
    return sqrt(in);
  }
  
} // namespace DataHandling
} // namespace Mantid
