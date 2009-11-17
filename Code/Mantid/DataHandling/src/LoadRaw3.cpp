//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidDataHandling/LoadRaw3.h"
#include "MantidDataHandling/ManagedRawFileWorkspace2D.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidAPI/XMLlogfile.h"
#include "MantidAPI/MemoryManager.h"
#include "MantidAPI/SpectraDetectorMap.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/FileProperty.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "LoadRaw/isisraw2.h"

#include <boost/shared_ptr.hpp>
#include "Poco/Path.h"
#include <cmath>
#include <cstdio> //Required for gcc 4.4
namespace Mantid
{
namespace DataHandling
{
// Register the algorithm into the algorithm factory
DECLARE_ALGORITHM(LoadRaw3)

using namespace Kernel;
using namespace API;

/// Constructor
LoadRaw3::LoadRaw3() :
  Algorithm(), isisRaw(new ISISRAW2), m_filename(), m_numberOfSpectra(0), m_numberOfPeriods(0), m_list(
      false), m_interval(false), m_spec_list(), m_spec_min(0), m_spec_max(unSetInt),
      m_specTimeRegimes(), m_prog(0.0), m_bmspeclist(false)
{
}

LoadRaw3::~LoadRaw3()
{
}

/// Initialisation method.
void LoadRaw3::init()
{
  // Extension checking is not case sensitive
  // MG 20/07/09: I've had to change these extensions so that the native Windows file dialog can recognise
  // the file types correctly
  std::vector<std::string> exts;
  exts.push_back("raw");
  exts.push_back("s*");

  declareProperty(new FileProperty("Filename", "", FileProperty::Load, exts),
      "The name of the RAW file to read, including its full or relative\n"
        "path. (N.B. case sensitive if running on Linux).");
  declareProperty(new WorkspaceProperty<Workspace> ("OutputWorkspace", "", Direction::Output),
      "The name of the workspace that will be created, filled with the\n"
        "read-in data and stored in the Analysis Data Service.  If the input\n"
        "RAW file contains multiple periods higher periods will be stored in\n"
        "separate workspaces called OutputWorkspace_PeriodNo.");

  BoundedValidator<int> *mustBePositive = new BoundedValidator<int> ();
  mustBePositive->setLower(1);
  declareProperty("SpectrumMin", 1, mustBePositive,
      "The index number of the first spectrum to read.  Only used if\n"
        "spectrum_max is set.");
  declareProperty("SpectrumMax", unSetInt, mustBePositive->clone(),
      "The number of the last spectrum to read. Only used if explicitly\n"
        "set.");

  declareProperty(new ArrayProperty<int> ("SpectrumList"),
      "A comma-separated list of individual spectra to read.  Only used if\n"
        "explicitly set.");
  m_cache_options.push_back("If Slow");
  m_cache_options.push_back("Always");
  m_cache_options.push_back("Never");
  declareProperty("Cache", "If Slow", new ListValidator(m_cache_options));
  declareProperty("LoadLogFiles", true, " Boolean option to load or skip log files.");

  m_monitorOptions.push_back("Include");
  m_monitorOptions.push_back("Exclude");
  m_monitorOptions.push_back("Separate");

  declareProperty(
      "LoadMonitors",
      "Include",
      new ListValidator(m_monitorOptions),
      "Use this option to control the loading of monitors.\n"
        "The defalut is Include option  which loads the monitors into the output workspace.\n"
        "Other options are Exclude and Separate.The Exclude option exludes monitors from the output workspace \n"
        "and the Separate option loads monitors into a separate workspace called OutputWorkspace_Monitor.\n");
}

/** Executes the algorithm. Reading in the file and creating and populating
 *  the output workspace
 *
 *  @throw Exception::FileError If the RAW file cannot be found/opened
 *  @throw std::invalid_argument If the optional properties are set to invalid values
 */
void LoadRaw3::exec()
{
  // Retrieve the filename from the properties
  m_filename = getPropertyValue("Filename");
  bool bLoadlogFiles = getProperty("LoadLogFiles");
  bool bincludeMonitors = true;
  bool bseparateMonitors = false;
  bool bexcludeMonitors = false;
  bincludeMonitors = isIncludeMonitors();
  if (!bincludeMonitors)
  {
    bseparateMonitors = isSeparateMonitors();
    bexcludeMonitors = isExcludeMonitors();
  }

  FILE* file = fopen(m_filename.c_str(), "rb");
  if (file == NULL)
  {
    g_log.error("Unable to open file " + m_filename);
    throw Exception::FileError("Unable to open File:", m_filename);
  }
  isisRaw->ioRAW(file, true);
  // This reads in the HDR_STRUCT run, user, title, date & time fields
  std::string title(isisRaw->hdr.hd_run, 69);
  // Insert some spaces to tidy the string up a bit
  title.insert(5, " ");
  title.insert(26, " ");
  title.insert(51, " ");
  g_log.information("*** Run title: " + title + " ***");

  // Read in the number of spectra in the RAW file
  m_numberOfSpectra = isisRaw->t_nsp1;
  // Read the number of periods in this file
  m_numberOfPeriods = isisRaw->t_nper;
  // Read the number of time channels (i.e. bins) from the RAW file
  const int channelsPerSpectrum = isisRaw->t_ntc1;
  // Read in the time bin boundaries
  m_lengthIn = channelsPerSpectrum + 1;

  // Call private method to validate the optional parameters, if set
  checkOptionalProperties();

  // Calculate the size of a workspace, given its number of periods & spectra to read
  const int total_specs = calculateWorkspaceSize();

  // If there is not enough memory use ManagedRawFileWorkspace2D.
  if (m_numberOfPeriods == 1 && MemoryManager::Instance().goForManagedWorkspace(total_specs, m_lengthIn,
      channelsPerSpectrum) && total_specs == m_numberOfSpectra)
  {
    goManagedRaw(bincludeMonitors, bexcludeMonitors, bseparateMonitors);
    return;
  }

  // Now check whether there is more than one time regime in use
  const int noTimeRegimes = isisRaw->daep.n_tr_shift;
  // Get the time channel array(s) and store in a vector inside a shared pointer
  std::vector<boost::shared_ptr<MantidVec> > timeChannelsVec =
      getTimeChannels(noTimeRegimes, m_lengthIn);

  // Need to extract the user-defined output workspace name
  Property *ws = getProperty("OutputWorkspace");
  std::string localWSName = ws->value();

  int histTotal = total_specs * m_numberOfPeriods;
  int histCurrent = -1;

  // Create the 2D workspace for the output
  DataObjects::Workspace2D_sptr localWorkspace = boost::dynamic_pointer_cast<DataObjects::Workspace2D>(
      WorkspaceFactory::Instance().create("Workspace2D", total_specs, m_lengthIn, m_lengthIn - 1));
  localWorkspace->setTitle(title);
  localWorkspace->getAxis(0)->unit() = UnitFactory::Instance().create("TOF");

  runLoadInstrument(localWorkspace);
  runLoadMappingTable(localWorkspace);

  WorkspaceGroup_sptr sptrWSGrp = WorkspaceGroup_sptr(new WorkspaceGroup);
  WorkspaceGroup_sptr monitorWSGrp;
  DataObjects::Workspace2D_sptr monitorWorkspace;
  int normalwsSpecs = 0;
  int monitorwsSpecs = 0;
  std::vector<int> monitorSpecList;
  if (bincludeMonitors)
  {
    setWorkspaceProperty("OutputWorkspace", title, sptrWSGrp, localWorkspace, false);
  }
  else
  {
    getmonitorSpectrumList(localWorkspace, monitorSpecList);
    //calculate the workspace size for normal workspace and monitor workspace
    calculateWorkspacesizes(monitorSpecList, total_specs, normalwsSpecs, monitorwsSpecs);
    // now create a workspace of size normalwsSpecs and set it as outputworkspace
    if (normalwsSpecs > 0)
    {
      localWorkspace = createWorkspace(normalwsSpecs, m_lengthIn);
      setWorkspaceProperty("OutputWorkspace", title, sptrWSGrp, localWorkspace, false);
    }
    if (normalwsSpecs <= 0 && bexcludeMonitors)
    {
      fclose(file);
      throw std::runtime_error(
          "All the selected spectra are monitors and Exclude monitors option selected ");
    }
    if (bseparateMonitors)
    {
      if (monitorwsSpecs > 0)
      { //create monitor workspace
        monitorWorkspace = createWorkspace(monitorwsSpecs, m_lengthIn);
        //create monitor workspace group
        monitorWSGrp = createGroupWorkspace();
        if (normalwsSpecs > 0)
        {
          std::string monitorWSName = localWSName + "_Monitors";
          declareProperty(new WorkspaceProperty<Workspace> ("MonitorWorkspace", monitorWSName,
              Direction::Output));
          setWorkspaceProperty("MonitorWorkspace", title, monitorWSGrp, monitorWorkspace, true);
        }
        else
        { //if only monitors range selcted
          //then set the monitor workspace as the outputworkspace
          setWorkspaceProperty("OutputWorkspace", title, monitorWSGrp, monitorWorkspace, false);
          localWorkspace = monitorWorkspace;
        }

      }
    }//end of if loop for separate monitors
  }

  // Loop over the number of periods in the raw file, putting each period in a separate workspace
  for (int period = 0; period < m_numberOfPeriods; ++period)
  {
    if (period > 0)
    {
      localWorkspace = boost::dynamic_pointer_cast<DataObjects::Workspace2D>(
          WorkspaceFactory::Instance().create(localWorkspace));
      localWorkspace->newSample();

      if (bseparateMonitors && monitorwsSpecs > 0)
      {
        monitorWorkspace = boost::dynamic_pointer_cast<DataObjects::Workspace2D>(
            WorkspaceFactory::Instance().create(monitorWorkspace));
        monitorWorkspace->newSample();
      }
      //localWorkspace->newInstrumentParameters(); ????
    }
    isisRaw->skipData(file, period * (m_numberOfSpectra + 1));
    int counter = 0;
    int monitorCount = 0;
    int counter1 = 0;
    for (int i = 1; i <= m_numberOfSpectra; ++i)
    {
      int histToRead = i + period * (m_numberOfSpectra + 1);
      if ((i >= m_spec_min && i < m_spec_max) || (m_list && find(m_spec_list.begin(), m_spec_list.end(),
          i) != m_spec_list.end()))
      {
        progress(m_prog, "Reading raw file data...");
        //if user selected  exclude monitors
        if (bexcludeMonitors)
        {
          if (isMonitor(monitorSpecList, i))
          {
            isisRaw->skipData(file, histToRead);
            continue;
          }
        }
        isisRaw->readData(file, histToRead);
        if (bincludeMonitors)
        {
          setWorkspaceData(localWorkspace, timeChannelsVec, counter, i, noTimeRegimes);
          //localWorkspace->getAxis(1)->spectraNo(counter) = i;
          ++counter;
        }
        if (bseparateMonitors)
        {
          if (isMonitor(monitorSpecList, i))
          { //g_log.error()<<"monitor spectrum no is "<<i<<" corresponding ws index is "<<monitorCount<<std::endl;
            setWorkspaceData(monitorWorkspace, timeChannelsVec, monitorCount, i, noTimeRegimes);
            ++monitorCount;
          }
          else
          {
            setWorkspaceData(localWorkspace, timeChannelsVec, counter1, i, noTimeRegimes);
            ++counter1;
          }
        }
        if (bexcludeMonitors)
        { //setWorkspaceData(localWorkspace,localWorkspace1,counter,timeChannelsVec,counter1,noTimeRegimes);
          setWorkspaceData(localWorkspace, timeChannelsVec, counter1, i, noTimeRegimes);
          ++counter1;
        }

        if (m_numberOfPeriods == 1)
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
        isisRaw->skipData(file, histToRead);
      }
    }
    // Just a sanity check
    //assert(counter == total_specs);
    if (period == 0)
    {
      // Only run the sub-algorithms once
      if (bLoadlogFiles)
        runLoadLog(localWorkspace);

      if (bseparateMonitors || bexcludeMonitors)
      {
        if (monitorWorkspace)
          runLoadLog(monitorWorkspace);
        runLoadInstrument(localWorkspace);
        runLoadMappingTable(localWorkspace);
        if (monitorWorkspace)
          runLoadInstrument(monitorWorkspace);
        if (monitorWorkspace)
          runLoadMappingTable(monitorWorkspace);
        if (monitorWorkspace)
          monitorWorkspace->getSample()->setProtonCharge(isisRaw->rpb.r_gd_prtn_chrg);
      }
      // Set the total proton charge for this run
      // (not sure how this works for multi_period files)
      localWorkspace->getSample()->setProtonCharge(isisRaw->rpb.r_gd_prtn_chrg);
    }
    else // We are working on a higher period of a multiperiod raw file
    {
      if (bLoadlogFiles)
      {
        runLoadLog(localWorkspace, period + 1);
        if (monitorWorkspace)
          runLoadLog(monitorWorkspace, period + 1);
      }
    }

    // check if values stored in logfiles should be used to define parameters of the instrument
    populateInstrumentParameters(localWorkspace);
    if (monitorWorkspace)
      populateInstrumentParameters(monitorWorkspace);

    // Assign the result to the output workspace property

    if (m_numberOfPeriods > 1)
    {
      if (bseparateMonitors)
      {
        if (normalwsSpecs > 0)
        {
          if (monitorwsSpecs > 0)
            setWorkspaceProperty(monitorWorkspace, monitorWSGrp, period, true);
          setWorkspaceProperty(localWorkspace, sptrWSGrp, period, false);
        }
        else
          setWorkspaceProperty(localWorkspace, sptrWSGrp, period, false);
      }
      else
      {
        setWorkspaceProperty(localWorkspace, sptrWSGrp, period, false);
      }
      // progress for workspace groups 
      m_prog = (double(period) / (m_numberOfPeriods - 1));
    }

  } // loop over periods
  // Clean up
  isisRaw.reset();
  fclose(file);
}
/** sets the workspace properties
 *  @param wsPtr  shared pointer to  workspace
 *  @param wsGrpSptr shared pointer to  group workspace
 *  @param  period period number
 *  @param bmonitors boolean flag to name  the workspaces
 */
void LoadRaw3::setWorkspaceProperty(DataObjects::Workspace2D_sptr wsPtr, WorkspaceGroup_sptr wsGrpSptr,
    const int period, bool bmonitors)
{
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
  wsGrpSptr->add(wsName);
  setProperty(outws, boost::dynamic_pointer_cast<DataObjects::Workspace2D>(wsPtr));

}
/** This method calaculates the size of normal workspace and monitor workspace
 *  @param monitorSpecList  a vector holding the monitors spectrum indexes
 *  @param total_specs  total number of spectra including monitors
 *  @param  normalwsSpecs  total specs for normal workspace
 *  @param monitorwsSpecs  total specs for monitor workspace
 */
void LoadRaw3::calculateWorkspacesizes(const std::vector<int>& monitorSpecList, const int total_specs,
    int& normalwsSpecs, int & monitorwsSpecs)
{
  if (!m_interval && !m_bmspeclist)
  {
    normalwsSpecs = total_specs - monitorSpecList.size();
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
      normalwsSpecs = total_specs - monitorwsSpecs;
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
          normalwsSpecs = total_specs - monitorwsSpecs;
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
        //nSize=m_spec_list.size()-mSize;
        normalwsSpecs = total_specs - monitorwsSpecs;

      }
    }//end of if loop for m_bmspeclist
  }

}

