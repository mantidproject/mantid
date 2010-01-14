//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidNexus/LoadMuonNexus2.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/FileProperty.h"
#include "MantidGeometry/Instrument/Detector.h"
#include "MantidAPI/Progress.h"
#include "MantidAPI/SpectraDetectorMap.h"
#include "MantidKernel/TimeSeriesProperty.h"

#include "Poco/Path.h"

#include <cmath>
#include <boost/shared_ptr.hpp>
#include "MantidNexus/NexusClasses.h"


namespace Mantid
{
  namespace NeXus
  {
    // Register the algorithm into the algorithm factory
    DECLARE_ALGORITHM(LoadMuonNexus2)

    using namespace Kernel;
    using namespace API;

    /// Empty default constructor
    LoadMuonNexus2::LoadMuonNexus2() : 
    Algorithm(),
      m_filename(), m_entrynumber(0), m_numberOfSpectra(0), m_numberOfPeriods(0), m_list(false),
      m_interval(false), m_spec_list(), m_spec_min(0), m_spec_max(EMPTY_INT())
    {}

    /// Initialisation method.
    void LoadMuonNexus2::init()
    {
      std::vector<std::string> exts;
      exts.push_back("nxs");
      declareProperty(new FileProperty("Filename", "", FileProperty::Load, exts),
        "The name of the Nexus file to load" );      
      /* declareProperty(new WorkspaceProperty<DataObjects::Workspace2D>("OutputWorkspace","",Direction::Output),
      "The name of the workspace to be created as the output of the\n"
      "algorithm. For multiperiod files, one workspace will be\n"
      "generated for each period");*/
      declareProperty(new WorkspaceProperty<Workspace>("OutputWorkspace","",Direction::Output),
        "The name of the workspace to be created as the output of the\n"
        "algorithm. For multiperiod files, one workspace will be\n"
        "generated for each period");


      BoundedValidator<int> *mustBePositive = new BoundedValidator<int>();
      mustBePositive->setLower(0);
      declareProperty( "SpectrumMin",0, mustBePositive,
        "Index number of the first spectrum to read, only used if\n"
        "spectrum_max is set and only for single period data\n"
        "(default 0)" );
      declareProperty( "SpectrumMax", EMPTY_INT(), mustBePositive->clone(),
        "Index of last spectrum to read, only for single period data\n"
        "(default the last spectrum)");

      declareProperty(new ArrayProperty<int>("SpectrumList"), 
        "Array, or comma separated list, of indexes of spectra to\n"
        "load");
      declareProperty("AutoGroup",false,
        "Determines whether the spectra are automatically grouped\n"
        "together based on the groupings in the NeXus file, only\n"
        "for single period data (default no)");

      declareProperty("EntryNumber", 0, mustBePositive->clone(),
        "The particular entry number to read (default: Load all workspaces and creates a workspace group)");
    }

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

      // Open the raw data group 'raw_data_1'
      NXEntry entry = root.openEntry("run");

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

      NXClass detectorGroup = entry.openNXGroup("instrument/detector_fb");

      NXInt spectrum_index = detectorGroup.openNXInt("spectrum_index");
      spectrum_index.load();
      m_numberOfSpectra = spectrum_index.dim0();

      // Call private method to validate the optional parameters, if set
      checkOptionalProperties();

      NXFloat timeBins = detectorGroup.openNXFloat("raw_time");
      timeBins.load();
      int nBins = timeBins.dim0();

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

      // Create the 2D workspace for the output
      DataObjects::Workspace2D_sptr localWorkspace = boost::dynamic_pointer_cast<DataObjects::Workspace2D>
        (WorkspaceFactory::Instance().create("Workspace2D",total_specs,nBins,nBins-1));
      // Set the unit on the workspace to TOF
      localWorkspace->getAxis(0)->unit() = UnitFactory::Instance().create("TOF");

