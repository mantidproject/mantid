//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidDataHandling/LoadISISNexus2.h"
#include "MantidDataHandling/LoadEventNexus.h"

#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/LogParser.h"
#include "MantidKernel/LogFilter.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidKernel/UnitFactory.h"

#include "MantidAPI/FileProperty.h"
#include "MantidAPI/RegisterFileLoader.h"

#include "MantidGeometry/Instrument/Detector.h"
#include "MantidGeometry/Instrument/XMLlogfile.h"

#include <nexus/NeXusFile.hpp>
#include <nexus/NeXusException.hpp>

#include <Poco/Path.h>
#include <Poco/DateTimeFormatter.h>
#include <Poco/DateTimeParser.h>
#include <Poco/DateTimeFormat.h>

#include <boost/lexical_cast.hpp>
#include <cmath>
#include <vector>
#include <sstream>
#include <cctype>
#include <functional>
#include <algorithm>

namespace Mantid
{
  namespace DataHandling
  {
    DECLARE_NEXUS_FILELOADER_ALGORITHM(LoadISISNexus2);

    using namespace Kernel;
    using namespace API;
    using namespace NeXus;
    using std::size_t;

    /// Empty default constructor
    LoadISISNexus2::LoadISISNexus2() : 
      m_filename(), m_instrument_name(), m_samplename(), m_numberOfSpectra(0), m_numberOfSpectraInFile(0), 
      m_numberOfPeriods(0), m_numberOfPeriodsInFile(0), m_numberOfChannels(0), m_numberOfChannelsInFile(0),
      m_have_detector(false),m_range_supplied(true), m_spec_min(0), m_spec_max(EMPTY_INT()),
      m_load_selected_spectra(false),m_specInd2specNum_map(),m_spec2det_map(),
      m_entrynumber(0), m_tof_data(), m_proton_charge(0.),
      m_spec(), m_monitors(), m_logCreator(), m_progress()
    {}

    /**
    * Return the confidence criteria for this algorithm can load the file
    * @param descriptor A descriptor for the file
    * @returns An integer specifying the confidence level. 0 indicates it will not be used
    */
    int LoadISISNexus2::confidence(Kernel::NexusDescriptor & descriptor) const
    {
      if(descriptor.pathOfTypeExists("/raw_data_1","NXentry")) return 80;
      return 0;
    }

    /// Initialization method.
    void LoadISISNexus2::init()
    {
      std::vector<std::string> exts;
      exts.push_back(".nxs");
      exts.push_back(".n*");
      declareProperty(new FileProperty("Filename", "", FileProperty::Load, exts),
        "The name of the Nexus file to load" );
      declareProperty(new WorkspaceProperty<Workspace>("OutputWorkspace","",Direction::Output));

      auto mustBePositive = boost::make_shared<BoundedValidator<int64_t> >();
      mustBePositive->setLower(0);
      declareProperty("SpectrumMin",(int64_t)0, mustBePositive);
      declareProperty("SpectrumMax",(int64_t)EMPTY_INT(), mustBePositive);
      declareProperty(new ArrayProperty<int64_t>("SpectrumList"));
      declareProperty("EntryNumber", (int64_t)0, mustBePositive,
        "The particular entry number to read (default: Load all workspaces and creates a workspace group)");
    }

