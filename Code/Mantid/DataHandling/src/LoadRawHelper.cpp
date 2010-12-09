//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidDataHandling/LoadRawHelper.h"
#include "MantidDataHandling/ManagedRawFileWorkspace2D.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidAPI/MemoryManager.h"
#include "MantidAPI/SpectraDetectorMap.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidGeometry/Instrument/XMLlogfile.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidAPI/FileProperty.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "LoadRaw/isisraw2.h"
#include "MantidDataHandling/LoadLog.h"
#include "MantidDataHandling/LoadInstrumentHelper.h"
#include "boost/date_time/gregorian/gregorian.hpp"

#include <boost/shared_ptr.hpp>
#include "Poco/Path.h"
#include "Poco/DateTimeParser.h"
#include <cmath>
#include <cstdio> //Required for gcc 4.4

namespace Mantid
{
  namespace DataHandling
  {

    using namespace Kernel;
    using namespace API;

    /// Constructor
    LoadRawHelper::LoadRawHelper() :
    Algorithm(),isisRaw(new ISISRAW2),
      m_list(false),m_spec_list(),m_spec_min(0),
      m_spec_max(EMPTY_INT()),m_specTimeRegimes(),m_bmspeclist(false)
    {
    }

    LoadRawHelper::~LoadRawHelper()
    {
    }

    /// Initialisation method.
    void LoadRawHelper::init()
    {
      std::vector<std::string> exts;
      exts.push_back(".raw");
      exts.push_back(".s*");
      exts.push_back(".add");
      declareProperty(new FileProperty("Filename", "", FileProperty::Load, exts),
        "The name of the RAW file to read, including its full or relative\n"
        "path. (N.B. case sensitive if running on Linux).");

      declareProperty(new WorkspaceProperty<Workspace> ("OutputWorkspace", "", Direction::Output),
        "The name of the workspace that will be created, filled with the\n"
        "read-in data and stored in the Analysis Data Service.  If the input\n"
        "RAW file contains multiple periods higher periods will be stored in\n"
        "separate workspaces called OutputWorkspace_PeriodNo.");


      m_cache_options.push_back("If Slow");
      m_cache_options.push_back("Always");
      m_cache_options.push_back("Never");
      declareProperty("Cache", "If Slow", new ListValidator(m_cache_options));

      declareProperty("LoadLogFiles", true, " Boolean option to load or skip log files.");

    }
    /**opens the raw file and returns the file pointer
    *@param fileName - name of the raw file
    *@return file pointer 
    */
    FILE*  LoadRawHelper::openRawFile(const std::string & fileName)
    {
      FILE* file = fopen(fileName.c_str(), "rb");
      if (file == NULL)
      {
        g_log.error("Unable to open file " + fileName);
        throw Exception::FileError("Unable to open File:", fileName);
      }
      // Need to check that the file is not a text file as the ISISRAW routines don't deal with these very well, i.e 
      // reading continues until a bad_alloc is encountered.
      if( isAscii(file) )
      {
        g_log.error() << "File \"" << fileName << "\" is not a valid RAW file.\n";
        fclose(file);
        throw std::invalid_argument("Incorrect file type encountered.");
      }

      return file;

    }
    /** Reads the run title and creates a string from it
    * @param file - pointer to the raw file
    * @param title  An output parameter that will contain the workspace title 
    */
    void LoadRawHelper::readTitle(FILE* file,std::string & title)
    {
      ioRaw(file, true);
      title = std::string(isisRaw->r_title, 80);
      g_log.information("*** Run title: " + title + " ***");
    }
    /**skips the histogram from raw file
    *@param file - pointer to the raw file
    *@param hist - postion in the file to skip
    */
    void LoadRawHelper::skipData(FILE* file,int hist)
    {
      isisRaw->skipData(file, hist);
    }
    /// calls isisRaw ioRaw.
    /// @param file the file pointer
    /// @param from_file unknown
    void LoadRawHelper::ioRaw(FILE* file,bool from_file )
    {
      isisRaw->ioRAW(file, from_file);
    }
    int LoadRawHelper::getNumberofTimeRegimes()
    {
      return isisRaw->daep.n_tr_shift;
    }

    void LoadRawHelper::reset()
    {
      isisRaw.reset();
    }

    /**reads the histogram from raw file
     * @param file - pointer to the raw file
     * @param hist - postion in the file to read
     * @return flag is data is read
     */
    bool LoadRawHelper::readData(FILE* file,int hist)
    {
      return isisRaw->readData(file, hist);
    }

    float LoadRawHelper::getProtonCharge()const
    {
      return isisRaw->rpb.r_gd_prtn_chrg;
    }

