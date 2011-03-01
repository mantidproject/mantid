//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidDataHandling/LoadDAE.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/MandatoryValidator.h"
#include "MantidAPI/SpectraDetectorMap.h"
#include "MantidDataHandling/LoadInstrumentHelper.h"
#include "MantidKernel/DateAndTime.h"

#include <cmath>
#include <boost/shared_array.hpp>
#include <Poco/Path.h>
#include <Poco/Thread.h>


#include "LoadDAE/idc.h"

namespace Mantid
{
  namespace DataHandling
  {
    // Register the algorithm into the algorithm factory
    DECLARE_ALGORITHM(LoadDAE)

    using namespace Kernel;
    using namespace API;

    // Initialise the logger
    Logger& LoadDAE::g_StaticLog = Logger::get("LoadDAE-IDC");

    /// Empty default constructor
    LoadDAE::LoadDAE() :
      Algorithm(), m_daename(""), m_numberOfSpectra(0), m_numberOfPeriods(0),
      m_list(false), m_interval(false), m_spec_list(), m_spec_min(0), m_spec_max(Mantid::EMPTY_INT()),m_firstRun(true)
    {}


    /// load data from the DAE
    void LoadDAE::loadData(const MantidVecPtr::ptr_type& tcbs,int hist, int& ispec, idc_handle_t dae_handle, const int& lengthIn,
        int* spectrum, DataObjects::Workspace2D_sptr localWorkspace, int* allData)
    {
      int ndims, dims_array[1];
      ndims = 1;
      dims_array[0] = lengthIn;

      int *data = 0;

      if (allData) data = allData + ispec*lengthIn;
      else
      {
        // Read in spectrum number ispec from DAE
        if (IDCgetdat(dae_handle, ispec, 1, spectrum, dims_array, &ndims) != 0)
        {
          g_log.error("Unable to read DATA from DAE " + m_daename);
          throw Exception::FileError("Unable to read DATA from DAE " , m_daename);
        }
        data = spectrum;
      }

      // Put it into a vector, discarding the 1st entry, which is rubbish
      // But note that the last (overflow) bin is kept
      MantidVec& Y = localWorkspace->dataY(hist);
      Y.assign(data + 1, data + lengthIn);
      // Create and fill another vector for the errors, containing sqrt(count)
      MantidVec& E = localWorkspace->dataE(hist);
      std::transform(Y.begin(), Y.end(), E.begin(), LoadDAE::dblSqrt);
      localWorkspace->setX(hist, tcbs);
    }

    /// Initialisation method.
    void LoadDAE::init()
    {
      //this->setWikiSummary("Loads data from the ISIS DATA acquisition system and stores it in a 2D [[workspace]] ([[Workspace2D]] class).");
      //this->setOptionalMessage("Loads data from the ISIS DATA acquisition system and stores it in a 2D workspace (Workspace2D class).");

      declareProperty("DAEname","", new MandatoryValidator<std::string>(),
        "The name of and path to the input DAE host.");

      declareProperty(new WorkspaceProperty<Workspace>("OutputWorkspace",
        "",Direction::Output),
        "The name of the workspace that will be created, filled with the\n"
        "read-in data and stored in the Analysis Data Service.  If the\n"
        "input data contain multiple periods higher periods will be\n"
        "stored in separate workspaces called OutputWorkspace_PeriodNo.");


      BoundedValidator<int> *mustBePositive = new BoundedValidator<int>();
      mustBePositive->setLower(0);
      declareProperty("SpectrumMin", 0, mustBePositive,
        "The number of the first spectrum to read (default 0).  Only used\n"
        "if spectrum_max is set and is not available for multiperiod data\n"
        "files.");
      declareProperty("SpectrumMax", Mantid::EMPTY_INT(), mustBePositive->clone(),
        "The number of the last spectrum to read.  Only used if explicitly\n"
        "set and is not available for multiperiod data files. ");
      declareProperty(new ArrayProperty<int>("SpectrumList"),
        "A comma-separated list of individual spectra to read.  Only used\n"
        "if explicitly set. Not available for multiperiod data files.");

      declareProperty("UpdateRate",0, mustBePositive->clone());
    }