    /** Executes the algorithm. Reading in the file and creating and populating
    *  the output workspace
    * 
    *  @throw Exception::FileError If the Nexus file cannot be found/opened
    *  @throw std::invalid_argument If the optional properties are set to invalid values
    */
    void LoadISISNexus2::exec()
    {
      m_filename = getPropertyValue("Filename");
      // Create the root Nexus class
      NXRoot root(m_filename);

      // "Open" the same file but with the C++ interface
      m_cppFile = new ::NeXus::File(root.m_fileID);

      // Open the raw data group 'raw_data_1'
      NXEntry entry = root.openEntry("raw_data_1");

      // Read in the instrument name from the Nexus file
      m_instrument_name = entry.getString("name");

      //Test if we have a detector block
      size_t ndets(0);
      try
      {
        NXClass det_class = entry.openNXGroup("detector_1");
        NXInt spectrum_index = det_class.openNXInt("spectrum_index");
        spectrum_index.load();
        ndets = spectrum_index.dim0();
        // We assume that this spectrum list increases monotonically
        m_spec = spectrum_index.sharedBuffer();
        m_spec_end = m_spec.get() + ndets;
        m_have_detector = true;
      }
      catch(std::runtime_error &)
      {
        ndets = 0;
      }

      NXInt nsp1 = entry.openNXInt("isis_vms_compat/NSP1");
      nsp1.load();
      NXInt udet = entry.openNXInt("isis_vms_compat/UDET");
      udet.load();
      NXInt spec = entry.openNXInt("isis_vms_compat/SPEC");
      spec.load();

      //Pull out the monitor blocks, if any exist
      size_t nmons(0);
      for(std::vector<NXClassInfo>::const_iterator it = entry.groups().begin(); 
        it != entry.groups().end(); ++it) 
      {
        if (it->nxclass == "NXmonitor") // Count monitors
        {
          NXInt index = entry.openNXInt(std::string(it->nxname) + "/spectrum_index");
          index.load();
          m_monitors[*index()] = it->nxname;
          ++nmons;
        }
      }

      if( ndets == 0 && nmons == 0 )
      {
        g_log.error() << "Invalid NeXus structure, cannot find detector or monitor blocks.";
        throw std::runtime_error("Inconsistent NeXus file structure.");
      }

      if( ndets == 0 )
      {

        //Grab the number of channels
        NXInt chans = entry.openNXInt(m_monitors.begin()->second + "/data");
        m_numberOfPeriodsInFile = m_numberOfPeriods = chans.dim0();
        m_numberOfSpectraInFile = m_numberOfSpectra = nmons;
        m_numberOfChannelsInFile = m_numberOfChannels = chans.dim2();
      }
      else 
      {
        NXData nxData = entry.openNXData("detector_1");
        NXInt data = nxData.openIntData();
        m_numberOfPeriodsInFile = m_numberOfPeriods = data.dim0();
        m_numberOfSpectraInFile = m_numberOfSpectra = nsp1[0];
        m_numberOfChannelsInFile = m_numberOfChannels = data.dim2();

        if( nmons > 0 && m_numberOfSpectra == static_cast<size_t>(data.dim1()) )
        {
          m_monitors.clear();
        }
      }
      const size_t x_length = m_numberOfChannels + 1;

      // Check input is consistent with the file, throwing if not
      checkOptionalProperties();
      // Fill up m_spectraBlocks
      size_t total_specs = prepareSpectraBlocks();

      m_progress = boost::shared_ptr<API::Progress>(new Progress(this, 0.0, 1.0, total_specs * m_numberOfPeriods));

      DataObjects::Workspace2D_sptr local_workspace = boost::dynamic_pointer_cast<DataObjects::Workspace2D>
        (WorkspaceFactory::Instance().create("Workspace2D", total_specs, x_length, m_numberOfChannels));
      // Set the units on the workspace to TOF & Counts
      local_workspace->getAxis(0)->unit() = UnitFactory::Instance().create("TOF");
      local_workspace->setYUnit("Counts");

      //
      // Load instrument and other data once then copy it later
      //
      m_progress->report("Loading instrument and run details");

      // load run details
      loadRunDetails(local_workspace, entry);

      // Test if IDF exists in Nexus otherwise load default instrument
      bool foundInstrument = LoadEventNexus::runLoadIDFFromNexus(m_filename, local_workspace, "raw_data_1", this);
      if(m_load_selected_spectra)
        m_spec2det_map = SpectrumDetectorMapping(spec(),udet(),udet.dim0());
      else
        local_workspace->updateSpectraUsing(SpectrumDetectorMapping(spec(),udet(),udet.dim0()));



      if (!foundInstrument) 
      {
        runLoadInstrument(local_workspace);
      }

      // Load logs and sample information
      m_cppFile->openPath(entry.path());      
      local_workspace->loadSampleAndLogInfoNexus(m_cppFile);

      // Load logs and sample information further information... See maintenance ticket #8697 
      loadSampleData(local_workspace, entry);
      m_progress->report("Loading logs");
      loadLogs(local_workspace, entry);

      // Load first period outside loop
      m_progress->report("Loading data");
      if( ndets > 0 )
      {
        //Get the X data
        NXFloat timeBins = entry.openNXFloat("detector_1/time_of_flight");
        timeBins.load();
        m_tof_data.reset(new MantidVec(timeBins(), timeBins() + x_length));
      }
      int64_t firstentry = (m_entrynumber > 0) ? m_entrynumber : 1;
      loadPeriodData(firstentry, entry, local_workspace);

      // Clone the workspace at this point to provide a base object for future workspace generation.
      DataObjects::Workspace2D_sptr period_free_workspace = boost::dynamic_pointer_cast<DataObjects::Workspace2D>
        (WorkspaceFactory::Instance().create(local_workspace));

      createPeriodLogs(firstentry, local_workspace);

      if( m_numberOfPeriods > 1 && m_entrynumber == 0 )
      {

        WorkspaceGroup_sptr wksp_group(new WorkspaceGroup);
        wksp_group->setTitle(local_workspace->getTitle());

        //This forms the name of the group
        const std::string base_name = getPropertyValue("OutputWorkspace") + "_";
        const std::string prop_name = "OutputWorkspace_";

        for( int p = 1; p <= m_numberOfPeriods; ++p )
        {
          std::ostringstream os;
          os << p;
          m_progress->report("Loading period " + os.str());
          if( p > 1 )
          {
            local_workspace = boost::dynamic_pointer_cast<DataObjects::Workspace2D>
              (WorkspaceFactory::Instance().create(period_free_workspace));
            loadPeriodData(p, entry, local_workspace);
            createPeriodLogs(p, local_workspace);
            // Check consistency of logs data for multi-period workspaces and raise warnings where necessary.
            validateMultiPeriodLogs(local_workspace);
          }
          declareProperty(new WorkspaceProperty<Workspace>(prop_name + os.str(), base_name + os.str(), Direction::Output));
          wksp_group->addWorkspace(local_workspace);
          setProperty(prop_name + os.str(), boost::static_pointer_cast<Workspace>(local_workspace));
        }
        // The group is the root property value
        setProperty("OutputWorkspace", boost::dynamic_pointer_cast<Workspace>(wksp_group));
      }
      else
      {
        setProperty("OutputWorkspace", boost::dynamic_pointer_cast<Workspace>(local_workspace));
      }

      // Clear off the member variable containers
      m_tof_data.reset();
      m_spec.reset();
      m_monitors.clear();
    }

