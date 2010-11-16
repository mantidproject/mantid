//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidNexus/LoadISISNexus.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/SpectraDetectorMap.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidKernel/LogParser.h"
#include "MantidGeometry/Instrument/Detector.h"
#include "MantidGeometry/Instrument/XMLlogfile.h"

#include "Poco/Path.h"
#include <Poco/DateTimeFormatter.h>
#include <Poco/DateTimeParser.h>
#include <Poco/DateTimeFormat.h>

#include "boost/lexical_cast.hpp"
#include <cmath>
#include <sstream>
#include <cctype>

namespace Mantid
{
  namespace NeXus
  {
    // Register the algorithm into the algorithm factory
    DECLARE_ALGORITHM(LoadISISNexus)

    using namespace Kernel;
    using namespace API;
    using Geometry::IInstrument;

    /// Empty default constructor
    LoadISISNexus::LoadISISNexus() : 
    Algorithm(), m_filename(), m_numberOfSpectra(0), m_numberOfPeriods(0),
      m_list(false), m_interval(false), m_spec_list(), m_spec_min(0), m_spec_max(Mantid::EMPTY_INT())
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

      BoundedValidator<int> *mustBePositive = new BoundedValidator<int>();
      mustBePositive->setLower(0);
      declareProperty("SpectrumMin", 0, mustBePositive);
      declareProperty("SpectrumMax", Mantid::EMPTY_INT(), mustBePositive->clone());
      declareProperty(new ArrayProperty<int>("SpectrumList"));
      declareProperty("EntryNumber", 0, mustBePositive->clone(),
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
      NXstatus stat=NXopen(m_filename.c_str(), NXACC_READ, &m_fileID);
      if(stat==NX_ERROR)
      {
        g_log.error("Unable to open File: " + m_filename);  
        throw Exception::FileError("Unable to open File:" , m_filename);  
      }

      // Open the raw data group 'raw_data_1'
      openNexusGroup("raw_data_1","NXentry");

      // Read in the instrument name from the Nexus file
      m_instrument_name = getNexusString("name");

      openNexusGroup("detector_1","NXdata");

      readDataDimensions();
      if(m_entrynumber!=0)
      {
        if(m_entrynumber>m_numberOfPeriods)
        {
          throw std::invalid_argument("Invalid Entry Number:Enter a valid number");
        }
        else
          m_numberOfPeriods=1;

      }

      const int lengthIn = m_numberOfChannels + 1;

      // Need to extract the user-defined output workspace name
      Property *ws = getProperty("OutputWorkspace");
      std::string localWSName = ws->value();
      // If multiperiod, will need to hold the Instrument, Sample & SpectraDetectorMap for copying
      boost::shared_ptr<IInstrument> instrument;
      boost::shared_ptr<Sample> sample;

      // Call private method to validate the optional parameters, if set
      checkOptionalProperties();

      getTimeChannels();

      closeNexusGroup();// go back to raw_data_1

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
      for (int period = 0; period < m_numberOfPeriods; ++period) {

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
          // Only run the sub-algorithms once
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

        openNexusGroup("detector_1","NXdata");

        int counter = 0;
        for (int i = m_spec_min; i < m_spec_max; ++i)
        {
          loadData(period, counter,i,localWorkspace ); // added -1 for NeXus
          counter++;
          prog.report();
        }
        // Read in the spectra in the optional list parameter, if set
        if (m_list)
        {
          for(unsigned int i=0; i < m_spec_list.size(); ++i)
          {
            loadData(period, counter,m_spec_list[i],localWorkspace );
            counter++;
            prog.report();
          }
        }

        closeNexusGroup();// go up from detector_1 to raw_data_1

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
          declareProperty(new WorkspaceProperty<DataObjects::Workspace2D>(outws,WSName,Direction::Output));
          if(wsGrpSptr)wsGrpSptr->add(WSName);
          setProperty(outws,localWorkspace);
        }
        else
        {
          setProperty(outputWorkspace,boost::dynamic_pointer_cast<Workspace>(localWorkspace));
        }

        // Assign the result to the output workspace property
        // setProperty(outputWorkspace,localWorkspace);

      }

      // Close the Nexus file
      NXclose(&m_fileID);
    }