    /**
    * Set the proton charge on the run object
    * @param run The run object
    */
    void  LoadRawHelper::setProtonCharge(API::Run& run)
    {
      run.setProtonCharge(getProtonCharge());
    }
    /** Stores the run number in the run logs
    *  @param run the workspace's run object
    */
    void LoadRawHelper::setRunNumber(API::Run& run)
    {
      std::string run_num = boost::lexical_cast<std::string>(isisRaw->r_number);
      run.addLogData( new PropertyWithValue<std::string>("run_number", run_num) );
    }
    /**reads workspace dimensions,number of periods etc from raw data
     * @param numberOfSpectra number of spectrums
     * @param numberOfPeriods number of periods
     * @param lengthIn size of workspace vectors
     * @param noTimeRegimes number of time regime.
     */
    void LoadRawHelper::readworkspaceParameters(int& numberOfSpectra,int& numberOfPeriods,int& lengthIn,int & noTimeRegimes )
    {
      // Read in the number of spectra in the RAW file
      m_numberOfSpectra=numberOfSpectra = isisRaw->t_nsp1;
      // Read the number of periods in this file
      numberOfPeriods = isisRaw->t_nper;
      // Read the number of time channels (i.e. bins) from the RAW file
      const int channelsPerSpectrum = isisRaw->t_ntc1;
      // Read in the time bin boundaries
      lengthIn = channelsPerSpectrum + 1;
      // Now check whether there is more than one time regime in use
      noTimeRegimes = isisRaw->daep.n_tr_shift;
    }
    /**This method creates shared pointer to a workspace 
     * @param ws_sptr shared pointer to the parent workspace
     * @param nVectors number of histograms in the workspace
     * @param xLengthIn size of workspace X vector
     * @param yLengthIn size of workspace Y vector
     * @return an empty workspace of the given parameters
     */
    DataObjects::Workspace2D_sptr LoadRawHelper::createWorkspace(DataObjects::Workspace2D_sptr ws_sptr,
      int nVectors,int xLengthIn,int yLengthIn)
    {
      DataObjects::Workspace2D_sptr empty;
      if(!ws_sptr)return empty;
      DataObjects::Workspace2D_sptr workspace = boost::dynamic_pointer_cast<DataObjects::Workspace2D>
        (WorkspaceFactory::Instance().create(ws_sptr,nVectors,xLengthIn,yLengthIn));
      return workspace;
    }

    /** This method creates pointer to workspace
    *  @param nVectors The number of vectors/histograms in the workspace
    *  @param xlengthIn The number of X data points/bin boundaries in each vector 
    *  @param ylengthIn The number of Y data points/bin boundaries in each vector 
    *  @param title title of the workspace
    *  @return Workspace2D_sptr shared pointer to the workspace
    */
    DataObjects::Workspace2D_sptr LoadRawHelper::createWorkspace(int nVectors, int xlengthIn,int ylengthIn,const std::string& title)
    {
      DataObjects::Workspace2D_sptr workspace;
      if(nVectors>0)
      {
        workspace =boost::dynamic_pointer_cast<DataObjects::Workspace2D>(WorkspaceFactory::Instance().create(
          "Workspace2D", nVectors, xlengthIn, ylengthIn));
        // Set the units
        workspace->getAxis(0)->unit() = UnitFactory::Instance().create("TOF");
        workspace->setYUnit("Counts");
        workspace->setTitle(title);

      }
      return workspace;
    }

    /**creates monitor workspace 
    *@param monws_sptr shared pointer to monitor workspace
    *@param normalws_sptr shared pointer to output workspace
    *@param mongrp_sptr shared pointer to monitor group workspace
    *@param mwsSpecs number of spectra in the monitor workspace
    *@param nwsSpecs number of spectra in the output workspace
    *@param numberOfPeriods total number of periods from raw file
    *@param lengthIn size of workspace vectors
    *@param title title of the workspace

    */
    void LoadRawHelper::createMonitorWorkspace(DataObjects::Workspace2D_sptr& monws_sptr,DataObjects::Workspace2D_sptr& normalws_sptr,
      WorkspaceGroup_sptr& mongrp_sptr,const int mwsSpecs,const int nwsSpecs,
      const int numberOfPeriods,const int lengthIn,const std::string title)
    {
      try
      { 
        //create monitor group workspace 
        mongrp_sptr = createGroupWorkspace(); //create workspace
        // create monitor workspace
        if(mwsSpecs>0)
        {
          if(normalws_sptr)
          {                               
            monws_sptr=createWorkspace(normalws_sptr,mwsSpecs,lengthIn,lengthIn-1);

          }
          else
          {
            monws_sptr=createWorkspace(mwsSpecs,lengthIn,lengthIn-1,title);
          }
        }
        if(!monws_sptr) return ;

        std::string wsName= getPropertyValue("OutputWorkspace");
        // if the normal output workspace size>0 then set the workspace as "MonitorWorkspace"
        // otherwise  set the workspace as "OutputWorkspace"
        if (nwsSpecs> 0)
        {               
          std::string monitorwsName = wsName + "_Monitors";
          declareProperty(new WorkspaceProperty<Workspace> ("MonitorWorkspace", monitorwsName,
            Direction::Output));
          setWorkspaceProperty("MonitorWorkspace", title, mongrp_sptr, monws_sptr,numberOfPeriods, true);
        }
        else
        { 
          //if only monitors range selected
          //then set the monitor workspace as the outputworkspace
          setWorkspaceProperty("OutputWorkspace", title, mongrp_sptr, monws_sptr,numberOfPeriods, false);
          //normalws_sptr = monws_sptr;
        }

      }
      catch(std::out_of_range& )
      {
        g_log.debug()<<"Error in creating monitor workspace"<<std::endl;
      }
      catch(std::runtime_error& )
      {
        g_log.debug()<<"Error in creating monitor workspace"<<std::endl;
      }
    }