      //g_log.error()<<" number of perioids= "<<m_numberOfPeriods<<std::endl;
      WorkspaceGroup_sptr wsGrpSptr=WorkspaceGroup_sptr(new WorkspaceGroup);
      if(m_numberOfPeriods>1)
      {	
        if(wsGrpSptr)wsGrpSptr->add(localWSName);
        setProperty("OutputWorkspace",boost::dynamic_pointer_cast<Workspace>(wsGrpSptr));
      }

      NXInt period_index = detectorGroup.openNXInt("period_index");
      period_index.load();

      NXData dataGroup = entry.openNXData("detector_fb");
      NXInt counts = dataGroup.openIntData();
      counts.load();

      API::Progress progress(this,0.,1.,m_numberOfPeriods * total_specs);
      // Loop over the number of periods in the Nexus file, putting each period in a separate workspace
      for (int period = 0; period < m_numberOfPeriods; ++period) {
        //if(m_entrynumber!=0)
        //{
        //  period=m_entrynumber-1;
        //  if(period!=0)
        //  {
        //    runLoadInstrument(localWorkspace );
        //    runLoadMappingTable(localWorkspace );
        //  }
        //}

        if (period == 0)
        {
          // Only run the sub-algorithms once
          runLoadInstrument(localWorkspace );
          localWorkspace->mutableSpectraMap().populate(spectrum_index(),spectrum_index(),m_numberOfSpectra);
        }
        else   // We are working on a higher period of a multiperiod raw file
        {
          localWorkspace =  boost::dynamic_pointer_cast<DataObjects::Workspace2D>
            (WorkspaceFactory::Instance().create(localWorkspace));
          localWorkspace->newSample();
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
          declareProperty(new WorkspaceProperty<DataObjects::Workspace2D>(outws,WSName,Direction::Output));
          if(wsGrpSptr)wsGrpSptr->add(WSName);
        }

        loadLogs(localWorkspace, entry, period);

        int counter = 0;
        for (int i = m_spec_min; i < m_spec_max; ++i)
        {
          loadData(counts,timeBins(),counter,period,i,localWorkspace);
          localWorkspace->getAxis(1)->spectraNo(counter) = spectrum_index[i];
          counter++;
          progress.report();
        }
        // Read in the spectra in the optional list parameter, if set
        if (m_list)
        {
          for(unsigned int i=0; i < m_spec_list.size(); ++i)
          {
            int k = m_spec_list[i];
            loadData(counts,timeBins(),counter,period,k,localWorkspace);
            localWorkspace->getAxis(1)->spectraNo(counter) = spectrum_index[k];
            counter++;
            progress.report();
          }
        }
        // Just a sanity check
        assert(counter == total_specs);

    //    bool autogroup = getProperty("AutoGroup");

    //    if (autogroup)
    //    {

    //      //Get the groupings
    //      int max_group = 0;
    //      // use a map for mapping group number and output workspace index in case 
    //      // there are group numbers > number of groups
    //      std::map<int,int> groups;
    //      m_groupings.resize(nxload.numDetectors);
    //      bool thereAreZeroes = false;
    //      for (int i =0; i < nxload.numDetectors; ++i)
    //      {
    //        int ig = nxload.detectorGroupings[i];
    //        if (ig == 0)
    //        {
    //          thereAreZeroes = true;
    //          continue;
    //        }
    //        m_groupings[i] = ig;
    //        if (groups.find(ig) == groups.end())
    //          groups[ig] = groups.size();
    //        if (ig > max_group) max_group = ig;
    //      }

    //      if (thereAreZeroes)
    //        for (int i =0; i < nxload.numDetectors; ++i)
    //        {
    //          int ig = nxload.detectorGroupings[i];
    //          if (ig == 0)
    //          {
    //            ig = ++max_group;
    //            m_groupings[i] = ig;
    //            groups[ig] = groups.size();
    //          }
    //        }

    //        int numHists = localWorkspace->getNumberHistograms();
    //        int ngroups = int(groups.size()); // number of groups

    //        // to output groups in ascending order
    //        {
    //          int i=0;
    //          for(std::map<int,int>::iterator it=groups.begin();it!=groups.end();it++,i++)
    //          {
    //            it->second = i;
    //            g_log.information()<<"group "<<it->first<<": ";
    //            bool first = true;
    //            int first_i,last_i;
    //            for(int i=0;i<numHists;i++)
    //              if (m_groupings[i] == it->first)
    //              {
    //                if (first) 
    //                {
    //                  first = false;
    //                  g_log.information()<<i;
    //                  first_i = i;
    //                }
    //                else
    //                {
    //                  if (first_i >= 0)
    //                  {
    //                    if (i > last_i + 1)
    //                    {
    //                      g_log.information()<<'-'<<i;
    //                      first_i = -1;
    //                    }
    //                  }
    //                  else
    //                  {
    //                    g_log.information()<<','<<i;
    //                    first_i = i;
    //                  }
    //                }
    //                last_i = i;
    //              }
    //              else
    //              {
    //                if (!first && first_i >= 0)
    //                {
    //                  if (last_i > first_i)
    //                    g_log.information()<<'-'<<last_i;
    //                  first_i = -1;
    //                }
    //              }
    //              if (first_i >= 0 && last_i > first_i)
    //                g_log.information()<<'-'<<last_i;
    //              g_log.information()<<'\n';
    //          }
    //        }

    //        //Create a workspace with only two spectra for forward and back
    //        DataObjects::Workspace2D_sptr  groupedWS = boost::dynamic_pointer_cast<DataObjects::Workspace2D>
    //          (API::WorkspaceFactory::Instance().create(localWorkspace, ngroups, localWorkspace->dataX(0).size(), localWorkspace->blocksize()));

    //        boost::shared_array<int> spec(new int[numHists]);
    //        boost::shared_array<int> dets(new int[numHists]);

    //        //Compile the groups
    //        for (int i = 0; i < numHists; ++i)
    //        {    
    //          int k = groups[ m_groupings[numHists*period + i] ];

    //          for (int j = 0; j < localWorkspace->blocksize(); ++j)
    //          {
    //            groupedWS->dataY(k)[j] = groupedWS->dataY(k)[j] + localWorkspace->dataY(i)[j];

    //            //Add the errors in quadrature
    //            groupedWS->dataE(k)[j] 
    //            = sqrt(pow(groupedWS->dataE(k)[j], 2) + pow(localWorkspace->dataE(i)[j], 2));
    //          }

    //          //Copy all the X data
    //          groupedWS->dataX(k) = localWorkspace->dataX(i);
    //          spec[i] = k + 1;
    //          dets[i] = i + 1;
    //        }

    //        m_groupings.clear();

    //        // All two spectra
    //        for(int k=0;k<ngroups;k++)
    //        {
    //          groupedWS->getAxis(1)->spectraNo(k)= k + 1;
    //        }

    //        groupedWS->mutableSpectraMap().populate(spec.get(),dets.get(),numHists);

    //        // Assign the result to the output workspace property
    //        if(m_numberOfPeriods>1)
    //          setProperty(outws,groupedWS);
    //        else
    //        {
    //          setProperty("OutputWorkspace",boost::dynamic_pointer_cast<Workspace>(groupedWS));

    //        }

    //    }
    //    else
    //    {
    //      // Assign the result to the output workspace property
          if(m_numberOfPeriods>1)
            setProperty(outws,localWorkspace);
          else
          {
            setProperty("OutputWorkspace",boost::dynamic_pointer_cast<Workspace>(localWorkspace));
          }

    //    }

      } // loop over periods

