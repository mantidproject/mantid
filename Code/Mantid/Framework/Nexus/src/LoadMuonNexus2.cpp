//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidNexus/LoadMuonNexus2.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidAPI/FileProperty.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidGeometry/Instrument/Detector.h"
#include "MantidAPI/Progress.h"
#include "MantidAPI/SpectraDetectorMap.h"
#include "MantidNexus/NexusClasses.h"

#include "MantidNexus/NeXusFile.hpp"
#include "MantidNexus/NeXusException.hpp"

#include "MantidAPI/LoadAlgorithmFactory.h"
#include <Poco/Path.h>
#include <boost/lexical_cast.hpp>
#include <boost/shared_ptr.hpp>

#include <cmath>
#include <numeric>

namespace Mantid
{
  namespace NeXus
  {
    // Register the algorithm into the algorithm factory
    DECLARE_ALGORITHM(LoadMuonNexus2)
    DECLARE_LOADALGORITHM(LoadMuonNexus2)
    using namespace Kernel;
    using namespace API;
    using Geometry::IInstrument;

    /// Empty default constructor
    LoadMuonNexus2::LoadMuonNexus2() : LoadMuonNexus()
    {}

    /** Executes the algorithm. Reading in the file and creating and populating
    *  the output workspace
    * 
    *  @throw Exception::FileError If the Nexus file cannot be found/opened
    *  @throw std::invalid_argument If the optional properties are set to invalid values
    */
    void LoadMuonNexus2::exec()
    {
      // Create the root Nexus class
      NXRoot root(getPropertyValue("Filename"));

      int iEntry = getProperty("EntryNumber");
      if (iEntry >= static_cast<int>(root.groups().size()) )
      {
        throw std::invalid_argument("EntryNumber is out of range");
      }

      // Open the data entry
      std::string entryName = root.groups()[iEntry].nxname;
      NXEntry entry = root.openEntry(entryName);

      NXInfo info = entry.getDataSetInfo("definition");
      if (info.stat == NX_ERROR)
      {
        info = entry.getDataSetInfo("analysis");
        if (info.stat == NX_OK && entry.getString("analysis") == "muonTD")
        {
          LoadMuonNexus::exec();
          return;
        }
        else
        {
          throw std::runtime_error("Unknown Muon Nexus file format");
        }
      }
      else
      {
        std::string definition = entry.getString("definition");
        if (info.stat == NX_ERROR || entry.getString("definition") != "pulsedTD")
        {
          throw std::runtime_error("Unknown Muon Nexus file format");
        }
      }

      // Read in the instrument name from the Nexus file
      m_instrument_name = entry.getString("instrument/name");
      
      // Read the number of periods in this file
      m_numberOfPeriods = entry.getInt("run/number_periods");

      // Need to extract the user-defined output workspace name
      Property *ws = getProperty("OutputWorkspace");
      std::string localWSName = ws->value();
      // If multiperiod, will need to hold the Instrument, Sample & SpectraDetectorMap for copying
      boost::shared_ptr<IInstrument> instrument;
      //-      boost::shared_ptr<SpectraDetectorMap> specMap;
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

      NXInt spectrum_index = dataGroup.openNXInt("spectrum_index");
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
        total_specs = m_spec_list.size();
        if (m_interval)
        {
          total_specs += (m_spec_max-m_spec_min+1);
        }
        else
        {
          m_spec_max = -1; // to stop entering the min - max loop
        }
      }
      else
      {
        total_specs = m_numberOfSpectra;
        // for nexus return all spectra
        m_spec_min = 1;
        m_spec_max = m_numberOfSpectra;  // was +1?
      }

      // Create the 2D workspace for the output
      DataObjects::Workspace2D_sptr localWorkspace = boost::dynamic_pointer_cast<DataObjects::Workspace2D>
        (WorkspaceFactory::Instance().create("Workspace2D",total_specs,nBins+1,nBins));
      // Set the unit on the workspace to TOF
      localWorkspace->getAxis(0)->unit() = UnitFactory::Instance().create("TOF");
      localWorkspace->setYUnit("Counts");

      //g_log.error()<<" number of perioids= "<<m_numberOfPeriods<<std::endl;
      WorkspaceGroup_sptr wsGrpSptr=WorkspaceGroup_sptr(new WorkspaceGroup);
      wsGrpSptr->setTitle(entry.getString("title"));

      if(m_numberOfPeriods>1)
      {	
        setProperty("OutputWorkspace",boost::dynamic_pointer_cast<Workspace>(wsGrpSptr));
      }

      NXInt period_index = dataGroup.openNXInt("period_index");
      period_index.load();

      NXInt counts = dataGroup.openIntData();
      counts.load();


      NXEntry entryTimeZero = root.openEntry("run/instrument/detector_fb");
      NXInfo infoTimeZero = entryTimeZero.getDataSetInfo("time_zero");
      if (infoTimeZero.stat != NX_ERROR)
      {
        double dum = root.getFloat("run/instrument/detector_fb/time_zero");
        setProperty("TimeZero", dum);
      }

