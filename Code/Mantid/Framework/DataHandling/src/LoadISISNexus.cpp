/*WIKI* 


*WIKI*/
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidDataHandling/LoadISISNexus.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/SpectraDetectorMap.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidKernel/LogParser.h"
#include "MantidGeometry/Instrument/Detector.h"
#include "MantidGeometry/Instrument/XMLlogfile.h"
#include "MantidKernel/BoundedValidator.h"

#include <Poco/Path.h>
#include <Poco/DateTimeFormatter.h>
#include <Poco/DateTimeParser.h>
#include <Poco/DateTimeFormat.h>

#include <boost/lexical_cast.hpp>
#include <cmath>
#include <sstream>
#include <cctype>

namespace Mantid
{
  namespace DataHandling
  {
    // Register the algorithm into the algorithm factory
    DECLARE_ALGORITHM(LoadISISNexus)

    using namespace Kernel;
    using namespace API;
    using Geometry::Instrument;
    using std::size_t;

    /// Empty default constructor
    LoadISISNexus::LoadISISNexus() : 
    Algorithm(), m_filename(), m_numberOfSpectra(0), m_numberOfPeriods(0),
      m_list(false), m_interval(false), m_spec_list(), m_spec_min(0),
      m_spec_max(Mantid::EMPTY_INT()), m_filehandle(NULL)
    {}

    /// Initialisation method.
    void LoadISISNexus::init()
    {
      std::vector<std::string> exts;
      exts.push_back(".nxs");
      exts.push_back(".n*");
      declareProperty(new FileProperty("Filename", "", FileProperty::Load, exts),
        "The name of the Nexus file to load" );
      declareProperty(new WorkspaceProperty<Workspace>("OutputWorkspace","",Direction::Output));

      auto mustBePositive = boost::make_shared<BoundedValidator<int> >();
      mustBePositive->setLower(0);
      declareProperty("SpectrumMin", 0, mustBePositive);
      declareProperty("SpectrumMax", Mantid::EMPTY_INT(), mustBePositive);
      declareProperty(new ArrayProperty<int>("SpectrumList"));
      declareProperty("EntryNumber", 0, mustBePositive,
        "The particular entry number to read (default: Load all workspaces and creates a workspace group)");
    }

