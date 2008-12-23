//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidDataHandling/LoadRaw2.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/FileValidator.h"
#include "MantidAPI/MemoryManager.h"
#include "MantidDataHandling/ManagedRawFileWorkspace2D.h"

#include "LoadRaw/isisraw2.h"

#include <cmath>
#include <boost/shared_ptr.hpp>
#include <iostream>

namespace Mantid
{
  namespace DataHandling
  {
    // Register the algorithm into the algorithm factory
    DECLARE_ALGORITHM(LoadRaw2)

    using namespace Kernel;
    using namespace API;

    // Initialise the logger
    Logger& LoadRaw2::g_log = Logger::get("LoadRaw2");

    /// Empty default constructor
    LoadRaw2::LoadRaw2() : 
      Algorithm(), m_filename(), m_numberOfSpectra(0), m_numberOfPeriods(0),
      m_list(false), m_interval(false), m_spec_list(), m_spec_min(0), m_spec_max(0)
    {
        isisRaw = new ISISRAW2;
    }
    LoadRaw2::~LoadRaw2()
    {
        delete isisRaw;
    }

    /// Initialisation method.
    void LoadRaw2::init()
    {
      std::vector<std::string> exts;
      exts.push_back("RAW");
      exts.push_back("raw");		
      declareProperty("Filename","",new FileValidator(exts));
      declareProperty(new WorkspaceProperty<DataObjects::Workspace2D>("OutputWorkspace","",Direction::Output));
      
      BoundedValidator<int> *mustBePositive = new BoundedValidator<int>();
      mustBePositive->setLower(0);
      declareProperty("spectrum_min",0, mustBePositive);
      declareProperty("spectrum_max",0, mustBePositive->clone());
      
      declareProperty(new ArrayProperty<int>("spectrum_list"));
      m_cache_options.push_back("If slow");
      m_cache_options.push_back("Always");
      m_cache_options.push_back("Never");
      declareProperty("Cache","If slow",new ListValidator(m_cache_options));

    }

