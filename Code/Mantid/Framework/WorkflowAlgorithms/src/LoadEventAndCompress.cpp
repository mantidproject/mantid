#include "MantidWorkflowAlgorithms/LoadEventAndCompress.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/VisibleWhenProperty.h"

namespace Mantid {
namespace WorkflowAlgorithms {

using std::string;
using namespace Kernel;
using namespace API;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(LoadEventAndCompress)

namespace {
#ifdef MPI_BUILD
    const bool USE_MPI = true;
#else
    const bool USE_MPI = false;
#endif
}

//----------------------------------------------------------------------------------------------
/** Constructor
 */
LoadEventAndCompress::LoadEventAndCompress() {}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
LoadEventAndCompress::~LoadEventAndCompress() {}

//----------------------------------------------------------------------------------------------

/// Algorithms name for identification. @see Algorithm::name
const string LoadEventAndCompress::name() const { return "LoadEventAndCompress"; }

/// Algorithm's version for identification. @see Algorithm::version
int LoadEventAndCompress::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const string LoadEventAndCompress::category() const {
  return "Workflow\\DataHandling";
}

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const string LoadEventAndCompress::summary() const {
  return "Load an event workspace by chunks and compress";
}

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void LoadEventAndCompress::init() {
//    auto algLoadEventNexus = FrameworkManager::Instance().createAlgorithm("LoadEventNexus");
    auto algLoadEventNexus = AlgorithmManager::Instance().createUnmanaged("LoadEventNexus");
//    algLoadEventNexus->init();

    algLoadEventNexus->initialize();

    auto prop = algLoadEventNexus->getPointerToProperty("Filename");
    declareProperty(prop);

//    std::vector<std::string> exts;
//    exts.push_back("_event.nxs");
//    exts.push_back(".nxs.h5");
//    exts.push_back(".nxs");
//    declareProperty(
//        new FileProperty("Filename", "", FileProperty::Load, exts),
//        "The name of the Event NeXus file to read, including its full or "
//        "relative path. "
//        "The file name is typically of the form INST_####_event.nxs (N.B. case "
//        "sensitive if running on Linux).");

    declareProperty(new WorkspaceProperty<Workspace>(
                              "OutputWorkspace", "", Direction::Output),
                          "The name of the output EventWorkspace or WorkspaceGroup in which to "
                          "load the EventNexus file.");

    declareProperty(new PropertyWithValue<double>("FilterByTofMin", EMPTY_DBL(),
                                                  Direction::Input),
                    "Optional: To exclude events that do not fall within a range "
                    "of times-of-flight. "
                    "This is the minimum accepted value in microseconds. Keep "
                    "blank to load all events.");

    declareProperty(new PropertyWithValue<double>("FilterByTofMax", EMPTY_DBL(),
                                                  Direction::Input),
                    "Optional: To exclude events that do not fall within a range "
                    "of times-of-flight. "
                    "This is the maximum accepted value in microseconds. Keep "
                    "blank to load all events.");

    declareProperty(new PropertyWithValue<double>("FilterByTimeStart",
                                                  EMPTY_DBL(), Direction::Input),
                    "Optional: To only include events after the provided start "
                    "time, in seconds (relative to the start of the run).");

    declareProperty(new PropertyWithValue<double>("FilterByTimeStop", EMPTY_DBL(),
                                                  Direction::Input),
                    "Optional: To only include events before the provided stop "
                    "time, in seconds (relative to the start of the run).");

    std::string grp1 = "Filter Events";
    setPropertyGroup("FilterByTofMin", grp1);
    setPropertyGroup("FilterByTofMax", grp1);
    setPropertyGroup("FilterByTimeStart", grp1);
    setPropertyGroup("FilterByTimeStop", grp1);

    declareProperty(
        new PropertyWithValue<string>("NXentryName", "", Direction::Input),
        "Optional: Name of the NXentry to load if it's not the default.");

    declareProperty(
        new PropertyWithValue<bool>("LoadMonitors", false, Direction::Input),
        "Load the monitors from the file (optional, default False).");

    declareProperty(
        new PropertyWithValue<bool>("MonitorsAsEvents", false, Direction::Input),
        "If present, load the monitors as events. '''WARNING:''' WILL "
        "SIGNIFICANTLY INCREASE MEMORY USAGE (optional, default False). ");

    declareProperty(new PropertyWithValue<double>("FilterMonByTofMin",
                                                  EMPTY_DBL(), Direction::Input),
                    "Optional: To exclude events from monitors that do not fall "
                    "within a range of times-of-flight. "
                    "This is the minimum accepted value in microseconds.");

    declareProperty(new PropertyWithValue<double>("FilterMonByTofMax",
                                                  EMPTY_DBL(), Direction::Input),
                    "Optional: To exclude events from monitors that do not fall "
                    "within a range of times-of-flight. "
                    "This is the maximum accepted value in microseconds.");

    declareProperty(new PropertyWithValue<double>("FilterMonByTimeStart",
                                                  EMPTY_DBL(), Direction::Input),
                    "Optional: To only include events from monitors after the "
                    "provided start time, in seconds (relative to the start of "
                    "the run).");

    declareProperty(new PropertyWithValue<double>("FilterMonByTimeStop",
                                                  EMPTY_DBL(), Direction::Input),
                    "Optional: To only include events from monitors before the "
                    "provided stop time, in seconds (relative to the start of "
                    "the run).");

    setPropertySettings(
        "MonitorsAsEvents",
        new VisibleWhenProperty("LoadMonitors", IS_EQUAL_TO, "1"));
    IPropertySettings *asEventsIsOn =
        new VisibleWhenProperty("MonitorsAsEvents", IS_EQUAL_TO, "1");
    setPropertySettings("FilterMonByTofMin", asEventsIsOn);
    setPropertySettings("FilterMonByTofMax", asEventsIsOn->clone());
    setPropertySettings("FilterMonByTimeStart", asEventsIsOn->clone());
    setPropertySettings("FilterMonByTimeStop", asEventsIsOn->clone());

    std::string grp4 = "Monitors";
    setPropertyGroup("LoadMonitors", grp4);
    setPropertyGroup("MonitorsAsEvents", grp4);
    setPropertyGroup("FilterMonByTofMin", grp4);
    setPropertyGroup("FilterMonByTofMax", grp4);
    setPropertyGroup("FilterMonByTimeStart", grp4);
    setPropertyGroup("FilterMonByTimeStop", grp4);

    auto mustBePositive = boost::make_shared<BoundedValidator<int>>();
    mustBePositive->setLower(1);
    declareProperty("SpectrumMin", (int32_t)EMPTY_INT(), mustBePositive,
                    "The number of the first spectrum to read.");
    declareProperty("SpectrumMax", (int32_t)EMPTY_INT(), mustBePositive,
                    "The number of the last spectrum to read.");
    declareProperty(new ArrayProperty<int32_t>("SpectrumList"),
                    "A comma-separated list of individual spectra to read.");

    declareProperty(
        new PropertyWithValue<bool>("MetaDataOnly", false, Direction::Input),
        "If true, only the meta data and sample logs will be loaded.");

    declareProperty(
        new PropertyWithValue<bool>("LoadLogs", true, Direction::Input),
        "Load the Sample/DAS logs from the file (default True).");
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void LoadEventAndCompress::exec() {
  // TODO Auto-generated execute stub
}

} // namespace WorkflowAlgorithms
} // namespace Mantid