    // Function object for remove_if STL algorithm
    namespace
    {
      //Check the numbers supplied are not in the range and erase the ones that are
      struct range_check
      { 
        range_check(int64_t min, int64_t max) : m_min(min), m_max(max) {}

        bool operator()(int64_t x)
        {
          return (x >= m_min && x <= m_max);
        }

      private:
        int64_t m_min;
        int64_t m_max;
      };

    }
    /**
    build the list of spectra to load and include into spectra-detectors map
    @param spec_list -- list of spectra to load 
    **/
    void  LoadISISNexus2::buildSpectraInd2SpectraNumMap(const std::vector<int64_t>  &spec_list)
    {

      int64_t ic(0);

      if(spec_list.size()>0)
      {
        auto start_point = spec_list.begin();
        for(auto it =start_point  ;it!=spec_list.end();it++)
        {
          ic = it-start_point;
          m_specInd2specNum_map.insert(std::pair<int64_t,specid_t>(ic,static_cast<specid_t>(*it)));
        }
      }
      else
      {
        if(m_range_supplied)
        {
          ic = m_specInd2specNum_map.size();
          for(int64_t i=m_spec_min;i<m_spec_max+1;i++)
            m_specInd2specNum_map.insert(std::pair<int64_t,specid_t>(i-m_spec_min+ic,static_cast<specid_t>(i)));

        }
      }
    }


    /**
    Check for a set of synthetic logs associated with multi-period log data. Raise warnings where necessary.
    */
    void LoadISISNexus2::validateMultiPeriodLogs(Mantid::API::MatrixWorkspace_sptr ws) 
    {
      const Run& run = ws->run();
      if(!run.hasProperty("current_period"))
      {
        g_log.warning("Workspace has no current_period log.");
      }
      if(!run.hasProperty("nperiods"))
      {
        g_log.warning("Workspace has no nperiods log");
      }
      if(!run.hasProperty("proton_charge_by_period"))
      {
        g_log.warning("Workspace has not proton_charge_by_period log");
      }
    }