/** This method sets the raw file data to workspace vectors
 *  @param newWorkspace  shared pointer to the  workspace
 *  @param timeChannelsVec  vector holding the X data
 *  @param  wsIndex  variable used for indexing the ouputworkspace
 *  @param  nspecNum  spectrum number
 *  @param noTimeRegimes   regime no.
 */
void LoadRaw3::setWorkspaceData(DataObjects::Workspace2D_sptr newWorkspace, const std::vector<
    boost::shared_ptr<MantidVec> >& timeChannelsVec, int wsIndex, int nspecNum, int noTimeRegimes)
{
  typedef double (*uf)(double);
  uf dblSqrt = std::sqrt;
  // But note that the last (overflow) bin is kept
  MantidVec& Y = newWorkspace->dataY(wsIndex);
  Y.assign(isisRaw->dat1 + 1, isisRaw->dat1 + m_lengthIn);
  // Fill the vector for the errors, containing sqrt(count)
  MantidVec& E = newWorkspace->dataE(wsIndex);
  std::transform(Y.begin(), Y.end(), E.begin(), dblSqrt);
  // Set the X vector pointer and spectrum number
  if (noTimeRegimes < 2)
    newWorkspace->setX(wsIndex, timeChannelsVec[0]);
  else
    // Use std::vector::at just incase spectrum missing from spec array
    newWorkspace->setX(wsIndex, timeChannelsVec.at(m_specTimeRegimes[nspecNum] - 1));
  newWorkspace->getAxis(1)->spectraNo(wsIndex) = nspecNum;

}
/** This method returns the monitor spectrun list 
 *  @param localWorkspace  shared pointer to  workspace 
 *  @param monitorSpecList a list holding the spectrum indexes of the monitors
 */
