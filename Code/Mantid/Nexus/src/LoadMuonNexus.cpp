//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidNexus/LoadMuonNexus.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/FileValidator.h"
#include "MantidGeometry/Detector.h"

#include "Poco/Path.h"

#include <cmath>
#include <boost/shared_ptr.hpp>
#include "MantidNexus/MuonNexusReader.h"

namespace Mantid
{
  namespace NeXus
  {
    // Register the algorithm into the algorithm factory
    DECLARE_ALGORITHM(LoadMuonNexus)

    using namespace Kernel;
    using namespace API;

    // Initialise the logger
    Logger& LoadMuonNexus::g_log = Logger::get("LoadMuonNexus");

    /// Empty default constructor
    LoadMuonNexus::LoadMuonNexus() : 
      Algorithm(), m_filename(), m_numberOfSpectra(0), m_numberOfPeriods(0),
      m_list(false), m_interval(false), m_spec_list(), m_spec_min(0), m_spec_max(0)
    {}

    /// Initialisation method.
    void LoadMuonNexus::init()
    {
      std::vector<std::string> exts;
      exts.push_back("NXS");
      exts.push_back("nxs");
      declareProperty("Filename","",new FileValidator(exts));
      declareProperty(new WorkspaceProperty<DataObjects::Workspace2D>("OutputWorkspace","",Direction::Output));
      
      BoundedValidator<int> *mustBePositive = new BoundedValidator<int>();
      mustBePositive->setLower(0);
      declareProperty("spectrum_min",0, mustBePositive);
      declareProperty("spectrum_max",0, mustBePositive->clone());
	    
      declareProperty("auto_group",false);
  
      declareProperty(new ArrayProperty<int>("spectrum_list"));
    }

    /** Executes the algorithm. Reading in the file and creating and populating
     *  the output workspace
     * 
     *  @throw Exception::FileError If the Nexus file cannot be found/opened
     *  @throw std::invalid_argument If the optional properties are set to invalid values
     */
    void LoadMuonNexus::exec()
    {
      // Retrieve the filename from the properties
      m_filename = getPropertyValue("Filename");

      MuonNexusReader nxload;
      if (nxload.readFromFile(m_filename) != 0)
      {
        g_log.error("Unable to open file " + m_filename);
        throw Exception::FileError("Unable to open File:" , m_filename);  
      }

      // Read in the instrument name from the Nexus file
      m_instrument_name = nxload.getInstrumentName();
      // Read in the number of spectra in the Nexus file
      m_numberOfSpectra = nxload.t_nsp1;
      // Read the number of periods in this file
      m_numberOfPeriods = nxload.t_nper;
      // Need to extract the user-defined output workspace name
      Property *ws = getProperty("OutputWorkspace");
      std::string localWSName = ws->value();
      // If multiperiod, will need to hold the Instrument, Sample & SpectraDetectorMap for copying
      boost::shared_ptr<IInstrument> instrument;
//-      boost::shared_ptr<SpectraDetectorMap> specMap;
      boost::shared_ptr<Sample> sample;
      
      // Call private method to validate the optional parameters, if set
      checkOptionalProperties();
           
      // Read the number of time channels (i.e. bins) from the Nexus file
      const int channelsPerSpectrum = nxload.t_ntc1;
      // Read in the time bin boundaries 
      const int lengthIn = channelsPerSpectrum + 1;
      float* timeChannels = new float[lengthIn];
      nxload.getTimeChannels(timeChannels, lengthIn);
      // Put the read in array into a vector (inside a shared pointer)
      boost::shared_ptr<MantidVec> timeChannelsVec
                          (new MantidVec(timeChannels, timeChannels + lengthIn));

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
        // for nexus return all spectra
        m_spec_min = 0; // changed to 0 for NeXus, was 1 for Raw
        m_spec_max = m_numberOfSpectra;  // was +1?
      }