    /**
    * Check the validity of the optional properties of the algorithm and identify if partial data should be loaded.
    */
    void LoadISISNexus2::checkOptionalProperties()
    {
      // optional properties specify that only some spectra have to be loaded
      m_load_selected_spectra = false;
      m_spec_min = getProperty("SpectrumMin");
      m_spec_max = getProperty("SpectrumMax");

      if( m_spec_min == 0 && m_spec_max == EMPTY_INT() )
      {
        m_range_supplied = false;
      }

      if( m_spec_min == 0 )
        m_spec_min = 1;
      else
        m_load_selected_spectra = true;


      if( m_spec_max == EMPTY_INT() )
        m_spec_max = m_numberOfSpectra;
      else
        m_load_selected_spectra = true;

      // Sanity check for min/max
      if( m_spec_min > m_spec_max )
      {
        throw std::invalid_argument("Inconsistent range properties. SpectrumMin is larger than SpectrumMax.");
      }

      if( static_cast<size_t>(m_spec_max) > m_numberOfSpectra )
      {
        std::string err="Inconsistent range property. SpectrumMax is larger than number of spectra: "+boost::lexical_cast<std::string>(m_numberOfSpectra ); 
        throw std::invalid_argument(err);
      }

      // Check the entry number
      m_entrynumber = getProperty("EntryNumber");
      if( static_cast<int>(m_entrynumber) > m_numberOfPeriods || m_entrynumber < 0 )
      {
        std::string err="Invalid entry number entered. File contains "+boost::lexical_cast<std::string>(m_numberOfPeriods )+ " period. ";
        throw std::invalid_argument(err);
      }
      if( m_numberOfPeriods == 1 )
      {
        m_entrynumber = 1;
      }

      //Check the list property
      std::vector<int64_t> spec_list = getProperty("SpectrumList");
      if( ! spec_list.empty() ) 
      {
        m_load_selected_spectra = true;

        // Sort the list so that we can check it's range
        std::sort(spec_list.begin(), spec_list.end());

        if( spec_list.back() > static_cast<int64_t>(m_numberOfSpectra) )
        {
          std::string err="A spectra number in the spectra list exceeds total number of "+boost::lexical_cast<std::string>(m_numberOfSpectra )+ " spectra ";
          throw std::invalid_argument(err);
        }

        //Check no negative numbers have been passed
        std::vector<int64_t>::iterator itr =
          std::find_if(spec_list.begin(), spec_list.end(), std::bind2nd(std::less<int>(), 0));
        if( itr != spec_list.end() )
        {
          throw std::invalid_argument("Negative SpectraList property encountered.");
        }
        range_check in_range(m_spec_min, m_spec_max);
        if( m_range_supplied )
        {
          spec_list.erase(remove_if(spec_list.begin(), spec_list.end(), in_range), spec_list.end());
          // combine spectra numbers from ranges and the list
          if (spec_list.size()>0)
          {
            for(int64_t i=m_spec_min;i<m_spec_max+1;i++)
              spec_list.push_back(static_cast<specid_t>(i));
            // Sort the list so that lower spectra indexes correspond to smaller spectra ID-s
            std::sort(spec_list.begin(), spec_list.end());

          }
        }
      }
      else
      {
        m_range_supplied = true;
      }
      if (m_load_selected_spectra)
      {
        buildSpectraInd2SpectraNumMap(spec_list);
      }

    }

    namespace
    {
      /// Compare two spectra blocks for ordering
      bool compareSpectraBlocks(const LoadISISNexus2::SpectraBlock& block1, const LoadISISNexus2::SpectraBlock& block2)
      {
        bool res = block1.last < block2.first;
        if ( !res )
        {
          assert( block2.last < block1.first );
        }
        return res;
      }
    }

