#include "MantidAlgorithms/FilterEvents.h"
#include "MantidKernel/System.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidAPI/FileProperty.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidDataObjects/SplittersWorkspace.h"

#include <sstream>

using namespace Mantid::Kernel;
using namespace Mantid::API;

namespace Mantid
{
namespace Algorithms
{

  DECLARE_ALGORITHM(FilterEvents)

  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  FilterEvents::FilterEvents()
  {
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  FilterEvents::~FilterEvents()
  {
  }
  
  void FilterEvents::initDocs()
  {

  }

  /*
   * Declare Inputs
   */
  void FilterEvents::init()
  {
    declareProperty(
      new API::WorkspaceProperty<DataObjects::EventWorkspace>("InputWorkspace","",Direction::Input),
      "An input event workspace" );

    declareProperty("OutputWorkspaceBaseName", "OutputWorkspace",
      "The base name to use for the output workspace" );

    declareProperty(new API::WorkspaceProperty<API::ITableWorkspace>("SplittersInformationWorkspace", "", Direction::Input, PropertyMode::Optional),
        "Optional output for the information of each splitter workspace index.");

    declareProperty(
        new API::WorkspaceProperty<DataObjects::SplittersWorkspace>("InputSplittersWorkspace", "", Direction::Input),
        "An input SpilltersWorskpace for filtering");

    this->declareProperty(new API::FileProperty("DetectorCalibrationFile", "", API::FileProperty::OptionalLoad, ".dat"),
        "Input pixel TOF calibration file in column data format");

    this->declareProperty("FilterByPulseTime", false,
        "Filter the event by its pulse time only for slow sample environment log.  This option can make execution of algorithm faster.  But it lowers precision.");

    return;
  }

  /*
   * Execution body
   */
  void FilterEvents::exec()
  {
    // 1. Get inputs
    mEventWorkspace = this->getProperty("InputWorkspace");
    mSplittersWorkspace = this->getProperty("InputSplittersWorkspace");
    std::string outputwsnamebase = this->getProperty("OutputWorkspaceBaseName");
    std::string detcalfilename = this->getProperty("DetectorCalibrationFile");
    mFilterByPulseTime = this->getProperty("FilterByPulseTime");

    mInformationWS = this->getProperty("SplittersInformationWorkspace");
    if (!mInformationWS)
    {
      mWithInfo = false;
    }
    else
    {
      mWithInfo = true;
    }

    // 2. Process inputs
    processSplittersWorkspace();
    createOutputWorkspaces(outputwsnamebase);
    importDetectorTOFCalibration(detcalfilename);

    // 3. Filter Events
    filterEventsBySplitters();

    return;
  }

  /*
   * Convert SplitterWorkspace object to TimeSplitterType (sorted vector)
   * and create a map for all workspace group number
   */
  void FilterEvents::processSplittersWorkspace()
  {
    // 1. Init data structure
    size_t numsplitters = mSplittersWorkspace->getNumberSplitters();
    mSplitters.reserve(numsplitters);

    // 2. Insert all splitters
    bool inorder = true;
    for (size_t i = 0; i < numsplitters; i ++)
    {
      mSplitters.push_back(mSplittersWorkspace->getSplitter(i));
      mWorkspaceGroups.insert(mSplitters.back().index());

      if (inorder && i > 0 && mSplitters[i] < mSplitters[i-1])
        inorder = false;
    }

    // 3. Order if not ordered and add workspace for events excluded
    if (!inorder)
    {
      std::sort(mSplitters.begin(), mSplitters.end());
    }
    mWorkspaceGroups.insert(-1);

    // 4. Add information
    if (mWithInfo)
    {
      if (mWorkspaceGroups.size() != mInformationWS->rowCount()+1)
      {
        g_log.warning() << "Input Splitters Workspace has different entries than input information workspaces. "
            << "  Information won't be written to output workspaces. " << std::endl;
        mWithInfo = false;
      }
    }

    return;
  }

  /*
   * Create a list of EventWorkspace for output
   */
  void FilterEvents::createOutputWorkspaces(std::string outputwsnamebase)
  {
    // Convert information workspace to map
    std::map<int, std::string> infomap;
    if (mWithInfo)
    {
      API::Column_const_sptr name = mInformationWS->getColumn("name");
      for (size_t ir = 0; ir < mInformationWS->rowCount(); ++ ir)
      {
        TableRowHelper row = mInformationWS->getRow(ir);
        int x = (*name)[ir];
      }
    }

    // Set up new workspaces
    std::set<int>::iterator groupit;
    for (groupit = mWorkspaceGroups.begin(); groupit != mWorkspaceGroups.end(); ++groupit)
    {
      // 1. Get workspace name
      int wsgroup = *groupit;
      std::stringstream wsname;
      wsname << outputwsnamebase << "_" << wsgroup;
      std::stringstream parname;
      parname << "OutputWorkspace_" << wsgroup;

      // 2. Generate one of the output workspaces & Copy geometry over. But we don't copy the data.
      DataObjects::EventWorkspace_sptr optws = boost::dynamic_pointer_cast<DataObjects::EventWorkspace>(
          API::WorkspaceFactory::Instance().create("EventWorkspace", mEventWorkspace->getNumberHistograms(), 2, 1));
      API::WorkspaceFactory::Instance().initializeFromParent(mEventWorkspace, optws, false);

      //    Add information
      if (mWithInfo)
      {
        std::string info;
        if (wsgroup < 0)
        {
          info = "Events that are filtered out. ";
        }
        else
        {
          // TODO More Code Here
          // mInformationWS->getR
          ;
        }
        optws->setComment(info);
        optws->setTitle(info);
      }

      // 3. Set to map
      mOutputWorkspaces.insert(std::make_pair(wsgroup, optws));

      // 4. Set to output workspace
      this->declareProperty(new API::WorkspaceProperty<DataObjects::EventWorkspace>(parname.str(), wsname.str(), Direction::Output), "Output");
      this->setProperty(parname.str(), optws);

      g_log.debug() << "DB9141  Output Workspace:  Group = " << wsgroup << "  Property Name = " << parname.str() <<
          " Workspace name = " << wsname.str() <<
          " with Number of events = " << optws->getNumberEvents() << std::endl;
    } // ENDFOR


    return;
  }

  /*
   * Import the detector calibration on TOF
   */
  void FilterEvents::importDetectorTOFCalibration(std::string detcalfilename)
  {
    detid_t indet;

    // 1. Check workspace
    if (!mEventWorkspace)
    {
      g_log.error() << "Required to import EventWorkspace before calling importCalibrationFile()" << std::endl;
      throw std::invalid_argument("Calling function in wrong order!");
    }

    // 2. Prepare output
    mCalibDetectorIDs.clear();
    mCalibOffsets.clear();
    size_t numhist = mEventWorkspace->getNumberHistograms();
    mCalibDetectorIDs.reserve(numhist);
    mCalibOffsets.reserve(numhist);

    // 3. Read file?
    bool readcalfile = true;
    if (detcalfilename.empty())
    {
      readcalfile = false;
    }

    if (readcalfile)
    {
      try{
        // a. Open file
        std::ifstream ifs;
        ifs.open(detcalfilename.c_str(), std::ios::in);

        double doffset;
        for (size_t i = 0; i < numhist; i ++)
        {
          // i. each pixel:  get detector ID from EventWorkspace
          const DataObjects::EventList events = mEventWorkspace->getEventList(i);
          std::set<detid_t> detids = events.getDetectorIDs();
          std::set<detid_t>::iterator detit;
          detid_t detid = 0;
          for (detit=detids.begin(); detit!=detids.end(); ++detit)
            detid = *detit;

          // ii. read file
          ifs >> indet >> doffset;

          // iii. store
          if (indet != detid){
            g_log.error() << "Calibration File Error!  Line " << i << " should read in pixel " << detid << "  but read in " << indet
                << "\nAbort to reading calibration file!"<< std::endl;
            readcalfile = false;
            break;
          }
          else if (doffset < 0 || doffset > 1.0)
          {
            g_log.error() << "Calibration File Error!  Line " << i << " have ratio offset outside (0,1) " << detid << "  but read in " << indet
                <<"\nAbort to reading calibration file!"<< std::endl;
            readcalfile = false;
            break;
          }
          else
          {
            mCalibDetectorIDs.push_back(detid);
            mCalibOffsets.push_back(doffset);
          }
        }
        ifs.close();
      }
      catch (std::ifstream::failure&)
      {
        g_log.error() << "Calibration File Error!  Open calibration/offset file " << detcalfilename << " error " << std::endl;
        mCalibDetectorIDs.clear();
        mCalibOffsets.clear();
        readcalfile = false;
      }
    } // If-readcalfile

    // 4. Use default/dummy offset calibration = 1.0
    if (!readcalfile)
    {
      g_log.notice() << "Using default detector offset/calibration" << std::endl;

      for (size_t i = 0; i < mEventWorkspace->getNumberHistograms(); i ++)
      {
        std::set<detid_t> detids = mEventWorkspace->getEventList(i).getDetectorIDs();
        std::set<detid_t>::iterator detit;
        detid_t detid = 0;
        for (detit=detids.begin(); detit!=detids.end(); ++detit)
          detid = *detit;

        mCalibDetectorIDs.push_back(detid);
        mCalibOffsets.push_back(1.0);
      }
    } // If NOT Read-calibration-file

    return;
  }

  /*
   * Main filtering method
   */
  void FilterEvents::filterEventsBySplitters()
  {
    size_t numberOfSpectra = mEventWorkspace->getNumberHistograms();
    std::map<int, DataObjects::EventWorkspace_sptr>::iterator wsiter;

    // 1. Loop over the histograms (detector spectra)
    // PARALLEL_FOR_NO_WSP_CHECK()
    for (int64_t i = 0; i < int64_t(numberOfSpectra); ++i)
    {
      // PARALLEL_START_INTERUPT_REGION

      // a) Get the output event lists (should be empty) to be a map
      std::map<int, DataObjects::EventList* > outputs;
      for (wsiter = mOutputWorkspaces.begin(); wsiter != mOutputWorkspaces.end(); ++ wsiter)
      {
        int index = wsiter->first;
        DataObjects::EventList* output_el = wsiter->second->getEventListPtr(i);
        outputs.insert(std::make_pair(index, output_el));
      }

      // b) and this is the input event list
      const DataObjects::EventList& input_el = mEventWorkspace->getEventList(i);

      // c) Perform the filtering (using the splitting function and just one output)
      input_el.splitByFullTime(mSplitters, outputs, mCalibOffsets[i]);

      // prog.report();
      // PARALLEL_END_INTERUPT_REGION
    }
    // PARALLEL_CHECK_INTERUPT_REGION

    // 2. Finish adding events and To split/filter the runs,
    for (wsiter = mOutputWorkspaces.begin(); wsiter != mOutputWorkspaces.end(); ++wsiter)
    {
      // 2a Done adding event
      wsiter->second->doneAddingEventLists();

      // 2b To split/filter the runs, make a vector with just the one output run
      // FIXME Complete split the "Run".
      std::vector< Run *> output_runs;
      output_runs.push_back( &wsiter->second->mutableRun() );
      mEventWorkspace->run().splitByTime(mSplitters, output_runs);
    }

    return;
  }
} // namespace Mantid
} // namespace Algorithms