void LoadRaw3::getmonitorSpectrumList(DataObjects::Workspace2D_sptr localWorkspace,
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
    monitorSpecList.assign(newVec.begin(), newVec.end());//spectrmap.getSpectra(m_monitordetectorList);
  }
  else
    g_log.error() << "monitor detector id list is empty  for the selected workspace" << std::endl;
}
/** This method checks the value of LoadMonitors property and returns true or false
 * @return true if Exclude Monitors option is selected,otherwise false
 */
bool LoadRaw3::isExcludeMonitors()
{
  bool bExclude;
  std::string monitorOption = getPropertyValue("LoadMonitors");
  monitorOption.compare("Exclude") ? (bExclude = false) : (bExclude = true);
  return bExclude;
}

/** This method checks the value of LoadMonitors property and returns true or false
 * @return true if Include Monitors option is selected,otherwise false
 */
bool LoadRaw3::isIncludeMonitors()
{
  bool bExclude;
  std::string monitorOption = getPropertyValue("LoadMonitors");
  monitorOption.compare("Include") ? (bExclude = false) : (bExclude = true);
  return bExclude;
}
/** This method checks the value of LoadMonitors property and returns true or false
 * @return true if Separate Monitors option is selected,otherwise false
 */
bool LoadRaw3::isSeparateMonitors()
{
  bool bSeparate;
  std::string monitorOption = getPropertyValue("LoadMonitors");
  monitorOption.compare("Separate") ? (bSeparate = false) : (bSeparate = true);
  return bSeparate;
}
/** This method checks given spectrum is a monitor
 *@param monitorIndexes a vector holding the list of monitors
 *@param spectrumNum  the requested spectrum number
 * @return true if it's a monitor 
 */
