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

*WIKI*/
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidDataHandling/LoadMuonNexus1.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidAPI/LoadAlgorithmFactory.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/Progress.h"
#include "MantidAPI/TableRow.h"
#include "MantidGeometry/Instrument/Detector.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/ListValidator.h"
#include "MantidNexus/MuonNexusReader.h"
#include "MantidNexus/NexusClasses.h"

#include <Poco/Path.h>
#include <limits>
#include <cmath>
#include <boost/shared_ptr.hpp>

namespace Mantid
{
  namespace DataHandling
  {
    // Register the algorithm into the algorithm factory
    DECLARE_ALGORITHM(LoadMuonNexus1)
    DECLARE_LOADALGORITHM(LoadMuonNexus1)

    /// Sets documentation strings for this algorithm
    void LoadMuonNexus1::initDocs()
    {
      this->setWikiSummary("The LoadMuonNexus algorithm will read the given NeXus Muon data file Version 1 and use the results to populate the named workspace. LoadMuonNexus may be invoked by [[LoadNexus]] if it is given a NeXus file of this type. ");
      this->setOptionalMessage("The LoadMuonNexus algorithm will read the given NeXus Muon data file Version 1 and use the results to populate the named workspace. LoadMuonNexus may be invoked by LoadNexus if it is given a NeXus file of this type.");
    }


    using namespace Kernel;
    using namespace API;
    using Geometry::Instrument;
    using namespace Mantid::NeXus;

    /// Empty default constructor
    LoadMuonNexus1::LoadMuonNexus1() : LoadMuonNexus() {}