    /** Executes the algorithm. Reading in the file and creating and populating
    *  the output workspace
    * 
    *  @throw Exception::FileError If the Nexus file cannot be found/opened
    *  @throw std::invalid_argument If the optional properties are set to invalid values
    */
    void LoadISISNexus::exec()
    {
      // Retrieve the filename from the properties
      m_filename = getPropertyValue("Filename");
      // Retrieve the entry number
      m_entrynumber = getProperty("EntryNumber");

      // Open NeXus file
      m_filehandle = new NeXus::File(m_filename);

      // Open the raw data group 'raw_data_1'
      m_filehandle->openGroup("raw_data_1","NXentry");

      // Read in the instrument name from the Nexus file
      m_instrument_name = getNexusString("name");

      m_filehandle->openGroup("detector_1","NXdata");

      readDataDimensions();
      if(m_entrynumber!=0)
      {
        if( static_cast<size_t>(m_entrynumber) > m_numberOfPeriods)
        {
          throw std::invalid_argument("Invalid Entry Number:Enter a valid number");
        }
        else
          m_numberOfPeriods=1;

      }

      const size_t lengthIn = m_numberOfChannels + 1;

      // Need to extract the user-defined output workspace name
      Property *ws = getProperty("OutputWorkspace");
      std::string localWSName = ws->value();
      // If multiperiod, will need to hold the Instrument, Sample & SpectraDetectorMap for copying
      boost::shared_ptr<Instrument> instrument;
      boost::shared_ptr<Sample> sample;

      // Call private method to validate the optional parameters, if set
      checkOptionalProperties();

      getTimeChannels();

      m_filehandle->closeGroup();// go back to raw_data_1

      // Calculate the size of a workspace, given its number of periods & spectra to read
      size_t total_specs;
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

      std::string outputWorkspace = "OutputWorkspace";

      WorkspaceGroup_sptr wsGrpSptr=WorkspaceGroup_sptr(new WorkspaceGroup);
      if(m_numberOfPeriods>1)
      { 
          setProperty(outputWorkspace,boost::dynamic_pointer_cast<Workspace>(wsGrpSptr));
      }


      // Create the 2D workspace for the output
      DataObjects::Workspace2D_sptr localWorkspace = boost::dynamic_pointer_cast<DataObjects::Workspace2D>
        (WorkspaceFactory::Instance().create("Workspace2D",total_specs,lengthIn,lengthIn-1));
      // Set the units on the workspace to TOF & Counts
      localWorkspace->getAxis(0)->unit() = UnitFactory::Instance().create("TOF");
      localWorkspace->setYUnit("Counts");
      const std::string title = getNexusString("title");
      localWorkspace->setTitle(title);

      Progress prog(this,0.,1.,total_specs*m_numberOfPeriods);
      // Loop over the number of periods in the Nexus file, putting each period in a separate workspace
      for (size_t period = 0; period < m_numberOfPeriods; ++period) {

        if(m_entrynumber!=0)
        {
          period=m_entrynumber-1;
          if(period!=0)
          {
            runLoadInstrument(localWorkspace );
            loadMappingTable(localWorkspace );
            loadRunDetails(localWorkspace);
            loadLogs(localWorkspace );
          }
        }
        if (period == 0)
        {
          // Only run the Child Algorithms once
          runLoadInstrument(localWorkspace );
          loadMappingTable(localWorkspace );
          loadRunDetails(localWorkspace);
          loadLogs(localWorkspace );
        }
        else   // We are working on a higher period of a multiperiod file
        {
          localWorkspace =  boost::dynamic_pointer_cast<DataObjects::Workspace2D>
            (WorkspaceFactory::Instance().create(localWorkspace));

          loadLogs(localWorkspace ,period);
        }

        m_filehandle->openGroup("detector_1","NXdata");

        size_t counter = 0;
        for (size_t i = static_cast<size_t>(m_spec_min); i < static_cast<size_t>(m_spec_max); ++i)
        {
          loadData(period, counter,i,localWorkspace ); // added -1 for NeXus
          counter++;
          prog.report();
        }
        // Read in the spectra in the optional list parameter, if set
        if (m_list)
        {
          for(size_t i=0; i < m_spec_list.size(); ++i)
          {
              loadData(period, counter, m_spec_list[i],localWorkspace );
            counter++;
            prog.report();
          }
        }

        m_filehandle->closeGroup();// go up from detector_1 to raw_data_1

        // Just a sanity check
        assert(counter == total_specs);

        std::string outws("");
        std::string outputWorkspace = "OutputWorkspace";
        if(m_numberOfPeriods>1)
        {

          std::stringstream suffix;
          suffix << (period+1);
          outws =outputWorkspace+"_"+suffix.str();
          std::string WSName = localWSName + "_" + suffix.str();
          declareProperty(new WorkspaceProperty<Workspace>(outws,WSName,Direction::Output));
          if(wsGrpSptr)wsGrpSptr->addWorkspace(localWorkspace);
          setProperty(outws,boost::static_pointer_cast<Workspace>(localWorkspace));
        }
        else
        {
          setProperty(outputWorkspace,boost::dynamic_pointer_cast<Workspace>(localWorkspace));
        }

        // Assign the result to the output workspace property
        // setProperty(outputWorkspace,localWorkspace);

      }

      // Close the Nexus file
      delete m_filehandle;
    }