    /** Executes the algorithm. Reading in the file and creating and populating
    *  the output workspace
    *
    *  @throw Exception::FileError If the RAW file cannot be found/opened
    *  @throw std::invalid_argument If the optional properties are set to invalid values
    */
    void LoadRawHelper::exec()
    {
    }


    /** Creates a TimeSeriesProperty<bool> showing times when a particular period was active.
     *  @param period The data period
     *  @return the times when requested period was active
     */
    Kernel::Property*  LoadRawHelper::createPeriodLog(int period)const
    {
      Kernel::TimeSeriesProperty<int>* periods = dynamic_cast< Kernel::TimeSeriesProperty<int>* >(m_perioids.get());
      if(!periods) return 0;
      std::ostringstream ostr;
      ostr<<period;
      Kernel::TimeSeriesProperty<bool>* p = new Kernel::TimeSeriesProperty<bool> ("period "+ostr.str());
      std::map<Kernel::DateAndTime, int> pMap = periods->valueAsMap();
      std::map<Kernel::DateAndTime, int>::const_iterator it = pMap.begin();
      if (it->second != period)
        p->addValue(it->first,false);
      for(;it!=pMap.end();it++)
        p->addValue(it->first, (it->second == period) );

      return p;
    }

    /** sets the workspace properties
    *  @param ws_sptr  shared pointer to  workspace
    *  @param grpws_sptr shared pointer to  group workspace
    *  @param  period period number
    *  @param bmonitors boolean flag to name  the workspaces
    */
    void LoadRawHelper::setWorkspaceProperty(DataObjects::Workspace2D_sptr ws_sptr, WorkspaceGroup_sptr grpws_sptr,
      const int period, bool bmonitors)
    {
      if(!ws_sptr) return;
      if(!grpws_sptr) return;
      std::string wsName;
      std::string outws;
      std::string outputWorkspace;
      std::string localWSName = getProperty("OutputWorkspace");
      std::stringstream suffix;
      suffix << (period + 1);
      if (bmonitors)
      {
        wsName = localWSName + "_Monitors" + "_" + suffix.str();
        outputWorkspace = "MonitorWorkspace";
      }
      else
      {
        wsName = localWSName + "_" + suffix.str();
        outputWorkspace = "OutputWorkspace";
      }
      outws = outputWorkspace + "_" + suffix.str();
      declareProperty(new WorkspaceProperty<DataObjects::Workspace2D> (outws, wsName, Direction::Output));
      grpws_sptr->add(wsName);
      setProperty(outws, boost::dynamic_pointer_cast<DataObjects::Workspace2D>(ws_sptr));
    }

    /** This method sets the workspace property
    *  @param propertyName property name for the workspace
    *  @param title title of the workspace
    *  @param grpws_sptr  shared pointer to group workspace
    *  @param ws_sptr  shared pointer to workspace
    *  @param numberOfPeriods numer periods in the raw file
    *  @param  bMonitor to identify the workspace is an output workspace or monitor workspace
    */
    void LoadRawHelper::setWorkspaceProperty(const std::string& propertyName, const std::string& title,
      WorkspaceGroup_sptr grpws_sptr, DataObjects::Workspace2D_sptr ws_sptr,int numberOfPeriods, bool bMonitor)
    {
      Property *ws = getProperty("OutputWorkspace");
	  if(!ws) return;
	  if(!grpws_sptr) return;
	  if(!ws_sptr)return;
      std::string wsName = ws->value();
      if (bMonitor)
	  {
        wsName += "_Monitors";
	  }
      
      ws_sptr->setTitle(title);
      ws_sptr->getAxis(0)->unit() = UnitFactory::Instance().create("TOF");
      if (numberOfPeriods > 1)
      {
        //grpws_sptr->add(wsName);
        setProperty(propertyName, boost::dynamic_pointer_cast<Workspace>(grpws_sptr));
      }
      else
      {
        setProperty(propertyName, boost::dynamic_pointer_cast<Workspace>(ws_sptr));
      }
    }