    /** Executes the algorithm. Reading in the file and creating and populating
    *  the output workspace
    * 
    *  @throw Exception::FileError If the Nexus file cannot be found/opened
    *  @throw std::invalid_argument If the optional properties are set to invalid values
    */
    void LoadMuonNexus1::exec()
    {
      // Retrieve the filename from the properties
      m_filename = getPropertyValue("Filename");
      // Retrieve the entry number
      m_entrynumber = getProperty("EntryNumber");

      NXRoot root(m_filename);
      NXEntry entry = root.openEntry("run/histogram_data_1");
      try
      {
        NXInfo info = entry.getDataSetInfo("time_zero");
        if (info.stat != NX_ERROR)
        {
          double dum = root.getFloat("run/histogram_data_1/time_zero");
          setProperty("TimeZero", dum);
        }
      }
      catch (...)
      {}

      try
      {
        NXInfo infoResolution = entry.getDataSetInfo("resolution");
        NXInt counts = root.openNXInt("run/histogram_data_1/counts");
        std::string firstGoodBin = counts.attributes("first_good_bin");
        if ( !firstGoodBin.empty() && infoResolution.stat != NX_ERROR )
        {
          double bin = static_cast<double>(boost::lexical_cast<int>(firstGoodBin));
          double bin_size = static_cast<double>(root.getInt("run/histogram_data_1/resolution"))/1000000.0;
          setProperty("FirstGoodData", bin*bin_size);
        }
      }
      catch (...)
      {}

      try
      { 
        std::vector<double>defaultDeadTimes;
        NXFloat deadTimes = root.openNXFloat("run/instrument/detector/deadtimes");
        deadTimes.load();

        int length = deadTimes.dim0();
        for (int i = 0; i < length; i++)
        {
          defaultDeadTimes.push_back(static_cast<double>(*(deadTimes() + i) ) );
        }
        setProperty("DeadTimes", defaultDeadTimes);
      }
      catch (...)
      {}

      NXEntry nxRun = root.openEntry("run");
      std::string title;
      std::string notes;
      try
      {
        title = nxRun.getString("title");
        notes = nxRun.getString("notes");
      }    
      catch (...)
      {}
      std::string run_num;
      try
      {
        run_num = boost::lexical_cast<std::string>(nxRun.getInt("number"));
      }
      catch (...)
      {}

      MuonNexusReader nxload;
      nxload.readFromFile(m_filename);

      // Read in the instrument name from the Nexus file
      m_instrument_name = nxload.getInstrumentName();
      // Read in the number of spectra in the Nexus file
      m_numberOfSpectra = nxload.t_nsp1;
      if(m_entrynumber!=0)
      {
        m_numberOfPeriods=1;
        if(m_entrynumber>nxload.t_nper)
        {
          throw std::invalid_argument("Invalid Entry Number:Enter a valid number");
        }
      }
      else
      {
        // Read the number of periods in this file
        m_numberOfPeriods = nxload.t_nper;
      }
      // Need to extract the user-defined output workspace name
      Property *ws = getProperty("OutputWorkspace");
      std::string localWSName = ws->value();
      // If multiperiod, will need to hold the Instrument & Sample for copying
      boost::shared_ptr<Instrument> instrument;
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
      int64_t total_specs;
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

      // Create the 2D workspace for the output
      DataObjects::Workspace2D_sptr localWorkspace = boost::dynamic_pointer_cast<DataObjects::Workspace2D>
        (WorkspaceFactory::Instance().create("Workspace2D",total_specs,lengthIn,lengthIn-1));
      localWorkspace->setTitle(title);
      localWorkspace->setComment(notes);
      localWorkspace->mutableRun().addLogData(new PropertyWithValue<std::string>("run_number", run_num));
      // Set the unit on the workspace to muon time, for now in the form of a Label Unit
      boost::shared_ptr<Kernel::Units::Label> lblUnit = 
        boost::dynamic_pointer_cast<Kernel::Units::Label>(UnitFactory::Instance().create("Label"));
      lblUnit->setLabel("Time","microsecond");
      localWorkspace->getAxis(0)->unit() = lblUnit;
      // Set y axis unit
      localWorkspace->setYUnit("Counts");

      WorkspaceGroup_sptr wsGrpSptr=WorkspaceGroup_sptr(new WorkspaceGroup);
      if(m_numberOfPeriods>1)
      {
        setProperty("OutputWorkspace",boost::dynamic_pointer_cast<Workspace>(wsGrpSptr));
      }

      API::Progress progress(this,0.,1.,m_numberOfPeriods * total_specs);
      // Loop over the number of periods in the Nexus file, putting each period in a separate workspace
      for (int64_t period = 0; period < m_numberOfPeriods; ++period) {
        if(m_entrynumber!=0)
        {
          period=m_entrynumber-1;
          if(period!=0)
          {
            loadRunDetails(localWorkspace);
            runLoadInstrument(localWorkspace );
          }
        }

        if (period == 0)
        {
          // Only run the Child Algorithms once
          loadRunDetails(localWorkspace);
          runLoadInstrument(localWorkspace );
          runLoadLog(localWorkspace );
          localWorkspace->populateInstrumentParameters();
        }
        else   // We are working on a higher period of a multiperiod raw file
        {
          localWorkspace =  boost::dynamic_pointer_cast<DataObjects::Workspace2D>
            (WorkspaceFactory::Instance().create(localWorkspace));
          localWorkspace->setTitle(title);
          localWorkspace->setComment(notes);
          //localWorkspace->newInstrumentParameters(); ???

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
          if (wsGrpSptr)
          {
            wsGrpSptr->addWorkspace( localWorkspace );
          }
        }

        size_t counter = 0;
        for (int64_t i = m_spec_min; i < m_spec_max; ++i)
        {
          // Shift the histogram to read if we're not in the first period
          specid_t histToRead = static_cast<specid_t>(i + period*total_specs);
          loadData(timeChannelsVec,counter,histToRead,nxload,lengthIn-1,localWorkspace ); // added -1 for NeXus
          counter++;
          progress.report();
        }
        // Read in the spectra in the optional list parameter, if set
        if (m_list)
        {
          for(size_t i=0; i < m_spec_list.size(); ++i)
          {
            loadData(timeChannelsVec,counter,m_spec_list[i],nxload,lengthIn-1, localWorkspace );
            counter++;
            progress.report();
          }
        }
        // Just a sanity check
        assert(counter == size_t(total_specs) );

        bool autogroup = getProperty("AutoGroup");

        if (autogroup)
        {

          //Get the groupings
          int64_t max_group = 0;
          // use a map for mapping group number and output workspace index in case 
          // there are group numbers > number of groups
          std::map<int64_t,int64_t> groups;
          m_groupings.resize(nxload.numDetectors);
          bool thereAreZeroes = false;
          for (int64_t i =0; i < static_cast<int64_t>(nxload.numDetectors); ++i)
          {
            int64_t ig = static_cast<int64_t>(nxload.detectorGroupings[i]);
            if (ig == 0)
            {
              thereAreZeroes = true;
              continue;
            }
            m_groupings[i] = static_cast<specid_t>(ig);
            if (groups.find(ig) == groups.end())
              groups[ig] = static_cast<int64_t>(groups.size());
            if (ig > max_group) max_group = ig;
          }

          if (thereAreZeroes)
            for (int64_t i =0; i < static_cast<int64_t>(nxload.numDetectors); ++i)
            {
              int64_t ig = static_cast<int64_t>(nxload.detectorGroupings[i]);
              if (ig == 0)
              {
                ig = ++max_group;
                m_groupings[i] = static_cast<specid_t>(ig);
                groups[ig] = groups.size();
              }
            }

            int numHists = static_cast<int>(localWorkspace->getNumberHistograms());
            size_t ngroups = groups.size(); // number of groups

            // to output groups in ascending order
            {
              int64_t i=0;
              for(std::map<int64_t,int64_t>::iterator it=groups.begin();it!=groups.end();++it,++i)
              {
                it->second = i;
                g_log.information()<<"group "<<it->first<<": ";
                bool first = true;
                int64_t first_i = -1 * std::numeric_limits<int64_t>::max();
                int64_t last_i = -1 * std::numeric_limits<int64_t>::max();
                for(int64_t i=0;i<static_cast<int64_t>(numHists);i++)
                  if (m_groupings[i] == it->first)
                  {
                    if (first) 
                    {
                      first = false;
                      g_log.information()<<i;
                      first_i = i;
                    }
                    else
                    {
                      if (first_i >= 0)
                      {
                        if (i > last_i + 1)
                        {
                          g_log.information()<<'-'<<i;
                          first_i = -1;
                        }
                      }
                      else
                      {
                        g_log.information()<<','<<i;
                        first_i = i;
                      }
                    }
                    last_i = i;
                  }
                  else
                  {
                    if (!first && first_i >= 0)
                    {
                      if (last_i > first_i)
                        g_log.information()<<'-'<<last_i;
                      first_i = -1;
                    }
                  }
                  if (first_i >= 0 && last_i > first_i)
                    g_log.information()<<'-'<<last_i;
                  g_log.information()<<'\n';
              }
            }

            //Create a workspace with only two spectra for forward and back
            DataObjects::Workspace2D_sptr  groupedWS = boost::dynamic_pointer_cast<DataObjects::Workspace2D>
              (API::WorkspaceFactory::Instance().create(localWorkspace, ngroups, localWorkspace->dataX(0).size(), localWorkspace->blocksize()));

            //Compile the groups
            for (int i = 0; i < numHists; ++i)
            {    
              specid_t k = static_cast<specid_t>(groups[ m_groupings[numHists*period + i] ]);

              for (detid_t j = 0; j < static_cast<detid_t>(localWorkspace->blocksize()); ++j)
              {
                groupedWS->dataY(k)[j] = groupedWS->dataY(k)[j] + localWorkspace->dataY(i)[j];

                //Add the errors in quadrature
                groupedWS->dataE(k)[j] 
                = sqrt(pow(groupedWS->dataE(k)[j], 2) + pow(localWorkspace->dataE(i)[j], 2));
              }

              //Copy all the X data
              groupedWS->dataX(k) = localWorkspace->dataX(i);
              ISpectrum * spec = groupedWS->getSpectrum(k);
              spec->setSpectrumNo(k+1);
              spec->setDetectorID(i+1);
            }

            m_groupings.clear();

            // All two spectra
            for(detid_t k=0; k<static_cast<detid_t>(ngroups); k++)
            {
              groupedWS->getAxis(1)->setValue(k, k + 1);
            }

            // Assign the result to the output workspace property
            if(m_numberOfPeriods>1)
              setProperty(outws, boost::dynamic_pointer_cast<Workspace>(groupedWS));
            else
            {
              setProperty("OutputWorkspace",boost::dynamic_pointer_cast<Workspace>(groupedWS));

            }

        }
        else
        {
          // Assign the result to the output workspace property
          if(m_numberOfPeriods>1)
            setProperty(outws,boost::dynamic_pointer_cast<Workspace>(localWorkspace));
          else
          {
            setProperty("OutputWorkspace",boost::dynamic_pointer_cast<Workspace>(localWorkspace));
          }

        }

      } // loop over periods

      // Clean up
      delete[] timeChannels;
    }