    /**
    * Analyze the spectra ranges and prepare a list contiguous blocks. Each monitor must be
    * in a separate block.
    * @return :: Number of spectra to load.
    */
    size_t LoadISISNexus2::prepareSpectraBlocks()
    {
      std::vector<int64_t> includedMonitors;
      // fill in the data block descriptor vector
      if ( ! m_specInd2specNum_map.empty() )
      {
        auto itSpec= m_specInd2specNum_map.begin();
        int64_t hist = itSpec->second;
        SpectraBlock block(hist,hist,false);
        itSpec++;
        for(; itSpec != m_specInd2specNum_map.end(); ++itSpec)
        {
          // try to put all consecutive numbers in same block
          bool isMonitor = m_monitors.find( hist ) != m_monitors.end();
          if ( isMonitor || itSpec->second!= hist + 1 )
          {
            block.last = hist;
            block.isMonitor =  isMonitor;
            m_spectraBlocks.push_back( block );
            if ( isMonitor ) includedMonitors.push_back( hist );
            block = SpectraBlock(itSpec ->second,itSpec ->second,false);
          }
          hist = itSpec ->second;
        }
        // push the last block
        hist = m_specInd2specNum_map.rbegin()->second;
        block.last = hist;
        if ( m_monitors.find( hist ) != m_monitors.end() )
        {
          includedMonitors.push_back( hist );
          block.isMonitor =  true;
        }
        m_spectraBlocks.push_back( block );

        return m_specInd2specNum_map.size();
      }

      // put in the spectra range, possibly breaking it into parts by monitors
      int64_t first = m_spec_min;
      for(int64_t hist = first; hist < m_spec_max; ++hist)
      {
        if ( m_monitors.find( hist ) != m_monitors.end() )
        {
          if ( hist != first )
          {
            m_spectraBlocks.push_back( SpectraBlock( first, hist - 1, false ) );
          }
          m_spectraBlocks.push_back( SpectraBlock( hist, hist, true) );
          includedMonitors.push_back( hist );
          first = hist + 1;
        }
      }
      if ( first == m_spec_max && m_monitors.find( first ) != m_monitors.end() )
      {
        m_spectraBlocks.push_back( SpectraBlock( first, m_spec_max, true ) );
        includedMonitors.push_back( m_spec_max );
      }
      else
      {
        m_spectraBlocks.push_back( SpectraBlock( first, m_spec_max, false ) );
      }

      // sort and check for overlapping
      if ( m_spectraBlocks.size() > 1 )
      {
        std::sort( m_spectraBlocks.begin(), m_spectraBlocks.end(), compareSpectraBlocks );
      }

      // remove monitors that weren't requested
      if ( m_monitors.size() != includedMonitors.size() )
      {
        if ( includedMonitors.empty() )
        {
          m_monitors.clear();
        }
        else
        {
          for(auto it = m_monitors.begin(); it != m_monitors.end(); )
          {
            if ( std::find( includedMonitors.begin(), includedMonitors.end(), it->first ) == includedMonitors.end() )
            {
              auto it1 = it;
              ++it;
              m_monitors.erase( it1 );
            }
            else
            {
              ++it;
            }
          }
        }
      }
      // count the number of spectra
      size_t nSpec = 0;
      for( auto it = m_spectraBlocks.begin(); it != m_spectraBlocks.end(); ++it )
      {
        nSpec += it->last - it->first + 1;
      }
      return nSpec;
    }

    /**
    * Load a given period into the workspace
    * @param period :: The period number to load (starting from 1) 
    * @param entry :: The opened root entry node for accessing the monitor and data nodes
    * @param local_workspace :: The workspace to place the data in
    */
    void LoadISISNexus2::loadPeriodData(int64_t period, NXEntry & entry, DataObjects::Workspace2D_sptr local_workspace)
    {
      int64_t hist_index = 0;
      int64_t period_index(period - 1);
      //int64_t first_monitor_spectrum = 0;

      for(auto block = m_spectraBlocks.begin(); block != m_spectraBlocks.end(); ++block)
      {
        if ( block->isMonitor )
        {
          auto it = m_monitors.find( block->first );
          assert( it != m_monitors.end() );
          NXData monitor = entry.openNXData(it->second);
          NXInt mondata = monitor.openIntData();
          m_progress->report("Loading monitor");
          mondata.load(1,static_cast<int>(period-1)); // TODO this is just wrong
          MantidVec& Y = local_workspace->dataY(hist_index);
          Y.assign(mondata(),mondata() + m_numberOfChannels);
          MantidVec& E = local_workspace->dataE(hist_index);
          std::transform(Y.begin(), Y.end(), E.begin(), dblSqrt);

          if (m_load_selected_spectra)
          {
            //local_workspace->getAxis(1)->setValue(hist_index, static_cast<specid_t>(it->first));
            auto spec = local_workspace->getSpectrum(hist_index);
            specid_t specID = m_specInd2specNum_map.at(hist_index);
            spec->setDetectorIDs(m_spec2det_map.getDetectorIDsForSpectrumNo(specID));
            spec->setSpectrumNo(specID);
          }

          NXFloat timeBins = monitor.openNXFloat("time_of_flight");
          timeBins.load();
          local_workspace->dataX(hist_index).assign(timeBins(),timeBins() + timeBins.dim0());
          hist_index++;
        }
        else if( m_have_detector )
        {
          NXData nxdata = entry.openNXData("detector_1");
          NXDataSetTyped<int> data = nxdata.openIntData();
          data.open();
          //Start with the list members that are lower than the required spectrum
          const int * const spec_begin = m_spec.get();
          // When reading in blocks we need to be careful that the range is exactly divisible by the block-size
          // and if not have an extra read of the left overs
          const int64_t blocksize = 8;
          const int64_t rangesize = block->last - block->first + 1;
          const int64_t fullblocks = rangesize / blocksize;
          int64_t spectra_no = block->first;

          // For this to work correctly, we assume that the spectrum list increases monotonically
          int64_t filestart = std::lower_bound(spec_begin,m_spec_end,spectra_no) - spec_begin;
          if( fullblocks > 0 )
          {
            for(int64_t i = 0; i < fullblocks; ++i)
            {
              loadBlock(data, blocksize, period_index, filestart, hist_index, spectra_no, local_workspace);
              filestart += blocksize;
            }
          }
          int64_t finalblock = rangesize - (fullblocks * blocksize);
          if( finalblock > 0 )
          {
            loadBlock(data, finalblock, period_index, filestart, hist_index, spectra_no,  local_workspace);
          }
        }
      }

      try
      {
        const std::string title = entry.getString("title");
        local_workspace->setTitle(title);
        // write the title into the log file (run object)
        local_workspace->mutableRun().addProperty("run_title", title, true);
      }
      catch (std::runtime_error &)
      {
        g_log.debug() << "No title was found in the input file, " << getPropertyValue("Filename") << std::endl;
      }
    }


