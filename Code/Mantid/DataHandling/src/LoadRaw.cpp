//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidDataHandling/LoadRaw.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/FileValidator.h"

#include "Poco/Path.h"

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
    LoadRaw::LoadRaw() :
      Algorithm(), m_filename(), m_numberOfSpectra(0), m_numberOfPeriods(0),
      m_list(false), m_interval(false), m_spec_list(), m_spec_min(0), m_spec_max(0)
    {}

    /// Initialisation method.
    void LoadRaw::init()
    {
      // Extension checking is not case sensitive
      std::vector<std::string> exts;
      exts.push_back("raw");
      exts.push_back("sav");
      exts.push_back("s[0-9][0-9]");

      declareProperty("Filename","",new FileValidator(exts));
      declareProperty(new WorkspaceProperty<DataObjects::Workspace2D>("OutputWorkspace","",Direction::Output));

      BoundedValidator<int> *mustBePositive = new BoundedValidator<int>();
      mustBePositive->setLower(1);
      declareProperty("spectrum_min",1, mustBePositive);
      declareProperty("spectrum_max",1, mustBePositive->clone());

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
      m_numberOfSpectra = iraw.t_nsp1;
      // Read the number of periods in this file
      m_numberOfPeriods = iraw.t_nper;
      // Need to extract the user-defined output workspace name
      Property *ws = getProperty("OutputWorkspace");
      std::string localWSName = ws->value();

      // Call private method to validate the optional parameters, if set
      checkOptionalProperties();

      // Read the number of time channels (i.e. bins) from the RAW file
      const int channelsPerSpectrum = iraw.t_ntc1;
      // Read in the time bin boundaries
      const int lengthIn = channelsPerSpectrum + 1;
      float* timeChannels = new float[lengthIn];
      iraw.getTimeChannels(timeChannels, lengthIn);
      // Put the read in array into a vector (inside a shared pointer)
      boost::shared_ptr<std::vector<double> > timeChannelsVec
                          (new std::vector<double>(timeChannels, timeChannels + lengthIn));
      // Create an array to hold the read-in data
      int* spectrum = new int[lengthIn];

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

      // Create the 2D workspace for the output
      DataObjects::Workspace2D_sptr localWorkspace = boost::dynamic_pointer_cast<DataObjects::Workspace2D>
               (WorkspaceFactory::Instance().create("Workspace2D",total_specs,lengthIn,lengthIn-1));
      localWorkspace->getAxis(0)->unit() = UnitFactory::Instance().create("TOF");

      // Loop over the number of periods in the raw file, putting each period in a separate workspace
      for (int period = 0; period < m_numberOfPeriods; ++period) {

        if ( period > 0 ) localWorkspace =  boost::dynamic_pointer_cast<DataObjects::Workspace2D>
                                              (WorkspaceFactory::Instance().create(localWorkspace));

        int counter = 0;
        for (int i = m_spec_min; i < m_spec_max; ++i)
        {
          // Shift the histogram to read if we're not in the first period
          int histToRead = i + period*total_specs;
          loadData(timeChannelsVec,counter,histToRead,iraw,lengthIn,spectrum,localWorkspace );
          counter++;
          if (++histCurrent % 100 == 0) progress(double(histCurrent)/histTotal);
          interruption_point();
        }
        // Read in the spectra in the optional list parameter, if set
        if (m_list)
        {
          for(unsigned int i=0; i < m_spec_list.size(); ++i)
          {
            loadData(timeChannelsVec,counter,m_spec_list[i],iraw,lengthIn,spectrum, localWorkspace );
            counter++;
            if (++histCurrent % 100 == 0) progress(double(histCurrent)/histTotal);
            interruption_point();
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
          // Set the total proton charge for this run
          // (not sure how this works for multi_period files)
          localWorkspace->getSample()->setProtonCharge(iraw.rpb.r_gd_prtn_chrg);
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
        }

        // Assign the result to the output workspace property
        setProperty(outputWorkspace,localWorkspace);

      } // loop over periods

      // Clean up
      delete[] timeChannels;
      delete[] spectrum;
    }

    /// Validates the optional 'spectra to read' properties, if they have been set
    void LoadRaw::checkOptionalProperties()
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
      }*/

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
     *  @param iraw     A reference to the ISISRAW object
     *  @param lengthIn The number of elements in a spectrum
     *  @param spectrum Pointer to the array into which the spectrum will be read
     *  @param localWorkspace A pointer to the workspace in which the data will be stored
     */
    void LoadRaw::loadData(const DataObjects::Histogram1D::RCtype::ptr_type& tcbs,int hist, int& i, ISISRAW& iraw, const int& lengthIn, int* spectrum, DataObjects::Workspace2D_sptr localWorkspace)
    {
      // Read in a spectrum
      memcpy(spectrum, iraw.dat1 + i * lengthIn, lengthIn * sizeof(int));
      // Put it into a vector, discarding the 1st entry, which is rubbish
      // But note that the last (overflow) bin is kept
      MantidVec& Y = localWorkspace->dataY(hist);
      Y.assign(spectrum + 1, spectrum + lengthIn);
      // Create and fill another vector for the errors, containing sqrt(count)
      MantidVec& E = localWorkspace->dataE(hist);
      std::transform(Y.begin(), Y.end(), E.begin(), dblSqrt);
      // Populate the workspace. Loop starts from 1, hence i-1
      localWorkspace->setX(hist, tcbs);

      localWorkspace->getAxis(1)->spectraNo(hist)= i;
      // NOTE: Raw numbers go straight into the workspace
      //     - no account taken of bin widths/units etc.
    }

    /// Run the sub-algorithm LoadInstrument (or LoadInstrumentFromRaw)
    void LoadRaw::runLoadInstrument(DataObjects::Workspace2D_sptr localWorkspace)
    {
      // Determine the search directory for XML instrument definition files (IDFs)
      std::string directoryName = Kernel::ConfigService::Instance().getString("instrumentDefinition.directory");
      if ( directoryName.empty() )
      {
        // This is the assumed deployment directory for IDFs, where we need to be relative to the
        // directory of the executable, not the current working directory.
        directoryName = Poco::Path(Mantid::Kernel::ConfigService::Instance().getBaseDir()).resolve("../Instrument").toString();  
      }
      const int stripPath = m_filename.find_last_of("\\/");
      std::string instrumentID = m_filename.substr(stripPath+1,3);  // get the 1st 3 letters of filename part
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

      // If loading instrument definition file fails, run LoadInstrumentFromRaw instead
      if ( ! loadInst->isExecuted() )
      {
        runLoadInstrumentFromRaw(localWorkspace);
      }
    }

    /// Run LoadInstrumentFromRaw as a sub-algorithm (only if loading from instrument definition file fails)
    void LoadRaw::runLoadInstrumentFromRaw(DataObjects::Workspace2D_sptr localWorkspace)
    {
      g_log.information() << "Instrument definition file not found. Attempt to load information about \n"
        << "the instrument from raw data file.\n";

      IAlgorithm_sptr loadInst = createSubAlgorithm("LoadInstrumentFromRaw");
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
        g_log.error("Unable to successfully run LoadInstrumentFromRaw sub-algorithm");
      }

      if ( ! loadInst->isExecuted() ) g_log.error("No instrument definition loaded");
    }

    /// Run the LoadMappingTable sub-algorithm to fill the SpectraToDetectorMap
    void LoadRaw::runLoadMappingTable(DataObjects::Workspace2D_sptr localWorkspace)
    {
      // Now determine the spectra to detector map calling sub-algorithm LoadMappingTable
      // There is a small penalty in re-opening the raw file but nothing major.
      IAlgorithm_sptr loadmap= createSubAlgorithm("LoadMappingTable");
      loadmap->setPropertyValue("Filename", m_filename);
      loadmap->setProperty<MatrixWorkspace_sptr>("Workspace",localWorkspace);
      try
      {
        loadmap->execute();
      }
      catch (std::runtime_error&)
      {
    	  g_log.error("Unable to successfully execute LoadMappingTable sub-algorithm");
      }

      if ( ! loadmap->isExecuted() ) g_log.error("LoadMappingTable sub-algorithm is not executed");
    }

    /// Run the LoadLog sub-algorithm
    void LoadRaw::runLoadLog(DataObjects::Workspace2D_sptr localWorkspace)
    {
      IAlgorithm_sptr loadLog = createSubAlgorithm("LoadLog");
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

    double LoadRaw::dblSqrt(double in)
    {
      return sqrt(in);
    }

  } // namespace DataHandling
} // namespace Mantid
