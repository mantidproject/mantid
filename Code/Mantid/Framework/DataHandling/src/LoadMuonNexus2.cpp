/*WIKI* 


The algorithm LoadMuonNexus will read a Muon Nexus data file (original format) and place the data
into the named workspace.
The file name can be an absolute or relative path and should have the extension
.nxs or .NXS.
If the file contains data for more than one period, a separate workspace will be generated for each.
After the first period the workspace names will have "_2", "_3", and so on, appended to the given workspace name.
For single period data, the optional parameters can be used to control which spectra are loaded into the workspace.
If spectrum_min and spectrum_max are given, then only that range to data will be loaded.
If a spectrum_list is given than those values will be loaded.
* TODO get XML descriptions of Muon instruments. This data is not in existing Muon Nexus files.
* TODO load the spectra detector mapping. This may be very simple for Muon instruments.

===Time series data===
The log data in the Nexus file (NX_LOG sections) will be loaded as TimeSeriesProperty data within the workspace.
Time is stored as seconds from the Unix epoch.

===Errors===

The error for each histogram count is set as the square root of the number of counts.

===Time bin data===

The ''corrected_times'' field of the Nexus file is used to provide time bin data and the bin edge values are calculated from these
bin centre times.

===Multiperiod data===

To determine if a file contains data from more than one period the field ''switching_states'' is read from the Nexus file.
If this value is greater than one it is taken to be the number of periods, <math>N_p</math> of the data.
In this case the <math>N_s</math> spectra in the ''histogram_data'' field are split with <math>N_s/N_p</math> assigned to each period.

===ChildAlgorithms used===

The ChildAlgorithms used by LoadMuonNexus are:
* LoadMuonLog - this reads log information from the Nexus file and uses it to create TimeSeriesProperty entries in the workspace.
* LoadInstrument - this algorithm looks for an XML description of the instrument and if found reads it.
* LoadIntstrumentFromNexus - this is called if the normal LoadInstrument fails. As the Nexus file has limited instrument data, this only populates a few fields.

==Previous Versions==
===Version 1===
Version 1 supports the loading version 1.0 of the muon nexus format.  This is still in active use, if the current version of LoadMuonNexus detects that it has been asked to load a previous version muon nexus file it will call the previous version of the algorithm to perform the task.

*WIKI*/
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidDataHandling/LoadMuonNexus2.h"
#include "MantidDataHandling/LoadMuonNexus1.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/Progress.h"
#include "MantidAPI/RegisterFileLoader.h"
#include "MantidGeometry/Instrument/Detector.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidNexus/NexusClasses.h"
#include <nexus/NeXusFile.hpp>
#include <nexus/NeXusException.hpp>

#include <Poco/Path.h>
#include <boost/lexical_cast.hpp>
#include <boost/shared_ptr.hpp>

#include <cmath>
#include <numeric>

namespace Mantid
{
  namespace DataHandling
  {
    // Register the algorithm into the algorithm factory
    DECLARE_HDF_FILELOADER_ALGORITHM(LoadMuonNexus2);

    using namespace Kernel;
    using namespace API;
    using Geometry::Instrument;
    using namespace Mantid::NeXus;

    /// Sets documentation strings for this algorithm
    void LoadMuonNexus2::initDocs()
    {
      this->setWikiSummary("The LoadMuonNexus algorithm will read the given NeXus Muon data file Version 2 and use the results to populate the named workspace. LoadMuonNexus may be invoked by [[LoadNexus]] if it is given a NeXus file of this type. ");
      this->setOptionalMessage("The LoadMuonNexus algorithm will read the given NeXus Muon data file Version 2 and use the results to populate the named workspace. LoadMuonNexus may be invoked by LoadNexus if it is given a NeXus file of this type.");
    }


    /// Empty default constructor
    LoadMuonNexus2::LoadMuonNexus2() : LoadMuonNexus()
    {}

    /** Executes the right version of the muon nexus loader: versions 1 or 2.
    * 
    *  @throw Exception::FileError If the Nexus file cannot be found/opened
    *  @throw std::invalid_argument If the optional properties are set to invalid values
    */
    void LoadMuonNexus2::exec()
    {
      std::string filePath = getPropertyValue("Filename");
      LoadMuonNexus1 load1; load1.initialize();

      int confidence1 = load1.confidence( filePath );
      int confidence2 = this->confidence( filePath );

      // if none can load the file throw
      if ( confidence1 < 80 && confidence2 < 80)
      {
        throw Kernel::Exception::FileError("Cannot open the file ", filePath);
      }

      if ( confidence2 > confidence1 )
      {
        // this loader
        doExec();
      }
      else
      {
        // version 1 loader
        IAlgorithm_sptr childAlg = createChildAlgorithm("LoadMuonNexus",0,1,true,1);
        auto version1Loader = boost::dynamic_pointer_cast<API::Algorithm>( childAlg );
        version1Loader->copyPropertiesFrom( *this );
        version1Loader->executeAsChildAlg();
        this->copyPropertiesFrom( *version1Loader );
        API::Workspace_sptr outWS = version1Loader->getProperty("OutputWorkspace");
        setProperty("OutputWorkspace", outWS);
      }
    }

