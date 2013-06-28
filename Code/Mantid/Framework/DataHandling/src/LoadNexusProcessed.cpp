/*WIKI*


The algorithm LoadNexusProcessed will read a Nexus data file created by [[SaveNexusProcessed]] and place the data
into the named workspace.
The file name can be an absolute or relative path and should have the extension
.nxs, .nx5 or .xml.
Warning - using XML format can be extremely slow for large data sets and generate very large files.
The optional parameters can be used to control which spectra are loaded into the workspace (not yet implemented).
If spectrum_min and spectrum_max are given, then only that range to data will be loaded.

A Mantid Nexus file may contain several workspace entries each labelled with an integer starting at 1.
By default the highest number workspace is read, earlier ones can be accessed by setting the EntryNumber.

If the saved data has a reference to an XML file defining instrument geometry this will be read.


===Time series data===
The log data in the Nexus file (NX_LOG sections) is loaded as TimeSeriesProperty data within the workspace.
Time is stored as seconds from the Unix epoch.
Only floating point logs are stored and loaded at present.

===Child algorithms used===

The Child Algorithms used by LoadMuonNexus are:
* LoadInstrument - this algorithm looks for an XML description of the instrument and if found reads it.


*WIKI*/
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/AlgorithmFactory.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/NumericAxis.h"
#include "MantidAPI/RegisterFileLoader.h"
#include "MantidAPI/TextAxis.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidDataHandling/LoadNexusProcessed.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/RebinnedOutput.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/DateAndTime.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidNexus/NexusClasses.h"
#include "MantidNexus/NexusFileIO.h"
#include <nexus/NeXusFile.hpp>
#include <boost/shared_ptr.hpp>
#include <cmath>
#include <Poco/DateTimeParser.h>
#include <Poco/Path.h>
#include <Poco/StringTokenizer.h>
#include "MantidDataObjects/PeaksWorkspace.h"

namespace Mantid
{
namespace DataHandling
{

// Register the algorithm into the algorithm factory
DECLARE_HDF_FILELOADER_ALGORITHM(LoadNexusProcessed);

/// Sets documentation strings for this algorithm
void LoadNexusProcessed::initDocs()
{
  this->setWikiSummary("The LoadNexusProcessed algorithm will read the given Nexus Processed data file containing a Mantid Workspace. The data is placed in the named workspace. LoadNexusProcessed may be invoked by [[LoadNexus]] if it is given a Nexus file of this type. ");
  this->setOptionalMessage("The LoadNexusProcessed algorithm will read the given Nexus Processed data file containing a Mantid Workspace. The data is placed in the named workspace. LoadNexusProcessed may be invoked by LoadNexus if it is given a Nexus file of this type.");
}

using namespace Mantid::NeXus;
using namespace DataObjects;
using namespace Kernel;
using namespace API;
using Geometry::Instrument_const_sptr;

/// Default constructor
LoadNexusProcessed::LoadNexusProcessed() : m_shared_bins(false), m_xbins(),
    m_axis1vals(), m_list(false), m_interval(false),
    m_spec_list(), m_spec_min(0), m_spec_max(Mantid::EMPTY_INT()),m_cppFile(NULL)
{
}

/// Delete NexusFileIO in destructor
LoadNexusProcessed::~LoadNexusProcessed()
{
  delete m_cppFile;
}

/**
 * Return the confidence with with this algorithm can load the file
 * @param descriptor A descriptor for the file
 * @returns An integer specifying the confidence level. 0 indicates it will not be used
 */
int LoadNexusProcessed::confidence(const Kernel::HDFDescriptor & descriptor) const
{
  if(descriptor.pathExists("/mantid_workspace_1")) return 80;
  else return 0;
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
      "The name of the Nexus file to read, as a full or relative path." );
  declareProperty(new WorkspaceProperty<Workspace> ("OutputWorkspace", "",
      Direction::Output),
                  "The name of the workspace to be created as the output of the algorithm.  A workspace of this name will be created and stored in the Analysis Data Service. For multiperiod files, one workspace may be generated for each period. Currently only one workspace can be saved at a time so multiperiod Mantid files are not generated.");



  // optional
  auto mustBePositive = boost::make_shared<BoundedValidator<int64_t> >();
  mustBePositive->setLower(0);

  declareProperty("SpectrumMin", (int64_t)1, mustBePositive,
                  "Number of first spectrum to read.");
  declareProperty("SpectrumMax", (int64_t)Mantid::EMPTY_INT(), mustBePositive,
                  "Number of last spectrum to read.");
  declareProperty(new ArrayProperty<int64_t> ("SpectrumList"),
                  "List of spectrum numbers to read.");
  declareProperty("EntryNumber", (int64_t)0, mustBePositive,
                  "The particular entry number to read. Default load all workspaces and creates a workspacegroup (default: read all entries)." );
}


//-------------------------------------------------------------------------------------------------
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

  // "Open" the same file but with the C++ interface
  m_cppFile = new ::NeXus::File(root.m_fileID);

  //Find out how many first level entries there are
  int64_t nperiods = static_cast<int64_t>(root.groups().size());

  // Check for an entry number property
  int64_t entrynumber = static_cast<int64_t>(getProperty("EntryNumber"));

  if( entrynumber > 0 && entrynumber > nperiods )
  {
    g_log.error() << "Invalid entry number specified. File only contains " << nperiods << " entries.\n";
    throw std::invalid_argument("Invalid entry number specified.");
  }