    //  // Clean up
    //  delete[] timeChannels;
    }

    /// Validates the optional 'spectra to read' properties, if they have been set
    void LoadMuonNexus2::checkOptionalProperties()
    {
      //read in the settings passed to the algorithm
      m_spec_list = getProperty("SpectrumList");
      m_spec_max = getProperty("SpectrumMax");
      //Are we using a list of spectra or all the spectra in a range?
      m_list = !m_spec_list.empty();
      m_interval = (m_spec_max != EMPTY_INT());
      if ( m_spec_max == EMPTY_INT() ) m_spec_max = 0;

      //// If a multiperiod dataset, ignore the optional parameters (if set) and print a warning
      //if ( m_numberOfPeriods > 1)
      //{
      //  if ( m_list || m_interval )
      //  {
      //    m_list = false;
      //    m_interval = false;
      //    g_log.warning("Ignoring spectrum properties in this multiperiod dataset");
      //  }
      //}

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

      // Check validity of spectra list property, if set
      if ( m_list )
      {
        const int minlist = *min_element(m_spec_list.begin(),m_spec_list.end());
        const int maxlist = *max_element(m_spec_list.begin(),m_spec_list.end());
        if ( maxlist > m_numberOfSpectra || minlist == 0)
        {
          g_log.error("Invalid list of spectra");
          throw std::invalid_argument("Inconsistent properties defined"); 
        } 
      }

    }