    /// Validates the optional 'spectra to read' properties, if they have been set
    void LoadISISNexus::checkOptionalProperties()
    {
      //read in the data supplied to the algorithm
      m_spec_list = getProperty("SpectrumList");
      m_spec_max = getProperty("SpectrumMax");
      //now check that data
      m_interval = !( m_spec_max == Mantid::EMPTY_INT() );
      if ( m_spec_max == Mantid::EMPTY_INT() ) m_spec_max = 0;
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

    /** Reads in a string value from the nexus file
    *  @param name The name of the data value in the nexus file. The name can be either absolute or relative
    *  @return The string's value
    *  @throw std::runtime_error in case of any error
    */
    std::string LoadISISNexus::getNexusString(const std::string& name)const
    {
      NXstatus stat=NXopendata(m_fileID,name.c_str());
      if(stat==NX_ERROR)
      {
        g_log.error("Cannot access data "+name);
        throw std::runtime_error("Cannot access data "+name);
      }

      int rank,dims[4],type;
      NXgetinfo(m_fileID,&rank,dims,&type);

      if (dims[0] == 0) return "";
      if (type != NX_CHAR)
      {
        g_log.error("Data "+name+" does not have NX_CHAR type as expected.");
        throw std::runtime_error("Data "+name+" does not have NX_CHAR type as expected.");
      }
      if (rank > 1) 
      {
        std::ostringstream ostr; ostr<<"Rank of data "<<name<<" is expected to be 2 ("<<rank<<" was found)";
        g_log.error(ostr.str());
        throw std::runtime_error(ostr.str());
      }

      boost::shared_array<char> buff( new char[dims[0]+1] );
      if (NX_ERROR == NXgetdata(m_fileID,buff.get()))
      {
        g_log.error("Error occured when reading data "+name);
        throw std::runtime_error("Error occured when reading data "+name);
      }
      buff[dims[0]]='\0'; // null terminate for copy
      stat=NXclosedata(m_fileID);

      return std::string( buff.get() );
    }

    /** Opens a Nexus group. This makes it the working group.
    *  @param name The group's name
    *  @param nx_class Nexus class of the group
    *  @throw std::runtime_error if the group cannot be opened
    */
    void LoadISISNexus::openNexusGroup(const std::string& name, const std::string& nx_class)const
    {
      if (NX_ERROR == NXopengroup(m_fileID,name.c_str(),nx_class.c_str())) 
      {
        g_log.error("Cannot open group "+name+" of class "+nx_class);
        throw std::runtime_error("Cannot open group "+name+" of class "+nx_class);
      }

    }

    /// Closes Nexus group
    void LoadISISNexus::closeNexusGroup()const
    {
      NXclosegroup(m_fileID);
    }

    /// Opens a Nexus data set
    void LoadISISNexus::openNexusData(const std::string& name)
    {
      if (NX_ERROR == NXopendata(m_fileID,name.c_str()))
      {
        g_log.error("Cannot open "+name);
        throw std::runtime_error("Cannot open "+name);
      };
    }

    /// Close a Nexus data set
    void LoadISISNexus::closeNexusData()
    {
      NXclosedata(m_fileID);
    }

    /// Get the data from Nexus
    void LoadISISNexus::getNexusData(void* p)
    {
      if (NX_ERROR == NXgetdata(m_fileID,p))
      {
        g_log.error("Error reading data");
        throw std::runtime_error("Error reading data");
      };
    }

    /// Get info for the open data set
    NexusInfo LoadISISNexus::getNexusInfo()
    {
      NexusInfo info;
      NXgetinfo(m_fileID,&info.rank,info.dims,&info.type);
      return info;
    }

    /** Group /raw_data_1/detector_1 must be open to call this function
    */
    void LoadISISNexus::readDataDimensions()
    {
      openNexusData("counts");

      int rank,type,dims[4];
      NXgetinfo(m_fileID,&rank,dims,&type);

      // Number of time channels and number of spectra made public
      m_numberOfPeriods  = dims[0];
      m_numberOfSpectra  = dims[1];
      m_numberOfChannels = dims[2];

      // Allocate memory for the counts buffer
      m_data.reset( new int[m_numberOfChannels] );

      closeNexusData();
    }

    void LoadISISNexus::getTimeChannels()
    {
      boost::shared_array<float> timeChannels( new float[m_numberOfChannels + 1] );

      openNexusData("time_of_flight");

      int rank,type,dims[4];
      NXgetinfo(m_fileID,&rank,dims,&type);

      if (m_numberOfChannels + 1 != dims[0])
      {
        g_log.error("Number of time channels does not match the data dimensions");
        throw std::runtime_error("Number of time channels does not match the data dimensions");
      }

      if (NX_ERROR == NXgetdata(m_fileID,timeChannels.get()))
      {
        g_log.error("Error reading time_of_flight");
        throw std::runtime_error("Error reading time_of_flight");
      };

      m_timeChannelsVec.reset(new MantidVec(timeChannels.get(), timeChannels.get() + m_numberOfChannels + 1));

      closeNexusData();
    }

    /** Group /raw_data_1/detector_1 must be open to call this function.
    *  loadMappingTable() must be done before any calls to this method.
    *  @param period The data period
    *  @param hist The index of the histogram in the workspace
    *  @param i The index of the histogram in the file
    *  @param localWorkspace The workspace
    */
    void LoadISISNexus::loadData(int period, int hist, int& i, DataObjects::Workspace2D_sptr localWorkspace)
    {
      openNexusData("counts");

      int rank,type,dims[4];
      NXgetinfo(m_fileID,&rank,dims,&type);

      int start[3], size[3];
      start[0] = period;
      start[1] = i;
      start[2] = 0;

      size[0] = 1;
      size[1] = 1;
      size[2] = m_numberOfChannels;

      if (NX_ERROR == NXgetslab(m_fileID, m_data.get(), start, size))
      {
        g_log.error("Error reading counts");
        throw std::runtime_error("Error reading counts");
      };

      MantidVec& Y = localWorkspace->dataY(hist);
      Y.assign(m_data.get(), m_data.get() + m_numberOfChannels);
      // Create and fill another vector for the errors, containing sqrt(count)
      MantidVec& E = localWorkspace->dataE(hist);
      std::transform(Y.begin(), Y.end(), E.begin(), dblSqrt);
      // Populate the workspace. Loop starts from 1, hence i-1
      localWorkspace->setX(hist, m_timeChannelsVec);
      localWorkspace->getAxis(1)->spectraNo(hist)= m_spec[i];

      closeNexusData();
    }

    /// Run the sub-algorithm LoadInstrument (or LoadInstrumentFromNexus)
    void LoadISISNexus::runLoadInstrument(DataObjects::Workspace2D_sptr localWorkspace)
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

      // Now execute the sub-algorithm. Catch and log any error, but don't stop.
      bool executionSuccessful(true);
      try
      {
        loadInst->setPropertyValue("Filename", fullPathIDF);
        loadInst->setProperty<MatrixWorkspace_sptr> ("Workspace", localWorkspace);
        loadInst->execute();
      }
      catch( std::invalid_argument&)
      {
        g_log.information("Invalid argument to LoadInstrument sub-algorithm");
        executionSuccessful = false;
      }
      catch (std::runtime_error&)
      {
        g_log.information("Unable to successfully run LoadInstrument sub-algorithm");
        executionSuccessful = false;
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
    *   @param ws The workspace which spectra-detector map is to be populated.
    */
    void LoadISISNexus::loadMappingTable(DataObjects::Workspace2D_sptr ws)
    {
      // Read in detector ids from isis compatibility section
      openNexusGroup("isis_vms_compat","IXvms");

      openNexusData("UDET");

      int rank,type,dims[4];
      NXgetinfo(m_fileID,&rank,dims,&type);

      int ndet = dims[0];

      boost::shared_array<int> udet(new int[ndet]);

      getNexusData(udet.get());

      closeNexusData();  // UDET

      closeNexusGroup(); // isis_vms_compat



      openNexusGroup("detector_1","NXdata");

      openNexusData("spectrum_index");

      NXgetinfo(m_fileID,&rank,dims,&type);

      if (dims[0] != ndet)
      {
        g_log.error("Cannot open load spectra-detector map: data sizes do not match.");
        throw std::runtime_error("Cannot open load spectra-detector map: data sizes do not match.");
      }

      m_spec.reset(new int[ndet]);

      getNexusData(m_spec.get());

      closeNexusData(); // spectrum_index
      closeNexusGroup(); // detector_1

      //Populate the Spectra Map with parameters
      ws->mutableSpectraMap().populate(m_spec.get(),udet.get(),ndet);

    }

    /**  Loag the run details from the file
    *   Group /raw_data_1 must be open.
    * @param localWorkspace The workspace details to use
    */
    void LoadISISNexus::loadRunDetails(DataObjects::Workspace2D_sptr localWorkspace)
    {
      API::Run & runDetails = localWorkspace->mutableRun();
      m_proton_charge = getEntryValue<double>("proton_charge");
      runDetails.setProtonCharge(m_proton_charge);
      int run_number = getEntryValue<int>("run_number");
      runDetails.addProperty("run_number", boost::lexical_cast<std::string>(run_number));


      openNexusGroup("isis_vms_compat","IXvms");
      char header[80];
      openNexusData("HDR");
      getNexusData(&header);
      closeNexusData();
      runDetails.addProperty("run_header", std::string(header,80));
      runDetails.addProperty("run_title", localWorkspace->getTitle());

      runDetails.addProperty("nspectra", getNXData<int>("NSP1"));
      runDetails.addProperty("nchannels", getNXData<int>("NTC1"));
      runDetails.addProperty("nperiods", getNXData<int>("NPER"));

      int rpb_int[32];
      openNexusData("IRPB");
      getNexusData(&rpb_int[0]);
      closeNexusData();

      float rpb_dbl[32];
      openNexusData("RRPB");
      getNexusData(&rpb_dbl[0]);
      closeNexusData();
      
      runDetails.addProperty("dur", rpb_int[0]);	// actual run duration
      runDetails.addProperty("durunits", rpb_int[1]);	// scaler for above (1=seconds)
      runDetails.addProperty("dur_freq", rpb_int[2]);  // testinterval for above (seconds)
      runDetails.addProperty("dmp", rpb_int[3]);       // dump interval
      runDetails.addProperty("dmp_units", rpb_int[4]);	// scaler for above
      runDetails.addProperty("dmp_freq", rpb_int[5]);	// interval for above
      runDetails.addProperty("freq", rpb_int[6]);	// 2**k where source frequency = 50 / 2**k
      runDetails.addProperty("gd_prtn_chrg", rpb_dbl[7]);  // good proton charge (uA.hour)
      runDetails.addProperty("tot_prtn_chrg", rpb_dbl[8]); // total proton charge (uA.hour)
      runDetails.addProperty("goodfrm",rpb_int[9]);	// good frames
      runDetails.addProperty("rawfrm", rpb_int[10]);	// raw frames
      runDetails.addProperty("dur_wanted", rpb_int[11]); // requested run duration (units as for "duration" above)
      runDetails.addProperty("dur_secs", rpb_int[12]);	// actual run duration in seconds
      runDetails.addProperty("mon_sum1", rpb_int[13]);	// monitor sum 1
      runDetails.addProperty("mon_sum2", rpb_int[14]);	// monitor sum 2
      runDetails.addProperty("mon_sum3",rpb_int[15]);	// monitor sum 3
      
      closeNexusGroup(); // isis_vms_compat

      char strvalue[19];
      openNexusData("end_time");
      getNexusData(&strvalue);
      closeNexusData();
      std::string end_time_iso(strvalue, 19);
      std::string end_date(""), end_time("");
      parseISODateTime(end_time_iso, end_date, end_time);
      runDetails.addProperty("enddate", end_date);
      runDetails.addProperty("endtime", end_time);

      openNexusData("start_time");
      getNexusData(&strvalue);
      closeNexusData();
      std::string start_time_iso = std::string(strvalue, 19);
      std::string start_date(""), start_time("");
      parseISODateTime(start_time_iso, start_date, start_time);
      runDetails.addProperty("startdate", start_date);
      runDetails.addProperty("starttime", start_time);

      runDetails.addProperty("rb_proposal",rpb_int[21]); // RB (proposal) number

      openNexusGroup("sample","NXsample");
      std::string sample_name = getNexusString("name");
      localWorkspace->mutableSample().setName(sample_name);
      closeNexusGroup();
    }

    /**
     * Parse an ISO formatted date-time string into separate date and time strings
     * @param datetime_iso The string containing the ISO formatted date-time
     * @param date An output parameter containing the date from the original string or ??-??-???? if the format is unknown
     * @param time An output parameter containing the time from the original string or ??:??:?? if the format is unknown
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
    *   @param ws The workspace to load the logs to.
    *   @param period The period of this workspace
    */
    void LoadISISNexus::loadLogs(DataObjects::Workspace2D_sptr ws,int period)
    {

      std::string stime = getNexusString("start_time");
      Kernel::DateAndTime start_t = Kernel::DateAndTime(stime);

      openNexusGroup("runlog","IXrunlog"); // open group - collection of logs

      boost::shared_array<char> nxname( new char[NX_MAXNAMELEN] );
      boost::shared_array<char> nxclass( new char[NX_MAXNAMELEN] );
      int nxdatatype;

      int stat = NX_OK;
      while(stat != NX_EOD)
      {
        stat=NXgetnextentry(m_fileID,nxname.get(),nxclass.get(),&nxdatatype);

        openNexusGroup(nxname.get(),nxclass.get()); // open a log group 
        {
          try
          {
            NexusInfo tinfo;
            boost::shared_array<float> times;

            openNexusData("time");
            {
              tinfo = getNexusInfo();
              if (tinfo.dims[0] < 0) throw std::runtime_error(""); // goto the next log
              times.reset(new float[tinfo.dims[0]]);
              getNexusData(times.get());
            }
            closeNexusData();

            openNexusData("value");
            {
              NexusInfo vinfo = getNexusInfo();
              if (vinfo.dims[0] != tinfo.dims[0]) throw std::runtime_error(""); // goto the next log

              if (vinfo.type == NX_CHAR)
              {
                Kernel::TimeSeriesProperty<std::string>* logv = new Kernel::TimeSeriesProperty<std::string>(nxname.get());
                boost::shared_array<char> value(new char[vinfo.dims[0] * vinfo.dims[1]]);
                getNexusData(value.get());
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
                if (std::string(nxname.get()) == "icp_event")
                {
                  LogParser parser(logv);
                  ws->mutableRun().addLogData(parser.createPeriodLog(period));
                  ws->mutableRun().addLogData(parser.createAllPeriodsLog());
                  ws->mutableRun().addLogData(parser.createRunningLog());
                }
              }
              else if (vinfo.type == NX_FLOAT32)
              {
                Kernel::TimeSeriesProperty<double>* logv = new Kernel::TimeSeriesProperty<double>(nxname.get());
                boost::shared_array<float> value(new float[vinfo.dims[0]]);
                getNexusData(value.get());
                for(int i=0;i<vinfo.dims[0];i++)
                {
                  Kernel::DateAndTime t = start_t + boost::posix_time::seconds(int(times[i]));
                  logv->addValue(t,value[i]);
                }
                ws->mutableRun().addLogData(logv);
              }
              else if (vinfo.type == NX_INT32)
              {
                Kernel::TimeSeriesProperty<double>* logv = new Kernel::TimeSeriesProperty<double>(nxname.get());
                boost::shared_array<int> value(new int[vinfo.dims[0]]);
                getNexusData(value.get());
                for(int i=0;i<vinfo.dims[0];i++)
                {
                  Kernel::DateAndTime t = start_t + boost::posix_time::seconds(int(times[i]));
                  logv->addValue(t,value[i]);
                }
                ws->mutableRun().addLogData(logv);
              }
              else
              {
                g_log.error()<<"Cannot read log data of this type ("<<vinfo.type<<")\n";
                throw std::runtime_error("Cannot read log data of this type");
              }


            }
            closeNexusData();  // value
          }
          catch(...)
          {
            closeNexusData();
          }
        }
        closeNexusGroup();
      } // loop over logs

      closeNexusGroup();

      ws->populateInstrumentParameters();
    }

    double LoadISISNexus::dblSqrt(double in)
    {
      return sqrt(in);
    }

  } // namespace DataHandling
} // namespace Mantid