    /** This method sets the raw file data to workspace vectors
    *  @param newWorkspace  shared pointer to the  workspace
    *  @param timeChannelsVec  vector holding the X data
    *  @param  wsIndex  variable used for indexing the ouputworkspace
    *  @param  nspecNum  spectrum number
    *  @param noTimeRegimes   regime no.
    *  @param lengthIn length of the workspace
    *  @param binStart start of bin
    */
    void LoadRawHelper::setWorkspaceData(DataObjects::Workspace2D_sptr newWorkspace, const std::vector<
      boost::shared_ptr<MantidVec> >& timeChannelsVec, int wsIndex, int nspecNum, int noTimeRegimes,int lengthIn,int binStart)
    {
      if(!newWorkspace)return;
      typedef double (*uf)(double);
      uf dblSqrt = std::sqrt;
      // But note that the last (overflow) bin is kept
      MantidVec& Y = newWorkspace->dataY(wsIndex);
      Y.assign(isisRaw->dat1 + binStart, isisRaw->dat1 + lengthIn);
      // Fill the vector for the errors, containing sqrt(count)
      MantidVec& E = newWorkspace->dataE(wsIndex);
      std::transform(Y.begin(), Y.end(), E.begin(), dblSqrt); 

      newWorkspace->getAxis(1)->spectraNo(wsIndex) = nspecNum;
      //for loadrawbin0
      if(binStart==0)
      {
        newWorkspace->setX(wsIndex, timeChannelsVec[0]);
        return ;
      }
      //for loadrawspectrum 0
      if(nspecNum==0)
      {
        newWorkspace->setX(wsIndex, timeChannelsVec[0]);
        return ;
      }
      // Set the X vector pointer and spectrum number
      if (noTimeRegimes < 2)
        newWorkspace->setX(wsIndex, timeChannelsVec[0]);
      else
      {

        // Use std::vector::at just incase spectrum missing from spec array
        newWorkspace->setX(wsIndex, timeChannelsVec.at(m_specTimeRegimes[nspecNum] - 1));
      }

    }

    /** This method returns the monitor spectrum list 
    *  @param localWorkspace  shared pointer to  workspace 
    *  @param monitorSpecList a list holding the spectrum indexes of the monitors
    */
    void LoadRawHelper::getmonitorSpectrumList(DataObjects::Workspace2D_sptr localWorkspace,
      std::vector<int>& monitorSpecList)
    {
      if (!m_monitordetectorList.empty())
      {
        const SpectraDetectorMap& specdetMap = localWorkspace->spectraMap();
        //get the monitor spectrum list from SpectraDetectorMap
        std::vector<int> specList = specdetMap.getSpectra(m_monitordetectorList);
        // remove duplicates by calling  sort & unique algorithms
        sort(specList.begin(), specList.end(), std::less<int>());
        std::vector<int>::iterator uEnd;
        uEnd = unique(specList.begin(), specList.end());
        std::vector<int> newVec;
        newVec.assign(specList.begin(), uEnd);
        //remove if zeroes are  there in the Spectra list
        std::vector<int>::iterator itr;
        itr = find(newVec.begin(), newVec.end(), 0);
        if (itr != newVec.end())
          newVec.erase(itr);
        monitorSpecList.assign(newVec.begin(), newVec.end());
      }
      else{
        g_log.error() << "monitor detector id list is empty  for the selected workspace" << std::endl;
      }
    }


    /** This method creates pointer to group workspace
    *  @return WorkspaceGroup_sptr shared pointer to the workspace
    */
    WorkspaceGroup_sptr LoadRawHelper::createGroupWorkspace()
    {
      WorkspaceGroup_sptr workspacegrp(new WorkspaceGroup);
      return workspacegrp;
    }

    /**
    * Check if a file is a text file
    * @param file The file pointer
    * @returns true if the file an ascii text file, false otherwise
    */
    bool LoadRawHelper::isAscii(FILE* file) const
    {  
      char data[256];
      int n = fread(data, 1, sizeof(data), file);
      char *pend = &data[n];
      fseek(file,0,SEEK_SET);
      /*
      * Call it a binary file if we find a non-ascii character in the 
      * first 256 bytes of the file.
      */
      for( char *p = data;  p < pend; ++p )
      {
        unsigned long ch = (unsigned long)*p;
        if( !(ch <= 0x7F) )
        {
          return false;
        }

      }
      return true;
    }



    /** Constructs the time channel (X) vector(s)
    *  @param regimes  The number of time regimes (if 1 regime, will actually contain 0)
    *  @param lengthIn The number of time channels
    *  @return The vector(s) containing the time channel boundaries, in a vector of shared ptrs
    */
    std::vector<boost::shared_ptr<MantidVec> > LoadRawHelper::getTimeChannels(const int& regimes,
      const int& lengthIn)
    {
      float* const timeChannels = new float[lengthIn];
      isisRaw->getTimeChannels(timeChannels, lengthIn);

      std::vector<boost::shared_ptr<MantidVec> > timeChannelsVec;
      if (regimes >= 2)
      {
        g_log.debug() << "Raw file contains " << regimes << " time regimes\n";
        // If more than 1 regime, create a timeChannelsVec for each regime
        for (int i = 0; i < regimes; ++i)
        {
          // Create a vector with the 'base' time channels
          boost::shared_ptr<MantidVec> channelsVec(new MantidVec(timeChannels, timeChannels + lengthIn));
          const double shift = isisRaw->daep.tr_shift[i];
          g_log.debug() << "Time regime " << i + 1 << " shifted by " << shift << " microseconds\n";
          // Add on the shift for this vector
          std::transform(channelsVec->begin(), channelsVec->end(), channelsVec->begin(), std::bind2nd(
            std::plus<double>(), shift));
          timeChannelsVec.push_back(channelsVec);
        }
        // In this case, also need to populate the map of spectrum-regime correspondence
        const int ndet = isisRaw->i_det;
        std::map<int, int>::iterator hint = m_specTimeRegimes.begin();
        for (int j = 0; j < ndet; ++j)
        {
          // No checking for consistency here - that all detectors for given spectrum
          // are declared to use same time regime. Will just use first encountered
          hint = m_specTimeRegimes.insert(hint, std::make_pair(isisRaw->spec[j], isisRaw->timr[j]));
        }
      }
      else // Just need one in this case
      {
        boost::shared_ptr<MantidVec> channelsVec(new MantidVec(timeChannels, timeChannels + lengthIn));
        timeChannelsVec.push_back(channelsVec);
      }
      // Done with the timeChannels C array so clean up
      delete[] timeChannels;
      return timeChannelsVec;
    }