    /** Function called by IDC routines to report an error. Passes the error through to the logger
     * @param status ::  The status code of the error (disregarded)
     * @param code ::    The error code (disregarded)
     * @param message :: The error message - passed to the logger at error level
     */
    void LoadDAE::IDCReporter(int status, int code, const char* message)
    {
      (void) status; (void) code; // Avoid compiler warning
      g_StaticLog.error(message);
    }

    /** Overwrites Algorithm method. If UpdateRate is set calls repeatedly loadDAE()until canceled.
     */
    void LoadDAE::exec()
    {
      // Launch UpdateDAE algorithm if UpdateRate > 0
      int rate = getProperty("UpdateRate");
      rate *= 1000;
      if (rate > 0)
      {
        Poco::Thread *thread = Poco::Thread::current();
        if (thread == 0)
        {
          g_log.error("Cannot execute UpdateDAE in the main thread.");
          throw std::runtime_error("Cannot execute UpdateDAE in the main thread.");
        }
        for(;;)
        {
          try
          {
            loadDAE();
            m_firstRun = false;
          }
          catch(CancelException&)
          {// it is the normal way of ending this algorithm
            try
            {
              // to end the algorithm normally restore the output workspace properties from the last iteration
              Workspace_sptr ws = AnalysisDataService::Instance().retrieve(getPropertyValue("OutputWorkspace"));
              WorkspaceGroup_sptr wsg = boost::dynamic_pointer_cast<WorkspaceGroup>(ws);
              if (wsg)
              {
                std::vector<std::string> wsNames = wsg->getNames();
                std::vector<std::string>::const_iterator it = wsNames.begin();
                for(;it != wsNames.end(); ++it)
                {
                  Workspace_sptr ws1 = AnalysisDataService::Instance().retrieve(*it);
                  std::stringstream propName;
                  propName << "OutputWorkspace";
                  {
                    propName << '_' << it - wsNames.begin();
                  }
                  setProperty(propName.str(),ws1);
                }
              }
              else
              {
                setProperty("OutputWorkspace",ws);
              }
            }
            catch(...)
            {
              // user canceled at the first run or deleted the output during sleep time
              throw ;//ex;
            }
            return;
          }
          // store the output workspace even if the algorithm has not finished
          const std::vector< Property*>& props = getProperties();
          for (unsigned int i = 0; i < props.size(); ++i)
          {
            IWorkspaceProperty *wsProp = dynamic_cast<IWorkspaceProperty*>(props[i]);
            if (wsProp)
            {
              try
              {
                wsProp->store();
              }
              catch (std::runtime_error&)
              {
                g_log.error("Error storing output workspace in AnalysisDataService");
                throw;
              }
            }
          }
          thread->sleep(rate);
        }
      }
      else
      {
        loadDAE();
      }
    }