    /** loadData
     *  Load the counts data from an NXInt into a workspace
     */
    void LoadMuonNexus2::loadData(const NXInt& counts,const float* timeBins,int wsIndex,
      int period,int spec,API::MatrixWorkspace_sptr localWorkspace)
    {
      MantidVec& X = localWorkspace->dataX(wsIndex);
      MantidVec& Y = localWorkspace->dataY(wsIndex);
      MantidVec& E = localWorkspace->dataE(wsIndex);
      int nBins = counts.dim2()-1; // It shouldn't be -1 here but timeBins has dim == counts.dim2() which is also wrong
      X.assign(timeBins,timeBins+nBins+1);
      int *data = &counts(period,spec,0);
      Y.assign(data,data+nBins);
      //for(int i=0;i<nBins;++i)
      //{
      //  Y[i] = counts(period,spec,i);
      //}
      std::transform(Y.begin(), Y.end(), E.begin(), dblSqrt);
    }

    /// Run the sub-algorithm LoadInstrument (or LoadInstrumentFromNexus)
    void LoadMuonNexus2::runLoadInstrument(API::MatrixWorkspace_sptr localWorkspace)
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

    /**  Load logs from Nexus file. Logs are expected to be in
    *   /run/sample group of the file. 
    *   @param ws The workspace to load the logs to.
    *   @param entry The Nexus entry
    *   @param period The period of this workspace
    */
    void LoadMuonNexus2::loadLogs(API::MatrixWorkspace_sptr ws, NXEntry & entry,int period)
    {

      std::string start_time = entry.getString("start_time");

      std::string sampleName = entry.getString("sample/name");
      NXMainClass runlogs = entry.openNXClass<NXMainClass>("sample");
      ws->mutableSample().setName(sampleName);

      for(std::vector<NXClassInfo>::const_iterator it=runlogs.groups().begin();it!=runlogs.groups().end();it++)
      {
        NXLog nxLog = runlogs.openNXLog(it->nxname);
        Kernel::Property* logv = nxLog.createTimeSeries(start_time);
        if (!logv) continue;
        ws->mutableSample().addLogData(logv);
        //if (it->nxname == "icp_event")
        //{
        //  LogParser parser(logv);
        //  ws->mutableSample().addLogData(parser.createPeriodLog(period));
        //  ws->mutableSample().addLogData(parser.createAllPeriodsLog());
        //  ws->mutableSample().addLogData(parser.createRunningLog());
        //}
      }

      ws->populateInstrumentParameters();
    }

    double LoadMuonNexus2::dblSqrt(double in)
    {
      return sqrt(in);
    }

  } // namespace DataHandling
} // namespace Mantid