    /// Run the sub-algorithm LoadInstrument (or LoadInstrumentFromRaw)
    /// @param fileName the raw file filename
    /// @param localWorkspace The workspace to load the instrument for
    void LoadRawHelper::runLoadInstrument(const std::string& fileName,DataObjects::Workspace2D_sptr localWorkspace)
    {
      g_log.debug("Loading the instrument definition...");
      progress(m_prog, "Loading the instrument geometry...");

      std::string instrumentID = isisRaw->i_inst; // get the instrument name
      size_t i = instrumentID.find_first_of(' '); // cut trailing spaces
      if (i != std::string::npos)
        instrumentID.erase(i);

      IAlgorithm_sptr loadInst= createSubAlgorithm("LoadInstrument");

      // Now execute the sub-algorithm. Catch and log any error, but don't stop.
      bool executionSuccessful(true);
      try
      {
        loadInst->setPropertyValue("InstrumentName", instrumentID);
        loadInst->setProperty<MatrixWorkspace_sptr> ("Workspace", localWorkspace);
        loadInst->execute();
      } catch (std::invalid_argument&)
      {
        g_log.information("Invalid argument to LoadInstrument sub-algorithm");
        executionSuccessful = false;
      } catch (std::runtime_error&)
      {
        g_log.information("Unable to successfully run LoadInstrument sub-algorithm");
        executionSuccessful = false;
      }

      // If loading instrument definition file fails, run LoadInstrumentFromRaw instead
      if (!executionSuccessful)
      {
        g_log.information() << "Instrument definition file " 
          << " not found. Attempt to load information about \n"
          << "the instrument from raw data file.\n";
        runLoadInstrumentFromRaw(fileName,localWorkspace);
      }
      else
      {
        m_monitordetectorList = loadInst->getProperty("MonitorList");
        std::vector<int>::const_iterator itr;
        for (itr = m_monitordetectorList.begin(); itr != m_monitordetectorList.end(); ++itr)
        {
          g_log.debug() << "Monitor detector id is " << (*itr) << std::endl;
        }
      }
    }

    /// Run LoadInstrumentFromRaw as a sub-algorithm (only if loading from instrument definition file fails)
    /// @param fileName the raw file filename
    /// @param localWorkspace The workspace to load the instrument for
    void LoadRawHelper::runLoadInstrumentFromRaw(const std::string& fileName,DataObjects::Workspace2D_sptr localWorkspace)
    {
      IAlgorithm_sptr loadInst = createSubAlgorithm("LoadInstrumentFromRaw");
      loadInst->setPropertyValue("Filename", fileName);
      // Set the workspace property to be the same one filled above
      loadInst->setProperty<MatrixWorkspace_sptr> ("Workspace", localWorkspace);

      // Now execute the sub-algorithm. Catch and log any error, but don't stop.
      try
      {
        loadInst->execute();
      } catch (std::runtime_error&)
      {
        g_log.error("Unable to successfully run LoadInstrumentFromRaw sub-algorithm");
      }
      m_monitordetectorList = loadInst->getProperty("MonitorList");
      std::vector<int>::const_iterator itr;
      for (itr = m_monitordetectorList.begin(); itr != m_monitordetectorList.end(); ++itr)
      {
        g_log.debug() << "Monitor dtector id is " << (*itr) << std::endl;

      }
      if (!loadInst->isExecuted())
      {
        g_log.error("No instrument definition loaded");
      }
    }

    /// Run the LoadMappingTable sub-algorithm to fill the SpectraToDetectorMap
    /// @param fileName the raw file filename
    /// @param localWorkspace The workspace to load the mapping table for
    void LoadRawHelper::runLoadMappingTable(const std::string& fileName,DataObjects::Workspace2D_sptr localWorkspace)
    {
      g_log.debug("Loading the spectra-detector mapping...");
      progress(m_prog, "Loading the spectra-detector mapping...");
      // Now determine the spectra to detector map calling sub-algorithm LoadMappingTable
      // There is a small penalty in re-opening the raw file but nothing major.
      IAlgorithm_sptr loadmap = createSubAlgorithm("LoadMappingTable");
      loadmap->setPropertyValue("Filename",fileName);
      loadmap->setProperty<MatrixWorkspace_sptr> ("Workspace", localWorkspace);
      try
      {
        loadmap->execute();
      } catch (std::runtime_error&)
      {
        g_log.error("Unable to successfully execute LoadMappingTable sub-algorithm");
      }

      if (!loadmap->isExecuted())
      {
        g_log.error("LoadMappingTable sub-algorithm is not executed");
      }

    }