    /// Validates the optional 'spectra to read' properties, if they have been set
    void LoadISISNexus::checkOptionalProperties()
    {
      //read in the data supplied to the algorithm
      m_spec_list = getProperty("SpectrumList");
      m_spec_max = getProperty("SpectrumMax");
      //now check that data
      m_interval = !( m_spec_max == Mantid::EMPTY_INT() );
      if ( m_spec_max == Mantid::EMPTY_INT() )
      {
          m_spec_max = 0;
      }

      m_list = !m_spec_list.empty();

      // If a multiperiod dataset, ignore the optional parameters (if set) and print a warning
      //if ( m_numberOfPeriods > 1)
      //{
      //  if ( m_list || m_interval )
      //  {
      //    m_list = false;
      //    m_interval = false;
      //    g_log.warning("Ignoring spectrum properties in this multiperiod dataset");
      //  }
      //}

      // Check validity of spectra list property, if set
      if ( m_list )
      {
        m_list = true;
        const size_t minlist = *min_element(m_spec_list.begin(),m_spec_list.end());
        const size_t maxlist = *max_element(m_spec_list.begin(),m_spec_list.end());
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
        if ( m_spec_max < m_spec_min || static_cast<size_t>(m_spec_max) > m_numberOfSpectra )
        {
          g_log.error("Invalid Spectrum min/max properties");
          throw std::invalid_argument("Inconsistent properties defined"); 
        }
      }
    }

    /** Reads in a string value from the nexus file
    *  @param name :: The name of the data value in the nexus file. The name can be either absolute or relative
    *  @return The string's value
    *  @throw std::runtime_error in case of any error
    */
    std::string LoadISISNexus::getNexusString(const std::string& name)const
    {
      m_filehandle->openData(name);
      std::string result = m_filehandle->getStrData();
      m_filehandle->closeData();
      return result;
    }

    /** Group /raw_data_1/detector_1 must be open to call this function
    */
    void LoadISISNexus::readDataDimensions()
    {
      m_filehandle->openData("counts");

      ::NeXus::Info info = m_filehandle->getInfo();

      // Number of time channels and number of spectra made public
      m_numberOfPeriods  = info.dims[0];
      m_numberOfSpectra  = info.dims[1];
      m_numberOfChannels = info.dims[2];

      // Allocate memory for the counts buffer
      m_data.reset( new int[m_numberOfChannels] );

      m_filehandle->closeData();
    }

    void LoadISISNexus::getTimeChannels()
    {
      boost::shared_array<float> timeChannels( new float[m_numberOfChannels + 1] );

      m_filehandle->openData("time_of_flight");

      ::NeXus::Info info = m_filehandle->getInfo();

      if (static_cast<int64_t>(m_numberOfChannels + 1) != info.dims[0])
      {
        g_log.error("Number of time channels does not match the data dimensions");
        throw std::runtime_error("Number of time channels does not match the data dimensions");
      }

      m_filehandle->getData(timeChannels.get());

      m_timeChannelsVec.reset(new MantidVec(timeChannels.get(), timeChannels.get() + m_numberOfChannels + 1));

      m_filehandle->closeData();
    }

    /** Group /raw_data_1/detector_1 must be open to call this function.
    *  loadMappingTable() must be done before any calls to this method.
    *  @param period :: The data period
    *  @param hist :: The index of the histogram in the workspace
    *  @param i :: The index of the histogram in the file
    *  @param localWorkspace :: The workspace
    */
      void LoadISISNexus::loadData(std::size_t period, std::size_t hist, std::size_t& i, DataObjects::Workspace2D_sptr localWorkspace)
    {
      m_filehandle->openData("counts");

      ::NeXus::Info info = m_filehandle->getInfo();
      std::vector<int64_t> start(3);
      std::vector<int64_t> size(3);
      start[0] = static_cast<int64_t>(period);
      start[1] = static_cast<int64_t>(i);
      start[2] = 0;

      size[0] = 1;
      size[1] = 1;
      size[2] = static_cast<int64_t>(m_numberOfChannels);

      m_filehandle->getSlab(m_data.get(), start, size);

      MantidVec& Y = localWorkspace->dataY(hist);
      Y.assign(m_data.get(), m_data.get() + m_numberOfChannels);
      // Create and fill another vector for the errors, containing sqrt(count)
      MantidVec& E = localWorkspace->dataE(hist);
      std::transform(Y.begin(), Y.end(), E.begin(), dblSqrt);
      // Populate the workspace. Loop starts from 1, hence i-1
      localWorkspace->setX(hist, m_timeChannelsVec);
      localWorkspace->getAxis(1)->spectraNo(hist)= m_spec[i];

      m_filehandle->closeData();
    }