  const std::string basename = "mantid_workspace_";
  if( nperiods == 1 || entrynumber > 0 )
  {  // Load one first level entry, specified if there are several
    if( entrynumber == 0 ) ++entrynumber;
    std::ostringstream os;
    os << entrynumber;
    API::Workspace_sptr workspace = loadEntry(root, basename + os.str(), 0, 1);
    //API::Workspace_sptr workspace = boost::static_pointer_cast<API::Workspace>(local_workspace);
    setProperty("OutputWorkspace", workspace);
  }
  else
  {  // Load all first level entries
    WorkspaceGroup_sptr wksp_group(new WorkspaceGroup);
    //This forms the name of the group
    std::string base_name = getPropertyValue("OutputWorkspace");
    // First member of group should be the group itself, for some reason!
    base_name += "_";
    const std::string prop_name = "OutputWorkspace_";
    double nperiods_d = static_cast<double>(nperiods);
    for( int64_t p = 1; p <= nperiods; ++p )
    {
      std::ostringstream os;
      os << p;
      Workspace_sptr local_workspace = loadEntry(root, basename + os.str(), static_cast<double>(p-1)/nperiods_d, 1./nperiods_d);
      declareProperty(new WorkspaceProperty<API::Workspace>(prop_name + os.str(), base_name + os.str(),
          Direction::Output));
      //wksp_group->add(base_name + os.str());
      wksp_group->addWorkspace(local_workspace);
      setProperty(prop_name + os.str(), local_workspace);
    }

    // The group is the root property value
    setProperty("OutputWorkspace", boost::static_pointer_cast<Workspace>(wksp_group));

  }

  m_axis1vals.clear();
}



//-------------------------------------------------------------------------------------------------
/** Load the event_workspace field
 *
 * @param wksp_cls
 * @param xbins
 * @param progressStart
 * @param progressRange
 * @return
 */
API::MatrixWorkspace_sptr LoadNexusProcessed::loadEventEntry(NXData & wksp_cls, NXDouble & xbins,
    const double& progressStart, const double& progressRange)
{
  NXDataSetTyped<int64_t> indices_data = wksp_cls.openNXDataSet<int64_t>("indices");
  indices_data.load();
  boost::shared_array<int64_t> indices = indices_data.sharedBuffer();
  int numspec = indices_data.dim0()-1;

  int num_xbins = xbins.dim0();
  if (num_xbins < 2) num_xbins = 2;
  EventWorkspace_sptr ws = boost::dynamic_pointer_cast<EventWorkspace>
  (WorkspaceFactory::Instance().create("EventWorkspace", numspec, num_xbins, num_xbins-1));

  // Set the YUnit label
  ws->setYUnit(indices_data.attributes("units"));
  std::string unitLabel = indices_data.attributes("unit_label");
  if (unitLabel.empty()) unitLabel = indices_data.attributes("units");
  ws->setYUnitLabel(unitLabel);

  //Handle optional fields.
  // TODO: Handle inconsistent sizes
  boost::shared_array<int64_t> pulsetimes;
  if (wksp_cls.isValid("pulsetime"))
  {
    NXDataSetTyped<int64_t> pulsetime = wksp_cls.openNXDataSet<int64_t>("pulsetime");
    pulsetime.load();
    pulsetimes = pulsetime.sharedBuffer();
  }

  boost::shared_array<double> tofs;
  if (wksp_cls.isValid("tof"))
  {
    NXDouble tof = wksp_cls.openNXDouble("tof");
    tof.load();
    tofs = tof.sharedBuffer();
  }

  boost::shared_array<float> error_squareds;
  if (wksp_cls.isValid("error_squared"))
  {
    NXFloat error_squared = wksp_cls.openNXFloat("error_squared");
    error_squared.load();
    error_squareds = error_squared.sharedBuffer();
  }

  boost::shared_array<float> weights;
  if (wksp_cls.isValid("weight"))
  {
    NXFloat weight = wksp_cls.openNXFloat("weight");
    weight.load();
    weights = weight.sharedBuffer();
  }

  // What type of event lists?
  EventType type = TOF;
  if (tofs && pulsetimes && weights && error_squareds)
    type = WEIGHTED;
  else if ((tofs && weights && error_squareds))
    type = WEIGHTED_NOTIME;
  else if (pulsetimes && tofs)
    type = TOF;
  else
    throw std::runtime_error("Could not figure out the type of event list!");

  // Create all the event lists
  PARALLEL_FOR_NO_WSP_CHECK()
  for (int wi=0; wi < numspec; wi++)
  {
    PARALLEL_START_INTERUPT_REGION
    int64_t index_start = indices[wi];
    int64_t index_end = indices[wi+1];
    if (index_end >= index_start)
    {
      EventList & el = ws->getEventList(wi);
      el.switchTo(type);

      // Allocate all the required memory
      el.reserve(index_end - index_start);
      el.clearDetectorIDs();

      for (int64_t i=index_start; i<index_end; i++)
      switch (type)
      {
      case TOF:
        el.addEventQuickly( TofEvent( tofs[i], DateAndTime(pulsetimes[i])) );
        break;
      case WEIGHTED:
        el.addEventQuickly( WeightedEvent( tofs[i], DateAndTime(pulsetimes[i]), weights[i], error_squareds[i]) );
        break;
      case WEIGHTED_NOTIME:
        el.addEventQuickly( WeightedEventNoTime( tofs[i], weights[i], error_squareds[i]) );
        break;
      }

      // Set the X axis
      if (this->m_shared_bins)
        el.setX(this->m_xbins);
      else
      {
        MantidVec x;
        x.resize(xbins.dim0());
        for (int i=0; i < xbins.dim0(); i++)
          x[i] = xbins(wi, i);
        el.setX(x);
      }
    }

    progress(progressStart + progressRange*(1.0/numspec));
    PARALLEL_END_INTERUPT_REGION
  }
  PARALLEL_CHECK_INTERUPT_REGION


  return ws;
}


//-------------------------------------------------------------------------------------------------
/**
 * Load a table
 */