    /// Run the LoadLog sub-algorithm
    /// @param fileName the raw file filename
    /// @param localWorkspace The workspace to load the logs for
    /// @param period The period number that the workspace holds
    void LoadRawHelper::runLoadLog(const std::string& fileName,DataObjects::Workspace2D_sptr localWorkspace, int period)
    {
      g_log.debug("Loading the log files...");
      progress(m_prog, "Reading log files...");
      IAlgorithm_sptr loadLog = createSubAlgorithm("LoadLog");
      // Pass through the same input filename
      loadLog->setPropertyValue("Filename", fileName);
      // Set the workspace property to be the same one filled above
      loadLog->setProperty<MatrixWorkspace_sptr> ("Workspace", localWorkspace);

      // Now execute the sub-algorithm. Catch and log any error, but don't stop.
      try
      {
        loadLog->execute();
      } catch (std::exception&)
      {
        g_log.error("Unable to successfully run LoadLog sub-algorithm");
      }

      if (!loadLog->isExecuted())
      {
        g_log.error("Unable to successfully run LoadLog sub-algorithm");
      }
      LoadLog* plog=dynamic_cast<LoadLog*>(loadLog.get());
      if(plog) m_perioids=plog->getPeriodsProperty();
    }

    /**
    * Pulls the run parameters from the ISIS Raw RPB structure and stores them as log entries on the 
    * workspace run object
    * @param localWorkspace The workspace to attach the information to
    * @param rawFile The handle to an ISIS Raw file
    */
    void LoadRawHelper::loadRunParameters(API::MatrixWorkspace_sptr localWorkspace, ISISRAW * const rawFile) const
    {
      ISISRAW * localISISRaw(NULL);
      if( !rawFile )
      {
        localISISRaw = isisRaw.get();
      }
      else
      {
        localISISRaw = rawFile;
      }      

      API::Run& runDetails = localWorkspace->mutableRun();
      // Run header is stored as consecutive char arrays adding up to a total of 80 bytes in the HDR_STRUCT
      const std::string run_header(localISISRaw->hdr.inst_abrv, 80);
      runDetails.addProperty("run_header", run_header);
      // Run title is stored in a different attribute
      runDetails.addProperty("run_title", std::string(localISISRaw->r_title,80));

      // Data details on run not the workspace
      runDetails.addProperty("nspectra", static_cast<int>(localISISRaw->t_nsp1));
      runDetails.addProperty("nchannels", static_cast<int>(localISISRaw->t_ntc1));
      runDetails.addProperty("nperiods", static_cast<int>(localISISRaw->t_nper));

      // RPB struct info
      runDetails.addProperty("dur", localISISRaw->rpb.r_dur);	// actual run duration
      runDetails.addProperty("durunits", localISISRaw->rpb.r_durunits);	// scaler for above (1=seconds)
      runDetails.addProperty("dur_freq",localISISRaw->rpb.r_dur_freq);  // testinterval for above (seconds)
      runDetails.addProperty("dmp", localISISRaw->rpb.r_dmp);       // dump interval
      runDetails.addProperty("dmp_units", localISISRaw->rpb.r_dmp_units);	// scaler for above
      runDetails.addProperty("dmp_freq",localISISRaw->rpb.r_dmp_freq);	// interval for above
      runDetails.addProperty("freq",localISISRaw->rpb.r_freq);	// 2**k where source frequency = 50 / 2**k
      runDetails.addProperty("gd_prtn_chrg", static_cast<double>(localISISRaw->rpb.r_gd_prtn_chrg));  // good proton charge (uA.hour)
      runDetails.addProperty("tot_prtn_chrg", static_cast<double>(localISISRaw->rpb.r_tot_prtn_chrg)); // total proton charge (uA.hour)
      runDetails.addProperty("goodfrm",localISISRaw->rpb.r_goodfrm);	// good frames
      runDetails.addProperty("rawfrm", localISISRaw->rpb.r_rawfrm);	// raw frames
      runDetails.addProperty("dur_wanted",localISISRaw->rpb.r_dur_wanted); // requested run duration (units as for "duration" above)
      runDetails.addProperty("dur_secs",localISISRaw->rpb.r_dur_secs );	// actual run duration in seconds
      runDetails.addProperty("mon_sum1", localISISRaw->rpb.r_mon_sum1);	// monitor sum 1
      runDetails.addProperty("mon_sum2",localISISRaw->rpb.r_mon_sum2);	// monitor sum 2
      runDetails.addProperty("mon_sum3",localISISRaw->rpb.r_mon_sum3);	// monitor sum 3
      runDetails.addProperty("rb_proposal",localISISRaw->rpb.r_prop); // RB (proposal) number

      // Note isis raw date format which is stored in DD-MMM-YYYY. Store dates in ISO 8601
      std::string isisDate = std::string(localISISRaw->rpb.r_enddate, 11);
      if ( isisDate[0] == ' ' ) isisDate[0] = '0';
      runDetails.addProperty("run_end", DateAndTime(isisDate.substr(7,4) + "-" + convertMonthLabelToIntStr(isisDate.substr(3,3)) 
        + "-" + isisDate.substr(0,2) + "T" + std::string(localISISRaw->rpb.r_endtime, 8)).to_ISO8601_string());
      isisDate = std::string(localISISRaw->hdr.hd_date, 11);
      if ( isisDate[0] == ' ' ) isisDate[0] = '0';
      runDetails.addProperty("run_start", DateAndTime(isisDate.substr(7,4) + "-" + convertMonthLabelToIntStr(isisDate.substr(3,3)) 
        + "-" + isisDate.substr(0,2) + "T" + std::string(localISISRaw->hdr.hd_time, 8)).to_ISO8601_string());
    }