    /** Read in a muon nexus file of the version 2.
    * 
    *  @throw Exception::FileError If the Nexus file cannot be found/opened
    *  @throw std::invalid_argument If the optional properties are set to invalid values
    */
    void LoadMuonNexus2::doExec()
    {
      // Create the root Nexus class
      NXRoot root(getPropertyValue("Filename"));

      int64_t iEntry = getProperty("EntryNumber");
      if (iEntry >= static_cast<int64_t>(root.groups().size()) )
      {
        throw std::invalid_argument("EntryNumber is out of range");
      }

      // Open the data entry
      m_entry_name = root.groups()[iEntry].nxname;
      NXEntry entry = root.openEntry(m_entry_name);

      // Read in the instrument name from the Nexus file
      m_instrument_name = entry.getString("instrument/name");

      // Read the number of periods in this file
      if ( entry.containsGroup("run") )
      {
        try
        {
          m_numberOfPeriods = entry.getInt("run/number_periods");
        }
        catch (::NeXus::Exception&)
        {
          //assume 1
          m_numberOfPeriods = 1;
        }
      }
      else
      {
        m_numberOfPeriods = 1;
      }

      // Need to extract the user-defined output workspace name
      Property *ws = getProperty("OutputWorkspace");
      std::string localWSName = ws->value();
      // If multiperiod, will need to hold the Instrument & Sample for copying
      boost::shared_ptr<Instrument> instrument;
      boost::shared_ptr<Sample> sample;

      std::string detectorName;
      // Only the first NXdata found
      for(unsigned int i=0; i< entry.groups().size(); i++)
      {
        std::string className = entry.groups()[i].nxclass;
        if (className == "NXdata")
        {
          detectorName = entry.groups()[i].nxname;
          break;
        }
      }
      NXData dataGroup = entry.openNXData(detectorName);

      Mantid::NeXus::NXInt spectrum_index = dataGroup.openNXInt("spectrum_index");
      spectrum_index.load();
      m_numberOfSpectra = spectrum_index.dim0();

      // Call private method to validate the optional parameters, if set
      checkOptionalProperties();

      NXFloat raw_time = dataGroup.openNXFloat("raw_time");
      raw_time.load();
      int nBins = raw_time.dim0();
      std::vector<double> timeBins;
      timeBins.assign(raw_time(),raw_time()+nBins);
      timeBins.push_back(raw_time[nBins-1]+raw_time[1]-raw_time[0]);

      // Calculate the size of a workspace, given its number of periods & spectra to read
      int total_specs;
      if( m_interval || m_list)
      {
        total_specs = static_cast<int>(m_spec_list.size());
        if (m_interval)
        {
          total_specs += static_cast<int>((m_spec_max-m_spec_min+1));
        }
        else
        {
          m_spec_max = -1; // to stop entering the min - max loop
        }
      }
      else
      {
        total_specs = static_cast<int>(m_numberOfSpectra);
        // for nexus return all spectra
        m_spec_min = 1;
        m_spec_max = m_numberOfSpectra;  // was +1?
      }

      // Create the 2D workspace for the output
      DataObjects::Workspace2D_sptr localWorkspace = boost::dynamic_pointer_cast<DataObjects::Workspace2D>
        (WorkspaceFactory::Instance().create("Workspace2D",total_specs,nBins+1,nBins));
      // Set the unit on the workspace to muon time, for now in the form of a Label Unit
      boost::shared_ptr<Kernel::Units::Label> lblUnit = 
        boost::dynamic_pointer_cast<Kernel::Units::Label>(UnitFactory::Instance().create("Label"));
      lblUnit->setLabel("Time","microsecond");
      localWorkspace->getAxis(0)->unit() = lblUnit;
      // Set y axis unit
      localWorkspace->setYUnit("Counts");

      //g_log.error()<<" number of perioids= "<<m_numberOfPeriods<<std::endl;
      WorkspaceGroup_sptr wsGrpSptr=WorkspaceGroup_sptr(new WorkspaceGroup);
      if ( entry.containsDataSet( "title" ) )
      {
        wsGrpSptr->setTitle(entry.getString("title"));
      }

      if ( entry.containsDataSet( "notes" ) )
      {
        wsGrpSptr->setComment(entry.getString("notes"));
      }

      if(m_numberOfPeriods>1)
      {
        setProperty("OutputWorkspace",boost::dynamic_pointer_cast<Workspace>(wsGrpSptr));
      }

      // period_index is currently unused
      //Mantid::NeXus::NXInt period_index = dataGroup.openNXInt("period_index");
      //period_index.load();

      Mantid::NeXus::NXInt counts = dataGroup.openIntData();
      counts.load();

      NXInstrument instr = entry.openNXInstrument("instrument");

      if ( instr.containsGroup( "detector_fb" ) )
      {
        NXDetector detector = instr.openNXDetector("detector_fb");
        if (detector.containsDataSet("time_zero"))
        {
          double dum = detector.getFloat("time_zero");
          setProperty("TimeZero", dum);
        }
        if (detector.containsDataSet("first_good_time"))
        {
          double dum = detector.getFloat("first_good_time");
          setProperty("FirstGoodData", dum);
        }
      }

      API::Progress progress(this,0.,1.,m_numberOfPeriods * total_specs);
      // Loop over the number of periods in the Nexus file, putting each period in a separate workspace
      for (int period = 0; period < m_numberOfPeriods; ++period) {

        if (period == 0)
        {
          // Only run the Child Algorithms once
          loadRunDetails(localWorkspace);
          runLoadInstrument(localWorkspace );
          loadLogs(localWorkspace, entry, period);
        }
        else   // We are working on a higher period of a multiperiod raw file
        {
          localWorkspace =  boost::dynamic_pointer_cast<DataObjects::Workspace2D>
            (WorkspaceFactory::Instance().create(localWorkspace));
        }

        std::string outws("");
        if(m_numberOfPeriods>1)
        {
          std::string outputWorkspace = "OutputWorkspace";
          std::stringstream suffix;
          suffix << (period+1);
          outws =outputWorkspace+"_"+suffix.str();
          std::string WSName = localWSName + "_" + suffix.str();
          declareProperty(new WorkspaceProperty<Workspace>(outws,WSName,Direction::Output));
          if(wsGrpSptr)wsGrpSptr->addWorkspace( localWorkspace );
        }

        // create spectrum -> index correspondence
        std::map<int,int> index_spectrum;
        for (int i = 0; i < m_numberOfSpectra; ++i)
        {
          index_spectrum[spectrum_index[i]] = i;
        }

        int counter = 0;
        for (int spec = static_cast<int>(m_spec_min); spec <= static_cast<int>(m_spec_max); ++spec)
        {
          int i = index_spectrum[spec]; // if spec not found i is 0
          loadData(counts,timeBins,counter,period,i,localWorkspace);
          localWorkspace->getAxis(1)->setValue(counter, spectrum_index[i]);
          counter++;
          progress.report();
        }

        // Read in the spectra in the optional list parameter, if set
        if (m_list)
        {
          for(unsigned int i=0; i < m_spec_list.size(); ++i)
          {
            int spec = m_spec_list[i];
            int k = index_spectrum[spec]; // if spec not found k is 0
            loadData(counts,timeBins,counter,period,k,localWorkspace);
            localWorkspace->getAxis(1)->setValue(counter, spectrum_index[k]);
            counter++;
            progress.report();
          }
        }
        // Just a sanity check
        assert(counter == total_specs);

        bool autogroup = getProperty("AutoGroup");

        if (autogroup)
        {
          g_log.warning("Autogrouping is not implemented for muon NeXus version 2 files");
        }

        // Assign the result to the output workspace property
        if(m_numberOfPeriods>1)
          setProperty(outws,boost::static_pointer_cast<Workspace>(localWorkspace));
        else
        {
          setProperty("OutputWorkspace",boost::dynamic_pointer_cast<Workspace>(localWorkspace));
        }

      } // loop over periods

    }