API::Workspace_sptr LoadNexusProcessed::loadTableEntry(NXEntry & entry)
{
  API::ITableWorkspace_sptr workspace;
  workspace = Mantid::API::WorkspaceFactory::Instance().createTable("TableWorkspace");

  NXData nx_tw = entry.openNXData("table_workspace");

  bool hasNumberOfRowBeenSet = false;
  //int numberOfRows = 0;

  int columnNumber = 1;
  do
  {
    std::string str = "column_" + boost::lexical_cast<std::string>(columnNumber);

    NXInfo info = nx_tw.getDataSetInfo(str.c_str());
    if (info.stat == NX_ERROR)
    {
	  // Assume we done last column of table
      break;
    }

    if ( info.type == NX_FLOAT64 )
    {
      NXDouble nxDouble = nx_tw.openNXDouble(str.c_str());
      std::string columnTitle = nxDouble.attributes("name");
      if (!columnTitle.empty())
      {
        workspace->addColumn("double", columnTitle);
        nxDouble.load();
        int length = nxDouble.dim0();
        if ( !hasNumberOfRowBeenSet )
        { 
          workspace->setRowCount(length);
          hasNumberOfRowBeenSet = true;
        }
        for (int i = 0; i < length; i++)
          workspace->cell<double>(i,columnNumber-1) = *(nxDouble() + i);
      }
    }
	else if ( info.type == NX_INT32 )
    {
      NXInt nxInt = nx_tw.openNXInt(str.c_str());
      std::string columnTitle = nxInt.attributes("name");
      if (!columnTitle.empty())
      {
        workspace->addColumn("int", columnTitle);
        nxInt.load();
        int length = nxInt.dim0();
        if ( !hasNumberOfRowBeenSet )
        { 
          workspace->setRowCount(length);
          hasNumberOfRowBeenSet = true;
        }
        for (int i = 0; i < length; i++)
          workspace->cell<int>(i,columnNumber-1) = *(nxInt() + i);
      }
    }
    else if ( info.type == NX_CHAR )
    {
      NXChar data = nx_tw.openNXChar(str.c_str());
      std::string columnTitle = data.attributes("name");
      if (!columnTitle.empty())
      {
        workspace->addColumn("str", columnTitle);
        int nRows = info.dims[0];
        if ( !hasNumberOfRowBeenSet )
        {
          workspace->setRowCount(nRows);
          hasNumberOfRowBeenSet = true;
        }
        int maxStr = info.dims[1];

        std::string fromCrap(maxStr,' ');

        data.load();
        for (int iR = 0; iR < nRows; iR++)
        {
          for (int i = 0; i < maxStr; i++)
            fromCrap[i] = *(data()+i+maxStr*iR);
          workspace->cell<std::string>(iR,columnNumber-1) = fromCrap;
        }
      }
    } 

    columnNumber++;
  
  } while ( 1 );

  return boost::static_pointer_cast<API::Workspace>(workspace);
}

//-------------------------------------------------------------------------------------------------
/**
 * Load peaks
 */
API::Workspace_sptr LoadNexusProcessed::loadPeaksEntry(NXEntry & entry)
{   
	//API::IPeaksWorkspace_sptr workspace;
	API::ITableWorkspace_sptr tWorkspace;
	//PeaksWorkspace_sptr workspace;
  tWorkspace = Mantid::API::WorkspaceFactory::Instance().createTable("PeaksWorkspace");

  PeaksWorkspace_sptr peakWS = boost::dynamic_pointer_cast<PeaksWorkspace>(tWorkspace);

  NXData nx_tw = entry.openNXData("peaks_workspace");


  int columnNumber = 1;
  int numberPeaks = 0;
  std::vector<std::string> columnNames;
  do
  {
    std::string str = "column_" + boost::lexical_cast<std::string>(columnNumber);

    NXInfo info = nx_tw.getDataSetInfo(str.c_str());
    if (info.stat == NX_ERROR)
    {
	  // Assume we done last column of table
      break;
    }

	// store column names
	columnNames.push_back(str);


	// determine number of peaks
	// here we assume that a peaks_table has always one column of doubles
	
    if ( info.type == NX_FLOAT64 )
    {
      NXDouble nxDouble = nx_tw.openNXDouble(str.c_str());
      std::string columnTitle = nxDouble.attributes("name");
      if (!columnTitle.empty() && numberPeaks==0)
      {
        numberPeaks = nxDouble.dim0();
      }
    }

	columnNumber++;

  } while ( 1 );


  //Get information from all but data group
  std::string parameterStr;
  // Hop to the right point
  m_cppFile->openPath(entry.path());
  try
  {
    // This loads logs, sample, and instrument.
    peakWS->loadExperimentInfoNexus(m_cppFile, parameterStr);
  }
  catch (std::exception & e)
  {
    g_log.information("Error loading Instrument section of nxs file");
    g_log.information(e.what());
  }


  // std::vector<API::IPeak*> p;
  for (int r = 0; r < numberPeaks; r++)
  {   
	  Kernel::V3D v3d;
	  v3d[2] = 1.0;
	  API::IPeak* p;
	  p = peakWS->createPeak(v3d);
	  peakWS->addPeak(*p);
  }



  for (size_t i = 0; i < columnNames.size(); i++)
  {
    const std::string str = columnNames[i];
    if ( !str.compare("column_1") )
    {
      NXInt nxInt = nx_tw.openNXInt(str.c_str());
      nxInt.load();

      for (int r = 0; r < numberPeaks; r++) {
        int ival = nxInt[r];
        if( ival != -1) peakWS->getPeak(r).setDetectorID( ival );
      }
    }

    if ( !str.compare("column_2") )
    {
      NXDouble nxDouble = nx_tw.openNXDouble(str.c_str());
      nxDouble.load();

      for (int r = 0; r < numberPeaks; r++) {
        double val = nxDouble[r];
        peakWS->getPeak(r).setH( val );
      }
    }

    if ( !str.compare("column_3") )
    {
      NXDouble nxDouble = nx_tw.openNXDouble(str.c_str());
      nxDouble.load();

      for (int r = 0; r < numberPeaks; r++) {
        double val = nxDouble[r];
        peakWS->getPeak(r).setK( val );
      }
    }

    if ( !str.compare("column_4") )
    {
      NXDouble nxDouble = nx_tw.openNXDouble(str.c_str());
      nxDouble.load();

      for (int r = 0; r < numberPeaks; r++) {
        double val = nxDouble[r];
        peakWS->getPeak(r).setL( val );
      }
    }

    if ( !str.compare("column_5") )
    {
      NXDouble nxDouble = nx_tw.openNXDouble(str.c_str());
      nxDouble.load();

      for (int r = 0; r < numberPeaks; r++) {
        double val = nxDouble[r];
        peakWS->getPeak(r).setIntensity( val );
      }
    }

    if ( !str.compare("column_6") )
    {
      NXDouble nxDouble = nx_tw.openNXDouble(str.c_str());
      nxDouble.load();

      for (int r = 0; r < numberPeaks; r++) {
        double val = nxDouble[r];
        peakWS->getPeak(r).setSigmaIntensity( val );
      }
    }



    if ( !str.compare("column_7") )
    {
      NXDouble nxDouble = nx_tw.openNXDouble(str.c_str());
      nxDouble.load();

      for (int r = 0; r < numberPeaks; r++) {
        double val = nxDouble[r];
        peakWS->getPeak(r).setBinCount( val );
      }
    }


    if ( !str.compare("column_10") )
    {
      NXDouble nxDouble = nx_tw.openNXDouble(str.c_str());
      nxDouble.load();

      for (int r = 0; r < numberPeaks; r++) {
        double val = nxDouble[r];
        peakWS->getPeak(r).setWavelength( val );
      }
    }

    if ( !str.compare("column_14") )
    {
      NXInt nxInt = nx_tw.openNXInt(str.c_str());
      nxInt.load();

      for (int r = 0; r < numberPeaks; r++) {
        int ival = nxInt[r];
        if( ival != -1) peakWS->getPeak(r).setRunNumber( ival );
      }
    }

  }

  return boost::static_pointer_cast<API::Workspace>(peakWS);
}