    /// Run the Child Algorithm LoadInstrument (or LoadInstrumentFromNexus)
    void LoadISISNexus::runLoadInstrument(DataObjects::Workspace2D_sptr localWorkspace)
    {

      IAlgorithm_sptr loadInst = createChildAlgorithm("LoadInstrument");

      // Now execute the Child Algorithm. Catch and log any error, but don't stop.
      try
      {
        loadInst->setPropertyValue("InstrumentName", m_instrument_name);
        loadInst->setProperty<MatrixWorkspace_sptr> ("Workspace", localWorkspace);
        loadInst->setProperty("RewriteSpectraMap", false);
        loadInst->execute();
      }
      catch( std::invalid_argument&)
      {
        g_log.information("Invalid argument to LoadInstrument Child Algorithm");
      }
      catch (std::runtime_error&)
      {
        g_log.information("Unable to successfully run LoadInstrument Child Algorithm");
      }

      // If loading instrument definition file fails, run LoadInstrumentFromNexus instead
      // This does not work at present as the example files do not hold the necessary data
      // but is a place holder. Hopefully the new version of Nexus Muon files should be more
      // complete.
      //if ( ! loadInst->isExecuted() )
      //{
      //    runLoadInstrumentFromNexus(localWorkspace);
      //}
    }

    /**  Populate the ws's spectra-detector map. The Nexus file must be in /raw_data_1 group.
    *   After finishing the file remains in the same group (/raw_data_1 group is open).
    *   @param ws :: The workspace which spectra-detector map is to be populated.
    */
    void LoadISISNexus::loadMappingTable(DataObjects::Workspace2D_sptr ws)
    {
      ::NeXus::Info info;

      // Read in detector ids from isis compatibility section
      m_filehandle->openGroup("isis_vms_compat","IXvms");
      m_filehandle->openData("UDET");
      info = m_filehandle->getInfo();
      int64_t ndet = info.dims[0];
      boost::shared_array<detid_t> udet(new detid_t[ndet]);
      m_filehandle->getData(udet.get());
      m_filehandle->closeData();  // UDET
      m_filehandle->closeGroup(); // isis_vms_compat



      m_filehandle->openGroup("detector_1","NXdata");
      m_filehandle->openData("spectrum_index");
      info = m_filehandle->getInfo();
      if (info.dims[0] != ndet)
      {
        g_log.error("Cannot open load spectra-detector map: data sizes do not match.");
        throw std::runtime_error("Cannot open load spectra-detector map: data sizes do not match.");
      }
      m_spec.reset(new specid_t[ndet]);
      m_filehandle->getData(m_spec.get());
      m_filehandle->closeData(); // spectrum_index
      m_filehandle->closeGroup(); // detector_1

      //Populate the Spectra Map with parameters
      ws->replaceSpectraMap(new SpectraDetectorMap(m_spec.get(),udet.get(),ndet));
    }