    /** Executes the algorithm. Reading in the file and creating and populating
     *  the output workspace
     * 
     *  @throw Exception::FileError If the RAW file cannot be found/opened
     *  @throw std::invalid_argument If the optional properties are set to invalid values
     */
    void LoadRaw2::exec()
    {
      // Retrieve the filename from the properties
      m_filename = getPropertyValue("Filename");

      //ISISRAW iraw(NULL);
      FILE* file = fopen(m_filename.c_str(),"rb");
      if (file == NULL)
      {
        g_log.error("Unable to open file " + m_filename);
        throw Exception::FileError("Unable to open File:" , m_filename);	  
      }
      isisRaw->ioRAW(file, true);
		
      // Read in the number of spectra in the RAW file
      m_numberOfSpectra = isisRaw->t_nsp1;
      // Read the number of periods in this file
      m_numberOfPeriods = isisRaw->t_nper;
      // Read the number of time channels (i.e. bins) from the RAW file 
      const int channelsPerSpectrum = isisRaw->t_ntc1;
      // Read in the time bin boundaries 
      const int lengthIn = channelsPerSpectrum + 1;

      // If there is not enough memory use ManagedRawFileWorkspace2D.
      if (MemoryManager::Instance().goForManagedWorkspace(m_numberOfSpectra,lengthIn,channelsPerSpectrum))
      {
          ManagedRawFileWorkspace2D *localWorkspace_ptr = new ManagedRawFileWorkspace2D;
          DataObjects::Workspace2D_sptr localWorkspace( dynamic_cast<DataObjects::Workspace2D*>(localWorkspace_ptr) );
          const std::string cache_option = getPropertyValue("Cache");
          int option = find(m_cache_options.begin(),m_cache_options.end(),cache_option) - m_cache_options.begin();
          progress(0.,"Reading raw file...");
          localWorkspace_ptr->setRawFile(m_filename,option);
          runLoadInstrument(localWorkspace );
          runLoadMappingTable(localWorkspace );
          runLoadLog(localWorkspace );
          localWorkspace->getSample()->setProtonCharge(isisRaw->rpb.r_gd_prtn_chrg);
          for (int i = 0; i < m_numberOfSpectra; ++i)
              localWorkspace->getAxis(1)->spectraNo(i)= i+1;
          setProperty("OutputWorkspace",localWorkspace);
          return;
      }

      float* timeChannels = new float[lengthIn];
      isisRaw->getTimeChannels(timeChannels, lengthIn);
      // Put the read in array into a vector (inside a shared pointer)
      boost::shared_ptr<std::vector<double> > timeChannelsVec
                          (new std::vector<double>(timeChannels, timeChannels + lengthIn));

      // Need to extract the user-defined output workspace name
      Property *ws = getProperty("OutputWorkspace");
      std::string localWSName = ws->value();
      // If multiperiod, will need to hold the Instrument, Sample & SpectraDetectorMap for copying
      boost::shared_ptr<IInstrument> instrument;
      boost::shared_ptr<SpectraDetectorMap> specMap;
      boost::shared_ptr<Sample> sample;
      
      // Call private method to validate the optional parameters, if set
      checkOptionalProperties();
            

      // Calculate the size of a workspace, given its number of periods & spectra to read
      int total_specs;
      if( m_interval || m_list)
      {
        total_specs = m_spec_list.size();
        if (m_interval)
        {
          total_specs += (m_spec_max-m_spec_min+1);
          m_spec_max += 1;
        }
      }
      else
      {
        total_specs = m_numberOfSpectra;
        // In this case want all the spectra, but zeroth spectrum is garbage so go from 1 to NSP1
        m_spec_min = 1;
        m_spec_max = m_numberOfSpectra + 1;
      }


      int histTotal = total_specs * m_numberOfPeriods;
      int histCurrent = -1;

      //if (dhdr.d_comp == 0) throw std::runtime_error("Oops..");
      // Loop over the number of periods in the raw file, putting each period in a separate workspace
      for (int period = 0; period < m_numberOfPeriods; ++period) {
        
        // Create the 2D workspace for the output
        DataObjects::Workspace2D_sptr localWorkspace = boost::dynamic_pointer_cast<DataObjects::Workspace2D>
                 (WorkspaceFactory::Instance().create("Workspace2D",total_specs,lengthIn,lengthIn-1));
        // Set the unit on the workspace to TOF
        localWorkspace->getAxis(0)->unit() = UnitFactory::Instance().create("TOF");
             

        isisRaw->skipData(period*(m_numberOfSpectra+1));
        int counter = 0;
        for (int i = 1; i <= m_numberOfSpectra; ++i)
        {
            int histToRead = i + period*(m_numberOfSpectra+1);
            if ((i >= m_spec_min && i < m_spec_max) || 
                (m_list && find(m_spec_list.begin(),m_spec_list.end(),i) != m_spec_list.end()))
            {
                isisRaw->readData(histToRead);
                // Put it into a vector, discarding the 1st entry, which is rubbish
                // But note that the last (overflow) bin is kept
                std::vector<double> v(isisRaw->dat1 + 1, isisRaw->dat1 + lengthIn);
                // Create and fill another vector for the errors, containing sqrt(count)
                std::vector<double> e(lengthIn-1);
                std::transform(v.begin(), v.end(), e.begin(), dblSqrt);
                // Populate the workspace. Loop starts from 1, hence i-1
                localWorkspace->setData(counter, v, e);
                localWorkspace->setX(counter, timeChannelsVec);
                localWorkspace->setErrorHelper(counter,GaussianErrorHelper::Instance());
                localWorkspace->getAxis(1)->spectraNo(counter)= i;
                // NOTE: Raw numbers go straight into the workspace 
                //     - no account taken of bin widths/units etc.
                ++counter;
                if (++histCurrent % 100 == 0) progress(double(histCurrent)/histTotal);
                interruption_point();
            }
            else
            {
                isisRaw->skipData(histToRead);
            }
        }

        // Just a sanity check
        assert(counter == total_specs);
      
        std::string outputWorkspace = "OutputWorkspace";
        if (period == 0)
        {
          // Only run the sub-algorithms once
          runLoadInstrument(localWorkspace );
          runLoadMappingTable(localWorkspace );
          runLoadLog(localWorkspace );
          // Cache these for copying to workspaces for later periods
          instrument = localWorkspace->getInstrument();
          specMap = localWorkspace->getSpectraMap();
          sample = localWorkspace->getSample();
          // Set the total proton charge for this run
          // (not sure how this works for multi_period files)
          sample->setProtonCharge(isisRaw->rpb.r_gd_prtn_chrg);
        }
        else   // We are working on a higher period of a multiperiod raw file
        {
          // Create a WorkspaceProperty for the new workspace of a higher period
          // The workspace name given in the OutputWorkspace property has _periodNumber appended to it
          //                (for all but the first period, which has no suffix)
          std::stringstream suffix;
          suffix << (period+1);
          outputWorkspace += suffix.str();
          std::string WSName = localWSName + "_" + suffix.str();
          declareProperty(new WorkspaceProperty<DataObjects::Workspace2D>(outputWorkspace,WSName,Direction::Output));
          g_log.information() << "Workspace " << WSName << " created. \n";
          // Copy the shared instrument, sample & spectramap onto the workspace for this period
          localWorkspace->setInstrument(instrument);
          localWorkspace->setSpectraMap(specMap);
          localWorkspace->setSample(sample);
        }
        
        // Assign the result to the output workspace property
        setProperty(outputWorkspace,localWorkspace);
        
      } // loop over periods
      
      // Clean up
      delete[] timeChannels;
      //delete[] spectrum;
      fclose(file);
    }