    /** Load in a single spectrum taken from a NeXus file
    *  @param tcbs ::     The vector containing the time bin boundaries
    *  @param hist ::     The workspace index
    *  @param i ::        The spectrum number
    *  @param nxload ::   A reference to the MuonNeXusReader object
    *  @param lengthIn :: The number of elements in a spectrum
    *  @param localWorkspace :: A pointer to the workspace in which the data will be stored
    */
    void LoadMuonNexus1::loadData(const MantidVecPtr::ptr_type& tcbs,size_t hist, specid_t& i,
      MuonNexusReader& nxload, const int64_t lengthIn, DataObjects::Workspace2D_sptr localWorkspace)
    {
      // Read in a spectrum
      // Put it into a vector, discarding the 1st entry, which is rubbish
      // But note that the last (overflow) bin is kept
      // For Nexus, not sure if above is the case, hence give all data for now
      MantidVec& Y = localWorkspace->dataY(hist);
      Y.assign(nxload.counts + i * lengthIn, nxload.counts + i * lengthIn + lengthIn);
      // Create and fill another vector for the errors, containing sqrt(count)
      MantidVec& E = localWorkspace->dataE(hist);
      typedef double (*uf)(double);
      uf dblSqrt = std::sqrt;
      std::transform(Y.begin(), Y.end(), E.begin(), dblSqrt);
      // Populate the workspace. Loop starts from 1, hence i-1
      localWorkspace->setX(hist, tcbs);
      localWorkspace->getAxis(1)->setValue(hist, static_cast<int>(hist) + 1);
    }