      NXEntry entryFGB = root.openEntry("run/instrument/detector_fb");
      NXInfo infoFGB = entryFGB.getDataSetInfo("first_good_time");
      if (infoFGB.stat != NX_ERROR)
      {
        double dum = root.getFloat("run/instrument/detector_fb/first_good_time");
        setProperty("FirstGoodData", dum);
      }


      API::Progress progress(this,0.,1.,m_numberOfPeriods * total_specs);
      // Loop over the number of periods in the Nexus file, putting each period in a separate workspace
      for (int period = 0; period < m_numberOfPeriods; ++period) {

        if (period == 0)
        {
          // Only run the sub-algorithms once
          loadRunDetails(localWorkspace);
          runLoadInstrument(localWorkspace );
          localWorkspace->mutableSpectraMap().populate(spectrum_index(),spectrum_index(),m_numberOfSpectra);
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
          if(wsGrpSptr)wsGrpSptr->add(WSName);
        }

        // create spectrum -> index correspondence
        std::map<int,int> index_spectrum;
        for (int i = 0; i < m_numberOfSpectra; ++i)
        {
          index_spectrum[spectrum_index[i]] = i;
        }

        int counter = 0;
        for (int spec = m_spec_min; spec <= m_spec_max; ++spec)
        {
          int i = index_spectrum[spec]; // if spec not found i is 0
          loadData(counts,timeBins,counter,period,i,localWorkspace);
          localWorkspace->getAxis(1)->spectraNo(counter) = spectrum_index[i];
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
            localWorkspace->getAxis(1)->spectraNo(counter) = spectrum_index[k];
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
    void LoadMuonNexus2::loadData(const NXInt& counts,const std::vector<double>& timeBins,int wsIndex,
      int period,int spec,API::MatrixWorkspace_sptr localWorkspace)
    {
      MantidVec& X = localWorkspace->dataX(wsIndex);
      MantidVec& Y = localWorkspace->dataY(wsIndex);
      MantidVec& E = localWorkspace->dataE(wsIndex);
      int nBins = counts.dim2();
      assert( nBins+1 == static_cast<int>(timeBins.size()) );
      X.assign(timeBins.begin(),timeBins.end());
      int *data = &counts(period,spec,0);
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

      for(std::vector<NXClassInfo>::const_iterator it=runlogs.groups().begin();it!=runlogs.groups().end();it++)
      {
        NXLog nxLog = runlogs.openNXLog(it->nxname);
        Kernel::Property* logv = nxLog.createTimeSeries(start_time);
        if (!logv) continue;
        ws->mutableRun().addLogData(logv);
      }

      ws->setTitle(entry.getString("title"));

      std::string run_num = boost::lexical_cast<std::string>(entry.getInt("run_number"));
      //The sample is left to delete the property
      ws->mutableRun().addLogData(new PropertyWithValue<std::string>("run_number", run_num));
    
      ws->populateInstrumentParameters();
    }

    /**This method does a quick file type check by looking at the first 100 bytes of the file 
    *  @param filePath- path of the file including name.
    *  @param nread :: no.of bytes read
    *  @param header :: The first 100 bytes of the file as a union
    *  @return true if the given file is of type which can be loaded by this algorithm
    */
    bool LoadMuonNexus2::quickFileCheck(const std::string& filePath,size_t nread,const file_header& header)
    {
      std::string extn=extension(filePath);
      bool bnexs(false);
      (!extn.compare("nxs")||!extn.compare("nx5"))?bnexs=true:bnexs=false;
      /*
      * HDF files have magic cookie in the first 4 bytes
      */
      if ( ((nread >= sizeof(unsigned)) && (ntohl(header.four_bytes) == g_hdf_cookie)) || bnexs )
      {
        //hdf
        return true;
      }
      else if ( (nread >= sizeof(g_hdf5_signature)) && (!memcmp(header.full_hdr, g_hdf5_signature, sizeof(g_hdf5_signature)))  )
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
    int LoadMuonNexus2::fileCheck(const std::string& filePath)
    {   
      int confidence(0);
      try
      {
	::NeXus::File file = ::NeXus::File(filePath);
	file.openPath("/run/analysis");
	std::string analysisType = file.getStrData();
	if( analysisType == "pulsedTD" )
	{
	  confidence = 80;
	}
	else if( analysisType == "muonTD" )
	{
	  confidence = 50;
	}
	else confidence = 0;
	file.close();
      }
      catch(::NeXus::Exception&)
      {
	confidence = 0;
      }
      return confidence;
    }

    /**  Log the run details from the file
    * @param localWorkspace :: The workspace details to use
    */
    void LoadMuonNexus2::loadRunDetails(DataObjects::Workspace2D_sptr localWorkspace)
    {
      API::Run & runDetails = localWorkspace->mutableRun();

      runDetails.addProperty("run_title", localWorkspace->getTitle());
 
      int numSpectra = localWorkspace->getNumberHistograms();
      runDetails.addProperty("nspectra", numSpectra);

      m_filename = getPropertyValue("Filename");
      NXRoot root(m_filename);
      std::string start_time = root.getString("run/start_time");
      runDetails.addProperty("run_start", start_time);
      std::string stop_time = root.getString("run/end_time");
      runDetails.addProperty("run_end", stop_time);
    }

  } // namespace DataHandling
} // namespace Mantid