bool LoadRaw3::isMonitor(const std::vector<int>& monitorIndexes, int spectrumNum)
{
  bool bMonitor;
  std::vector<int>::const_iterator itr;
  itr = find(monitorIndexes.begin(), monitorIndexes.end(), spectrumNum);
  (itr != monitorIndexes.end()) ? (bMonitor = true) : (bMonitor = false);
  return bMonitor;
}
/** This method creates pointer to workspace
 *@param nVectors The number of vectors/histograms in the workspace
 *@param  lengthIn The number of X data points/bin boundaries in each vector 
 *@return Workspace2D_sptr shared pointer to the workspace
 */
DataObjects::Workspace2D_sptr LoadRaw3::createWorkspace(int nVectors, int lengthIn)
{
  DataObjects::Workspace2D_sptr monitorWorkspace =
      boost::dynamic_pointer_cast<DataObjects::Workspace2D>(WorkspaceFactory::Instance().create(
          "Workspace2D", nVectors, lengthIn, lengthIn - 1));
  return monitorWorkspace;
}
/** This method creates pointer to group workspace
 * @return WorkspaceGroup_sptr shared pointer to the workspace
 */
WorkspaceGroup_sptr LoadRaw3::createGroupWorkspace()
{
  WorkspaceGroup_sptr workspacegrp(new WorkspaceGroup);
  return workspacegrp;
}
/** This method sets the workspace property
 * @param propertyName property name for the workspace
 * @param title title of the workspace
 * @param grpWS  shared pointer to group workspace
 * @param workspace  shared pointer to workspace
 * @param  bMonitor to identify the workspace is an output workspace or monitor workspace
 */
