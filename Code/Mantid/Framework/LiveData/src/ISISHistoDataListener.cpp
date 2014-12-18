#include "MantidLiveData/ISISHistoDataListener.h"
#include "MantidAPI/LiveListenerFactory.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/AlgorithmFactory.h"
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/SpectrumDetectorMapping.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/WarningSuppressions.h"
#include "MantidGeometry/Instrument.h"

#if (GCC_VERSION >= 40400 && GCC_VERSION < 40500) // 4.4.0 < GCC > 4.5.0
  // Avoid compiler warnings on gcc 4.4 from unused static constants in isisds_command.h
  GCC_DIAG_OFF(unused-variable)
#endif
#include "LoadDAE/idc.h"

#include <boost/lexical_cast.hpp>

#include <algorithm>

using namespace Mantid::API;
using namespace Mantid::Geometry;
using Mantid::Kernel::ConfigService;

namespace Mantid
{
namespace LiveData
{
  DECLARE_LISTENER(ISISHistoDataListener)

  namespace
  {
    /// static logger
    Kernel::Logger g_log("ISISHistoDataListener");
  }

  /// Constructor
  ISISHistoDataListener::ISISHistoDataListener() : ILiveListener(), isInitilized(false), m_daeHandle( NULL )
  {
  }
    
  /// Destructor
  ISISHistoDataListener::~ISISHistoDataListener()
  {
    if ( m_daeHandle )
    {
      IDCclose(&m_daeHandle);
    }
  }
  
  /** Function called by IDC routines to report an error. Passes the error through to the logger
  * @param status ::  The status code of the error (disregarded)
  * @param code ::    The error code (disregarded)
  * @param message :: The error message - passed to the logger at error level
  */
  void ISISHistoDataListener::IDCReporter(int status, int code, const char* message)
  {
    (void) status; (void) code; // Avoid compiler warning
    g_log.error(message);
  }

  /** Connect to the specified address and checks that is valid
    *  @param address   The IP address and port to contact (port is ignored).
    *  @return True if the connection was successfully established
    */
  bool ISISHistoDataListener::connect(const Poco::Net::SocketAddress& address)
  {
    m_daeName = address.toString();
    // remove the port part
    auto i = m_daeName.find(':');
    if ( i != std::string::npos )
    {
      m_daeName.erase( i );
    }

    // set IDC reporter function for errors
    IDCsetreportfunc(&ISISHistoDataListener::IDCReporter);

    if (IDCopen(m_daeName.c_str(), 0, 0, &m_daeHandle,address.port()) != 0)
    {
      m_daeHandle = NULL;
      return false;
    }

    m_numberOfPeriods = getInt("NPER");
    m_numberOfSpectra = getInt("NSP1");
    m_numberOfBins = getInt("NTC1");

//    std::cerr << "number of periods " << m_numberOfPeriods << std::endl;
//    std::cerr << "number of spectra " << m_numberOfSpectra << std::endl;
//    std::cerr << "number of bins " << m_numberOfBins << std::endl;

    return true;
  }

  bool ISISHistoDataListener::isConnected()
  {
    if ( m_daeHandle == NULL ) return false;
    // try to read a parameter, success means connected
    int sv_dims_array[1] = { 1 }, sv_ndims = 1, buffer;
    int stat = IDCgetpari(m_daeHandle, "NPER", &buffer, sv_dims_array, &sv_ndims);
    return stat == 0;
  }

  ILiveListener::RunStatus ISISHistoDataListener::runStatus()
  {
    // In a run by default
    return Running;
  }

  int ISISHistoDataListener::runNumber() const
  {
    // Not available it would seem - just return 0
    return 0;
  }

  void ISISHistoDataListener::start(Kernel::DateAndTime /*startTime*/) // Ignore the start time
  {
    return;
  }