    /**
    * Creates period log data in the workspace
    * @param period :: period number
    * @param local_workspace :: workspace to add period log data to.
    */
    void LoadISISNexus2::createPeriodLogs(int64_t period, DataObjects::Workspace2D_sptr local_workspace)
    {
      m_logCreator->addPeriodLogs(static_cast<int>(period), local_workspace->mutableRun());
    }


    /**
    * Perform a call to nxgetslab, via the NexusClasses wrapped methods for a given block-size
    * @param data :: The NXDataSet object
    * @param blocksize :: The block-size to use
    * @param period :: The period number
    * @param start :: The index within the file to start reading from (zero based)
    * @param hist :: The workspace index to start reading into
    * @param spec_num :: The spectrum number that matches the hist variable
    * @param local_workspace :: The workspace to fill the data with
    */
    void LoadISISNexus2::loadBlock(NXDataSetTyped<int> & data, int64_t blocksize, int64_t period, int64_t start,
      int64_t &hist, int64_t& spec_num,
      DataObjects::Workspace2D_sptr local_workspace)
    {
      data.load(static_cast<int>(blocksize), static_cast<int>(period), static_cast<int>(start)); // TODO this is just wrong
      int *data_start = data();
      int *data_end = data_start + m_numberOfChannels;
      int64_t final(hist + blocksize);
      while( hist < final )
      {
        m_progress->report("Loading data");
        MantidVec& Y = local_workspace->dataY(hist);
        Y.assign(data_start, data_end);
        data_start += m_numberOfChannels; data_end += m_numberOfChannels;
        MantidVec& E = local_workspace->dataE(hist);
        std::transform(Y.begin(), Y.end(), E.begin(), dblSqrt);
        // Populate the workspace. Loop starts from 1, hence i-1
        local_workspace->setX(hist, m_tof_data);
        if (m_load_selected_spectra)
        {
          //local_workspace->getAxis(1)->setValue(hist, static_cast<specid_t>(spec_num));
          auto spec = local_workspace->getSpectrum(hist);
          specid_t specID = m_specInd2specNum_map.at(hist);
          // set detectors corresponding to spectra Number
          spec->setDetectorIDs(m_spec2det_map.getDetectorIDsForSpectrumNo(specID));
          // set correct spectra Number          
          spec->setSpectrumNo(specID);
        }

        ++hist;
        ++spec_num;
      }
    }

    /// Run the Child Algorithm LoadInstrument (or LoadInstrumentFromNexus)
    void LoadISISNexus2::runLoadInstrument(DataObjects::Workspace2D_sptr localWorkspace)
    {

      IAlgorithm_sptr loadInst = createChildAlgorithm("LoadInstrument");

      // Now execute the Child Algorithm. Catch and log any error, but don't stop.
      bool executionSuccessful(true);
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
        executionSuccessful = false;
      }
      catch (std::runtime_error&)
      {
        g_log.information("Unable to successfully run LoadInstrument Child Algorithm");
        executionSuccessful = false;
      }
      if( executionSuccessful )
      {
        // If requested update the instrument to positions in the data file
        const Geometry::ParameterMap & pmap = localWorkspace->instrumentParameters();
        if( pmap.contains(localWorkspace->getInstrument()->getComponentID(),"det-pos-source") )
        {
          boost::shared_ptr<Geometry::Parameter> updateDets = pmap.get(localWorkspace->getInstrument()->getComponentID(),"det-pos-source");
          std::string value = updateDets->value<std::string>();
          if(value.substr(0,8)  == "datafile" )
          {
            IAlgorithm_sptr updateInst = createChildAlgorithm("UpdateInstrumentFromFile");
            updateInst->setProperty<MatrixWorkspace_sptr>("Workspace", localWorkspace);
            updateInst->setPropertyValue("Filename", m_filename);
            if(value  == "datafile-ignore-phi" )
            {
              updateInst->setProperty("IgnorePhi", true);
              g_log.information("Detector positions in IDF updated with positions in the data file except for the phi values");
            }
            else 
            {
              g_log.information("Detector positions in IDF updated with positions in the data file");
            }
            // We want this to throw if it fails to warn the user that the information is not correct.
            updateInst->execute();
          }
        }
      }

    }