    /// To help transforming date stored in ISIS raw file into iso 8601
    /// @param month
    /// @return month as string integer e.g. 01
    std::string LoadRawHelper::convertMonthLabelToIntStr(std::string month) const
    {
      std::transform(month.begin(), month.end(), month.begin(), toupper);

      if ( month == "JAN" )
        return "01";
      if ( month == "FEB" )
        return "02";
      if ( month == "MAR" )
        return "03";
      if ( month == "APR" )
        return "04";
      if ( month == "MAY" )
        return "05";
      if ( month == "JUN" )
        return "06";
      if ( month == "JUL" )
        return "07";
      if ( month == "AUG" )
        return "08";
      if ( month == "SEP" )
        return "09";
      if ( month == "OCT" )
        return "10";
      if ( month == "NOV" )
        return "11";
      if ( month == "DEC" )
        return "12";
    }

    ///sets optional properties for the loadraw algorithm
    /// @param spec_min The minimum spectra number
    /// @param spec_max The maximum spectra number
    /// @param spec_list The list of Spectra to be included
    void LoadRawHelper::setOptionalProperties(const int& spec_min,const int& spec_max,const std::vector<int>& spec_list)
    {
      m_spec_min=spec_min;
      m_spec_max=spec_max;
      m_spec_list.assign(spec_list.begin(),spec_list.end());
    }

    /// Validates the optional 'spectra to read' properties, if they have been set
    void LoadRawHelper::checkOptionalProperties()
    {
      //read in the settings passed to the algorithm
      /*m_spec_list = getProperty("SpectrumList");
      m_spec_max = getProperty("SpectrumMax");
      m_spec_min = getProperty("SpectrumMin");*/

      m_list = !m_spec_list.empty();
      m_bmspeclist = !m_spec_list.empty();
      m_interval = (m_spec_max != EMPTY_INT()) || (m_spec_min != 1);
      if (m_spec_max == EMPTY_INT())
        m_spec_max = 1;
      // Check validity of spectra list property, if set
      if (m_list)
      {
        m_list = true;
        if (m_spec_list.size() == 0)
        {
          m_list = false;
        }
        else
        {
          const int minlist = *min_element(m_spec_list.begin(), m_spec_list.end());
          const int maxlist = *max_element(m_spec_list.begin(), m_spec_list.end());
          if (maxlist >m_numberOfSpectra || minlist <= 0)
          {
            g_log.error("Invalid list of spectra");
            throw std::invalid_argument("Inconsistent properties defined");
          }
        }
      }
      // Check validity of spectra range, if set
      if (m_interval)
      {
        m_interval = true;
        //m_spec_min = getProperty("SpectrumMin");
        if (m_spec_min != 1 && m_spec_max == 1)
          m_spec_max = m_numberOfSpectra;
        if (m_spec_max < m_spec_min || m_spec_max >m_numberOfSpectra)
        {
          g_log.error("Invalid Spectrum min/max properties");
          throw std::invalid_argument("Inconsistent properties defined");
        }

      }
    }
    /**
     * Calculates the total number of spectra in the workspace, given the input properties
     * @return the size of the workspace (number of spectra)
     */
    int LoadRawHelper::calculateWorkspaceSize()
    {
      int total_specs(0);
      if (m_interval || m_list)
      {
        if (m_interval)
        {
          if (m_spec_min != 1 && m_spec_max == 1)
            m_spec_max = m_numberOfSpectra;

          m_total_specs=total_specs = (m_spec_max - m_spec_min + 1);
          m_spec_max += 1;
        }
        else
          total_specs = 0;

        if (m_list)
        {
          if (m_interval)
          {
            for (std::vector<int>::iterator it = m_spec_list.begin(); it != m_spec_list.end();)
              if (*it >= m_spec_min && *it < m_spec_max)
              {
                it = m_spec_list.erase(it);
              }
              else
                it++;
          }
          if (m_spec_list.size() == 0)
            m_list = false;
          total_specs += m_spec_list.size();
          m_total_specs=total_specs;

        }
      }
      else
      {
        total_specs = m_numberOfSpectra;
        m_total_specs=total_specs;
        // In this case want all the spectra, but zeroth spectrum is garbage so go from 1 to NSP1
        m_spec_min = 1;
        m_spec_max = m_numberOfSpectra + 1;
      }
      return total_specs;
    }