    /** Executes the algorithm. Reading in the file and creating and populating
     *  the output workspace
     *
     *  @throw Exception::FileError If the DAE cannot be found/opened
     *  @throw std::invalid_argument If the optional properties are set to invalid values
     */
    void LoadDAE::loadDAE()
    {
      int sv_dims_array[1] = { 1 }, sv_ndims = 1;   // used for rading single values with IDC routines
      int dims_array[2];
      // Retrieve the filename from the properties
      m_daename = getPropertyValue("DAEname");

      idc_handle_t dae_handle;

      // set IDC reporter function for errors
      IDCsetreportfunc(&LoadDAE::IDCReporter);

      if (IDCopen(m_daename.c_str(), 0, 0, &dae_handle) != 0)
      {
        g_log.error("Unable to open DAE " + m_daename);
        throw Exception::FileError("Unable to open DAE:" , m_daename);
      }

      // Read in the number of spectra in the DAE
      IDCgetpari(dae_handle, "NSP1", &m_numberOfSpectra, sv_dims_array, &sv_ndims);
      // Read the number of periods
      IDCgetpari(dae_handle, "NPER", &m_numberOfPeriods, sv_dims_array, &sv_ndims);
      // Need to extract the user-defined output workspace name
      Property *ws = getProperty("OutputWorkspace");
      std::string localWSName = ws->value();

      // Call private method to validate the optional parameters, if set
      checkOptionalProperties();

      // Read the number of time channels (i.e. bins) from the RAW file
      if (IDCgetpari(dae_handle, "NTC1", &m_channelsPerSpectrum, sv_dims_array, &sv_ndims) != 0)
      {
        g_log.error("Unable to read NTC1 from DAE " + m_daename);
        throw Exception::FileError("Unable to read NTC1 from DAE " , m_daename);
      }

      // Read in the time bin boundaries
      const int lengthIn = m_channelsPerSpectrum + 1;
      boost::shared_array<float> timeChannels(new float[lengthIn]);

      dims_array[0] = lengthIn;
      if (IDCgetparr(dae_handle, "RTCB1", timeChannels.get(), dims_array, &sv_ndims) != 0)
      {
        g_log.error("Unable to read RTCB1 from DAE " + m_daename);
        throw Exception::FileError("Unable to read RTCB1 from DAE " , m_daename);
      }
      // Put the read in array into a vector (inside a shared pointer)
      boost::shared_ptr<MantidVec> timeChannelsVec
                          (new MantidVec(timeChannels.get(), timeChannels.get() + lengthIn));
      // Create an array to hold the read-in data
      boost::shared_array<int> spectrum(new int[lengthIn]);

      // Read the instrument name
      char* iName = 0;
      dims_array[0] = 4;
      if (IDCAgetparc(dae_handle, "NAME", &iName, dims_array, &sv_ndims) != 0)
      {
          g_log.error("Unable to read NAME from DAE " + m_daename);
          throw Exception::FileError("Unable to read NAME from DAE " , m_daename);
      };

      // Read the proton charge
      float rpb[32];
      dims_array[0] = 32;
      if (IDCgetparr(dae_handle, "RRPB", rpb, dims_array, &sv_ndims) != 0)
      {
          g_log.error("Unable to read RRPB from DAE " + m_daename);
          throw Exception::FileError("Unable to read RRPB from DAE " , m_daename);
      };
      m_proton_charge = rpb[8];

      // Calculate the size of a workspace, given its number of periods & spectra to read
      long total_specs;
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

      // Decide if we can read in all the data at once
      boost::shared_array<int> allData;
      int ndata = (m_numberOfSpectra+1)*(m_channelsPerSpectrum+1)*m_numberOfPeriods*4;
      if (ndata/1000000 < 10) // arbitrary number
      {
        dims_array[0] = ndata;
        allData.reset(new int[ ndata ]);
        // and read them in
        int ret = IDCgetpari(dae_handle, "CNT1", allData.get(), dims_array, &sv_ndims);
        if (ret < 0)
        {
          allData.reset();
          throw std::runtime_error("");
        }
      }

      // Create the 2D workspace for the output
      DataObjects::Workspace2D_sptr localWorkspace = boost::dynamic_pointer_cast<DataObjects::Workspace2D>
        (WorkspaceFactory::Instance().create("Workspace2D",total_specs,lengthIn,lengthIn-1)); 

      // workspace group added to handle  multi periods
      WorkspaceGroup_sptr wsGrpSptr=WorkspaceGroup_sptr(new WorkspaceGroup);
      if(m_numberOfPeriods>1)
      {   
         setProperty("OutputWorkspace",boost::dynamic_pointer_cast<Workspace>(wsGrpSptr));
      }

      // Set the unit on the workspace to TOF
      localWorkspace->getAxis(0)->unit() = UnitFactory::Instance().create("TOF");

      loadSpectraMap(dae_handle, localWorkspace);

      int histTotal = total_specs * m_numberOfPeriods;
      int histCurrent = -1;
      // Loop over the number of periods in the raw file, putting each period in a separate workspace
      for (int period = 0; period < m_numberOfPeriods; ++period) {

        if ( period > 0 )
        {
            localWorkspace =  boost::dynamic_pointer_cast<DataObjects::Workspace2D>
                (WorkspaceFactory::Instance().create(localWorkspace));
            //localWorkspace->newInstrumentParameters(); ????
        }

        int counter = 0;
        for (int i = m_spec_min; i < m_spec_max; ++i)
        {
          // Shift the histogram to read if we're not in the first period
          int histToRead = i + period*(total_specs+1);
          loadData(timeChannelsVec,counter,histToRead,dae_handle,lengthIn,spectrum.get(),localWorkspace,allData.get() );
          localWorkspace->getAxis(1)->spectraNo(counter)= i;
          counter++;
          if (++histCurrent % 10 == 0) progress(double(histCurrent)/histTotal);
          interruption_point();
        }
        // Read in the spectra in the optional list parameter, if set
        if (m_list)
        {
          for(unsigned int i=0; i < m_spec_list.size(); ++i)
          {
            loadData(timeChannelsVec,counter,m_spec_list[i],dae_handle,lengthIn,spectrum.get(), localWorkspace,allData.get() );
            localWorkspace->getAxis(1)->spectraNo(counter)= i;
            counter++;
            if (++histCurrent % 10 == 0) progress(double(histCurrent)/histTotal);
            interruption_point();
          }
        }

        // Just a sanity check
        assert(counter == total_specs);

        std::string outputWorkspace = "OutputWorkspace";
        if (period == 0)
        {
          // Only run the sub-algorithms once
          runLoadInstrument(localWorkspace, iName);
          //runLoadLog(localWorkspace );
          // Set the total proton charge for this run
          localWorkspace->mutableRun().setProtonCharge(m_proton_charge);
        }
        if(m_numberOfPeriods>1)
        {
          std::stringstream suffix;
          suffix << (period+1);
          std::string outws("");
          outws=outputWorkspace+"_"+suffix.str();
          std::string WSName = localWSName + "_" + suffix.str();
          if (m_firstRun)
          {
            declareProperty(new WorkspaceProperty<Workspace>(outws,WSName,Direction::Output));
          }
          g_log.information() << "Workspace " << WSName << " created. \n";
          if(wsGrpSptr)wsGrpSptr->add(WSName);
          // Assign the result to the output workspace property
          setProperty(outws,boost::dynamic_pointer_cast<Workspace>(localWorkspace));
        }
        else 
          setProperty(outputWorkspace,boost::dynamic_pointer_cast<Workspace>(localWorkspace));

      } // loop over periods

      if (IDCclose(&dae_handle) != 0)
      {
        g_log.error("Unable to close DAE " + m_daename);
        throw Exception::FileError("Unable to close DAE:" , m_daename);
      }
      // Can't delete this here - it's allocated with malloc deep in the C code.
      // Just accept the small leak.
      //if (iName) delete[] iName;

    }