    /** loadData
    *  Load the counts data from an NXInt into a workspace
    */
    void LoadMuonNexus2::loadData(const Mantid::NeXus::NXInt& counts,const std::vector<double>& timeBins,int wsIndex,
      int period,int spec,API::MatrixWorkspace_sptr localWorkspace)
    {
      MantidVec& X = localWorkspace->dataX(wsIndex);
      MantidVec& Y = localWorkspace->dataY(wsIndex);
      MantidVec& E = localWorkspace->dataE(wsIndex);
      X.assign(timeBins.begin(),timeBins.end());

      int nBins = 0;
      int *data = NULL;

      if ( counts.rank() == 3 )
      {
        nBins = counts.dim2();
        data = &counts(period,spec,0);
      }
      else if ( counts.rank() == 2 )
      {
        nBins = counts.dim1();
        data = &counts(spec,0);
      }
      else
      {
        throw std::runtime_error("Data have unsupported dimansionality");
      }
      assert( nBins+1 == static_cast<int>(timeBins.size()) );

      Y.assign(data,data+nBins);
      typedef double (*uf)(double);
      uf dblSqrt = std::sqrt;
      std::transform(Y.begin(), Y.end(), E.begin(), dblSqrt);
    }

    /**  Load logs from Nexus file. Logs are expected to be in
    *   /run/sample group of the file. 
    *   @param ws :: The workspace to load the logs to.
    *   @param entry :: The Nexus entry
    *   @param period :: The period of this workspace
    */
    void LoadMuonNexus2::loadLogs(API::MatrixWorkspace_sptr ws, NXEntry & entry,int period)
    {
      //Avoid compiler warning 
      (void)period;

      std::string start_time = entry.getString("start_time");

      std::string sampleName = entry.getString("sample/name");
      NXMainClass runlogs = entry.openNXClass<NXMainClass>("sample");
      ws->mutableSample().setName(sampleName);

      for(std::vector<NXClassInfo>::const_iterator it=runlogs.groups().begin();it!=runlogs.groups().end();++it)
      {
        NXLog nxLog = runlogs.openNXLog(it->nxname);
        Kernel::Property* logv = nxLog.createTimeSeries(start_time);
        if (!logv) continue;
        ws->mutableRun().addLogData(logv);
      }

      ws->setTitle(entry.getString("title"));

      if ( entry.containsDataSet("notes") )
      {
        ws->setComment(entry.getString("notes"));
      }

      std::string run_num = boost::lexical_cast<std::string>(entry.getInt("run_number"));
      //The sample is left to delete the property
      ws->mutableRun().addLogData(new PropertyWithValue<std::string>("run_number", run_num));

      ws->populateInstrumentParameters();
    }