    /// Validates the optional 'spectra to read' properties, if they have been set
    void LoadRaw2::checkOptionalProperties()
    {
      Property *specList = getProperty("spectrum_list");
      m_list = !(specList->isDefault());
      Property *specMax = getProperty("spectrum_max");
      m_interval = !(specMax->isDefault());
      
      /*/ If a multiperiod dataset, ignore the optional parameters (if set) and print a warning
      if ( m_numberOfPeriods > 1)
      {
        if ( m_list || m_interval )
        {
          m_list = false;
          m_interval = false;
          g_log.warning("Ignoring spectrum properties in this multiperiod dataset");
        }
      }//*/

      // Check validity of spectra list property, if set
      if ( m_list )
      {
        m_list = true;
        m_spec_list = getProperty("spectrum_list");
        const int minlist = *min_element(m_spec_list.begin(),m_spec_list.end());
        const int maxlist = *max_element(m_spec_list.begin(),m_spec_list.end());
        if ( maxlist > m_numberOfSpectra || minlist == 0)
        {
          g_log.error("Invalid list of spectra");
          throw std::invalid_argument("Inconsistent properties defined"); 
        } 
      }
           
      // Check validity of spectra range, if set
      if ( m_interval )
      {
        m_interval = true;
        m_spec_min = getProperty("spectrum_min");
        m_spec_max = getProperty("spectrum_max");
        if ( m_spec_max < m_spec_min || m_spec_max > m_numberOfSpectra )
        {
          g_log.error("Invalid Spectrum min/max properties");
          throw std::invalid_argument("Inconsistent properties defined"); 
        }
      }
    }
    