//-------------------------------------------------------------------------------------------------
/**
 * Load a single entry into a workspace
 * @param root :: The opened root node
 * @param entry_name :: The entry name
 * @param progressStart :: The percentage value to start the progress reporting for this entry
 * @param progressRange :: The percentage range that the progress reporting should cover
 * @returns A 2D workspace containing the loaded data
 */
API::Workspace_sptr LoadNexusProcessed::loadEntry(NXRoot & root, const std::string & entry_name,
    const double& progressStart, const double& progressRange)
{
  progress(progressStart,"Opening entry " + entry_name + "...");

  NXEntry mtd_entry = root.openEntry(entry_name);

  if (mtd_entry.containsGroup("table_workspace"))
  {
    return loadTableEntry(mtd_entry);
  } 

  if (mtd_entry.containsGroup("peaks_workspace"))
  {
    return loadPeaksEntry(mtd_entry);
  }  


  // Determine workspace type and name of group containing workspace characteristics
  bool isEvent = false;
  std::string workspaceType = "Workspace2D";
  std::string group_name = "workspace";
  if (mtd_entry.containsGroup("event_workspace"))
  {
    isEvent = true;
    group_name = "event_workspace";
  }
  else if (mtd_entry.containsGroup("offsets_workspace"))
  {
    workspaceType = "OffsetsWorkspace";
    group_name = "offsets_workspace";
  }

  // Get workspace characteristics
  NXData wksp_cls = mtd_entry.openNXData(group_name);

  // Axis information
  // "X" axis
  NXDouble xbins = wksp_cls.openNXDouble("axis1");
  xbins.load();
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

  // The workspace being worked on
  API::MatrixWorkspace_sptr local_workspace;
  size_t nspectra;
  int64_t nchannels;

  // -------- Process as event ? --------------------
  if (isEvent)
  {
    local_workspace = loadEventEntry(wksp_cls, xbins, progressStart, progressRange);
    nspectra = local_workspace->getNumberHistograms();
    nchannels = local_workspace->blocksize();
  }
  else
  {
    NXDataSetTyped<double> data = wksp_cls.openDoubleData();
    nspectra = data.dim0();
    nchannels = data.dim1();
    //// validate the optional spectrum parameters, if set
    checkOptionalProperties(nspectra);
    // Actual number of spectra in output workspace (if only a range was going to be loaded)
    size_t total_specs=calculateWorkspacesize(nspectra);

    //// Create the 2D workspace for the output
    bool hasFracArea = false;
    if (wksp_cls.isValid("frac_area"))
    {
      // frac_area entry is the signal for a RebinnedOutput workspace
      hasFracArea = true;
      workspaceType.clear();
      workspaceType = "RebinnedOutput";
    }
    local_workspace = boost::dynamic_pointer_cast<API::MatrixWorkspace>
    (WorkspaceFactory::Instance().create(workspaceType, total_specs, xlength, nchannels));
    try
    {
      local_workspace->setTitle(mtd_entry.getString("title"));
    }
    catch (std::runtime_error&)
    {
      g_log.debug() << "No title was found in the input file, " << getPropertyValue("Filename") << std::endl;
    }

    // Set the YUnit label
    local_workspace->setYUnit(data.attributes("units"));
    std::string unitLabel = data.attributes("unit_label");
    if (unitLabel.empty()) unitLabel = data.attributes("units");
    local_workspace->setYUnitLabel(unitLabel);
    
    readBinMasking(wksp_cls, local_workspace);
    NXDataSetTyped<double> errors = wksp_cls.openNXDouble("errors");
    NXDataSetTyped<double> fracarea = wksp_cls.openNXDouble("errors");
    if (hasFracArea)
    {
      fracarea = wksp_cls.openNXDouble("frac_area");
    }

    int64_t blocksize(8);
    //const int fullblocks = nspectra / blocksize;
    //size of the workspace
    int64_t fullblocks = total_specs / blocksize;
    int64_t read_stop = (fullblocks * blocksize);
    const double progressBegin = progressStart+0.25*progressRange;
    const double progressScaler = 0.75*progressRange;
    int64_t hist_index = 0;
    int64_t wsIndex=0;
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
          int interval_specs=static_cast<int>(m_spec_max-m_spec_min);
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
            progress(progressBegin+progressScaler*static_cast<double>(hist_index)/static_cast<double>(read_stop),"Reading workspace data...");
            loadBlock(data, errors, fracarea, hasFracArea, blocksize, nchannels, hist_index,wsIndex, local_workspace);
          }
          int64_t finalblock = m_spec_max-1 - read_stop;
          if( finalblock > 0 )
          {
            loadBlock(data, errors, fracarea, hasFracArea, finalblock, nchannels, hist_index,wsIndex,local_workspace);
          }
        }
        // if spectrum list property is set read each spectrum separately by setting blocksize=1
        if(m_list)
        {
          std::vector<int64_t>::iterator itr=m_spec_list.begin();
          for(;itr!=m_spec_list.end();++itr)
          {
            int64_t specIndex=(*itr)-1;
            progress(progressBegin+progressScaler*static_cast<double>(specIndex)/static_cast<double>(m_spec_list.size()),"Reading workspace data...");
            loadBlock(data, errors, fracarea, hasFracArea, static_cast<int64_t>(1), nchannels, specIndex,wsIndex, local_workspace);
          }

        }
      }
      else
      {
        for( ; hist_index < read_stop; )
        {
          progress(progressBegin+progressScaler*static_cast<double>(hist_index)/static_cast<double>(read_stop),"Reading workspace data...");
          loadBlock(data, errors, fracarea, hasFracArea, blocksize, nchannels, hist_index,wsIndex, local_workspace);
        }
        int64_t finalblock = total_specs - read_stop;
        if( finalblock > 0 )
        {
          loadBlock(data, errors, fracarea, hasFracArea, finalblock, nchannels, hist_index,wsIndex,local_workspace);
        }
      }

    }
    else
    {
      if(m_interval||m_list)
      {
        if(m_interval)
        {
          int64_t interval_specs=m_spec_max-m_spec_min;
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
            progress(progressBegin+progressScaler*static_cast<double>(hist_index)/static_cast<double>(read_stop),"Reading workspace data...");
            loadBlock(data, errors, fracarea, hasFracArea, xbins, blocksize, nchannels, hist_index,wsIndex,local_workspace);
          }
          int64_t finalblock = m_spec_max-1 - read_stop;
          if( finalblock > 0 )
          {
            loadBlock(data, errors, fracarea, hasFracArea, xbins, finalblock, nchannels, hist_index,wsIndex, local_workspace);
          }
        }
        //
        if(m_list)
        {
          std::vector<int64_t>::iterator itr=m_spec_list.begin();
          for(;itr!=m_spec_list.end();++itr)
          {
            int64_t specIndex=(*itr)-1;
            progress(progressBegin+progressScaler*static_cast<double>(specIndex)/static_cast<double>(read_stop),"Reading workspace data...");
            loadBlock(data, errors, fracarea, hasFracArea, xbins, 1, nchannels, specIndex,wsIndex,local_workspace);
          }

        }
      }
      else
      {
        for( ; hist_index < read_stop; )
        {
          progress(progressBegin+progressScaler*static_cast<double>(hist_index)/static_cast<double>(read_stop),"Reading workspace data...");
          loadBlock(data, errors, fracarea, hasFracArea, xbins, blocksize, nchannels, hist_index,wsIndex,local_workspace);
        }
        int64_t finalblock = total_specs - read_stop;
        if( finalblock > 0 )
        {
          loadBlock(data, errors, fracarea, hasFracArea, xbins, finalblock, nchannels, hist_index,wsIndex, local_workspace);
        }
      }
    }
  } //end of NOT an event -------------------------------



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

  // Setting a unit onto a TextAxis makes no sense.
  if ( unit2 == "TextAxis" )
  {
    Mantid::API::TextAxis* newAxis = new Mantid::API::TextAxis(nspectra);
    local_workspace->replaceAxis(1, newAxis);
  }
  else if ( unit2 != "spectraNumber" )
  {
    try
    {
      Mantid::API::NumericAxis* newAxis = new Mantid::API::NumericAxis(nspectra);
      local_workspace->replaceAxis(1, newAxis);
      newAxis->unit() = UnitFactory::Instance().create(unit2);
    }
    catch( std::runtime_error & )
    {
      g_log.information() << "Axis 1 set to unitless quantity \"" << unit2 << "\"\n";
    }
  }


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
  std::string parameterStr;

  progress(progressStart+0.05*progressRange,"Reading the sample details...");

  // Hop to the right point
  m_cppFile->openPath(mtd_entry.path());
  try
  {
    // This loads logs, sample, and instrument.
    local_workspace->loadExperimentInfoNexus(m_cppFile, parameterStr);
  }
  catch (std::exception & e)
  {
    g_log.information("Error loading Instrument section of nxs file");
    g_log.information(e.what());
  }

  // Now assign the spectra-detector map
  readInstrumentGroup(mtd_entry, local_workspace);

  // Parameter map parsing
  progress(progressStart+0.11*progressRange,"Reading the parameter maps...");
  local_workspace->readParameterMap(parameterStr);


  if ( ! local_workspace->getAxis(1)->isSpectra() )
  { // If not a spectra axis, load the axis data into the workspace. (MW 25/11/10)
    loadNonSpectraAxis(local_workspace, wksp_cls);
  }

  progress(progressStart+0.15*progressRange,"Reading the workspace history...");
  m_cppFile->openPath(mtd_entry.path());
  try
  {
    local_workspace->history().loadNexus(m_cppFile);
  }
  catch (std::out_of_range&)
  {
    g_log.warning() << "Error in the workspaces algorithm list, its processing history is incomplete\n";
  }

  progress(progressStart+0.2*progressRange,"Reading the workspace history...");

  return boost::static_pointer_cast<API::Workspace>(local_workspace);
}