    /// Validates the optional 'spectra to read' properties, if they have been set
    void LoadDAE::checkOptionalProperties()
    {
      //read in the data supplied to the algorithm
      m_spec_list = getProperty("SpectrumList");
      m_spec_max = getProperty("SpectrumMax");
      //check that data
      m_list = !m_spec_list.empty();
      m_interval = !(m_spec_max == Mantid::EMPTY_INT());
      if ( m_spec_max == Mantid::EMPTY_INT() ) m_spec_max = 0;

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
        m_spec_list = getProperty("SpectrumList");
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
        m_spec_min = getProperty("SpectrumMin");
        if ( m_spec_max < m_spec_min || m_spec_max > m_numberOfSpectra )
        {
          g_log.error("Invalid Spectrum min/max properties");
          throw std::invalid_argument("Inconsistent properties defined");
        }
      }
    }


    /** Run the sub-algorithm LoadInstrument (or LoadInstrumentFromRaw).
     *  @param localWorkspace :: The workspace
     *  @param iName :: The instrument name
     */
    void LoadDAE::runLoadInstrument(DataObjects::Workspace2D_sptr localWorkspace, const char* iName)
    {
      // instrument name
      std::string instrumentID = iName; // get the instrument name
      size_t i = instrumentID.find_first_of(' '); // cut trailing spaces
      if (i != std::string::npos) instrumentID.erase(i);

      IAlgorithm_sptr loadInst = createSubAlgorithm("LoadInstrument");
      bool successfulExecution(true);
      // Now execute the sub-algorithm. Catch and log any error, but don't stop.
      try
      {
        loadInst->setPropertyValue("InstrumentName", instrumentID);
        loadInst->setProperty<MatrixWorkspace_sptr>("Workspace",localWorkspace);
        loadInst->execute();
      }
      catch(std::invalid_argument &)
      {
        successfulExecution = false;
        g_log.information("Invalid argument to LoadInstrument sub-algorithm");
      }
      catch (std::runtime_error&)
      {
        successfulExecution = false;
        g_log.information("Unable to successfully run LoadInstrument sub-algorithm");
      }

    }