    /** Read in a single spectrum from the raw file
     *  @param tcbs     The vector containing the time bin boundaries
     *  @param hist     The workspace index
     *  @param i        The spectrum number
     *  @param lengthIn The number of elements in a spectrum
     *  @param spectrum Pointer to the array into which the spectrum will be read
     *  @param localWorkspace A pointer to the workspace in which the data will be stored
     */
    void LoadRaw2::loadData(const DataObjects::Histogram1D::RCtype::ptr_type& tcbs,int hist, int& i, const int& lengthIn, int* spectrum, DataObjects::Workspace2D_sptr localWorkspace)
    {
      // Read in a spectrum
//      memcpy(spectrum, iraw.dat1 + i * lengthIn, lengthIn * sizeof(int));
      memcpy(spectrum, isisRaw->dat1 + i * lengthIn, lengthIn * sizeof(int));
      // Put it into a vector, discarding the 1st entry, which is rubbish
      // But note that the last (overflow) bin is kept
      std::vector<double> v(spectrum + 1, spectrum + lengthIn);
      // Create and fill another vector for the errors, containing sqrt(count)
      std::vector<double> e(lengthIn-1);
      std::transform(v.begin(), v.end(), e.begin(), dblSqrt);
      // Populate the workspace. Loop starts from 1, hence i-1
      localWorkspace->setData(hist, v, e);
      localWorkspace->setX(hist, tcbs);
      localWorkspace->setErrorHelper(hist,GaussianErrorHelper::Instance());
      localWorkspace->getAxis(1)->spectraNo(hist)= i;
      // NOTE: Raw numbers go straight into the workspace 
      //     - no account taken of bin widths/units etc.
    }

    /// Run the sub-algorithm LoadInstrument (or LoadInstrumentFromRaw)
    void LoadRaw2::runLoadInstrument(DataObjects::Workspace2D_sptr localWorkspace)
    {
      // Determine the search directory for XML instrument definition files (IDFs)
      std::string directoryName = Kernel::ConfigService::Instance().getString("instrumentDefinition.directory");      
      if ( directoryName.empty() ) directoryName = "../Instrument";  // This is the assumed deployment directory for IDFs

      const int stripPath = m_filename.find_last_of("\\/");
      std::string instrumentID = m_filename.substr(stripPath+1,3);  // get the 1st 3 letters of filename part
      // force ID to upper case
      std::transform(instrumentID.begin(), instrumentID.end(), instrumentID.begin(), toupper);
      std::string fullPathIDF = directoryName + "/" + instrumentID + "_Definition.xml";
      
      Algorithm_sptr loadInst = createSubAlgorithm("LoadInstrument");
      loadInst->setPropertyValue("Filename", fullPathIDF);
      loadInst->setProperty<Workspace_sptr>("Workspace",localWorkspace);

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
        runLoadInstrumentFromRaw(localWorkspace);
      }
    }
    
    /// Run LoadInstrumentFromRaw as a sub-algorithm (only if loading from instrument definition file fails)
    void LoadRaw2::runLoadInstrumentFromRaw(DataObjects::Workspace2D_sptr localWorkspace)
    {
      g_log.information() << "Instrument definition file not found. Attempt to load information about \n"
        << "the instrument from raw data file.\n";

      Algorithm_sptr loadInst = createSubAlgorithm("LoadInstrumentFromRaw");
      loadInst->setPropertyValue("Filename", m_filename);
      // Set the workspace property to be the same one filled above
      loadInst->setProperty<Workspace_sptr>("Workspace",localWorkspace);

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
    
    /// Run the LoadMappingTable sub-algorithm to fill the SpectraToDetectorMap
    void LoadRaw2::runLoadMappingTable(DataObjects::Workspace2D_sptr localWorkspace)
    {
      // Now determine the spectra to detector map calling sub-algorithm LoadMappingTable
      // There is a small penalty in re-opening the raw file but nothing major. 
      Algorithm_sptr loadmap= createSubAlgorithm("LoadMappingTable");
      loadmap->setPropertyValue("Filename", m_filename);
      loadmap->setProperty<Workspace_sptr>("Workspace",localWorkspace);
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
    void LoadRaw2::runLoadLog(DataObjects::Workspace2D_sptr localWorkspace)
    {
      Algorithm_sptr loadLog = createSubAlgorithm("LoadLog");
      // Pass through the same input filename
      loadLog->setPropertyValue("Filename",m_filename);
      // Set the workspace property to be the same one filled above
      loadLog->setProperty<Workspace_sptr>("Workspace",localWorkspace);

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

    double LoadRaw2::dblSqrt(double in)
    {
      return sqrt(in);
    }

  } // namespace DataHandling
} // namespace Mantid