//-------------------------------------------------------------------------------------------------
/**
 * Read the instrument group
 * @param mtd_entry :: The node for the current workspace
 * @param local_workspace :: The workspace to attach the instrument
 */
void LoadNexusProcessed::readInstrumentGroup(NXEntry & mtd_entry, API::MatrixWorkspace_sptr local_workspace)
{
  //Instrument information
  NXInstrument inst = mtd_entry.openNXInstrument("instrument");
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
  boost::shared_array<int> det_list(new int);
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
  boost::shared_array<int> spectra(new int);
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
  int index=0;

  for(int i = 1; i <= nspectra; ++i)
  { 
      int spectrum(-1);
      if( have_spectra ) spectrum = spectra[i-1];
      else spectrum = i+1 ;

      if ((i >= m_spec_min && i < m_spec_max )||(m_list && find(m_spec_list.begin(), m_spec_list.end(),
        i) != m_spec_list.end()))
      {
        ISpectrum * spec = local_workspace->getSpectrum(index);
        if( m_axis1vals.empty() )
        {
          spec->setSpectrumNo(spectrum);
        }
        else
        {
          spec->setSpectrumNo(static_cast<specid_t>(m_axis1vals[i-1]));
        }
        ++index;

        int start = det_index[i-1];
        int end = start + det_count[i-1];
        assert( end <= ndets );
        spec->setDetectorIDs(std::set<detid_t>(det_list.get()+start,det_list.get()+end));
      }
  }
}



