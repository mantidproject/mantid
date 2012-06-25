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

===Subalgorithms used===

The subalgorithms used by LoadMuonNexus are:
* LoadMuonLog - this reads log information from the Nexus file and uses it to create TimeSeriesProperty entries in the workspace.
* LoadInstrument - this algorithm looks for an XML description of the instrument and if found reads it.
* LoadIntstrumentFromNexus - this is called if the normal LoadInstrument fails. As the Nexus file has limited instrument data, this only populates a few fields.

*WIKI*/
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidDataHandling/LoadMuonNexus.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidAPI/FileProperty.h"
#include "MantidGeometry/Instrument/Detector.h"
#include "MantidAPI/Progress.h"
#include "MantidAPI/SpectraDetectorMap.h"
#include "MantidAPI/TableRow.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/ListValidator.h"

#include <Poco/Path.h>
#include <limits>
#include <cmath>
#include <boost/shared_ptr.hpp>
#include "MantidNexus/MuonNexusReader.h"
#include "MantidNexus/NexusClasses.h"
#include "MantidNexusCPP/NeXusFile.hpp"
#include "MantidNexusCPP/NeXusException.hpp"

namespace Mantid
{
  namespace DataHandling
  {
    // Register the algorithm into the algorithm factory
    DECLARE_ALGORITHM(LoadMuonNexus)

    /// Sets documentation strings for this algorithm
    void LoadMuonNexus::initDocs()
    {
      this->setWikiSummary("The LoadMuonNexus algorithm will read the given NeXus Muon data file Version 1 and use the results to populate the named workspace. LoadMuonNexus may be invoked by [[LoadNexus]] if it is given a NeXus file of this type. ");
      this->setOptionalMessage("The LoadMuonNexus algorithm will read the given NeXus Muon data file Version 1 and use the results to populate the named workspace. LoadMuonNexus may be invoked by LoadNexus if it is given a NeXus file of this type.");
    }


    using namespace Kernel;
    using namespace API;
    using Geometry::Instrument;
    using namespace Mantid::NeXus;

    /// Empty default constructor
    LoadMuonNexus::LoadMuonNexus() : 
    m_filename(), m_entrynumber(0), m_numberOfSpectra(0), m_numberOfPeriods(0), m_list(false),
      m_interval(false), m_spec_list(), m_spec_min(0), m_spec_max(EMPTY_INT())
    {}

    /// Initialisation method.
    void LoadMuonNexus::init()
    {
      declareProperty(new FileProperty("Filename", "", FileProperty::Load, ".nxs"),
        "The name of the Nexus file to load" );      

      declareProperty(new WorkspaceProperty<Workspace>("OutputWorkspace","",Direction::Output),
        "The name of the workspace to be created as the output of the\n"
        "algorithm. For multiperiod files, one workspace will be\n"
        "generated for each period");


      auto mustBePositive = boost::make_shared<BoundedValidator<int64_t> >();
      mustBePositive->setLower(0);
      declareProperty( "SpectrumMin",(int64_t)0, mustBePositive,
        "Index number of the first spectrum to read, only used if\n"
        "spectrum_max is set and only for single period data\n"
        "(default 0)" );
      declareProperty( "SpectrumMax",(int64_t)EMPTY_INT(), mustBePositive,
        "Index of last spectrum to read, only for single period data\n"
        "(default the last spectrum)");

      declareProperty(new ArrayProperty<specid_t>("SpectrumList"), 
        "Array, or comma separated list, of indexes of spectra to\n"
        "load");
      declareProperty("AutoGroup",false,
        "Determines whether the spectra are automatically grouped\n"
        "together based on the groupings in the NeXus file, only\n"
        "for single period data (default no)");

      declareProperty("EntryNumber", (int64_t)0, mustBePositive,
        "The particular entry number to read (default: Load all workspaces and creates a workspace group)");

      std::vector<std::string> FieldOptions;
      FieldOptions.push_back("Transverse");
      FieldOptions.push_back("Longitudinal");
      declareProperty("MainFieldDirection","Transverse", boost::make_shared<StringListValidator>(FieldOptions),
        "Output the main field direction if specified in Nexus file (default Transverse)", Direction::Output);

      declareProperty("TimeZero", 0.0, "Time zero in units of micro-seconds (default to 0.0)", Direction::Output);
      declareProperty("FirstGoodData", 0.0, "First good data in units of micro-seconds (default to 0.0)", Direction::Output);
      
      std::vector<double> defaultDeadTimes;
      declareProperty("DeadTimes", defaultDeadTimes, 
                      "The name of the vector in which to store the list of deadtimes for each spectrum", Direction::Output);
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
      catch (::NeXus::Exception&)
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
      catch (::NeXus::Exception&)
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
      catch (::NeXus::Exception&)
      {}

      NXEntry nxRun = root.openEntry("run");
      std::string title;
      std::string notes;
      try
      {
        title = nxRun.getString("title");
        notes = nxRun.getString("notes");
      }    
      catch (::NeXus::Exception&)
      {}
      std::string run_num;
      try
      {
        run_num = boost::lexical_cast<std::string>(nxRun.getInt("number"));
      }
      catch (::NeXus::Exception&)
      {}

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
      // If multiperiod, will need to hold the Instrument, Sample & SpectraDetectorMap for copying
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
            runLoadMappingTable(localWorkspace );
          }
        }