  /**
   * Read the data from the DAE.
   * @return :: A workspace with the data.
   */
  boost::shared_ptr<Workspace> ISISHistoDataListener::extractData()
  {
    if ( !m_daeHandle ) 
    {
      g_log.error("DAE is not connected");
      throw Kernel::Exception::FileError("DAE is not connected ", m_daeName);
    }

    m_dataReset = false;
    isInitilized = true;

    // check that the dimensions haven't change since last time
    int numberOfPeriods = getInt("NPER");
    int numberOfSpectra = getInt("NSP1");
    int numberOfBins = getInt("NTC1");
    if ( numberOfPeriods != m_numberOfPeriods || numberOfSpectra != m_numberOfSpectra ||
      numberOfBins != m_numberOfBins)
    {
      g_log.error("Data dimensions changed");
      throw Kernel::Exception::FileError("Data dimensions changed", m_daeName);
    }

    // buffer to read loat values in
    std::vector<float> floatBuffer;

    // read in the bin boundaries
    getFloatArray( "RTCB1", floatBuffer, numberOfBins + 1);
    // copy them into a MantidVec
    m_bins.reset(new MantidVec(floatBuffer.begin(), floatBuffer.end()));

    // read in the proton charge
    getFloatArray( "RRPB", floatBuffer, 32);
    const double protonCharge = floatBuffer[8];

    // find out the number of histograms in the output workspace
    const size_t numberOfHistograms = m_specList.empty() ? m_numberOfSpectra : m_specList.size();

    // Create the 2D workspace for the output
    auto localWorkspace = WorkspaceFactory::Instance().create( "Workspace2D", numberOfHistograms, numberOfBins + 1, numberOfBins ); 
    
    // Set the unit on the workspace to TOF
    localWorkspace->getAxis(0)->unit() = Kernel::UnitFactory::Instance().create("TOF");
    localWorkspace->setYUnit("Counts");
    loadSpectraMap(localWorkspace);

    // cut the spectra numbers into chunks
    std::vector<int> index, count;
    calculateIndicesForReading(index, count);
    
    // create a workspace group in case the data are multiperiod
    auto workspaceGroup = API::WorkspaceGroup_sptr( new API::WorkspaceGroup );
    // loop over periods and spectra and fill in the output workspace
    for(int period = 0; period < m_numberOfPeriods; ++period)
    {
      if ( period > 0 )
      {
        // create a new matrix workspace similar to the previous, copy over the instrument info
        localWorkspace = WorkspaceFactory::Instance().create( localWorkspace );
        workspaceGroup->addWorkspace( localWorkspace );
      }
      size_t workspaceIndex = 0;
      for(size_t i = 0; i < index.size(); ++i)
      {
        getData(period, index[i], count[i], localWorkspace, workspaceIndex);
        workspaceIndex += count[i];
      }

      if (period == 0)
      {
        // Only run the Child Algorithms once
        runLoadInstrument( localWorkspace, getString("NAME") );
        if ( m_numberOfPeriods > 1 )
        {
          // adding first ws to the group after loading instrument
          // otherwise ws can be lost.
          workspaceGroup->addWorkspace( localWorkspace );
        }
        // Set the total proton charge for this run
        localWorkspace->mutableRun().setProtonCharge(protonCharge);
      }
    }

    if ( m_numberOfPeriods > 1 )
    {
      return workspaceGroup;
    }

    return localWorkspace;
  }

  /**
   * Read an integer parameter from the DAE
   * @param par :: Parameter name
   */
  int ISISHistoDataListener::getInt(const std::string& par) const
  {
    int sv_dims_array[1] = { 1 }, sv_ndims = 1, buffer;
    int stat = IDCgetpari(m_daeHandle, par.c_str(), &buffer, sv_dims_array, &sv_ndims);
    if ( stat != 0 )
    {
      g_log.error("Unable to read " + par + " from DAE " + m_daeName);
      throw Kernel::Exception::FileError("Unable to read " + par + " from DAE " , m_daeName);
    }
    return buffer;
  }