    /**  Log the run details from the file
    * @param localWorkspace :: The workspace details to use
    */
    void LoadMuonNexus1::loadRunDetails(DataObjects::Workspace2D_sptr localWorkspace)
    {
      API::Run & runDetails = localWorkspace->mutableRun();

      runDetails.addProperty("run_title", localWorkspace->getTitle(), true);

      int numSpectra = static_cast<int>(localWorkspace->getNumberHistograms());
      runDetails.addProperty("nspectra", numSpectra);

      NXRoot root(m_filename);
      try
      {
        std::string start_time = root.getString("run/start_time");
        runDetails.addProperty("run_start", start_time);
      }
      catch (std::runtime_error &)
      {
        g_log.warning("run/start_time is not available, run_start log not added.");
      }


      try
      {
        std::string stop_time = root.getString("run/stop_time");
        runDetails.addProperty("run_end", stop_time);
      }
      catch (std::runtime_error &)
      {
        g_log.warning("run/stop_time is not available, run_end log not added.");
      }

      try
      {
        std::string dur = root.getString("run/duration");
        runDetails.addProperty("dur", dur);
        runDetails.addProperty("durunits", 1);  // 1 means second here
        runDetails.addProperty("dur_secs", dur);  
      }
      catch (std::runtime_error &)
      {
        g_log.warning("run/duration is not available, dur log not added.");
      }

      // Get number of good frames
      NXEntry runInstrumentBeam = root.openEntry("run/instrument/beam");
      NXInfo infoGoodTotalFrames = runInstrumentBeam.getDataSetInfo("frames_good");
      if (infoGoodTotalFrames.stat != NX_ERROR)
      {
        int dum = root.getInt("run/instrument/beam/frames_good");
        runDetails.addProperty("goodfrm", dum);
      }

    }