      // Loop over the number of periods in the Nexus file, putting each period in a separate workspace
      for (int period = 0; period < m_numberOfPeriods; ++period) {
        
        // Create the 2D workspace for the output
        DataObjects::Workspace2D_sptr localWorkspace = boost::dynamic_pointer_cast<DataObjects::Workspace2D>
                 (WorkspaceFactory::Instance().create("Workspace2D",total_specs,lengthIn,lengthIn-1));
        // Set the unit on the workspace to TOF
        localWorkspace->getAxis(0)->unit() = UnitFactory::Instance().create("TOF");
 
        int counter = 0;
        for (int i = m_spec_min; i < m_spec_max; ++i)
        {
          // Shift the histogram to read if we're not in the first period
          int histToRead = i + period*total_specs;
          loadData(timeChannelsVec,counter,histToRead,nxload,lengthIn-1,localWorkspace ); // added -1 for NeXus
          counter++;
        }
        // Read in the spectra in the optional list parameter, if set
        if (m_list)
        {
          for(unsigned int i=0; i < m_spec_list.size(); ++i)
          {
            loadData(timeChannelsVec,counter,m_spec_list[i],nxload,lengthIn-1, localWorkspace );
            counter++;
          }
        }
        // Just a sanity check
        assert(counter == total_specs);
      
        std::string outputWorkspace = "OutputWorkspace";
        if (period == 0)
        {
          // Only run the sub-algorithms once
          runLoadInstrument(localWorkspace );
//-          runLoadMappingTable(localWorkspace );
          runLoadLog(localWorkspace );
          // Cache these for copying to workspaces for later periods
          instrument = localWorkspace->getInstrument();
//-          specMap = localWorkspace->getSpectraMap();
          sample = localWorkspace->getSample();
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
//-          localWorkspace->setSpectraMap(specMap);
          localWorkspace->setSample(sample);
        }
        
      bool autogroup = getProperty("auto_group");
      
      if (autogroup)
     {

          //Get the groupings
          for (int i =0; i < nxload.numDetectors; ++i)
          {
                  m_groupings.push_back(nxload.detectorGroupings[i]);
          }
	  	  
	    //Create a workspace with only two spectra for forward and back
	    DataObjects::Workspace2D_sptr  groupedWS 
		= boost::dynamic_pointer_cast<DataObjects::Workspace2D>
                 (API::WorkspaceFactory::Instance().create(localWorkspace, 2, localWorkspace->dataX(0).size(), localWorkspace->blocksize()));
	  
	  int numHists = localWorkspace->getNumberHistograms();
	  
	  //Compile the groups
	    for (int i = 0; i < numHists; ++i)
	    {    
		    for (int j = 0; j < localWorkspace->blocksize(); ++j)
			{
				groupedWS->dataY(m_groupings[numHists*period + i] -1)[j] = groupedWS->dataY(m_groupings[numHists*period + i] -1)[j] + localWorkspace->dataY(i)[j];
				
				//Add the errors in quadrature
				groupedWS->dataE(m_groupings[numHists*period + i] -1)[j] 
					= sqrt(pow(groupedWS->dataE(m_groupings[numHists*period + i] -1)[j], 2) + pow(localWorkspace->dataE(i)[j], 2));
			}
						
			//Copy all the X data
			groupedWS->dataX(m_groupings[numHists*period + i] -1) = localWorkspace->dataX(i);
	    }
	    
	    m_groupings.clear();
	    
	    //Copy other information
	    groupedWS->setInstrument(instrument);
	    groupedWS->setSample(sample);
	    	    
	    // Assign the result to the output workspace property
	    setProperty(outputWorkspace,groupedWS);
      }
      else
      {
         // Assign the result to the output workspace property
        setProperty(outputWorkspace,localWorkspace);
      }
      
      } // loop over periods
      
      // Clean up
      delete[] timeChannels;
    }