  /**
   * Read a string parameter from the DAE
   * @param par :: Parameter name
   */
  std::string ISISHistoDataListener::getString(const std::string& par) const
  {
    const int maxSize = 1024;
    char buffer[maxSize];
    int dim = maxSize, ndims = 1;
    if (IDCgetparc(m_daeHandle, par.c_str(), (char*)buffer, &dim, &ndims) != 0)
    {
      g_log.error("Unable to read " + par + " from DAE " + m_daeName);
      throw Kernel::Exception::FileError("Unable to read " + par + " from DAE " , m_daeName);
    };
    return std::string(buffer, dim);
  }

  /** Sets a list of spectra to be extracted. Default is reading all available spectra.
    * @param specList :: A vector with spectra indices.
    */
  void ISISHistoDataListener::setSpectra(const std::vector<specid_t>& specList) 
  {
    // after listener has created its first workspace the spectra numbers cannot be changed
    if ( !isInitilized ) 
    {
      m_specList = specList;
    }
  }

  /**
   * Read an array of floats from the DAE
   * @param par :: Array name
   * @param arr :: A vector to store the values
   * @param dim :: Size of the array
   */
  void ISISHistoDataListener::getFloatArray(const std::string& par, std::vector<float>& arr, const size_t dim)
  {
    int dims = static_cast<int>(dim), ndims = 1;
    arr.resize( dim );
    if (IDCgetparr(m_daeHandle, par.c_str(), arr.data(), &dims, &ndims) != 0)
    {
      g_log.error("Unable to read " + par + " from DAE " + m_daeName);
      throw Kernel::Exception::FileError("Unable to read " + par + " from DAE " , m_daeName);
    }
  }

  /**
   * Read an array of ints from the DAE
   * @param par :: Array name
   * @param arr :: A vector to store the values
   * @param dim :: Size of the array
   */
  void ISISHistoDataListener::getIntArray(const std::string& par, std::vector<int>& arr, const size_t dim)
  {
    int dims = static_cast<int>(dim), ndims = 1;
    arr.resize( dim );
    if (IDCgetpari(m_daeHandle, par.c_str(), arr.data(), &dims, &ndims) != 0)
    {
      g_log.error("Unable to read " + par + " from DAE " + m_daeName);
      throw Kernel::Exception::FileError("Unable to read " + par + " from DAE " , m_daeName);
    }
  }

  /**
   * Split up all spectra into chunks 
   * @param index :: Vector of first indices of a chunk.
   * @param count :: Numbers of spectra in each chunk.
   */
  void ISISHistoDataListener::calculateIndicesForReading(std::vector<int>& index, std::vector<int>& count)
  {
    // max number of spectra that could be read in in one go
    int maxNumberOfSpectra = 1024 * 1024 / ( m_numberOfBins * (int)sizeof(int) );
    if ( maxNumberOfSpectra == 0 )
    {
      maxNumberOfSpectra = 1;
    }

    if ( m_specList.empty() )
    {
      // make sure the chunk sizes < maxNumberOfSpectra
      int spec = 1;
      int n = m_numberOfSpectra;
      while( n > 0 )
      {
        if ( n < maxNumberOfSpectra )
        {
          index.push_back( spec );
          count.push_back( n );
          break;
        }
        else
        {
          index.push_back( spec );
          count.push_back( maxNumberOfSpectra );
          n -= maxNumberOfSpectra;
          spec += maxNumberOfSpectra;
        }
      }
    }
    else
    {
      // combine consecutive spectra but don't exceed the maxNumberOfSpectra
      size_t i0 = 0;
      specid_t spec = m_specList[i0];
      for(size_t i = 1; i < m_specList.size(); ++i)
      {
        specid_t next = m_specList[i];
        if ( next - m_specList[i-1] > 1 || static_cast<int>(i - i0) >= maxNumberOfSpectra )
        {
          index.push_back( spec );
          count.push_back( static_cast<int>( i - i0 ) );
          i0 = i;
          spec = next;
        }
      }
      index.push_back( spec );
      count.push_back( static_cast<int>( m_specList.size() - i0 ) );
    }

  }