        if (period == 0)
        {
          // Only run the sub-algorithms once
          loadRunDetails(localWorkspace);
          runLoadInstrument(localWorkspace );
          runLoadMappingTable(localWorkspace );
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

            boost::shared_array<specid_t> spec(new specid_t[numHists]);
            boost::shared_array<detid_t> dets(new detid_t[numHists]);

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
              spec[i] = k + 1;
              dets[i] = i + 1;
            }

            m_groupings.clear();

            // All two spectra
            for(detid_t k=0; k<static_cast<detid_t>(ngroups); k++)
            {
              groupedWS->getAxis(1)->spectraNo(k)= k + 1;
            }

            groupedWS->replaceSpectraMap(new API::SpectraDetectorMap(spec.get(),dets.get(),numHists));

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

    /// Validates the optional 'spectra to read' properties, if they have been set
    void LoadMuonNexus::checkOptionalProperties()
    {
      //read in the settings passed to the algorithm
      m_spec_list = getProperty("SpectrumList");
      m_spec_max = getProperty("SpectrumMax");
      //Are we using a list of spectra or all the spectra in a range?
      m_list = !m_spec_list.empty();
      m_interval = (m_spec_max != EMPTY_INT());
      if ( m_spec_max == EMPTY_INT() ) m_spec_max = 0;

      // Check validity of spectra list property, if set
      if ( m_list )
      {
        const specid_t minlist = *min_element(m_spec_list.begin(),m_spec_list.end());
        const specid_t maxlist = *max_element(m_spec_list.begin(),m_spec_list.end());
        if ( maxlist > m_numberOfSpectra || minlist == 0)
        {
          g_log.error("Invalid list of spectra");
          throw std::invalid_argument("Inconsistent properties defined"); 
        } 
      }

      // Check validity of spectra range, if set
      if ( m_interval )
      {
        m_spec_min = getProperty("SpectrumMin");
        if ( m_spec_max < m_spec_min || m_spec_max > m_numberOfSpectra )
        {
          g_log.error("Invalid Spectrum min/max properties");
          throw std::invalid_argument("Inconsistent properties defined"); 
        }
      }
    }

    /** Load in a single spectrum taken from a NeXus file
    *  @param tcbs ::     The vector containing the time bin boundaries
    *  @param hist ::     The workspace index
    *  @param i ::        The spectrum number
    *  @param nxload ::   A reference to the MuonNeXusReader object
    *  @param lengthIn :: The number of elements in a spectrum
    *  @param localWorkspace :: A pointer to the workspace in which the data will be stored
    */
    void LoadMuonNexus::loadData(const MantidVecPtr::ptr_type& tcbs,size_t hist, specid_t& i,
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
      localWorkspace->getAxis(1)->spectraNo(hist)= static_cast<int>(hist) + 1;
    }