void LoadRaw3::setWorkspaceProperty(const std::string & propertyName, const std::string& title,
    WorkspaceGroup_sptr grpWS, DataObjects::Workspace2D_sptr workspace, bool bMonitor)
{
  Property *ws = getProperty("OutputWorkspace");
  std::string wsName = ws->value();
  if (bMonitor)
    wsName += "_Monitors";
  workspace->setTitle(title);
  workspace->getAxis(0)->unit() = UnitFactory::Instance().create("TOF");
  if (m_numberOfPeriods > 1)
  {
    grpWS->add(wsName);
    setProperty(propertyName, boost::dynamic_pointer_cast<Workspace>(grpWS));
  }
  else
  {
    setProperty(propertyName, boost::dynamic_pointer_cast<Workspace>(workspace));
  }
}

/// Validates the optional 'spectra to read' properties, if they have been set
void LoadRaw3::checkOptionalProperties()
{
  //read in the settings passed to the algorithm
  m_spec_list = getProperty("SpectrumList");
  m_spec_max = getProperty("SpectrumMax");
  m_spec_min = getProperty("SpectrumMin");

  m_list = !m_spec_list.empty();
  m_bmspeclist = !m_spec_list.empty();
  m_interval = (m_spec_max != unSetInt) || (m_spec_min != 1);
  if (m_spec_max == unSetInt)
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
      if (maxlist > m_numberOfSpectra || minlist <= 0)
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
    m_spec_min = getProperty("SpectrumMin");
    if (m_spec_min != 1 && m_spec_max == 1)
      m_spec_max = m_numberOfSpectra;
    if (m_spec_max < m_spec_min || m_spec_max > m_numberOfSpectra)
    {
      g_log.error("Invalid Spectrum min/max properties");
      throw std::invalid_argument("Inconsistent properties defined");
    }

  }
}

/// Calculates the total number of spectra in the workspace, given the input properties
int LoadRaw3::calculateWorkspaceSize()
{
  int total_specs(0);
  if (m_interval || m_list)
  {
    if (m_interval)
    {
      if (m_spec_min != 1 && m_spec_max == 1)
        m_spec_max = m_numberOfSpectra;

      total_specs = (m_spec_max - m_spec_min + 1);
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

    }
  }
  else
  {
    total_specs = m_numberOfSpectra;
    // In this case want all the spectra, but zeroth spectrum is garbage so go from 1 to NSP1
    m_spec_min = 1;
    m_spec_max = m_numberOfSpectra + 1;
  }
  return total_specs;
}