    /** Populate spectra-detector map
        @param dae_handle :: The internal DAE identifier
        @param localWorkspace :: The workspace
     */
    void LoadDAE::loadSpectraMap(idc_handle_t dae_handle, DataObjects::Workspace2D_sptr localWorkspace)
    {
      // Read in the number of detectors
      int ndet, dims_array[1],sv_ndims = 1;
      dims_array[0] = 1;
      if (IDCgetpari(dae_handle, "NDET", &ndet, dims_array, &sv_ndims) != 0)
      {
          g_log.error("Unable to read NDET from DAE " + m_daename);
          throw Exception::FileError("Unable to read NDET from DAE " , m_daename);
      };

      boost::shared_array<int> udet(new int[ndet]);
      dims_array[0] = ndet;
      sv_ndims = 1;
      int res = 0;
      if ((res = IDCgetpari(dae_handle, "UDET", udet.get(), dims_array, &sv_ndims)) != 0)
      {
        g_log.error("Unable to read detector information (UDET) from DAE " + m_daename);
      }
      else
      {
        boost::shared_array<int> spec(new int[ndet]);
        if (IDCgetpari(dae_handle, "SPEC", spec.get(), dims_array, &sv_ndims) != 0)
        {
          g_log.error("Unable to read detector information (SPEC) from DAE " + m_daename);
          throw Exception::FileError("Unable to read detector information (SPEC) from DAE " , m_daename);
        }
        localWorkspace->mutableSpectraMap().populate(spec.get(), udet.get(), ndet);
      }

    }

    /**  Log the run details from the file
    * @param localWorkspace :: The workspace details to use
    */
    void LoadDAE::loadRunDetails(DataObjects::Workspace2D_sptr localWorkspace)
    {
      API::Run & runDetails = localWorkspace->mutableRun();

      runDetails.addProperty("run_title", localWorkspace->getTitle());
 
      int numSpectra = localWorkspace->getNumberHistograms();
      runDetails.addProperty("nspectra", numSpectra);

      runDetails.addProperty("run_start", Kernel::DateAndTime::get_current_time().to_ISO8601_string());
    }

    double LoadDAE::dblSqrt(double in)
    {
      return sqrt(in);
    }

  } // namespace DataHandling
} // namespace Mantid