    /**  Log the run details from the file
    * @param localWorkspace :: The workspace details to use
    */
    void LoadMuonNexus::loadRunDetails(DataObjects::Workspace2D_sptr localWorkspace)
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

    /// Run the sub-algorithm LoadInstrument (or LoadInstrumentFromNexus)
    void LoadMuonNexus::runLoadInstrument(DataObjects::Workspace2D_sptr localWorkspace)
    {

      IAlgorithm_sptr loadInst = createSubAlgorithm("LoadInstrument");

      // Now execute the sub-algorithm. Catch and log any error, but don't stop.
      try
      {
        loadInst->setPropertyValue("InstrumentName", m_instrument_name);
        loadInst->setProperty<MatrixWorkspace_sptr> ("Workspace", localWorkspace);
        loadInst->setProperty("RewriteSpectraMap", false);
        loadInst->execute();
      }
      catch( std::invalid_argument&)
      {
        g_log.information("Invalid argument to LoadInstrument sub-algorithm");
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

      // Now execute the sub-algorithm. Catch and log any error, but don't stop.
      bool executionSuccessful(true);
      try
      {
        loadInst->setPropertyValue("Filename", m_filename);
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

      if ( !executionSuccessful ) g_log.error("No instrument definition loaded");      
    }

    /// Run the LoadMappingTable sub-algorithm to fill the SpectraToDetectorMap
    void LoadMuonNexus::runLoadMappingTable(DataObjects::Workspace2D_sptr localWorkspace)
    {
      NXRoot root(m_filename);
      NXInt number = root.openNXInt("run/instrument/detector/number");
      number.load();
      detid_t ndet = static_cast<detid_t>(number[0]/m_numberOfPeriods);
      boost::shared_array<detid_t> det(new detid_t[ndet]);
      for(detid_t i=0;i<ndet;i++)
      {
        det[i] = i + 1;
      }
      localWorkspace->replaceSpectraMap(new API::SpectraDetectorMap(det.get(),det.get(),ndet));
    }

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
      catch (std::logic_error&)
      {
        g_log.error("Unable to successfully run LoadLog sub-algorithm");
      }

      if ( ! loadLog->isExecuted() ) g_log.error("Unable to successfully run LoadLog sub-algorithm");


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

    /**This method does a quick file type check by looking at the first 100 bytes of the file 
    *  @param filePath- path of the file including name.
    *  @param nread :: no.of bytes read
    *  @param header :: The first 100 bytes of the file as a union
    *  @return true if the given file is of type which can be loaded by this algorithm
    */
    bool LoadMuonNexus::quickFileCheck(const std::string& filePath,size_t nread,const file_header& header)
    {
      std::string extn=extension(filePath);
      bool bnexs(false);
      (!extn.compare("nxs")||!extn.compare(".nx5"))?bnexs=true:bnexs=false;
      /*
      * HDF files have magic cookie in the first 4 bytes
      */
      if ( ((nread >= sizeof(unsigned)) && (ntohl(header.four_bytes) == g_hdf_cookie)) || bnexs )
      {
        //hdf
        return true;
      }
      else if ( (nread >= sizeof(g_hdf5_signature)) && 
        (!memcmp(header.full_hdr, g_hdf5_signature, sizeof(g_hdf5_signature))) )
      { 
        //hdf5
        return true;
      }
      return false;

    }
    /**checks the file by opening it and reading few lines 
    *  @param filePath :: name of the file inluding its path
    *  @return an integer value how much this algorithm can load the file 
    */
    int LoadMuonNexus::fileCheck(const std::string& filePath)
    {     
      using namespace ::NeXus;
      int confidence(0);
      try
      {
        ::NeXus::File file = ::NeXus::File(filePath);
        file.openPath("/run/analysis");
        std::string analysisType = file.getStrData();
        std::string compareString = "muon";
        if( analysisType.compare(0,compareString.length(),compareString) == 0  )
        {
          // If all this succeeded then we'll assume this is an ISIS Muon NeXus file
          confidence = 80;
        }
        file.close();
      }
      catch(::NeXus::Exception&)
      {
      }
      return confidence;

    }

  } // namespace DataHandling
} // namespace Mantid