/// Creates a ManagedRawFileWorkspace2D
void LoadRaw3::goManagedRaw(bool bincludeMonitors, bool bexcludeMonitors, bool bseparateMonitors)
{
  const std::string cache_option = getPropertyValue("Cache");
  bool bLoadlogFiles = getProperty("LoadLogFiles");
  int option = find(m_cache_options.begin(), m_cache_options.end(), cache_option)
      - m_cache_options.begin();
  progress(m_prog, "Reading raw file data...");
  DataObjects::Workspace2D_sptr localWorkspace = DataObjects::Workspace2D_sptr(
      new ManagedRawFileWorkspace2D(m_filename, option));
  m_prog = 0.2;
  runLoadInstrument(localWorkspace);
  m_prog = 0.4;
  runLoadMappingTable(localWorkspace);
  m_prog = 0.5;
  if (bLoadlogFiles)
  {
    runLoadLog(localWorkspace);
    //if(monitorWorkspace)runLoadLog(monitorWorkspace);
  }
  localWorkspace->getSample()->setProtonCharge(isisRaw->rpb.r_gd_prtn_chrg);
  m_prog = 0.7;
  progress(m_prog);
  for (int i = 0; i < m_numberOfSpectra; ++i)
  {
    localWorkspace->getAxis(1)->spectraNo(i) = i + 1;
  }
  m_prog = 0.9;
  populateInstrumentParameters(localWorkspace);
  separateOrexcludeMonitors(localWorkspace, bincludeMonitors, bexcludeMonitors, bseparateMonitors);
  m_prog = 1.0;
  progress(m_prog);
  setProperty("OutputWorkspace", boost::dynamic_pointer_cast<Workspace>(localWorkspace));

}
/** This method separates/excludes monitors from output workspace and creates a separate workspace for monitors
 * @param localWorkspace shared pointer to workspace
 * @param bincludeMonitors boolean  variable for including monitors
 * @param bexcludeMonitors  boolean variable for excluding monitors
 * @param bseparateMonitors  boolean variable for separating the monitor workspace from output workspace
 */
void LoadRaw3::separateOrexcludeMonitors(DataObjects::Workspace2D_sptr localWorkspace,
    bool bincludeMonitors, bool bexcludeMonitors, bool bseparateMonitors)
{
  std::vector<boost::shared_ptr<MantidVec> > timeChannelsVec;
  std::vector<int> monitorSpecList;
  std::vector<int> monitorwsList;
  int noTimeRegimes = 0;
  DataObjects::Workspace2D_sptr monitorWorkspace;
  FILE *file;
  getmonitorSpectrumList(localWorkspace, monitorSpecList);
  //calculateWorkspacesizes(monitorSpecList,m_numberOfSpectra,normalwsSpecs,monitorwsSpecs);
  if (bseparateMonitors && monitorSpecList.size() > 0)
  {
    Property *ws = getProperty("OutputWorkspace");
    std::string localWSName = ws->value();
    std::string monitorWSName = localWSName + "_Monitors";
    declareProperty(new WorkspaceProperty<Workspace> ("MonitorWorkspace", monitorWSName,
        Direction::Output));
    //create monitor workspace
    monitorWorkspace = createWorkspace(monitorSpecList.size(), m_lengthIn);
    //monitorWorkspace->setTitle(title);
    monitorWorkspace->getAxis(0)->unit() = UnitFactory::Instance().create("TOF");
    setProperty("MonitorWorkspace", boost::dynamic_pointer_cast<Workspace>(monitorWorkspace));
    file = fopen(m_filename.c_str(), "rb");
    if (file == NULL)
    {
      throw Exception::FileError("Unable to open File:", m_filename);
    }
    isisRaw->ioRAW(file, true);
  }
  // Now check whether there is more than one time regime in use
  noTimeRegimes = isisRaw->daep.n_tr_shift;
  // Get the time channel array(s) and store in a vector inside a shared pointer
  timeChannelsVec = getTimeChannels(noTimeRegimes, m_lengthIn);
  //read raw file
  if (bseparateMonitors && monitorSpecList.size() > 0)
    isisRaw->readData(file, 0);
  int monitorwsIndex = 0;
  for (int i = 0; i < m_numberOfSpectra; ++i)
  {
    int histToRead = i + 1;
    if (bseparateMonitors && monitorSpecList.size() > 0)
    {
      isisRaw->readData(file, histToRead);
    }
    if ((bseparateMonitors && monitorSpecList.size() > 0) || bexcludeMonitors)
    {
      if (isMonitor(monitorSpecList, i + 1))
      {
        std::map<int, int> wsIndexmap;
        localWorkspace->getAxis(1)->getSpectraIndexMap(wsIndexmap);
        std::map<int, int>::const_iterator wsItr;
        wsItr = wsIndexmap.find(i + 1);
        if (wsItr != wsIndexmap.end())
          monitorwsList.push_back(wsItr->second);
        if (bseparateMonitors)
        { //g_log.error()<<"Monitor WS Index for managed workspace is "<<wsItr->second<<std::endl;
          monitorWorkspace->getAxis(1)->spectraNo(monitorwsIndex) = i + 1;
          setWorkspaceData(monitorWorkspace, timeChannelsVec, monitorwsIndex, i + 1, noTimeRegimes);
          ++monitorwsIndex;
        }
      }
    }

  }
  if ((bseparateMonitors && monitorwsList.size() > 0) || bexcludeMonitors)
  {
    localWorkspace->setMonitorList(monitorwsList);
    localWorkspace->sethistogramNumbers(m_numberOfSpectra - monitorwsList.size());
    if (bseparateMonitors)
    {
      fclose(file);
      monitorWorkspace->getSample()->setProtonCharge(isisRaw->rpb.r_gd_prtn_chrg);
      if (monitorWorkspace)
      {
        runLoadInstrument(monitorWorkspace);
        runLoadMappingTable(monitorWorkspace);
        runLoadLog(monitorWorkspace);
        populateInstrumentParameters(monitorWorkspace);
      }
    }
  }

}
/** Constructs the time channel (X) vector(s)
 *  @param regimes  The number of time regimes (if 1 regime, will actually contain 0)
 *  @param lengthIn The number of time channels
 *  @return The vector(s) containing the time channel boundaries, in a vector of shared ptrs
 */