    /**  Log the run details from the file
    * @param localWorkspace :: The workspace details to use
    */
    void LoadMuonNexus2::loadRunDetails(DataObjects::Workspace2D_sptr localWorkspace)
    {
      API::Run & runDetails = localWorkspace->mutableRun();

      runDetails.addProperty("run_title", localWorkspace->getTitle(), true);

      int numSpectra = static_cast<int>(localWorkspace->getNumberHistograms());
      runDetails.addProperty("nspectra", numSpectra);

      m_filename = getPropertyValue("Filename");
      NXRoot root(m_filename);
      NXEntry entry = root.openEntry(m_entry_name);

      std::string start_time = entry.getString("start_time");
      runDetails.addProperty("run_start", start_time);

      std::string stop_time = entry.getString("end_time");
      runDetails.addProperty("run_end", stop_time);

      if ( entry.containsGroup( "run" ) )
      {
        NXClass runRun = entry.openNXGroup("run");

        if ( runRun.containsDataSet("good_total_frames") )
        {
          int dum = runRun.getInt("good_total_frames");
          runDetails.addProperty("goodfrm", dum);
        }

        if ( runRun.containsDataSet("number_periods") )
        {
          int dum = runRun.getInt("number_periods");
          runDetails.addProperty("nperiods", dum);
        }
      }

      {  // Duration taken to be stop_time minus stat_time
        DateAndTime start(start_time);
        DateAndTime end(stop_time);
        double duration_in_secs = DateAndTime::secondsFromDuration( end - start);
        runDetails.addProperty("dur_secs",duration_in_secs);
      }
    }

    /**
     * Return the confidence with with this algorithm can load the file
     * @param descriptor A descriptor for the file
     * @returns An integer specifying the confidence level. 0 indicates it will not be used
     */
    int LoadMuonNexus2::confidence(const Kernel::HDFDescriptor & descriptor) const
    {
      const auto & firstEntryNameType = descriptor.firstEntryNameType();
      const std::string root = "/" + firstEntryNameType.first;
      if(!descriptor.pathExists(root + "/definition")) return 0;

      bool upperIDF(true);
      if(descriptor.pathExists(root + "/IDF_version")) upperIDF = true;
      else
      {
        if(descriptor.pathExists(root + "/idf_version")) upperIDF = false;
        else return 0;
      }

      try
      {
        NXRoot root(descriptor.filename());
        NXEntry entry = root.openFirstEntry();

        std::string versionField = "idf_version";
        if(upperIDF) versionField = "IDF_version";

        if ( entry.getInt(versionField) != 2 ) return 0;
        std::string definition = entry.getString("definition");
        if ( definition == "muonTD" || definition == "pulsedTD" )
        {
          // If all this succeeded then we'll assume this is an ISIS Muon NeXus file version 2
          return 81;
        }
      }
      catch( ... )
      {
      }
      return 0;
    }

  } // namespace DataHandling
} // namespace Mantid
