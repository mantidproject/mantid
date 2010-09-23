//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidNexus/LoadNexusProcessed.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidAPI/SpectraDetectorMap.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/DateAndTime.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/AlgorithmFactory.h"
#include "MantidNexus/NexusClasses.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidKernel/ArrayProperty.h"
#include "Poco/Path.h"
#include "Poco/DateTimeParser.h"
#include "Poco/StringTokenizer.h"
#include <cmath>
#include <boost/shared_ptr.hpp>

namespace Mantid
{
namespace NeXus
{

// Register the algorithm into the algorithm factory
DECLARE_ALGORITHM(LoadNexusProcessed)

        using namespace Kernel;
using namespace API;

/// Default constructor
LoadNexusProcessed::LoadNexusProcessed() : Algorithm(), m_shared_bins(false), m_xbins(),
    m_axis1vals(), m_list(false), m_interval(false),
    m_spec_list(), m_spec_min(0), m_spec_max(Mantid::EMPTY_INT())
{
  NXMDisableErrorReporting();
}

/// Delete NexusFileIO in destructor
LoadNexusProcessed::~LoadNexusProcessed()
{
}

/** Initialisation method.
 *
 */
void LoadNexusProcessed::init()
{
  // Declare required input parameters for algorithm
  std::vector<std::string> exts;
  exts.push_back(".nxs");
  exts.push_back(".nx5");
  exts.push_back(".xml");
  declareProperty(new FileProperty("Filename", "", FileProperty::Load, exts),
      "The name of the processed Nexus file to load" );
  declareProperty(new WorkspaceProperty<Workspace> ("OutputWorkspace", "",
      Direction::Output),
      "The name of the workspace to be created as the output of the\n"
      "algorithm. For multiperiod files, one workspace may be\n"
      "generated for each period. Currently only one workspace can\n"
      "be saved at a time so multiperiod Mantid files are not\n"
      "generated");


  // optional
  BoundedValidator<int> *mustBePositive = new BoundedValidator<int> ();
  mustBePositive->setLower(0);

  declareProperty("SpectrumMin", 1, mustBePositive,
      "Index number of the first spectrum to read, only used if\n"
      "spectrum_max is set and only for single period data, not\n"
      " yet implemented (default 0)");
  declareProperty("SpectrumMax", Mantid::EMPTY_INT(), mustBePositive->clone(),
      "Index of last spectrum to read, only for single period data,\n"
      " not yet implemented (default the last spectrum).");
  declareProperty(new ArrayProperty<int> ("SpectrumList"),
      "Array, or comma separated list, of indexes of spectra to\n"
      "load. Not yet implemented.");
  declareProperty("EntryNumber", 0, mustBePositive->clone(),
      "The particular entry number to read (default: read all entries)" );
}

/** Executes the algorithm. Reading in the file and creating and populating
 *  the output workspace
 *
 *  @throw runtime_error Thrown if algorithm cannot execute
 */
void LoadNexusProcessed::exec()
{
  progress(0,"Opening file...");

  //Throws an approriate exception if there is a problem with file access
  NXRoot root(getPropertyValue("Filename"));

  //Find out how many first level entries there are
  int nperiods = root.groups().size();

  // Check for an entry number property
  int entrynumber = getProperty("EntryNumber");

  if( entrynumber > 0 && entrynumber > nperiods )
  {
    g_log.error() << "Invalid entry number specified. File only contains " << nperiods << " entries.\n";
    throw std::invalid_argument("Invalid entry number specified.");
  }

  const std::string basename = "mantid_workspace_";
  if( nperiods == 1 || entrynumber > 0 )
  {
    if( entrynumber == 0 ) ++entrynumber;
    std::ostringstream os;
    os << entrynumber;
    DataObjects::Workspace2D_sptr local_workspace = loadEntry(root, basename + os.str(), 0, 1);
    API::Workspace_sptr workspace = boost::static_pointer_cast<API::Workspace>(local_workspace);
    setProperty("OutputWorkspace", workspace);
  }
  else
  {
    WorkspaceGroup_sptr wksp_group(new WorkspaceGroup);
    //This forms the name of the group
    std::string base_name = getPropertyValue("OutputWorkspace");
    // First member of group should be the group itself, for some reason!
    wksp_group->add(base_name);
    base_name += "_";
    const std::string prop_name = "OutputWorkspace_";
    for( int p = 1; p <= nperiods; ++p )
    {
      std::ostringstream os;
      os << p;
      DataObjects::Workspace2D_sptr local_workspace = loadEntry(root, basename + os.str(), (p-1)/nperiods, 1/nperiods);
      declareProperty(new WorkspaceProperty<DataObjects::Workspace2D>(prop_name + os.str(), base_name + os.str(),
          Direction::Output));
      wksp_group->add(base_name + os.str());
      setProperty(prop_name + os.str(), local_workspace);
    }

    // The group is the root property value
    setProperty("OutputWorkspace", boost::static_pointer_cast<Workspace>(wksp_group));

  }

  m_axis1vals.clear();
}
/**
 * Load a single entry into a workspace
 * @param root The opened root node
 * @param entry_name The entry name
 * @param progressStart The percentage value to start the progress reporting for this entry
 * @param progressRange The percentage range that the progress reporting should cover
 * @returns A 2D workspace containing the loaded data
 */
DataObjects::Workspace2D_sptr LoadNexusProcessed::loadEntry(NXRoot & root, const std::string & entry_name,
    const double& progressStart, const double& progressRange)
{
  progress(progressStart,"Opening entry " + entry_name + "...");

  NXEntry mtd_entry = root.openEntry(entry_name);
  // Get workspace characteristics
  NXData wksp_cls = mtd_entry.openNXData("workspace");

  // Axis information
  // "X" axis
  NXDouble xbins = wksp_cls.openNXDouble("axis1");
  std::string unit1 = xbins.attributes("units");
  // Non-uniform x bins get saved as a 2D 'axis1' dataset
  int xlength(-1);
  if( xbins.rank() == 2 )
  {
    xlength = xbins.dim1();
    m_shared_bins = false;
  }
  else if( xbins.rank() == 1 )
  {
    xlength = xbins.dim0();
    m_shared_bins = true;
    xbins.load();
    m_xbins.access().assign(xbins(), xbins() + xlength);
  }
  else
  {
    throw std::runtime_error("Unknown axis1 dimension encountered.");
  }

  // MatrixWorkspace axis 1
  NXDouble axis2 = wksp_cls.openNXDouble("axis2");
  std::string unit2 = axis2.attributes("units");

  NXDataSetTyped<double> data = wksp_cls.openDoubleData();
  int nspectra = data.dim0();
  int nchannels = data.dim1();

  //// validate the optional spectrum parameters, if set
  checkOptionalProperties(nspectra);

  //size of the workspace
  int total_specs=calculateWorkspacesize(nspectra);

  //// Create the 2D workspace for the output
  DataObjects::Workspace2D_sptr local_workspace = boost::dynamic_pointer_cast<DataObjects::Workspace2D>
  (WorkspaceFactory::Instance().create("Workspace2D", total_specs, xlength, nchannels));
  try
  {
    local_workspace->setTitle(mtd_entry.getString("title"));
  }
  catch (std::runtime_error)
  {
    g_log.debug() << "No title was found in the input file, " << getPropertyValue("Filename") << std::endl;
  }

  //Units
  try
  {
    local_workspace->getAxis(0)->unit() = UnitFactory::Instance().create(unit1);
    //If this doesn't throw then it is a numeric access so grab the data so we can set it later
    axis2.load();
    m_axis1vals = MantidVec(axis2(), axis2() + axis2.dim0());
  }
  catch( std::runtime_error & )
  {
    g_log.information() << "Axis 0 set to unitless quantity \"" << unit1 << "\"\n";
  }

  try
  {
    local_workspace->getAxis(1)->unit() = UnitFactory::Instance().create(unit2);
  }
  catch( std::runtime_error & )
  {
    g_log.information() << "Axis 1 set to unitless quantity \"" << unit2 << "\"\n";
  }
  local_workspace->setYUnit(data.attributes("units"));
  std::string unitLabel = data.attributes("unit_label");
  if (unitLabel.empty()) unitLabel = data.attributes("units");
  local_workspace->setYUnitLabel(unitLabel);

  //Are we a distribution
  std::string dist = xbins.attributes("distribution");
  if( dist == "1" )
  {
    local_workspace->isDistribution(true);
  }
  else
  {
    local_workspace->isDistribution(false);
  }

  //Get information from all but data group
  progress(progressStart+0.05*progressRange,"Reading the instrument details...");
  readInstrumentGroup(mtd_entry, local_workspace);

  progress(progressStart+0.1*progressRange,"Reading the sample details...");
  readSampleGroup(mtd_entry, local_workspace);

  progress(progressStart+0.15*progressRange,"Reading the workspace history...");
  try
  {
    readAlgorithmHistory(mtd_entry, local_workspace);
  }
  catch (std::out_of_range)
  {
    g_log.warning() << "Error in the workspaces algorithm list, its processing history is incomplete\n";
  }

  progress(progressStart+0.2*progressRange,"Reading the workspace history...");
  readParameterMap(mtd_entry, local_workspace);

  readBinMasking(wksp_cls, local_workspace);
  NXDataSetTyped<double> errors = wksp_cls.openNXDouble("errors");

  int blocksize(8);
  //const int fullblocks = nspectra / blocksize;
  int fullblocks = total_specs / blocksize;
  int read_stop = (fullblocks * blocksize);
  const double progressBegin = progressStart+0.25*progressRange;
  const double progressScaler = 0.75*progressRange;
  int hist_index = 0;
  int wsIndex=0;
  if( m_shared_bins )
  {
    //if spectrum min,max,list properties are set
    if(m_interval||m_list)
    {
      //if spectrum max,min properties are set read the data as a block(multiple of 8) and
      //then read the remaining data as finalblock
      if(m_interval)
      {
        //specs at the min-max interval
        int interval_specs=m_spec_max-m_spec_min;
        fullblocks=(interval_specs)/blocksize;
        read_stop = (fullblocks * blocksize)+m_spec_min-1;

        if(interval_specs<blocksize)
        {
          blocksize=total_specs;
          read_stop=m_spec_max-1;
        }
        hist_index=m_spec_min-1;

        for( ; hist_index < read_stop; )
        {
          progress(progressBegin+progressScaler*hist_index/read_stop,"Reading workspace data...");
          loadBlock(data, errors, blocksize, nchannels, hist_index,wsIndex, local_workspace);
        }
        int finalblock = m_spec_max-1 - read_stop;
        if( finalblock > 0 )
        {
          loadBlock(data, errors, finalblock, nchannels, hist_index,wsIndex,local_workspace);
        }
      }
      // if spectrum list property is set read each spectrum separately by setting blocksize=1
      if(m_list)
      {
        std::vector<int>::iterator itr=m_spec_list.begin();
        for(;itr!=m_spec_list.end();++itr)
        {
          int specIndex=(*itr)-1;
          progress(progressBegin+progressScaler*specIndex/m_spec_list.size(),"Reading workspace data...");
          loadBlock(data, errors, 1, nchannels, specIndex,wsIndex, local_workspace);
        }

      }
    }
    else
    {
      for( ; hist_index < read_stop; )
      {
        progress(progressBegin+progressScaler*hist_index/read_stop,"Reading workspace data...");
        loadBlock(data, errors, blocksize, nchannels, hist_index,wsIndex, local_workspace);
      }
      int finalblock = total_specs - read_stop;
      if( finalblock > 0 )
      {
        loadBlock(data, errors, finalblock, nchannels, hist_index,wsIndex,local_workspace);
      }
    }

  }
  else
  {
    if(m_interval||m_list)
    {
      if(m_interval)
      {
        int interval_specs=m_spec_max-m_spec_min;
        fullblocks=(interval_specs)/blocksize;
        read_stop = (fullblocks * blocksize)+m_spec_min-1;

        if(interval_specs<blocksize)
        {
          blocksize=interval_specs;
          read_stop=m_spec_max-1;
        }
        hist_index=m_spec_min-1;

        for( ; hist_index < read_stop; )
        {
          progress(progressBegin+progressScaler*hist_index/read_stop,"Reading workspace data...");
          loadBlock(data, errors, xbins, blocksize, nchannels, hist_index,wsIndex,local_workspace);
        }
        int finalblock = m_spec_max-1 - read_stop;
        if( finalblock > 0 )
        {
          loadBlock(data, errors, xbins, finalblock, nchannels, hist_index,wsIndex, local_workspace);
        }
      }
      //
      if(m_list)
      {
        std::vector<int>::iterator itr=m_spec_list.begin();
        for(;itr!=m_spec_list.end();++itr)
        {
          int specIndex=(*itr)-1;
          progress(progressBegin+progressScaler*specIndex/read_stop,"Reading workspace data...");
          loadBlock(data, errors, xbins, 1, nchannels, specIndex,wsIndex,local_workspace);
        }

      }
    }
    else
    {
      for( ; hist_index < read_stop; )
      {
        progress(progressBegin+progressScaler*hist_index/read_stop,"Reading workspace data...");
        loadBlock(data, errors, xbins, blocksize, nchannels, hist_index,wsIndex,local_workspace);
      }
      int finalblock = total_specs - read_stop;
      if( finalblock > 0 )
      {
        loadBlock(data, errors, xbins, finalblock, nchannels, hist_index,wsIndex, local_workspace);
      }
    }
  }

  return local_workspace;
}

/**
 * Read the instrument group
 * @param mtd_entry The node for the current workspace
 * @param local_workspace The workspace to attach the instrument
 */
void LoadNexusProcessed::readInstrumentGroup(NXEntry & mtd_entry, DataObjects::Workspace2D_sptr local_workspace)
{
  //Instrument information
  NXInstrument inst = mtd_entry.openNXInstrument("instrument");
  std::string instname("");
  try
  {
    instname = inst.getString("name");
  }
  catch(std::runtime_error & )
  {
    return;
  }
  runLoadInstrument(instname, local_workspace);

  if ( ! inst.containsGroup("detector") )
  {
    g_log.information() << "Detector block not found. The workspace will not contain any detector information.\n";
    return;
  }

  //Populate the spectra-detector map
  NXDetector detgroup = inst.openNXDetector("detector");

  //Read necessary arrays from the file
  // Detector list contains a list of all of the detector numbers. If it not present then we can't update the spectra
  // map
  int ndets(-1);
  boost::shared_array<int> det_list(NULL);
  try
  {
    NXInt detlist_group = detgroup.openNXInt("detector_list");
    ndets = detlist_group.dim0();
    detlist_group.load();
    det_list.swap(detlist_group.sharedBuffer());
  }
  catch(std::runtime_error &)
  {
    g_log.information() << "detector_list block not found. The workspace will not contain any detector information."
        << std::endl;
    return;
  }

  //Detector count contains the number of detectors associated with each spectra
  NXInt det_count = detgroup.openNXInt("detector_count");
  det_count.load();
  //Detector index - contains the index of the detector in the workspace
  NXInt det_index = detgroup.openNXInt("detector_index");
  det_index.load();
  int nspectra = det_index.dim0();

  //Spectra block - Contains spectrum numbers for each workspace index
  // This might not exist so wrap and check. If it doesn't exist create a default mapping
  bool have_spectra(true);
  boost::shared_array<int> spectra(NULL);
  try
  {
    NXInt spectra_block = detgroup.openNXInt("spectra");
    spectra_block.load();
    spectra.swap(spectra_block.sharedBuffer());
  }
  catch(std::runtime_error &)
  {
    have_spectra = false;
  }

  //Now build the spectra list
  int *spectra_list = new int[ndets];
  API::Axis *axis1 = local_workspace->getAxis(1);

  int index=0;

  for(int i = 1; i <= nspectra; ++i)
  {
    if ((i >= m_spec_min && i < m_spec_max )||(m_list && find(m_spec_list.begin(), m_spec_list.end(),
        i) != m_spec_list.end()))
    {

      int spectrum(-1);
      if( have_spectra ) spectrum = spectra[i-1];
      else spectrum = i+1 ;

      if( m_axis1vals.empty() )
      {
        axis1->spectraNo(index) = spectrum;
      }
      else
      {
        axis1->setValue(index, m_axis1vals[i-1]);
      }

      int offset = det_index[i-1];
      int detcount = det_count[i-1];
      for(int j = 0; j < detcount; j++)
      {
        spectra_list[offset + j] = spectrum;
      }
      ++index;
    }
  }



  local_workspace->mutableSpectraMap().populate(spectra_list, det_list.get(), ndets);
  delete[] spectra_list;
}

/**
 * Read the instrument group
 * @param mtd_entry The node for the current workspace
 * @param local_workspace The workspace to attach the instrument
 */
void LoadNexusProcessed::readSampleGroup(NXEntry & mtd_entry, DataObjects::Workspace2D_sptr local_workspace)
{
  NXMainClass sample = mtd_entry.openNXClass<NXMainClass>("sample");
  try
  {
    local_workspace->mutableSample().setName(sample.getString("name"));
  }
  catch(std::runtime_error & )
  {
  }

  // Log data is stored as NXlog classes where a time series contains time & value attributes and a single value contains
  // only a value attribute
  const std::vector<NXClassInfo> & logs = sample.groups();
  std::vector<NXClassInfo>::const_iterator iend = logs.end();
  for( std::vector<NXClassInfo>::const_iterator itr = logs.begin(); itr != iend; ++itr )
  {
    std::string entryName = itr->nxname;
    if( addSampleProperty(sample, entryName, local_workspace->mutableSample()) )
    {
      continue;
    }
    if( itr->nxclass == "NXlog" )
    {
      NXLog log_entry = sample.openNXLog(itr->nxname);
      Kernel::Property *prop = log_entry.createProperty();
      if( prop )
      {
        local_workspace->mutableRun().addLogData(prop);
      }
      else
      {
        g_log.warning() << "Cannot load log property \"" << entryName << "\"\n";
      }
    }
  }
  // Now any datasets that are not grouped
  const std::vector<NXInfo> & attrs = sample.datasets();
  std::vector<NXInfo>::const_iterator aend = attrs.end();
  for( std::vector<NXInfo>::const_iterator itr = attrs.begin(); itr != aend; ++itr )
  {
    addSampleProperty(sample, itr->nxname, local_workspace->mutableSample());
  }

  // For the proton charge check that we haven't already loaded a log of the same name. If so
  // set the proton charge from the gd_proton_chrg entry
  if( local_workspace->run().hasProperty("proton_charge") )
  {
    try
    {
      Kernel::Property *charge_log = local_workspace->run().getProperty("gd_prtn_chrg");
      local_workspace->mutableRun().setProtonCharge(boost::lexical_cast<double>(charge_log->value()));
    }
    catch(boost::bad_lexical_cast & )
    {
      g_log.warning() << "Cannot make double from gd_prtn_chrg property\n";
    }
    catch(Kernel::Exception::NotFoundError & )
    {
      g_log.warning() << "Cannot access gd_prtn_chrg property.\n";
    }
  }
  else
  {
    try
    {
      local_workspace->mutableRun().setProtonCharge(sample.getDouble("proton_charge"));
    }
    catch(std::runtime_error & )
    {
      g_log.warning() << "Cannot access single-valued proton charge field.\n";
    }
  }

}

/** Add a property to the sample object
 * @param sample_entry The NX entry for the sample group
 * @param entryName The name of the entry
 * @param sampleDetails The sample object to add the property to
 * @returns True if the property has been handled, false otherwise
 */
bool LoadNexusProcessed::addSampleProperty(NXMainClass & sample_entry, const std::string & entryName, API::Sample& sampleDetails)
{
  // There are 4 special names:  geom_id, geom_thickness, geom_width and geom_height that are properties of the sample
  // and for historic reasons must be searched for first as Timeseries properties (as they were first written) and then
  // as simple attributes of the sample group
  if( entryName == "geom_id" || entryName == "geom_thickness" ||
      entryName == "geom_width" || entryName == "geom_height" )
  {
    //Try NXLog time series entry first
    Kernel::TimeSeriesProperty<double> *prop(NULL);
    try
    {
      NXLog logEntry = sample_entry.openNXLog(entryName);
      prop = dynamic_cast<Kernel::TimeSeriesProperty<double>*>(logEntry.createTimeSeries());
    }
    catch(std::runtime_error &)
    {
      prop = NULL;
    }

    if( prop )
    {
      //TODO: This is not likely the right way to do this; time "0" does not mean much.
      //  However this is the way it was coded, so I am leaving as is
      double value = prop->getSingleValue( boost::posix_time::from_time_t( static_cast<std::time_t>(0) ) );
      if( entryName == "geom_id" )
      {
        sampleDetails.setGeometryFlag(static_cast<int>(value));
      }
      else
      {
        if( entryName == "geom_thickness" ) sampleDetails.setThickness(value);
        else if( entryName == "geom_width" ) sampleDetails.setWidth(value);
        else if( entryName == "geom_height" ) sampleDetails.setHeight(value);
        else {}
      }
    }
    //Assume it's a single attribute value
    else
    {
      if( entryName == "geom_id" )
      {
        sampleDetails.setGeometryFlag(sample_entry.getInt(entryName));
      }
      else
      {
        double value = sample_entry.getDouble(entryName);
        if( entryName == "geom_thickness" ) sampleDetails.setThickness(value);
        else if( entryName == "geom_width" ) sampleDetails.setWidth(value);
        else if( entryName == "geom_height" ) sampleDetails.setHeight(value);
        else {}
      }
    }
    return true;
  }
  // Entry has not been handled
  return false;
}


/**
 * Binary predicate function object to sort the AlgorithmHistory vector by execution order
 * @param elem1 first element in the vector
 * @param elem2 second element in the vecor
 */
bool UDlesserExecCount(NXClassInfo elem1,NXClassInfo elem2)
{

  std::basic_string <char>::size_type index1, index2;
  static const std::basic_string <char>::size_type npos = -1;
  std::string num1,num2;
  //find the number after "_" in algorithm name ( eg:MantidAlogorthm_1)
  index1=elem1.nxname.find("_");
  if(index1!=npos)
  {
    num1=elem1.nxname.substr(index1+1,elem1.nxname.length()-index1);
  }
  index2=elem2.nxname.find("_");
  if(index2!=npos)
  {
    num2=elem2.nxname.substr(index2+1,elem2.nxname.length()-index2);
  }
  std::stringstream  is1,is2;
  is1<<num1;
  is2<<num2;

  int execNum1=-1;int execNum2=-1;
  is1>>execNum1;
  is2>>execNum2;

  if(execNum1<execNum2)
    return true;
  else
    return false;
}

/**
 * Read the algorithm history from the "mantid_workspace_i/process" group
 * @param mtd_entry The node for the current workspace
 * @param local_workspace The workspace to attach the history to
 *  @throw out_of_range an algorithm history entry doesn't have the excepted number of entries
 */
void LoadNexusProcessed::readAlgorithmHistory(NXEntry & mtd_entry, DataObjects::Workspace2D_sptr local_workspace)
{
  int exeCount=0;
  NXMainClass history = mtd_entry.openNXClass<NXMainClass>("process");
  //Group will contain a class for each algorithm, called MantidAlgorithm_i and then an
  //environment class
  //const std::vector<NXClassInfo> & classes = history.groups();
  std::vector<NXClassInfo>&  classes = history.groups();
  //sort by execution order - to execute the script generated by algorithmhistory in proper order
  sort(classes.begin(),classes.end(),UDlesserExecCount);
  std::vector<NXClassInfo>::const_iterator iend = classes.end();
  for( std::vector<NXClassInfo>::const_iterator itr = classes.begin(); itr != iend; ++itr )
  {
    if( itr->nxname.find("MantidAlgorithm") != std::string::npos )
    {
      NXNote entry(history,itr->nxname);
      entry.openLocal();
      const std::vector<std::string> & info = entry.data();
      const int nlines = info.size();
      if( nlines < 4 )
      {// ignore badly formed history entries
        continue;
      }

      std::string algName, dummy, temp;
      // get the name and version of the algorithm
      getWordsInString(info[NAME], dummy, algName, temp);

      //Chop of the v from the version string
      size_t numStart = temp.find('v');
      // this doesn't abort if the version string doesn't contain a v
      numStart = numStart != 1 ? 1 : 0;
      temp = std::string(temp.begin() + numStart, temp.end());
      const int version = boost::lexical_cast<int>(temp);

      //Get the execution date/time
      std::string date, time;
      getWordsInString(info[EXEC_TIME], dummy, dummy, date, time);
      Poco::DateTime start_timedate;
      //This is needed by the Poco parsing function
      int tzdiff(-1);
      if( !Poco::DateTimeParser::tryParse(Mantid::NeXus::g_processed_datetime, date + " " + time, start_timedate, tzdiff))
      {
        g_log.warning() << "Error parsing start time in algorithm history entry." << "\n";
        return;
      }
      //Get the duration
      getWordsInString(info[EXEC_DUR], dummy, dummy, temp, dummy);
      double dur = boost::lexical_cast<double>(temp);
      if ( dur < 0.0 )
      {
        g_log.warning() << "Error parsing start time in algorithm history entry." << "\n";
        return;
      }
      //API::AlgorithmHistory alg_hist(algname, version, start_timedate.timestamp().epochTime(), dur);

      ++exeCount;
      //Convert the timestamp to time_t to dateAndTime
      Mantid::Kernel::dateAndTime utc_start = Mantid::Kernel::DateAndTime::from_time_t( start_timedate.timestamp().epochTime() );
      //Create the algorithm history
      API::AlgorithmHistory alg_hist(algName, version, utc_start, dur,exeCount);

      //Add property information
      for( int index = static_cast<int>(PARAMS)+1;index < nlines;++index )
      {
        const std::string line = info[index];
        std::string::size_type colon = line.find(":");
        std::string::size_type comma = line.find(",");
        //Each colon has a space after it
        std::string prop_name = line.substr(colon + 2, comma - colon - 2);
        colon = line.find(":", comma);
        comma = line.find(",", colon);
        std::string prop_value = line.substr(colon + 2, comma - colon - 2);
        colon = line.find(":", comma);
        comma = line.find(",", colon);
        std::string is_def = line.substr(colon + 2, comma - colon - 2);
        colon = line.find(":", comma);
        comma = line.find(",", colon);
        std::string direction = line.substr(colon + 2, comma - colon - 2);
        unsigned int direc(Mantid::Kernel::Direction::asEnum(direction));
        alg_hist.addProperty(prop_name, prop_value, (is_def[0] == 'Y'), direc);
      }
      local_workspace->history().addAlgorithmHistory(alg_hist);
      entry.close();
    }
  }

}
/** If the first string contains exactly three words separated by spaces
 *  these words will be copied into each of the following strings that were passed
 *  @param[in] words3 a string with 3 words separated by spaces
 *  @param[out] w1 the first word in the input string
 *  @param[out] w2 the second word in the input string
 *  @param[out] w3 the third word in the input string
 *  @throw out_of_range if there aren't exaltly three strings in the word
 */
void LoadNexusProcessed::getWordsInString(const std::string & words3, std::string & w1, std::string & w2, std::string & w3 )
{
  Poco::StringTokenizer data(words3, " ", Poco::StringTokenizer::TOK_TRIM);
  if (data.count() != 3)
  {
    g_log.warning() << "Algorithm list line " + words3 + " is not of the correct format\n";
    throw std::out_of_range(words3);
  }

  w1 = data[0];
  w2 = data[1];
  w3 = data[2];
}

/** If the first string contains exactly four words separated by spaces
 *  these words will be copied into each of the following strings that were passed
 *  @param[in] words4 a string with 4 words separated by spaces
 *  @param[out] w1 the first word in the input string
 *  @param[out] w2 the second word in the input string
 *  @param[out] w3 the third word in the input string
 *  @param[out] w4 the fourth word in the input string
 *  @throw out_of_range if there aren't exaltly four strings in the word
 */
void LoadNexusProcessed::getWordsInString(const std::string & words4, std::string & w1, std::string & w2, std::string & w3, std::string & w4)
{
  Poco::StringTokenizer data(words4, " ", Poco::StringTokenizer::TOK_TRIM);
  if (data.count() != 4)
  {
    g_log.warning() << "Algorithm list line " + words4 + " is not of the correct format\n";
    throw std::out_of_range(words4);
  }

  w1 = data[0];
  w2 = data[1];
  w3 = data[2];
  w4 = data[3];
}

/**
 * Read the parameter map from the mantid_workspace_i/instrument_parameter_map group.
 * @param mtd_entry The entry object that points to the root node
 * @param local_workspace The workspace to read into
 */
void LoadNexusProcessed::readParameterMap(NXEntry & mtd_entry,
    DataObjects::Workspace2D_sptr local_workspace)
{
  NXNote pmap_node = mtd_entry.openNXNote("instrument_parameter_map");
  if( pmap_node.data().empty() ) return;

  const std::string & details =  pmap_node.data().front();
  Geometry::ParameterMap& pmap = local_workspace->instrumentParameters();
  IInstrument_sptr instr = local_workspace->getBaseInstrument();

  int options = Poco::StringTokenizer::TOK_IGNORE_EMPTY;
  options += Poco::StringTokenizer::TOK_TRIM;
  Poco::StringTokenizer splitter(details, "|", options);

  Poco::StringTokenizer::Iterator iend = splitter.end();
  //std::string prev_name;
  for( Poco::StringTokenizer::Iterator itr = splitter.begin(); itr != iend; ++itr )
  {
    Poco::StringTokenizer tokens(*itr, ";");
    if( tokens.count() != 4 ) continue;
    std::string comp_name = tokens[0];
    //if( comp_name == prev_name ) continue; this blocks reading in different parameters of the same component. RNT
    //prev_name = comp_name;
    Geometry::IComponent* comp = 0;
    if (comp_name.find("detID:") != std::string::npos)
    {
      int detID = atoi(comp_name.substr(6).c_str());
      comp = instr->getDetector(detID).get();
      if (!comp)
      {
        g_log.warning()<<"Cannot find detector "<<detID<<'\n';
        continue;
      }
    }
    else
    {
      comp = instr->getComponentByName(comp_name).get();
      if (!comp)
      {
        g_log.warning()<<"Cannot find component "<<comp_name<<'\n';
        continue;
      }
    }
    if( !comp ) continue;
    pmap.add(tokens[1], comp, tokens[2], tokens[3]);
  }
}

/**
 * Read the bin masking information from the mantid_workspace_i/workspace group.
 * @param wksp_cls The data group
 * @param local_workspace The workspace to read into
 */
void LoadNexusProcessed::readBinMasking(NXData & wksp_cls, DataObjects::Workspace2D_sptr local_workspace)
{
  if (wksp_cls.getDataSetInfo("masked_spectra").stat == NX_ERROR)
  {
    return;
  }
  NXInt spec = wksp_cls.openNXInt("masked_spectra");
  spec.load();
  NXInt bins = wksp_cls.openNXInt("masked_bins");
  bins.load();
  NXDouble weights = wksp_cls.openNXDouble("mask_weights");
  weights.load();
  const int n = spec.dim0();
  const int n1 = n - 1;
  for(int i = 0; i < n; ++i)
  {
    int si = spec(i,0);
    int j0 = spec(i,1);
    int j1 = i < n1 ? spec(i+1,1) : bins.dim0();
    for(int j = j0; j < j1; ++j)
    {
      local_workspace->maskBin(si,bins[j],weights[j]);
    }
  }
}


/** Run the sub-algorithm LoadInstrument (as for LoadRaw)
 * @param inst_name The name written in the Nexus file
 * @param localWorkspace The workspace to insert the instrument into
 */
void LoadNexusProcessed::runLoadInstrument(const std::string & inst_name,
    DataObjects::Workspace2D_sptr localWorkspace)
{
  // Determine the search directory for XML instrument definition files (IDFs)
  std::string directoryName = Kernel::ConfigService::Instance().getString("instrumentDefinition.directory");
  if (directoryName.empty())
  {
    // This is the assumed deployment directory for IDFs, where we need to be relative to the
    // directory of the executable, not the current working directory.
    directoryName = Poco::Path(Mantid::Kernel::ConfigService::Instance().getBaseDir()).resolve(
        "../Instrument").toString();
  }

  // For Nexus Mantid processed, Instrument XML file name is read from nexus
  std::string instrumentID = inst_name;
  // force ID to upper case
  std::transform(instrumentID.begin(), instrumentID.end(), instrumentID.begin(), toupper);
  std::string fullPathIDF = directoryName + "/" + instrumentID + "_Definition.xml";

  IAlgorithm_sptr loadInst = createSubAlgorithm("LoadInstrument");

  // Now execute the sub-algorithm. Catch and log any error, but don't stop.
  try
  {
    loadInst->setPropertyValue("Filename", fullPathIDF);
    loadInst->setProperty<MatrixWorkspace_sptr> ("Workspace", localWorkspace);
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

}

/**
 * Perform a call to nxgetslab, via the NexusClasses wrapped methods for a given blocksize. This assumes that the
 * xbins have alread been cached
 * @param data The NXDataSet object of y values
 * @param errors The NXDataSet object of error values
 * @param blocksize The blocksize to use
 * @param nchannels The number of channels for the block
 * @param hist The workspace index to start reading into
 * @param local_workspace A pointer to the workspace
 */
void LoadNexusProcessed::loadBlock(NXDataSetTyped<double> & data, NXDataSetTyped<double> & errors,
    int blocksize, int nchannels, int &hist,
    DataObjects::Workspace2D_sptr local_workspace)
{
  data.load(blocksize,hist);
  errors.load(blocksize,hist);
  double *data_start = data();
  double *data_end = data_start + nchannels;
  double *err_start = errors();
  double *err_end = err_start + nchannels;
  int final(hist + blocksize);
  while( hist < final )
  {
    MantidVec& Y = local_workspace->dataY(hist);
    Y.assign(data_start, data_end);
    data_start += nchannels; data_end += nchannels;
    MantidVec& E = local_workspace->dataE(hist);
    E.assign(err_start, err_end);
    err_start += nchannels; err_end += nchannels;
    local_workspace->setX(hist, m_xbins);
    ++hist;
  }
}

/**
 * Perform a call to nxgetslab, via the NexusClasses wrapped methods for a given blocksize. This assumes that the
 * xbins have alread been cached
 * @param data The NXDataSet object of y values
 * @param errors The NXDataSet object of error values
 * @param blocksize The blocksize to use
 * @param nchannels The number of channels for the block
 * @param hist The workspace index to start reading into
 * @param wsIndex The workspace index to save data into
 * @param local_workspace A pointer to the workspace
 */

void LoadNexusProcessed::loadBlock(NXDataSetTyped<double> & data, NXDataSetTyped<double> & errors,
    int blocksize, int nchannels, int &hist,int& wsIndex,
    DataObjects::Workspace2D_sptr local_workspace)
{
  data.load(blocksize,hist);
  errors.load(blocksize,hist);
  double *data_start = data();
  double *data_end = data_start + nchannels;
  double *err_start = errors();
  double *err_end = err_start + nchannels;
  int final(hist + blocksize);
  while( hist < final )
  {
    MantidVec& Y = local_workspace->dataY(wsIndex);
    Y.assign(data_start, data_end);
    data_start += nchannels; data_end += nchannels;
    MantidVec& E = local_workspace->dataE(wsIndex);
    E.assign(err_start, err_end);
    err_start += nchannels; err_end += nchannels;
    local_workspace->setX(wsIndex, m_xbins);
    ++hist;
    ++wsIndex;

  }
}

/**
 * Perform a call to nxgetslab, via the NexusClasses wrapped methods for a given blocksize. The xbins are read along with
 * each call to the data/error loading
 * @param data The NXDataSet object of y values
 * @param errors The NXDataSet object of error values
 * @param xbins The xbin NXDataSet
 * @param blocksize The blocksize to use
 * @param nchannels The number of channels for the block
 * @param hist The workspace index to start reading into
 * @param wsIndex The workspace index to save data into
 * @param local_workspace A pointer to the workspace
 */
void LoadNexusProcessed::loadBlock(NXDataSetTyped<double> & data, NXDataSetTyped<double> & errors, NXDouble & xbins,
    int blocksize, int nchannels, int &hist, int& wsIndex,
    DataObjects::Workspace2D_sptr local_workspace)
{
  data.load(blocksize,hist);
  double *data_start = data();
  double *data_end = data_start + nchannels;
  errors.load(blocksize,hist);
  double *err_start = errors();
  double *err_end = err_start + nchannels;
  xbins.load(blocksize, hist);
  const int nxbins(nchannels + 1);
  double *xbin_start = xbins();
  double *xbin_end = xbin_start + nxbins;
  int final(hist + blocksize);
  while( hist < final )
  {
    MantidVec& Y = local_workspace->dataY(wsIndex);
    Y.assign(data_start, data_end);
    data_start += nchannels; data_end += nchannels;
    MantidVec& E = local_workspace->dataE(wsIndex);
    E.assign(err_start, err_end);
    err_start += nchannels; err_end += nchannels;
    MantidVec& X = local_workspace->dataX(wsIndex);
    X.assign(xbin_start, xbin_end);
    xbin_start += nxbins; xbin_end += nxbins;
    ++hist;
    ++wsIndex;
  }
}


/**
 *Validates the optional 'spectra to read' properties, if they have been set
 * @param numberofspectra number of spectrum
 */
void LoadNexusProcessed::checkOptionalProperties(const int numberofspectra )
{
  //read in the settings passed to the algorithm
  m_spec_list = getProperty("SpectrumList");
  m_spec_max = getProperty("SpectrumMax");
  m_spec_min = getProperty("SpectrumMin");
  //Are we using a list of spectra or all the spectra in a range?
  m_list = !m_spec_list.empty();
  m_interval = (m_spec_max != Mantid::EMPTY_INT()) || (m_spec_min != 1);
  if ( m_spec_max == Mantid::EMPTY_INT() ) m_spec_max = 1;

  // Check validity of spectra list property, if set
  if (m_list)
  {
    m_list = true;
    const int minlist = *min_element(m_spec_list.begin(), m_spec_list.end());
    const int maxlist = *max_element(m_spec_list.begin(), m_spec_list.end());
    if (maxlist > numberofspectra || minlist == 0)
    {
      g_log.error("Invalid list of spectra");
      throw std::invalid_argument("Inconsistent properties defined");
    }
  }

  // Check validity of spectra range, if set
  if (m_interval)
  {
    m_interval = true;
    m_spec_min = getProperty("SpectrumMin");
    if (m_spec_min != 1 && m_spec_max == 1)
    {
      m_spec_max = numberofspectra;
    }
    if (m_spec_max < m_spec_min || m_spec_max >numberofspectra)
    {
      g_log.error("Invalid Spectrum min/max properties");
      throw std::invalid_argument("Inconsistent properties defined");
    }
  }
}

/**
 * Calculate the size of a workspace
 * @param numberofspectra number of spectrums
 * @return the size of a workspace
 */
int LoadNexusProcessed::calculateWorkspacesize(const int numberofspectra)
{
  // Calculate the size of a workspace, given its number of spectra to read
  int total_specs;
  if( m_interval || m_list)
  {
    if (m_interval)
    {
      if (m_spec_min != 1 && m_spec_max == 1)
      {
        m_spec_max = numberofspectra;
      }
      total_specs = (m_spec_max-m_spec_min+1);
      m_spec_max += 1;
    }
    else
      total_specs = 0;

    if (m_list)
    {
      if (m_interval)
      {
        for(std::vector<int>::iterator it=m_spec_list.begin();it!=m_spec_list.end();)
          if (*it >= m_spec_min && *it <m_spec_max)
          {
            it = m_spec_list.erase(it);
          }
          else
            it++;

      }
      if (m_spec_list.size() == 0) m_list = false;
      total_specs += m_spec_list.size();
    }
  }
  else
  {
    total_specs = numberofspectra;
    m_spec_min = 1;
    m_spec_max = numberofspectra +1;
  }
  return total_specs;
}



} // namespace NeXus
} // namespace Mantid