//-------------------------------------------------------------------------------------------------
/**
* Loads the information contained in non-Spectra (ie, Text or Numeric) axis in the Nexus
* file into the workspace.
* @param local_workspace :: pointer to workspace object
* @param data :: reference to the NeXuS data for the axis
*/
void LoadNexusProcessed::loadNonSpectraAxis(API::MatrixWorkspace_sptr local_workspace, NXData & data)
{
  Mantid::API::Axis* axis = local_workspace->getAxis(1);

  if ( axis->isNumeric() )
  {
    NXDouble axisData = data.openNXDouble("axis2");
    axisData.load();
    for ( int i = 0; i < static_cast<int>(axis->length()); i++ )
    {
      axis->setValue(i, axisData[i]);
    }
  }
  else if ( axis->isText() )
  {
    // We must cast the axis object to TextAxis so we may use ->setLabel
    Mantid::API::TextAxis* textAxis = dynamic_cast<Mantid::API::TextAxis*>(axis);
    NXChar axisData = data.openNXChar("axis2");
    axisData.load();
    std::string axisLabels = axisData();    
    // Use boost::tokenizer to split up the input
    boost::char_separator<char> sep("\n");
    boost::tokenizer<boost::char_separator<char> > tokenizer(axisLabels, sep);
    boost::tokenizer<boost::char_separator<char> >::iterator tokIter;
    int i = 0;
    for ( tokIter = tokenizer.begin(); tokIter != tokenizer.end(); ++tokIter )
    {
      textAxis->setLabel(i, *tokIter);
      ++i;
    }
  }
}




/**
 * Binary predicate function object to sort the AlgorithmHistory vector by execution order
 * @param elem1 :: first element in the vector
 * @param elem2 :: second element in the vecor
 */
bool UDlesserExecCount(NXClassInfo elem1,NXClassInfo elem2)
{
  std::string::size_type index1, index2;
  std::string num1,num2;
  //find the number after "_" in algorithm name ( eg:MantidAlogorthm_1)
  index1=elem1.nxname.find("_");
  if ( index1 != std::string::npos )
  {
    num1=elem1.nxname.substr(index1+1,elem1.nxname.length()-index1);
  }
  index2=elem2.nxname.find("_");
  if ( index2 != std::string::npos )
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

//
////-------------------------------------------------------------------------------------------------
///**
// * Read the algorithm history from the "mantid_workspace_i/process" group
// * @param mtd_entry :: The node for the current workspace
// * @param local_workspace :: The workspace to attach the history to
// *  @throw out_of_range an algorithm history entry doesn't have the excepted number of entries
// */
//void LoadNexusProcessed::readAlgorithmHistory(NXEntry & mtd_entry, API::MatrixWorkspace_sptr local_workspace)
//{
//
//  NXMainClass history = mtd_entry.openNXClass<NXMainClass>("process");
//  //Group will contain a class for each algorithm, called MantidAlgorithm_i and then an
//  //environment class
//  //const std::vector<NXClassInfo> & classes = history.groups();
//  std::vector<NXClassInfo>&  classes = history.groups();
//  //sort by execution order - to execute the script generated by algorithmhistory in proper order
//  sort(classes.begin(),classes.end(),UDlesserExecCount);
//  std::vector<NXClassInfo>::const_iterator iend = classes.end();
//  for( std::vector<NXClassInfo>::const_iterator itr = classes.begin(); itr != iend; ++itr )
//  {
//    if( itr->nxname.find("MantidAlgorithm") != std::string::npos )
//    {
//      NXNote entry(history,itr->nxname);
//      entry.openLocal();
//      const std::vector<std::string> & info = entry.data();
//      const size_t nlines = info.size();
//      if( nlines < 4 )
//      {// ignore badly formed history entries
//        continue;
//      }
//
//      std::string algName, dummy, temp;
//      // get the name and version of the algorithm
//      getWordsInString(info[NAME], dummy, algName, temp);
//
//      //Chop of the v from the version string
//      size_t numStart = temp.find('v');
//      // this doesn't abort if the version string doesn't contain a v
//      numStart = numStart != 1 ? 1 : 0;
//      temp = std::string(temp.begin() + numStart, temp.end());
//      const int version = boost::lexical_cast<int>(temp);
//
//      //Get the execution date/time
//      std::string date, time;
//      getWordsInString(info[EXEC_TIME], dummy, dummy, date, time);
//      Poco::DateTime start_timedate;
//      //This is needed by the Poco parsing function
//      int tzdiff(-1);
//      if( !Poco::DateTimeParser::tryParse(Mantid::NeXus::g_processed_datetime, date + " " + time, start_timedate, tzdiff))
//      {
//        g_log.warning() << "Error parsing start time in algorithm history entry." << "\n";
//        return;
//      }
//      //Get the duration
//      getWordsInString(info[EXEC_DUR], dummy, dummy, temp, dummy);
//      double dur = boost::lexical_cast<double>(temp);
//      if ( dur < 0.0 )
//      {
//        g_log.warning() << "Error parsing start time in algorithm history entry." << "\n";
//        return;
//      }
//      //Convert the timestamp to time_t to DateAndTime
//      Mantid::Kernel::DateAndTime utc_start;
//      utc_start.set_from_time_t( start_timedate.timestamp().epochTime() );
//      //Create the algorithm history
//      API::AlgorithmHistory alg_hist(algName, version, utc_start, dur,Algorithm::g_execCount);
//      // Simulate running an algorithm
//      ++Algorithm::g_execCount;
//
//      //Add property information
//      for( size_t index = static_cast<size_t>(PARAMS)+1;index < nlines;++index )
//      {
//        const std::string line = info[index];
//        std::string::size_type colon = line.find(":");
//        std::string::size_type comma = line.find(",");
//        //Each colon has a space after it
//        std::string prop_name = line.substr(colon + 2, comma - colon - 2);
//        colon = line.find(":", comma);
//        comma = line.find(", Default?", colon);
//        std::string prop_value = line.substr(colon + 2, comma - colon - 2);
//        colon = line.find(":", comma);
//        comma = line.find(", Direction", colon);
//        std::string is_def = line.substr(colon + 2, comma - colon - 2);
//        colon = line.find(":", comma);
//        comma = line.find(",", colon);
//        std::string direction = line.substr(colon + 2, comma - colon - 2);
//        unsigned int direc(Mantid::Kernel::Direction::asEnum(direction));
//        alg_hist.addProperty(prop_name, prop_value, (is_def[0] == 'Y'), direc);
//      }
//      local_workspace->history().addHistory(alg_hist);
//      entry.close();
//    }
//  }
//
//}


//-------------------------------------------------------------------------------------------------
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



//-------------------------------------------------------------------------------------------------
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



//-------------------------------------------------------------------------------------------------
/**
 * Read the bin masking information from the mantid_workspace_i/workspace group.
 * @param wksp_cls :: The data group
 * @param local_workspace :: The workspace to read into
 */
void LoadNexusProcessed::readBinMasking(NXData & wksp_cls, API::MatrixWorkspace_sptr local_workspace)
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
      local_workspace->flagMasked(si,bins[j],weights[j]);
    }
  }
}