    /// Run LoadInstrumentFromNexus as a Child Algorithm (only if loading from instrument definition file fails)
    void LoadMuonNexus1::runLoadInstrumentFromNexus(DataObjects::Workspace2D_sptr localWorkspace)
    {
      g_log.information() << "Instrument definition file not found. Attempt to load information about \n"
        << "the instrument from nexus data file.\n";

      IAlgorithm_sptr loadInst = createChildAlgorithm("LoadInstrumentFromNexus");

      // Now execute the Child Algorithm. Catch and log any error, but don't stop.
      bool executionSuccessful(true);
      try
      {
        loadInst->setPropertyValue("Filename", m_filename);
        loadInst->setProperty<MatrixWorkspace_sptr> ("Workspace", localWorkspace);
        loadInst->execute();
      }
      catch( std::invalid_argument&)
      {
        g_log.information("Invalid argument to LoadInstrument Child Algorithm");
        executionSuccessful = false;
      }
      catch (std::runtime_error&)
      {
        g_log.information("Unable to successfully run LoadInstrument Child Algorithm");
        executionSuccessful = false;
      }

      if ( !executionSuccessful ) g_log.error("No instrument definition loaded");      
    }

    /// Run the LoadLog Child Algorithm
    void LoadMuonNexus1::runLoadLog(DataObjects::Workspace2D_sptr localWorkspace)
    {
      IAlgorithm_sptr loadLog = createChildAlgorithm("LoadMuonLog");
      // Pass through the same input filename
      loadLog->setPropertyValue("Filename",m_filename);
      // Set the workspace property to be the same one filled above
      loadLog->setProperty<MatrixWorkspace_sptr>("Workspace",localWorkspace);

      // Now execute the Child Algorithm. Catch and log any error, but don't stop.
      try
      {
        loadLog->execute();
      }
      catch (std::runtime_error&)
      {
        g_log.error("Unable to successfully run LoadLog Child Algorithm");
      }
      catch (std::logic_error&)
      {
        g_log.error("Unable to successfully run LoadLog Child Algorithm");
      }

      if ( ! loadLog->isExecuted() ) g_log.error("Unable to successfully run LoadLog Child Algorithm");


      NXRoot root(m_filename);

      try
      {
        NXChar orientation = root.openNXChar("run/instrument/detector/orientation");
        // some files have no data there
        orientation.load();

        if (orientation[0] == 't')
        {
          Kernel::TimeSeriesProperty<double>* p = new Kernel::TimeSeriesProperty<double>("fromNexus");
          std::string start_time = root.getString("run/start_time");
          p->addValue(start_time,-90.0);
          localWorkspace->mutableRun().addLogData(p);
          setProperty("MainFieldDirection", "Transverse");
        }
        else
        {
          setProperty("MainFieldDirection", "Longitudinal");
        }
      }
      catch(...)
      {
        setProperty("MainFieldDirection", "Longitudinal");
      }

    }

    /**checks the file by opening it and reading few lines 
    *  @param filePath :: name of the file inluding its path
    *  @return an integer value how much this algorithm can load the file 
    */
    int LoadMuonNexus1::fileCheck(const std::string& filePath)
    {     
      try
      {
        NXRoot root(filePath);
        NXEntry entry = root.openFirstEntry();
        if ( ! entry.containsDataSet( "analysis" ) ) return 0;
        std::string versionField = "IDF_version";
        if ( ! entry.containsDataSet( versionField ) )
        {
          versionField = "idf_version";
          if ( ! entry.containsDataSet( versionField ) ) return 0;
        }
        if ( entry.getInt( versionField ) != 1 ) return 0;
        std::string definition = entry.getString( "analysis" );
        if ( definition == "muonTD" || definition == "pulsedTD" )
        {
          // If all this succeeded then we'll assume this is an ISIS Muon NeXus file version 1
          return 81;
        }
      }
      catch(...)
      {
      }
      return 0;
    }

  } // namespace DataHandling
} // namespace Mantid