    /**  Loag the run details from the file
    *   Group /raw_data_1 must be open.
    * @param localWorkspace :: The workspace details to use
    */
    void LoadISISNexus::loadRunDetails(DataObjects::Workspace2D_sptr localWorkspace)
    {
      API::Run & runDetails = localWorkspace->mutableRun();
      m_proton_charge = getNXData<double>("proton_charge");
      runDetails.setProtonCharge(m_proton_charge);
      int run_number = getNXData<int>("run_number");
      runDetails.addProperty("run_number", boost::lexical_cast<std::string>(run_number));


      m_filehandle->openGroup("isis_vms_compat","IXvms");
      char header[80];
      m_filehandle->openData("HDR");
      m_filehandle->getData(&header);
      m_filehandle->closeData();
      runDetails.addProperty("run_header", std::string(header,80));
      runDetails.addProperty("run_title", localWorkspace->getTitle(), true);

      runDetails.addProperty("nspectra", getNXData<int>("NSP1"));
      runDetails.addProperty("nchannels", getNXData<int>("NTC1"));
      runDetails.addProperty("nperiods", getNXData<int>("NPER"));

      int rpb_int[32];
      m_filehandle->openData("IRPB");
      m_filehandle->getData(&rpb_int[0]);
      m_filehandle->closeData();

      float rpb_dbl[32];
      m_filehandle->openData("RRPB");
      m_filehandle->getData(&rpb_dbl[0]);
      m_filehandle->closeData();
      
      runDetails.addProperty("dur", rpb_int[0]);        // actual run duration
      runDetails.addProperty("durunits", rpb_int[1]);   // scaler for above (1=seconds)
      runDetails.addProperty("dur_freq", rpb_int[2]);  // testinterval for above (seconds)
      runDetails.addProperty("dmp", rpb_int[3]);       // dump interval
      runDetails.addProperty("dmp_units", rpb_int[4]);  // scaler for above
      runDetails.addProperty("dmp_freq", rpb_int[5]);   // interval for above
      runDetails.addProperty("freq", rpb_int[6]);       // 2**k where source frequency = 50 / 2**k
      runDetails.addProperty("gd_prtn_chrg", rpb_dbl[7]);  // good proton charge (uA.hour)
      runDetails.addProperty("tot_prtn_chrg", rpb_dbl[8]); // total proton charge (uA.hour)
      runDetails.addProperty("goodfrm",rpb_int[9]);     // good frames
      runDetails.addProperty("rawfrm", rpb_int[10]);    // raw frames
      runDetails.addProperty("dur_wanted", rpb_int[11]); // requested run duration (units as for "duration" above)
      runDetails.addProperty("dur_secs", rpb_int[12]);  // actual run duration in seconds
      runDetails.addProperty("mon_sum1", rpb_int[13]);  // monitor sum 1
      runDetails.addProperty("mon_sum2", rpb_int[14]);  // monitor sum 2
      runDetails.addProperty("mon_sum3",rpb_int[15]);   // monitor sum 3
      
      m_filehandle->closeGroup(); // isis_vms_compat

      char strvalue[19];
      m_filehandle->openData("end_time");
      m_filehandle->getData(&strvalue);
      m_filehandle->closeData();
      std::string end_time_iso(strvalue, 19);
      runDetails.addProperty("run_end", end_time_iso);

      m_filehandle->openData("start_time");
      m_filehandle->getData(&strvalue);
      m_filehandle->closeData();
      std::string start_time_iso = std::string(strvalue, 19);
      runDetails.addProperty("run_start", start_time_iso);

      runDetails.addProperty("rb_proposal",rpb_int[21]); // RB (proposal) number

      m_filehandle->openGroup("sample","NXsample");
      std::string sample_name = getNexusString("name");
      localWorkspace->mutableSample().setName(sample_name);
      m_filehandle->closeGroup();
    }

    /**
     * Parse an ISO formatted date-time string into separate date and time strings
     * @param datetime_iso :: The string containing the ISO formatted date-time
     * @param date :: An output parameter containing the date from the original string or ??-??-???? if the format is unknown
     * @param time :: An output parameter containing the time from the original string or ??:??:?? if the format is unknown
     */
    void LoadISISNexus::parseISODateTime(const std::string & datetime_iso, std::string & date, std::string & time) const
    {
      try
      {
        Poco::DateTime datetime_output;
        int timezone_diff(0);
        Poco::DateTimeParser::parse(Poco::DateTimeFormat::ISO8601_FORMAT, datetime_iso, datetime_output, timezone_diff);
        date = Poco::DateTimeFormatter::format(datetime_output, "%d-%m-%Y", timezone_diff);
        time = Poco::DateTimeFormatter::format(datetime_output, "%H:%M:%S", timezone_diff);
      }
      catch(Poco::SyntaxException&)
      {
        date = "\?\?-\?\?-\?\?\?\?";
        time = "\?\?:\?\?:\?\?";
        g_log.warning() << "Cannot parse end time from entry in Nexus file.\n";
      }
    }