/**
 * Perform a call to nxgetslab, via the NexusClasses wrapped methods for a given blocksize. This assumes that the
 * xbins have alread been cached
 * @param data :: The NXDataSet object of y values
 * @param errors :: The NXDataSet object of error values
 * @param farea :: The NXDataSet object of fraction area values
 * @param hasFArea :: Flag to signal a RebinnedOutput workspace is in use
 * @param blocksize :: The blocksize to use
 * @param nchannels :: The number of channels for the block
 * @param hist :: The workspace index to start reading into
 * @param local_workspace :: A pointer to the workspace
 */
void LoadNexusProcessed::loadBlock(NXDataSetTyped<double> & data,
                                   NXDataSetTyped<double> & errors,
                                   NXDataSetTyped<double> & farea,
                                   bool hasFArea,
                                   int64_t blocksize,
                                   int64_t nchannels, int64_t &hist,
                                   API::MatrixWorkspace_sptr local_workspace)
{
  data.load(static_cast<int>(blocksize),static_cast<int>(hist));
  errors.load(static_cast<int>(blocksize),static_cast<int>(hist));
  double *data_start = data();
  double *data_end = data_start + nchannels;
  double *err_start = errors();
  double *err_end = err_start + nchannels;
  double *farea_start = NULL;
  double *farea_end = NULL;
  RebinnedOutput_sptr rb_workspace;
  if (hasFArea)
  {
    farea.load(static_cast<int>(blocksize),static_cast<int>(hist));
    farea_start = farea();
    farea_end = farea_start + nchannels;
    rb_workspace = boost::dynamic_pointer_cast<RebinnedOutput>(local_workspace);
  }
  int64_t final(hist + blocksize);
  while( hist < final )
  {
    MantidVec& Y = local_workspace->dataY(hist);
    Y.assign(data_start, data_end);
    data_start += nchannels; data_end += nchannels;
    MantidVec& E = local_workspace->dataE(hist);
    E.assign(err_start, err_end);
    err_start += nchannels; err_end += nchannels;
    if (hasFArea)
    {
      MantidVec& F = rb_workspace->dataF(hist);
      F.assign(farea_start, farea_end);
      farea_start += nchannels;
      farea_end += nchannels;
    }
    local_workspace->setX(hist, m_xbins);
    ++hist;
  }
}

/**
 * Perform a call to nxgetslab, via the NexusClasses wrapped methods for a given blocksize. This assumes that the
 * xbins have alread been cached
 * @param data :: The NXDataSet object of y values
 * @param errors :: The NXDataSet object of error values
 * @param farea :: The NXDataSet object of fraction area values
 * @param hasFArea :: Flag to signal a RebinnedOutput workspace is in use
 * @param blocksize :: The blocksize to use
 * @param nchannels :: The number of channels for the block
 * @param hist :: The workspace index to start reading into
 * @param wsIndex :: The workspace index to save data into
 * @param local_workspace :: A pointer to the workspace
 */