  /**
   * Read spectra from the DAE
   * @param period :: Current period index
   * @param index :: First spectrum index
   * @param count :: Number of spectra to read
   * @param workspace :: Workspace to store the data
   * @param workspaceIndex :: index in workspace to store data
   */
  void ISISHistoDataListener::getData(int period, int index, int count, API::MatrixWorkspace_sptr workspace, size_t workspaceIndex)
  {
    const size_t bufferSize = count * (m_numberOfBins + 1) * sizeof(int);
    std::vector<int> dataBuffer( bufferSize );
    // Read in spectra from DAE
    int ndims = 2, dims[2];
    dims[0] = count;
    dims[1] = m_numberOfBins + 1;

    int spectrumIndex = index + period * (m_numberOfSpectra + 1);
    if (IDCgetdat(m_daeHandle, spectrumIndex, count, dataBuffer.data(), dims, &ndims) != 0)
    {
      g_log.error("Unable to read DATA from DAE " + m_daeName);
      throw Kernel::Exception::FileError("Unable to read DATA from DAE " , m_daeName);
    }

    for(size_t i = 0; i < static_cast<size_t>(count); ++i)
    {
      size_t wi = workspaceIndex + i;
      workspace->setX(wi, m_bins);
      MantidVec& y = workspace->dataY( wi );
      MantidVec& e = workspace->dataE( wi );
      workspace->getSpectrum(wi)->setSpectrumNo(index + static_cast<specid_t>(i));
      size_t shift = i * (m_numberOfBins + 1) + 1;
      y.assign( dataBuffer.begin() + shift, dataBuffer.begin() + shift + y.size() );
      std::transform( y.begin(), y.end(), e.begin(), dblSqrt );
    }
  }

    /** Populate spectra-detector map
        @param localWorkspace :: The workspace
     */
    void ISISHistoDataListener::loadSpectraMap(MatrixWorkspace_sptr localWorkspace)
    {
      // Read in the number of detectors
      int ndet = getInt( "NDET" );

      std::vector<int> udet;
      std::vector<int> spec;
      getIntArray( "UDET", udet, ndet);
      getIntArray( "SPEC", spec, ndet);
      localWorkspace->updateSpectraUsing(SpectrumDetectorMapping(spec, udet));
    }

    /** Run the Child Algorithm LoadInstrument (or LoadInstrumentFromRaw).
     *  @param localWorkspace :: The workspace
     *  @param iName :: The instrument name
     */
    void ISISHistoDataListener::runLoadInstrument(MatrixWorkspace_sptr localWorkspace, const std::string& iName)
    {
      auto loadInst = API::AlgorithmFactory::Instance().create("LoadInstrument",-1);
      if ( !loadInst ) return;
      loadInst->initialize();
      try
      {
        loadInst->setPropertyValue("InstrumentName", iName);
        loadInst->setProperty<MatrixWorkspace_sptr>("Workspace",localWorkspace);
        loadInst->setProperty("RewriteSpectraMap", false);
        loadInst->executeAsChildAlg();
      }
      catch(std::invalid_argument &)
      {
        g_log.information("Invalid argument to LoadInstrument Child Algorithm");
      }
      catch (std::runtime_error&)
      {
        g_log.information("Unable to successfully run LoadInstrument Child Algorithm");
      }
      if (API::AnalysisDataService::Instance().doesExist("Anonymous"))
      {
          // LoadInstrument adds the workspace to ADS as Anonymous
          // we don't want it there
          API::AnalysisDataService::Instance().remove("Anonymous");
      }
    }

    /// Personal wrapper for sqrt to allow msvs to compile
    double ISISHistoDataListener::dblSqrt(double in)
    {
      return sqrt( in );
    }

} // namespace LiveData
} // namespace Mantid