    /**
    * Load data about the run
    *   @param local_workspace :: The workspace to load the run information in to
    *   @param entry :: The Nexus entry
    */
    void LoadISISNexus2::loadRunDetails(DataObjects::Workspace2D_sptr local_workspace, NXEntry & entry)
    {
      API::Run & runDetails = local_workspace->mutableRun();
      // Charge is stored as a float
      m_proton_charge = static_cast<double>(entry.getFloat("proton_charge"));
      runDetails.setProtonCharge(m_proton_charge);

      std::string run_num = boost::lexical_cast<std::string>(entry.getInt("run_number"));
      runDetails.addProperty("run_number", run_num);

      //
      // Some details are only stored in the VMS comparability block so we'll pull everything from there
      // for consistency

      NXClass vms_compat = entry.openNXGroup("isis_vms_compat");
      // Run header
      NXChar char_data = vms_compat.openNXChar("HDR");
      char_data.load();

      // Space-separate the fields
      char * nxsHdr = char_data();
      char header[86] = {};
      const size_t byte = sizeof(char);
      const char fieldSep(' ');
      size_t fieldWidths[7] = {3, 5, 20, 24, 12, 8, 8};

      char * srcStart = nxsHdr;
      char * destStart = header;
      for(size_t i = 0; i < 7; ++i)
      {
        size_t width = fieldWidths[i];
        memcpy(destStart, srcStart, width*byte);
        if( i < 6 ) // no space after last field
        {
          srcStart += width;
          destStart += width;
          memset(destStart, fieldSep, byte); // insert separator
          destStart += 1;
        }
      }
      runDetails.addProperty("run_header", std::string(header, header + 86));

      // Data details on run not the workspace
      runDetails.addProperty("nspectra", static_cast<int>(m_numberOfSpectraInFile));
      runDetails.addProperty("nchannels", static_cast<int>(m_numberOfChannelsInFile));
      runDetails.addProperty("nperiods", static_cast<int>(m_numberOfPeriodsInFile));

      // RPB struct info
      NXInt rpb_int = vms_compat.openNXInt("IRPB");
      rpb_int.load();
      runDetails.addProperty("dur", rpb_int[0]);        // actual run duration
      runDetails.addProperty("durunits", rpb_int[1]);   // scaler for above (1=seconds)
      runDetails.addProperty("dur_freq", rpb_int[2]);  // testinterval for above (seconds)
      runDetails.addProperty("dmp", rpb_int[3]);       // dump interval
      runDetails.addProperty("dmp_units", rpb_int[4]);  // scaler for above
      runDetails.addProperty("dmp_freq", rpb_int[5]);   // interval for above
      runDetails.addProperty("freq", rpb_int[6]);       // 2**k where source frequency = 50 / 2**k

      // Now double data
      NXFloat rpb_dbl = vms_compat.openNXFloat("RRPB");
      rpb_dbl.load();
      runDetails.addProperty("gd_prtn_chrg", static_cast<double>(rpb_dbl[7]));  // good proton charge (uA.hour)
      runDetails.addProperty("tot_prtn_chrg", static_cast<double>(rpb_dbl[8])); // total proton charge (uA.hour)
      runDetails.addProperty("goodfrm",rpb_int[9]);     // good frames
      runDetails.addProperty("rawfrm", rpb_int[10]);    // raw frames
      runDetails.addProperty("dur_wanted", rpb_int[11]); // requested run duration (units as for "duration" above)
      runDetails.addProperty("dur_secs", rpb_int[12]);  // actual run duration in seconds
      runDetails.addProperty("mon_sum1", rpb_int[13]);  // monitor sum 1
      runDetails.addProperty("mon_sum2", rpb_int[14]);  // monitor sum 2
      runDetails.addProperty("mon_sum3",rpb_int[15]);   // monitor sum 3

      // End date and time is stored separately in ISO format in the "raw_data1/endtime" class
      char_data = entry.openNXChar("end_time");
      char_data.load();
      std::string end_time_iso = std::string(char_data(), 19);
      runDetails.addProperty("run_end", end_time_iso);

      char_data = entry.openNXChar("start_time");
      char_data.load();
      std::string start_time_iso = std::string(char_data(), 19);
      runDetails.addProperty("run_start", start_time_iso);


      runDetails.addProperty("rb_proposal",rpb_int[21]); // RB (proposal) number
      vms_compat.close();
    }