void LoadNexusProcessed::loadBlock(NXDataSetTyped<double> & data,
                                   NXDataSetTyped<double> & errors,
                                   NXDataSetTyped<double> & farea,
                                   bool hasFArea,
                                   int64_t blocksize, int64_t nchannels,
                                   int64_t &hist,int64_t& wsIndex,
                                   API::MatrixWorkspace_sptr local_workspace)
{
  data.load(static_cast<int>(blocksize),static_cast<int>(hist));
  errors.load(static_cast<int>(blocksize),static_cast<int>(hist));
  double *data_start = data();
  double *data_end = data_start + nchannels;
  double *err_start = errors();
  double *err_end = err_start + nchannels;
  double *farea_start = NULL;
  double *farea_end = NULL;
  RebinnedOutput_sptr rb_workspace;
  if (hasFArea)
  {
    farea.load(static_cast<int>(blocksize),static_cast<int>(hist));
    farea_start = farea();
    farea_end = farea_start + nchannels;
    rb_workspace = boost::dynamic_pointer_cast<RebinnedOutput>(local_workspace);
  }
  int64_t final(hist + blocksize);
  while( hist < final )
  {
    MantidVec& Y = local_workspace->dataY(wsIndex);
    Y.assign(data_start, data_end);
    data_start += nchannels; data_end += nchannels;
    MantidVec& E = local_workspace->dataE(wsIndex);
    E.assign(err_start, err_end);
    err_start += nchannels; err_end += nchannels;
    if (hasFArea)
    {
      MantidVec& F = rb_workspace->dataF(wsIndex);
      F.assign(farea_start, farea_end);
      farea_start += nchannels;
      farea_end += nchannels;
    }
    local_workspace->setX(wsIndex, m_xbins);
    ++hist;
    ++wsIndex;

  }
}

/**
 * Perform a call to nxgetslab, via the NexusClasses wrapped methods for a given blocksize. The xbins are read along with
 * each call to the data/error loading
 * @param data :: The NXDataSet object of y values
 * @param errors :: The NXDataSet object of error values
 * @param farea :: The NXDataSet object of fraction area values
 * @param hasFArea :: Flag to signal a RebinnedOutput workspace is in use
 * @param xbins :: The xbin NXDataSet
 * @param blocksize :: The blocksize to use
 * @param nchannels :: The number of channels for the block
 * @param hist :: The workspace index to start reading into
 * @param wsIndex :: The workspace index to save data into
 * @param local_workspace :: A pointer to the workspace
 */
void LoadNexusProcessed::loadBlock(NXDataSetTyped<double> & data,
                                   NXDataSetTyped<double> & errors,
                                   NXDataSetTyped<double> & farea,
                                   bool hasFArea,
                                   NXDouble & xbins,
                                   int64_t blocksize, int64_t nchannels,
                                   int64_t &hist, int64_t& wsIndex,
                                   API::MatrixWorkspace_sptr local_workspace)
{
  data.load(static_cast<int>(blocksize),static_cast<int>(hist));
  double *data_start = data();
  double *data_end = data_start + nchannels;
  errors.load(static_cast<int>(blocksize),static_cast<int>(hist));
  double *err_start = errors();
  double *err_end = err_start + nchannels;
  double *farea_start = NULL;
  double *farea_end = NULL;
  RebinnedOutput_sptr rb_workspace;
  if (hasFArea)
  {
    farea.load(static_cast<int>(blocksize),static_cast<int>(hist));
    farea_start = farea();
    farea_end = farea_start + nchannels;
    rb_workspace = boost::dynamic_pointer_cast<RebinnedOutput>(local_workspace);
  }
  xbins.load(static_cast<int>(blocksize),static_cast<int>(hist));
  const int64_t nxbins(nchannels + 1);
  double *xbin_start = xbins();
  double *xbin_end = xbin_start + nxbins;
  int64_t final(hist + blocksize);
  while( hist < final )
  {
    MantidVec& Y = local_workspace->dataY(wsIndex);
    Y.assign(data_start, data_end);
    data_start += nchannels; data_end += nchannels;
    MantidVec& E = local_workspace->dataE(wsIndex);
    E.assign(err_start, err_end);
    err_start += nchannels; err_end += nchannels;
    if (hasFArea)
    {
      MantidVec& F = rb_workspace->dataF(wsIndex);
      F.assign(farea_start, farea_end);
      farea_start += nchannels;
      farea_end += nchannels;
    }
    MantidVec& X = local_workspace->dataX(wsIndex);
    X.assign(xbin_start, xbin_end);
    xbin_start += nxbins; xbin_end += nxbins;
    ++hist;
    ++wsIndex;
  }
}


/**
 *Validates the optional 'spectra to read' properties, if they have been set
 * @param numberofspectra :: number of spectrum
 */
void LoadNexusProcessed::checkOptionalProperties(const std::size_t numberofspectra )
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
    const int64_t minlist = *min_element(m_spec_list.begin(), m_spec_list.end());
    const int64_t maxlist = *max_element(m_spec_list.begin(), m_spec_list.end());
    if (maxlist > static_cast<int>(numberofspectra) || minlist == 0)
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
    if (m_spec_max < m_spec_min || m_spec_max > static_cast<int>(numberofspectra))
    {
      g_log.error("Invalid Spectrum min/max properties");
      throw std::invalid_argument("Inconsistent properties defined");
    }
  }
}

/**
 * Calculate the size of a workspace
 * @param numberofspectra :: number of spectrums
 * @return the size of a workspace
 */
size_t LoadNexusProcessed::calculateWorkspacesize(const std::size_t numberofspectra)
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
      total_specs = static_cast<int>(m_spec_max-m_spec_min+1);
      m_spec_max += 1;
    }
    else
      total_specs = 0;

    if (m_list)
    {
      if (m_interval)
      {
        for(std::vector<int64_t>::iterator it=m_spec_list.begin();it!=m_spec_list.end();)
          if (*it >= m_spec_min && *it <m_spec_max)
          {
            it = m_spec_list.erase(it);
          }
          else
            ++it;

      }
      if (m_spec_list.size() == 0) m_list = false;
      total_specs += static_cast<int>(m_spec_list.size());
    }
  }
  else
  {
    total_specs = static_cast<int>(numberofspectra);
    m_spec_min = 1;
    m_spec_max = static_cast<int>(numberofspectra) +1;
  }
  return total_specs;
}

} // namespace DataHandling
} // namespace Mantid