    /// Validates the optional 'spectra to read' properties, if they have been set
    void LoadMuonNexus::checkOptionalProperties()
    {
      Property *specList = getProperty("spectrum_list");
      m_list = !(specList->isDefault());
      Property *specMax = getProperty("spectrum_max");
      m_interval = !(specMax->isDefault());
      
      // If a multiperiod dataset, ignore the optional parameters (if set) and print a warning
      if ( m_numberOfPeriods > 1)
      {
        if ( m_list || m_interval )
        {
          m_list = false;
          m_interval = false;
          g_log.warning("Ignoring spectrum properties in this multiperiod dataset");
        }
      }

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
    
    /** Load in a single spectrum taken from a NeXus file
     *  @param tcbs     The vector containing the time bin boundaries
     *  @param hist     The workspace index
     *  @param i        The spectrum number
     *  @param nxload   A reference to the MuonNeXusReader object
     *  @param lengthIn The number of elements in a spectrum
     *  @param localWorkspace A pointer to the workspace in which the data will be stored
     */
    void LoadMuonNexus::loadData(const DataObjects::Histogram1D::RCtype::ptr_type& tcbs,int hist, int& i,
               MuonNexusReader& nxload, const int& lengthIn, DataObjects::Workspace2D_sptr localWorkspace)
    {
      // Read in a spectrum
      // Put it into a vector, discarding the 1st entry, which is rubbish
      // But note that the last (overflow) bin is kept
      // For Nexus, not sure if above is the case, hence give all data for now
      MantidVec& Y = localWorkspace->dataY(hist);
      Y.assign(nxload.counts + i * lengthIn, nxload.counts + i * lengthIn + lengthIn);
      // Create and fill another vector for the errors, containing sqrt(count)
      MantidVec& E = localWorkspace->dataE(hist);
      std::transform(Y.begin(), Y.end(), E.begin(), dblSqrt);
      // Populate the workspace. Loop starts from 1, hence i-1
      localWorkspace->setX(hist, tcbs);
      localWorkspace->getAxis(1)->spectraNo(hist)= i;
    }

    /// Run the sub-algorithm LoadInstrument (or LoadInstrumentFromNexus)
    void LoadMuonNexus::runLoadInstrument(DataObjects::Workspace2D_sptr localWorkspace)
    {
      // Determine the search directory for XML instrument definition files (IDFs)
      std::string directoryName = Kernel::ConfigService::Instance().getString("instrumentDefinition.directory");      
      if ( directoryName.empty() )
      {
        // This is the assumed deployment directory for IDFs, where we need to be relative to the
        // directory of the executable, not the current working directory.
        directoryName = Poco::Path(Mantid::Kernel::ConfigService::Instance().getBaseDir()).resolve("../Instrument").toString();  
      }
      //const int stripPath = m_filename.find_last_of("\\/");
      // For Nexus, Instrument name given by MuonNexusReader from Nexus file
      std::string instrumentID = m_instrument_name; //m_filename.substr(stripPath+1,3);  // get the 1st 3 letters of filename part
      // force ID to upper case
      std::transform(instrumentID.begin(), instrumentID.end(), instrumentID.begin(), toupper);
      std::string fullPathIDF = directoryName + "/" + instrumentID + "_Definition.xml";
      
      IAlgorithm_sptr loadInst = createSubAlgorithm("LoadInstrument");
      loadInst->setPropertyValue("Filename", fullPathIDF);
      loadInst->setProperty<MatrixWorkspace_sptr>("Workspace",localWorkspace);

      // Now execute the sub-algorithm. Catch and log any error, but don't stop.
      try
      {
        loadInst->execute();
      }
      catch (std::runtime_error&)
      {
        g_log.information("Unable to successfully run LoadInstrument sub-algorithm");
      }

      // If loading instrument definition file fails, run LoadInstrumentFromNexus instead
      // This does not work at present as the example files do not hold the necessary data
      // but is a place holder. Hopefully the new version of Nexus Muon files should be more
      // complete.
      if ( ! loadInst->isExecuted() )
      {
        runLoadInstrumentFromNexus(localWorkspace);
      }
    }
    
    /// Run LoadInstrumentFromNexus as a sub-algorithm (only if loading from instrument definition file fails)
    void LoadMuonNexus::runLoadInstrumentFromNexus(DataObjects::Workspace2D_sptr localWorkspace)
    {
      g_log.information() << "Instrument definition file not found. Attempt to load information about \n"
        << "the instrument from nexus data file.\n";

      IAlgorithm_sptr loadInst = createSubAlgorithm("LoadInstrumentFromNexus");
      loadInst->setPropertyValue("Filename", m_filename);
      // Set the workspace property to be the same one filled above
      loadInst->setProperty<MatrixWorkspace_sptr>("Workspace",localWorkspace);

      // Now execute the sub-algorithm. Catch and log any error, but don't stop.
      try
      {
        loadInst->execute();
      }
      catch (std::runtime_error&)
      {
        g_log.error("Unable to successfully run LoadInstrumentFromNexus sub-algorithm");
      }

      if ( ! loadInst->isExecuted() ) g_log.error("No instrument definition loaded");      
    }
//-    
//-    /// Run the LoadMappingTable sub-algorithm to fill the SpectraToDetectorMap
//-    void LoadMuonNexus::runLoadMappingTable(DataObjects::Workspace2D_sptr localWorkspace)
//-    {
//-      // Now determine the spectra to detector map calling sub-algorithm LoadMappingTable
//-      // There is a small penalty in re-opening the raw file but nothing major. 
//-      Algorithm_sptr loadmap= createSubAlgorithm("LoadMappingTable");
//-      loadmap->setPropertyValue("Filename", m_filename);
//-      loadmap->setProperty<Workspace_sptr>("Workspace",localWorkspace);
//-      try
//-      {
//-        loadmap->execute();  
//-      }
//-      catch (std::runtime_error& err)
//-      {
//-          g_log.error("Unable to successfully execute LoadMappingTable sub-algorithm");
//-      }
//-      
//-      if ( ! loadmap->isExecuted() ) g_log.error("LoadMappingTable sub-algorithm is not executed");
//-    }

    /// Run the LoadLog sub-algorithm
    void LoadMuonNexus::runLoadLog(DataObjects::Workspace2D_sptr localWorkspace)
    {
      IAlgorithm_sptr loadLog = createSubAlgorithm("LoadMuonLog");
      // Pass through the same input filename
      loadLog->setPropertyValue("Filename",m_filename);
      // Set the workspace property to be the same one filled above
      loadLog->setProperty<MatrixWorkspace_sptr>("Workspace",localWorkspace);

      // Now execute the sub-algorithm. Catch and log any error, but don't stop.
      try
      {
        loadLog->execute();
      }
      catch (std::runtime_error&)
      {
        g_log.error("Unable to successfully run LoadLog sub-algorithm");
      }

      if ( ! loadLog->isExecuted() ) g_log.error("Unable to successfully run LoadLog sub-algorithm");
    }

    double LoadMuonNexus::dblSqrt(double in)
    {
      return sqrt(in);
    }

  } // namespace DataHandling
} // namespace Mantid