    /**
    * Parse an ISO formatted date-time string into separate date and time strings
    * @param datetime_iso :: The string containing the ISO formatted date-time
    * @param date :: An output parameter containing the date from the original string or ??-??-???? if the format is unknown
    * @param time :: An output parameter containing the time from the original string or ??-??-?? if the format is unknown
    */
    void LoadISISNexus2::parseISODateTime(const std::string & datetime_iso, std::string & date, std::string & time) const
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

    /**
    * Load data about the sample
    *   @param local_workspace :: The workspace to load the logs to.
    *   @param entry :: The Nexus entry
    */
    void LoadISISNexus2::loadSampleData(DataObjects::Workspace2D_sptr local_workspace, NXEntry & entry)
    {
      /// Sample geometry
      NXInt spb = entry.openNXInt("isis_vms_compat/SPB");
      // Just load the index we need, not the whole block. The flag is the third value in
      spb.load(1, 2);
      int geom_id = spb[0];
      local_workspace->mutableSample().setGeometryFlag(spb[0]);

      NXFloat rspb = entry.openNXFloat("isis_vms_compat/RSPB");
      // Just load the indices we need, not the whole block. The values start from the 4th onward
      rspb.load(3, 3);
      double thick(rspb[0]), height(rspb[1]), width(rspb[2]);
      local_workspace->mutableSample().setThickness(thick);
      local_workspace->mutableSample().setHeight(height);
      local_workspace->mutableSample().setWidth(width);

      g_log.debug() << "Sample geometry -  ID: " << geom_id << ", thickness: " << thick << ", height: " << height << ", width: " << width << "\n";
    }

    /**  Load logs from Nexus file. Logs are expected to be in
    *   /raw_data_1/runlog group of the file. Call to this method must be done
    *   within /raw_data_1 group.
    *   @param ws :: The workspace to load the logs to.
    *   @param entry :: Nexus entry
    */
    void LoadISISNexus2::loadLogs(DataObjects::Workspace2D_sptr ws, NXEntry & entry)
    {
      IAlgorithm_sptr alg = createChildAlgorithm("LoadNexusLogs", 0.0, 0.5);
      alg->setPropertyValue("Filename", this->getProperty("Filename"));
      alg->setProperty<MatrixWorkspace_sptr>("Workspace", ws);
      try
      {
        alg->executeAsChildAlg();
      }
      catch(std::runtime_error&)
      {
        g_log.warning() << "Unable to load run logs. There will be no log "
          << "data associated with this workspace\n";
        return;
      }
      // For ISIS Nexus only, fabricate an additional log containing an array of proton charge information from the periods group.
      try
      {
        NXClass protonChargeClass = entry.openNXGroup("periods");
        NXFloat periodsCharge = protonChargeClass.openNXFloat("proton_charge");
        periodsCharge.load();
        size_t nperiods = periodsCharge.dim0();
        std::vector<double> chargesVector(nperiods);
        std::copy(periodsCharge(), periodsCharge() + nperiods, chargesVector.begin());
        ArrayProperty<double>* protonLogData = new ArrayProperty<double>("proton_charge_by_period", chargesVector);
        ws->mutableRun().addProperty(protonLogData);  
      }
      catch(std::runtime_error&)
      {
        this->g_log.debug("Cannot read periods information from the nexus file. This group may be absent.");
      }
      // Populate the instrument parameters.
      ws->populateInstrumentParameters();

      // Make log creator object and add the run status log
      m_logCreator.reset(new ISISRunLogs(ws->run(), m_numberOfPeriods));
      m_logCreator->addStatusLog(ws->mutableRun());
    }

    double LoadISISNexus2::dblSqrt(double in)
    {
      return sqrt(in);
    }

  } // namespace DataHandling
} // namespace Mantid