std::vector<boost::shared_ptr<MantidVec> > LoadRaw3::getTimeChannels(const int& regimes,
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
void LoadRaw3::runLoadInstrument(DataObjects::Workspace2D_sptr localWorkspace)
{
  g_log.debug("Loading the instrument definition...");
  progress(m_prog, "Loading the instrument geometry...");
  // Determine the search directory for XML instrument definition files (IDFs)
  std::string directoryName = Kernel::ConfigService::Instance().getString(
      "instrumentDefinition.directory");
  if (directoryName.empty())
  {
    // This is the assumed deployment directory for IDFs, where we need to be relative to the
    // directory of the executable, not the current working directory.
    directoryName = Poco::Path(Mantid::Kernel::ConfigService::Instance().getBaseDir()).resolve(
        "../Instrument").toString();
  }

  std::string instrumentID = isisRaw->i_inst; // get the instrument name
  size_t i = instrumentID.find_first_of(' '); // cut trailing spaces
  if (i != std::string::npos)
    instrumentID.erase(i);

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
    runLoadInstrumentFromRaw(localWorkspace);
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
void LoadRaw3::runLoadInstrumentFromRaw(DataObjects::Workspace2D_sptr localWorkspace)
{
  g_log.information() << "Instrument definition file not found. Attempt to load information about \n"
      << "the instrument from raw data file.\n";

  IAlgorithm_sptr loadInst = createSubAlgorithm("LoadInstrumentFromRaw");
  loadInst->setPropertyValue("Filename", m_filename);
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
    ;
  }
  if (!loadInst->isExecuted())
    g_log.error("No instrument definition loaded");
}

/// Run the LoadMappingTable sub-algorithm to fill the SpectraToDetectorMap
void LoadRaw3::runLoadMappingTable(DataObjects::Workspace2D_sptr localWorkspace)
{
  g_log.debug("Loading the spectra-detector mapping...");
  progress(m_prog, "Loading the spectra-detector mapping...");
  // Now determine the spectra to detector map calling sub-algorithm LoadMappingTable
  // There is a small penalty in re-opening the raw file but nothing major.
  IAlgorithm_sptr loadmap = createSubAlgorithm("LoadMappingTable");
  loadmap->setPropertyValue("Filename", m_filename);
  loadmap->setProperty<MatrixWorkspace_sptr> ("Workspace", localWorkspace);
  try
  {
    loadmap->execute();
  } catch (std::runtime_error&)
  {
    g_log.error("Unable to successfully execute LoadMappingTable sub-algorithm");
  }

  if (!loadmap->isExecuted())
    g_log.error("LoadMappingTable sub-algorithm is not executed");

}

/// Run the LoadLog sub-algorithm
void LoadRaw3::runLoadLog(DataObjects::Workspace2D_sptr localWorkspace, int period)
{
  g_log.debug("Loading the log files...");
  progress(m_prog, "Reading log files...");
  IAlgorithm_sptr loadLog = createSubAlgorithm("LoadLog");
  // Pass through the same input filename
  loadLog->setPropertyValue("Filename", m_filename);
  // Set the workspace property to be the same one filled above
  loadLog->setProperty<MatrixWorkspace_sptr> ("Workspace", localWorkspace);
  loadLog->setProperty("Period", period);

  // Now execute the sub-algorithm. Catch and log any error, but don't stop.
  try
  {
    loadLog->execute();
  } catch (std::runtime_error&)
  {
    g_log.error("Unable to successfully run LoadLog sub-algorithm");
  }

  if (!loadLog->isExecuted())
    g_log.error("Unable to successfully run LoadLog sub-algorithm");
}

/** Add parameters to the instrument parameter map that are defined in instrument
 *  definition file and for which logfile data are available
 *
 *  @param localWorkspace A pointer to a workspace
 */
void LoadRaw3::populateInstrumentParameters(DataObjects::Workspace2D_sptr localWorkspace)
{
  g_log.debug("Populating the instrument parameters...");
  progress(m_prog, "Populating the instrument parameters...");
  // Get instrument and sample
  boost::shared_ptr<Instrument> instrument;
  boost::shared_ptr<Sample> sample;
  instrument = localWorkspace->getBaseInstrument();
  sample = localWorkspace->getSample();

  // Get the data in the logfiles associated with the raw data
  const std::vector<Kernel::Property*>& logfileProp = sample->getLogData();

  // Get pointer to parameter map that we may add parameters to and information about
  // the parameters that my be specified in the instrument definition file (IDF)
  Geometry::ParameterMap& paramMap = localWorkspace->instrumentParameters();
  std::multimap<std::string, boost::shared_ptr<DataHandling::XMLlogfile> >& paramInfoFromIDF =
      instrument->getLogfileCache();

  // iterator to browse throw the multimap: paramInfoFromIDF
  std::multimap<std::string, boost::shared_ptr<DataHandling::XMLlogfile> >::const_iterator it;
  std::pair<std::multimap<std::string, boost::shared_ptr<DataHandling::XMLlogfile> >::iterator,
      std::multimap<std::string, boost::shared_ptr<DataHandling::XMLlogfile> >::iterator> ret;

  // loop over all logfiles and see if any of these are associated with parameters in the
  // IDF
  unsigned int N = logfileProp.size();
  for (unsigned int i = 0; i < N; i++)
  {
    // Remove the path, the run number and extension from logfile filename

    std::string logFilename = logfileProp[i]->name();
    std::string filenamePart = Poco::Path(logFilename).getFileName(); // get filename part only
    if (filenamePart.size() > 4 && filenamePart.rfind('.') == filenamePart.size() - 4)
    {
      filenamePart = filenamePart.erase(filenamePart.size() - 4, filenamePart.size()); // remove extension
      filenamePart = filenamePart.substr(9); // remove front run number part
    }

    // See if filenamePart matches any logfile-IDs in IDF. If this add parameter to parameter map
    ret = paramInfoFromIDF.equal_range(filenamePart);
    for (it = ret.first; it != ret.second; ++it)
    {
      double value = ((*it).second)->createParamValue(
          static_cast<Kernel::TimeSeriesProperty<double>*> (logfileProp[i]));

      // special case if parameter name is "x", "y" or "z" and "rot"

      std::string paramN = ((*it).second)->m_paramName;
      if (paramN.compare("x") == 0 || paramN.compare("y") == 0 || paramN.compare("z") == 0)
        paramMap.addPositionCoordinate(((*it).second)->m_component, paramN, value);
      else if (paramN.compare("rot") == 0)
        paramMap.addRotationParam(((*it).second)->m_component, paramN, value);
      else
        paramMap.addDouble(((*it).second)->m_component, paramN, value);
    }
  } // finished looping over logfiles


  // Check if parameters have been specified using the 'value' attribute rather than the 'logfile-id' attribute
  // All such parameters have been stored using the key = "".
  ret = paramInfoFromIDF.equal_range("");
  TimeSeriesProperty<double>* dummy = NULL;
  for (it = ret.first; it != ret.second; ++it)
  {
    double value = ((*it).second)->createParamValue(dummy);

    // special case if parameter name is "x", "y" or "z" and "rot"
    std::string paramN = ((*it).second)->m_paramName;
    if (paramN.compare("x") == 0 || paramN.compare("y") == 0 || paramN.compare("z") == 0)
      paramMap.addPositionCoordinate(((*it).second)->m_component, paramN, value);
    else if (paramN.compare("rot") == 0)
      paramMap.addRotationParam(((*it).second)->m_component, paramN, value);
    else
      paramMap.addDouble(((*it).second)->m_component, paramN, value);
  }
}

} // namespace DataHandling
} // namespace Mantid
