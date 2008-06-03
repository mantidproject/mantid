//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidDataHandling/LoadRaw.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/ArrayProperty.h"

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

    // Initialise the logger
    Logger& LoadRaw::g_log = Logger::get("LoadRaw");

    /// Empty default constructor
    LoadRaw::LoadRaw()
    {}

    /// Initialisation method.
    void LoadRaw::init()
    {
      declareProperty("Filename","",new MandatoryValidator);
      declareProperty(new WorkspaceProperty<DataObjects::Workspace2D>("OutputWorkspace","",Direction::Output));
      
      BoundedValidator<int> *mustBePositive = new BoundedValidator<int>();
      mustBePositive->setLower(0);
      declareProperty("spectrum_min",0, mustBePositive);
      declareProperty("spectrum_max",0, mustBePositive->clone());
      
      declareProperty(new ArrayProperty<int>("spectrum_list"));
    }

    /** Executes the algorithm. Reading in the file and creating and populating
     *  the output workspace
     * 
     *  @throw Exception::FileError If the RAW file cannot be found/opened
     *  @throw std::invalid_argument If the optional properties are set to invalid values
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

      // Read in the number of spectra in the RAW file
      const int numberOfSpectra = iraw.t_nsp1;
      
      Property *specList = getProperty("spectrum_list");
      bool list = !(specList->isDefault());
      Property *specMax = getProperty("spectrum_max");
      bool interval = !(specMax->isDefault());

      std::vector<int> spec_list;
      if ( list )
      {
        list = true;
        spec_list = getProperty("spectrum_list");
        const int minlist = *min_element(spec_list.begin(),spec_list.end());
        const int maxlist = *max_element(spec_list.begin(),spec_list.end());
        if ( maxlist > numberOfSpectra || minlist == 0)
        {
          g_log.error("Invalid list of spectra");
          throw std::invalid_argument("Inconsistent properties defined"); 
        } 
      }
           
      int spec_min, spec_max;
      if ( interval )
      {
        interval = true;
        spec_min = getProperty("spectrum_min");
        spec_max = getProperty("spectrum_max");
        if ( spec_max < spec_min || spec_max > numberOfSpectra )
        {
          g_log.error("Invalid Spectrum min/max properties");
          throw std::invalid_argument("Inconsistent properties defined"); 
        }
      }
      
      std::cout << "Periods: " << iraw.t_nper << std::endl;

      // Read the number of time channels (i.e. bins) from the RAW file 
      const int channelsPerSpectrum = iraw.t_ntc1;
      // Read in the time bin boundaries 
      const int lengthIn = channelsPerSpectrum + 1;    
      float* timeChannels = new float[lengthIn];
      iraw.getTimeChannels(timeChannels, lengthIn);
      // Put the read in array into a vector (inside a shared pointer)
      boost::shared_ptr<std::vector<double> > timeChannelsVec
                          (new std::vector<double>(timeChannels, timeChannels + lengthIn));

      int* spectrum = new int[lengthIn];

      int total_specs;
      if( interval || list)
      {
        total_specs = spec_list.size();
        if (interval)
        {
          total_specs += (spec_max-spec_min+1);
          spec_max += 1;
        }
      }
      else
      {
        total_specs = numberOfSpectra;
        // In this case want all the spectra, but zeroth spectrum is garbage so go from 1 to NSP1
        spec_min = 1;
        spec_max = numberOfSpectra + 1;
      }

      // Create the 2D workspace for the output
      m_localWorkspace = boost::dynamic_pointer_cast<DataObjects::Workspace2D>
        (API::WorkspaceFactory::Instance().create("Workspace2D",total_specs,lengthIn,lengthIn-1));
      // Set the unit on the workspace to TOF
      m_localWorkspace->getAxis(0)->unit() = UnitFactory::Instance().create("TOF");
      
      int counter = 0;
      for (int i = spec_min; i < spec_max; ++i)
      {
        loadData(timeChannelsVec,counter,i,iraw,lengthIn,spectrum);
        counter++;
      }
      if (list)
      {
        for(unsigned int i=0; i < spec_list.size(); ++i)
        {
          loadData(timeChannelsVec,counter,spec_list[i],iraw,lengthIn,spectrum);
          counter++;
        }
      }
      // Just a sanity check
      assert(counter == total_specs);
      
      // Assign the result to the output workspace property
      setProperty("OutputWorkspace",m_localWorkspace);

      // Clean up
      delete[] timeChannels;
      delete[] spectrum;

      // Run the sub-algorithms (LoadInstrument & LoadRaw)
      runLoadInstrument();
      runLoadMappingTable();
      runLoadLog();

      return;
    }

    /** Read in a single spectrum from the raw file
     *  @param tcbs     The vector containing the time bin boundaries
     *  @param hist     The workspace index
     *  @param i        The spectrum number
     *  @param iraw     A reference to the ISISRAW object
     *  @param lengthIn The number of elements in a spectrum
     *  @param spectrum Pointer to the array into which the spectrum will be read
     */
    void LoadRaw::loadData(const DataObjects::Histogram1D::RCtype::ptr_type& tcbs,int hist, int& i, ISISRAW& iraw, const int& lengthIn, int* spectrum)
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
      m_localWorkspace->setData(hist, v, e);
      m_localWorkspace->setX(hist, tcbs);
      m_localWorkspace->setErrorHelper(hist,GaussianErrorHelper::Instance());
      m_localWorkspace->getAxis(1)->spectraNo(hist)= i;
      // NOTE: Raw numbers go straight into the workspace 
      //     - no account taken of bin widths/units etc.
    }

    /// Run the sub-algorithm LoadInstrument (or LoadInstrumentFromRaw)
    void LoadRaw::runLoadInstrument()
    {
      // Determine the search directory for XML instrument definition files (IDFs)
      std::string directoryName = Kernel::ConfigService::Instance().getString("instrumentDefinition.directory");      
      if ( directoryName.empty() ) directoryName = "../Instrument";  // This is the assumed deployment directory for IDFs

      std::string instrumentID = m_filename.substr(0,3);  // get the 1st 3 letters of filename part
      // force ID to upper case
      std::transform(instrumentID.begin(), instrumentID.end(), instrumentID.begin(), toupper);
      std::string fullPathIDF = directoryName + "/" + instrumentID + "_Definition.xml";

      Algorithm_sptr loadInst = createSubAlgorithm("LoadInstrument");
      loadInst->setPropertyValue("Filename", fullPathIDF);
      loadInst->setProperty<Workspace_sptr>("Workspace",m_localWorkspace);

      // Now execute the sub-algorithm. Catch and log any error, but don't stop.
      try
      {
        loadInst->execute();
      }
      catch (std::runtime_error& err)
      {
        g_log.information("Unable to successfully run LoadInstrument sub-algorithm");
      }

      // If loading instrument definition file fails, run LoadInstrumentFromRaw instead
      if ( ! loadInst->isExecuted() )
      {
        g_log.information() << "Instrument definition file not found. Attempt to load information about \n"
          << "the instrument from raw data file.\n";

        Algorithm_sptr loadInst = createSubAlgorithm("LoadInstrumentFromRaw");
        loadInst->setPropertyValue("Filename", m_filename);
        // Set the workspace property to be the same one filled above
        loadInst->setProperty<Workspace_sptr>("Workspace",m_localWorkspace);

        // Now execute the sub-algorithm. Catch and log any error, but don't stop.
        try
        {
          loadInst->execute();
        }
        catch (std::runtime_error& err)
        {
          g_log.error("Unable to successfully run LoadInstrumentFromRaw sub-algorithm");
        }

        if ( ! loadInst->isExecuted() ) g_log.error("No instrument definition loaded");
      }
    }
    
    /// Run the LoadMappingTable sub-algorithm to fill the SpectraToDetectorMap
    void LoadRaw::runLoadMappingTable()
    {
      // Now determine the spectra to detector map calling sub-algorithm LoadMappingTable
      // There is a small penalty in re-opening the raw file but nothing major. 
      Algorithm_sptr loadmap= createSubAlgorithm("LoadMappingTable");
      loadmap->setPropertyValue("Filename", m_filename);
      loadmap->setProperty<Workspace_sptr>("Workspace",m_localWorkspace);
      try
      {
        loadmap->execute();  
      }
      catch (std::runtime_error& err)
      {
    	  g_log.error("Unable to successfully execute LoadMappingTable sub-algorithm");
      }
      
      if ( ! loadmap->isExecuted() ) g_log.error("LoadMappingTable sub-algorithm is not executed");
    }

    /// Run the LoadLog sub-algorithm
    void LoadRaw::runLoadLog()
    {
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

      if ( ! loadLog->isExecuted() ) g_log.error("Unable to successfully run LoadLog sub-algorithm");
    }

    double LoadRaw::dblSqrt(double in)
    {
      return sqrt(in);
    }

  } // namespace DataHandling
} // namespace Mantid