    /**  Load logs from Nexus file. Logs are expected to be in
    *   /raw_data_1/runlog group of the file. Call to this method must be done
    *   within /raw_data_1 group.
    *   @param ws :: The workspace to load the logs to.
    *   @param period :: The period of this workspace
    */
    void LoadISISNexus::loadLogs(DataObjects::Workspace2D_sptr ws,std::size_t period)
    {

      std::string stime = getNexusString("start_time");
      Kernel::DateAndTime start_t = Kernel::DateAndTime(stime);

      m_filehandle->openGroup("runlog","IXrunlog"); // open group - collection of logs
      std::map<std::string, std::string> logs = m_filehandle->getEntries();

      for (auto log = logs.begin(); log!= logs.end(); ++log)
      {

        m_filehandle->openGroup(log->first,log->second); // open a log group

        // get the time array for the log
        m_filehandle->openData("time");
        ::NeXus::Info tinfo = m_filehandle->getInfo();
        if (tinfo.dims[0] < 0)
        {
          m_filehandle->closeData();
          continue; // goto the next log
        }
        boost::shared_array<float> times;
        times.reset(new float[tinfo.dims[0]]);
        m_filehandle->getData(times.get());
        m_filehandle->closeData(); // time

        // get the value array and create the log
        m_filehandle->openData("value");
        ::NeXus::Info vinfo = m_filehandle->getInfo();
        if (vinfo.dims[0] != tinfo.dims[0])
        {
          m_filehandle->closeData();
          continue; // goto the next log
        }

        if (vinfo.type == NX_CHAR)
        {
          Kernel::TimeSeriesProperty<std::string>* logv = new Kernel::TimeSeriesProperty<std::string>(log->first);
          boost::shared_array<char> value(new char[vinfo.dims[0] * vinfo.dims[1]]);
          m_filehandle->getData(value.get());
          for(int i=0;i<vinfo.dims[0];i++)
          {
            Kernel::DateAndTime t = start_t + boost::posix_time::seconds(int(times[i]));
            for(int j=0;j<vinfo.dims[1];j++)
            {
              char* c = value.get()+i*vinfo.dims[1] + j;
              if (!isprint(*c)) *c = ' ';
            }
            *(value.get()+(i+1)*vinfo.dims[1]-1) = 0; // ensure the terminating zero
            logv->addValue(t,std::string(value.get()+i*vinfo.dims[1]));
          }
          ws->mutableRun().addLogData(logv);
          if (std::string(log->first) == "icp_event")
          {
            LogParser parser(logv);
            ws->mutableRun().addLogData(parser.createPeriodLog(static_cast<int>(period)));
            ws->mutableRun().addLogData(parser.createCurrentPeriodLog(static_cast<int>(period)));
            ws->mutableRun().addLogData(parser.createAllPeriodsLog());
            ws->mutableRun().addLogData(parser.createRunningLog());
          }
        }
        else
        {
          Kernel::TimeSeriesProperty<double>* logv = new Kernel::TimeSeriesProperty<double>(log->first);
          std::vector<double> value;
          m_filehandle->getDataCoerce(value);
          for(int64_t i=0;i<vinfo.dims[0];i++)
          {
            Kernel::DateAndTime t = start_t + boost::posix_time::seconds(int(times[i]));
            logv->addValue(t,value[i]);
          }
          ws->mutableRun().addLogData(logv);
        }
        m_filehandle->closeData();  // value

        m_filehandle->closeGroup();
      } // loop over logs

      m_filehandle->closeGroup();

      ws->populateInstrumentParameters();
    }

    double LoadISISNexus::dblSqrt(double in)
    {
      return sqrt(in);
    }

    /**
     * Get the first entry from an NX data group
     * @param name :: The group name
     * @returns The data value
     */
    template<class TYPE>
    TYPE LoadISISNexus::getNXData(const std::string & name)
    {
      TYPE value;
      m_filehandle->readData(name, value);
      return value;
    }

  } // namespace DataHandling
} // namespace Mantid