    /// calculate workspace sizes.
    /// @param monitorSpecList the vector of the monitor spectra
    /// @param normalwsSpecs the spectra for the detector workspace
    /// @param monitorwsSpecs the spectra for the monitor workspace
    void LoadRawHelper::calculateWorkspacesizes(const std::vector<int>& monitorSpecList, 
      int& normalwsSpecs, int & monitorwsSpecs)
    {
      if (!m_interval && !m_bmspeclist)
      {
        normalwsSpecs = m_total_specs - monitorSpecList.size();
        monitorwsSpecs = monitorSpecList.size();
        g_log.debug() << "normalwsSpecs   when m_interval  & m_bmspeclist are  false is  " << normalwsSpecs
          << "  monitorwsSpecs is " << monitorwsSpecs << std::endl;
      }
      else if (m_interval || m_bmspeclist)
      {
        int msize = 0;
        if (m_interval)
        {
          std::vector<int>::const_iterator itr1;
          for (itr1 = monitorSpecList.begin(); itr1 != monitorSpecList.end(); ++itr1)
          {
            if (*itr1 >= m_spec_min && *itr1 < m_spec_max)
              ++msize;
          }
          monitorwsSpecs = msize;
          normalwsSpecs = m_total_specs - monitorwsSpecs;
          g_log.debug() << "normalwsSpecs when  m_interval true is  " << normalwsSpecs
            << "  monitorwsSpecs is " << monitorwsSpecs << std::endl;
        }
        if (m_bmspeclist)
        {
          if (m_interval)
          {
            std::vector<int>::iterator itr;
            for (itr = m_spec_list.begin(); itr != m_spec_list.end();)
            { //if  the m_spec_list elements are in the range between m_spec_min & m_spec_max
              if (*itr >= m_spec_min && *itr < m_spec_max)
                itr = m_spec_list.erase(itr);
              else
                ++itr;
            }
            if (m_spec_list.size() == 0)
            {
              g_log.debug() << "normalwsSpecs is " << normalwsSpecs << "  monitorwsSpecs is "
                << monitorwsSpecs << std::endl;
            }
            else
            { //at this point there are monitors in the list which are not in the min& max range
              // so find those  monitors  count and calculate the workspace specs 
              std::vector<int>::const_iterator itr;
              std::vector<int>::const_iterator monitr;
              int monCounter = 0;
              for (itr = m_spec_list.begin(); itr != m_spec_list.end(); ++itr)
              {
                monitr = find(monitorSpecList.begin(), monitorSpecList.end(), *itr);
                if (monitr != monitorSpecList.end())
                  ++monCounter;
              }
              monitorwsSpecs += monCounter;
              normalwsSpecs = m_total_specs - monitorwsSpecs;
              g_log.debug() << "normalwsSpecs is  " << normalwsSpecs << "  monitorwsSpecs is "
                << monitorwsSpecs << std::endl;
            }
          }//end if loop for m_interval  
          else
          { //if only List true
            int mSize = 0;
            std::vector<int>::const_iterator itr;
            std::vector<int>::const_iterator monitr;
            for (itr = m_spec_list.begin(); itr != m_spec_list.end(); ++itr)
            {
              monitr = find(monitorSpecList.begin(), monitorSpecList.end(), *itr);
              if (monitr != monitorSpecList.end())
              {
                ++mSize;
              }
            }
            monitorwsSpecs = mSize;
            normalwsSpecs = m_total_specs - monitorwsSpecs;

          }
        }//end of if loop for m_bmspeclist
      }

    }

    void LoadRawHelper::loadSpectra(FILE* file,const int& period,const int& total_specs,
      DataObjects::Workspace2D_sptr ws_sptr,std::vector<boost::shared_ptr<MantidVec> > timeChannelsVec)
    {
      int histCurrent = -1;
      int wsIndex=0;
      int numberOfPeriods=isisRaw->t_nper;
      int histTotal = total_specs * numberOfPeriods;
      int noTimeRegimes=getNumberofTimeRegimes();
      int lengthIn = isisRaw->t_ntc1+1;

      //loop through spectra
      for (int i = 1; i <= m_numberOfSpectra; ++i)
      {
        int histToRead = i + period * (m_numberOfSpectra + 1);
        if ((i >= m_spec_min && i < m_spec_max) || 
           (m_list && find(m_spec_list.begin(), m_spec_list.end(),i) != m_spec_list.end()))
        {
          progress(m_prog, "Reading raw file data...");

          //read spectrum from raw file
          readData(file, histToRead);
          //set worksapce data
          setWorkspaceData(ws_sptr, timeChannelsVec, wsIndex, i,noTimeRegimes,lengthIn,1);
          ++wsIndex;

          if (numberOfPeriods == 1)
          {
            if (++histCurrent % 100 == 0)
            {
              m_prog = double(histCurrent) / histTotal;
            }
            interruption_point();
          }

        }
        else
        {
          skipData(file, histToRead);
        }
      }

    }

  } // namespace DataHandling
} // namespace Mantid
