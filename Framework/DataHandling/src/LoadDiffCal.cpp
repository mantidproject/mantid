#include "MantidDataHandling/LoadDiffCal.h"

#include "MantidAPI/FileProperty.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Progress.h"
#include "MantidAPI/TableRow.h"
#include "MantidDataHandling/LoadCalFile.h"
#include "MantidDataObjects/GroupingWorkspace.h"
#include "MantidDataObjects/MaskWorkspace.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"

#include <cmath>
#include <H5Cpp.h>

namespace Mantid {
namespace DataHandling {

using Mantid::API::FileProperty;
using Mantid::API::MatrixWorkspace;
using Mantid::API::MatrixWorkspace_sptr;
using Mantid::API::Progress;
using Mantid::API::ITableWorkspace;
using Mantid::API::ITableWorkspace_sptr;
using Mantid::API::WorkspaceProperty;
using Mantid::DataObjects::GroupingWorkspace_sptr;
using Mantid::DataObjects::MaskWorkspace_sptr;
using Mantid::DataObjects::Workspace2D;
using Mantid::Kernel::Direction;
using Mantid::Kernel::PropertyWithValue;

using namespace H5;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(LoadDiffCal)

//----------------------------------------------------------------------------------------------
/** Constructor
 */
LoadDiffCal::LoadDiffCal() {}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
LoadDiffCal::~LoadDiffCal() {}

//----------------------------------------------------------------------------------------------

/// Algorithms name for identification. @see Algorithm::name
const std::string LoadDiffCal::name() const { return "LoadDiffCal"; }

/// Algorithm's version for identification. @see Algorithm::version
int LoadDiffCal::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string LoadDiffCal::category() const {
  return "DataHandling\\Instrument;Diffraction\\DataHandling";
}

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string LoadDiffCal::summary() const {
  return "Loads a calibration file for powder diffraction";
}

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void LoadDiffCal::init() {
  // 3 properties for getting the right instrument
  LoadCalFile::getInstrument3WaysInit(this);

  declareProperty(new FileProperty("Filename", "", FileProperty::Load,
                                   {".h5", ".hd5", ".hdf", ".cal"}),
                  "Path to the .h5 file.");

  declareProperty(new PropertyWithValue<bool>("MakeGroupingWorkspace", true,
                                              Direction::Input),
                  "Set to true to create a GroupingWorkspace with called "
                  "WorkspaceName_group.");

  declareProperty(
      new PropertyWithValue<bool>("MakeCalWorkspace", true, Direction::Input),
      "Set to true to create a CalibrationWorkspace with called "
      "WorkspaceName_cal.");

  declareProperty(
      new PropertyWithValue<bool>("MakeMaskWorkspace", true, Direction::Input),
      "Set to true to create a MaskWorkspace with called WorkspaceName_mask.");

  declareProperty(
      new PropertyWithValue<std::string>("WorkspaceName", "", Direction::Input),
      "The base of the output workspace names. Names will have '_group', "
      "'_cal', '_mask' appended to them.");

  std::string grpName("Calibration Validation");
  declareProperty("TofMin", 0., "Minimum for TOF axis. Defaults to 0.");
  declareProperty("TofMax", EMPTY_DBL(),
                  "Maximum for TOF axis. Defaults to Unused.");
  setPropertyGroup("TofMin", grpName);
  setPropertyGroup("TofMax", grpName);
}

namespace { // anonymous

std::string readString(H5File &file, const std::string &path) {
  try {
    DataSet data = file.openDataSet(path);
    std::string value;
    data.read(value, data.getDataType(), data.getSpace());
    return value;
  } catch (H5::FileIException &e) {
    UNUSED_ARG(e);
    return "";
  } catch (H5::GroupIException &e) {
    UNUSED_ARG(e);
    return "";
  }
}

template <typename NumT>
std::vector<NumT> readArrayCoerce(DataSet &dataset,
                                  const DataType &desiredDataType) {
  std::vector<NumT> result;
  DataType dataType = dataset.getDataType();
  DataSpace dataSpace = dataset.getSpace();

  if (desiredDataType == dataType) {
    result.resize(dataSpace.getSelectNpoints());
    dataset.read(&result[0], dataType, dataSpace);
  } else if (PredType::NATIVE_UINT32 == dataType) {
    std::vector<uint32_t> temp(dataSpace.getSelectNpoints());
    dataset.read(&temp[0], dataType, dataSpace);
    result.assign(temp.begin(), temp.end());
  } else if (PredType::NATIVE_FLOAT == dataType) {
    std::vector<float> temp(dataSpace.getSelectNpoints());
    dataset.read(&temp[0], dataType, dataSpace);
    for (auto it = temp.begin(); it != temp.end(); ++it)
      result.push_back(static_cast<NumT>(*it));
  } else {
    throw DataTypeIException();
  }

  return result;
}

bool endswith(const std::string &str, const std::string &ending) {
  if (ending.size() > str.size()) {
    return false;
  }

  return std::equal(str.begin() + str.size() - ending.size(), str.end(),
                    ending.begin());
}

void setGroupWSProperty(API::Algorithm *alg, const std::string &prefix,
                        GroupingWorkspace_sptr wksp) {
  alg->declareProperty(
      new WorkspaceProperty<DataObjects::GroupingWorkspace>(
          "OutputGroupingWorkspace", prefix + "_group", Direction::Output),
      "Set the the output GroupingWorkspace, if any.");
  alg->setProperty("OutputGroupingWorkspace", wksp);
}

void setMaskWSProperty(API::Algorithm *alg, const std::string &prefix,
                       MaskWorkspace_sptr wksp) {
  alg->declareProperty(
      new WorkspaceProperty<DataObjects::MaskWorkspace>(
          "OutputMaskWorkspace", prefix + "_mask", Direction::Output),
      "Set the the output MaskWorkspace, if any.");
  alg->setProperty("OutputMaskWorkspace", wksp);
}

void setCalWSProperty(API::Algorithm *alg, const std::string &prefix,
                      ITableWorkspace_sptr wksp) {
  alg->declareProperty(
      new WorkspaceProperty<ITableWorkspace>(
          "OutputCalWorkspace", prefix + "_cal", Direction::Output),
      "Set the output Diffraction Calibration workspace, if any.");
  alg->setProperty("OutputCalWorkspace", wksp);
}

} // anonymous namespace

std::vector<double> LoadDiffCal::readDoubleArray(Group &group,
                                                 const std::string &name) {
  std::vector<double> result;

  try {
    DataSet dataset = group.openDataSet(name);
    result = readArrayCoerce<double>(dataset, PredType::NATIVE_DOUBLE);
  } catch (H5::GroupIException &e) {
    UNUSED_ARG(e);
    g_log.information() << "Failed to open dataset \"" << name << "\"\n";
  } catch (H5::DataTypeIException &e) {
    UNUSED_ARG(e);
    g_log.information() << "DataSet \"" << name << "\" should be double"
                        << "\n";
  }

  for (size_t i = 0; i < result.size(); ++i) {
    if (std::abs(result[i]) < 1.e-10) {
      result[i] = 0.;
    } else if (result[i] != result[i]) { // check for NaN
      result[i] = 0.;
    }
  }

  return result;
}

std::vector<int32_t> LoadDiffCal::readInt32Array(Group &group,
                                                 const std::string &name) {
  std::vector<int32_t> result;

  try {
    DataSet dataset = group.openDataSet(name);
    result = readArrayCoerce<int32_t>(dataset, PredType::NATIVE_INT32);
  } catch (H5::GroupIException &e) {
    UNUSED_ARG(e);
    g_log.information() << "Failed to open dataset \"" << name << "\"\n";
  } catch (H5::DataTypeIException &e) {
    UNUSED_ARG(e);
    g_log.information() << "DataSet \"" << name << "\" should be int32"
                        << "\n";
  }

  return result;
}

void LoadDiffCal::getInstrument(H5File &file) {
  // don't bother if there isn't a mask or grouping requested
  bool makeMask = getProperty("MakeMaskWorkspace");
  bool makeGrouping = getProperty("MakeGroupingWorkspace");
  if ((!makeMask) & (!makeGrouping))
    return;

  // see if the user specified the instrument independently
  if (LoadCalFile::instrumentIsSpecified(this)) {
    m_instrument = LoadCalFile::getInstrument3Ways(this);
    return;
  }

  std::string idf =
      readString(file, "/calibration/instrument/instrument_source");
  std::string instrumentName = readString(file, "/calibration/instrument/name");

  g_log.debug() << "IDF : " << idf << "\n"
                << "NAME: " << instrumentName << "\n";

  API::Algorithm_sptr childAlg =
      this->createChildAlgorithm("LoadInstrument", 0.0, 0.1);
  MatrixWorkspace_sptr tempWS(new Workspace2D());
  childAlg->setProperty<MatrixWorkspace_sptr>("Workspace", tempWS);
  if (idf.empty()) {
    childAlg->setPropertyValue("InstrumentName", instrumentName);
  } else {
    childAlg->setPropertyValue("Filename", idf);
  }
  childAlg->setProperty("RewriteSpectraMap",
                        Mantid::Kernel::OptionalBool(false));
  childAlg->executeAsChildAlg();

  m_instrument = tempWS->getInstrument();

  g_log.information() << "Loaded instrument \"" << m_instrument->getName()
                      << "\" from \"" << m_instrument->getFilename() << "\"\n";
}

void LoadDiffCal::makeGroupingWorkspace(const std::vector<int32_t> &detids,
                                        const std::vector<int32_t> &groups) {
  bool makeWS = getProperty("MakeGroupingWorkspace");
  if (!makeWS) {
    g_log.information("Not making a GroupingWorkspace");
    return;
  }

  size_t numDet = detids.size();
  Progress progress(this, .4, .6, numDet);

  GroupingWorkspace_sptr wksp =
      boost::make_shared<DataObjects::GroupingWorkspace>(m_instrument);
  wksp->setTitle(m_filename);
  wksp->mutableRun().addProperty("Filename", m_filename);

  for (size_t i = 0; i < numDet; ++i) {
    detid_t detid = static_cast<detid_t>(detids[i]);
    wksp->setValue(detid, groups[i]);
    progress.report();
  }

  setGroupWSProperty(this, m_workspaceName, wksp);
}

void LoadDiffCal::makeMaskWorkspace(const std::vector<int32_t> &detids,
                                    const std::vector<int32_t> &use) {
  bool makeWS = getProperty("MakeMaskWorkspace");
  if (!makeWS) {
    g_log.information("Not making a MaskWorkspace");
    return;
  }

  size_t numDet = detids.size();
  Progress progress(this, .6, .8, numDet);

  MaskWorkspace_sptr wksp =
      boost::make_shared<DataObjects::MaskWorkspace>(m_instrument);
  wksp->setTitle(m_filename);
  wksp->mutableRun().addProperty("Filename", m_filename);

  for (size_t i = 0; i < numDet; ++i) {
    bool shouldUse = (use[i] > 0);
    detid_t detid = static_cast<detid_t>(detids[i]);
    // in maskworkspace 0=use, 1=dontuse
    wksp->setMasked(detid, !shouldUse);
    wksp->setValue(detid, (shouldUse ? 0. : 1.));
    progress.report();
  }

  setMaskWSProperty(this, m_workspaceName, wksp);
}

namespace { // anonymous namespace

double calcTofMin(const double difc, const double difa, const double tzero,
                  const double tofmin) {
  if (difa == 0.) {
    if (tzero != 0.) {
      // check for negative d-spacing
      return std::max<double>(-1. * tzero, tofmin);
    }
  } else if (difa > 0) {
    // check for imaginary part in quadratic equation
    return std::max<double>(tzero - .25 * difc * difc / difa, tofmin);
  }

  // everything else is fine so just return supplied tofmin
  return tofmin;
}

double calcTofMax(const double difc, const double difa, const double tzero,
                  const double tofmax) {
  if (difa < 0.) {
    // check for imaginary part in quadratic equation
    return std::min<double>(tzero - .25 * difc * difc / difa, tofmax);
  }

  // everything else is fine so just return supplied tofmax
  return tofmax;
}

} // end of anonymous namespace

void LoadDiffCal::makeCalWorkspace(const std::vector<int32_t> &detids,
                                   const std::vector<double> &difc,
                                   const std::vector<double> &difa,
                                   const std::vector<double> &tzero,
                                   const std::vector<int32_t> &dasids,
                                   const std::vector<double> &offsets) {
  bool makeWS = getProperty("MakeCalWorkspace");
  if (!makeWS) {
    g_log.information("Not making a calibration workspace");
    return;
  }

  size_t numDet = detids.size();
  Progress progress(this, .8, 1., numDet);

  bool haveDasids = !dasids.empty();
  bool haveOffsets = !offsets.empty();

  double tofMin = getProperty("TofMin");
  double tofMax = getProperty("TofMax");
  bool useTofMax = !isEmpty(tofMax);

  ITableWorkspace_sptr wksp = boost::make_shared<DataObjects::TableWorkspace>();
  wksp->setTitle(m_filename);
  wksp->addColumn("int", "detid");
  wksp->addColumn("double", "difc");
  wksp->addColumn("double", "difa");
  wksp->addColumn("double", "tzero");
  // only add these columns if they have values
  if (haveDasids)
    wksp->addColumn("int", "dasid");
  if (haveOffsets)
    wksp->addColumn("double", "offset");

  // columns for valid range of data
  wksp->addColumn("double", "tofmin");
  if (useTofMax)
    wksp->addColumn("double", "tofmax");

  size_t badCount = 0;
  for (size_t i = 0; i < numDet; ++i) {
    API::TableRow newrow = wksp->appendRow();
    newrow << detids[i];
    newrow << difc[i];
    newrow << difa[i];
    newrow << tzero[i];
    if (haveDasids)
      newrow << dasids[i];
    if (haveOffsets)
      newrow << offsets[i];

    // calculate tof range for information
    const double tofMinRow = calcTofMin(difc[i], difa[i], tzero[i], tofMin);
    std::stringstream msg;
    if (tofMinRow != tofMin) {
      msg << "TofMin shifted from " << tofMin << " to " << tofMinRow << " ";
    }
    newrow << tofMinRow;
    if (useTofMax) {
      const double tofMaxRow = calcTofMax(difc[i], difa[i], tzero[i], tofMax);
      newrow << tofMaxRow;

      if (tofMaxRow != tofMax) {
        msg << "TofMax shifted from " << tofMax << " to " << tofMaxRow;
      }
    }
    if (!msg.str().empty()) {
      badCount += 1;
      std::stringstream longMsg;
      longMsg << "[detid=" << detids[i];
      if (haveDasids)
        longMsg << ", dasid=" << dasids[i];
      longMsg << "] " << msg.str();
      this->g_log.warning(longMsg.str());
    }

    progress.report();
  }
  if (badCount > 0) {
    this->g_log.warning() << badCount
                          << " rows have reduced time-of-flight range\n";
  }

  setCalWSProperty(this, m_workspaceName, wksp);
}

void LoadDiffCal::runLoadCalFile() {
  bool makeCalWS = getProperty("MakeCalWorkspace");
  bool makeMaskWS = getProperty("MakeMaskWorkspace");
  bool makeGroupWS = getProperty("MakeGroupingWorkspace");

  auto alg = createChildAlgorithm("LoadCalFile", 0., 1.);
  alg->setPropertyValue("CalFilename", m_filename);
  alg->setPropertyValue("InputWorkspace", getPropertyValue("InputWorkspace"));
  alg->setPropertyValue("InstrumentName", getPropertyValue("InstrumentName"));
  alg->setPropertyValue("InstrumentFilename",
                        getPropertyValue("InstrumentFilename"));
  alg->setProperty<bool>("MakeOffsetsWorkspace", makeCalWS);
  alg->setProperty<bool>("MakeGroupingWorkspace", makeGroupWS);
  alg->setProperty<bool>("MakeMaskWorkspace", makeMaskWS);
  alg->setPropertyValue("WorkspaceName", m_workspaceName);
  alg->executeAsChildAlg();

  if (makeCalWS) {
    ITableWorkspace_sptr wksp = alg->getProperty("OutputCalWorkspace");
    setCalWSProperty(this, m_workspaceName, wksp);
  }

  if (makeMaskWS) {
    MatrixWorkspace_sptr wksp = alg->getProperty("OutputMaskWorkspace");
    setMaskWSProperty(
        this, m_workspaceName,
        boost::dynamic_pointer_cast<DataObjects::MaskWorkspace>(wksp));
  }

  if (makeGroupWS) {
    GroupingWorkspace_sptr wksp = alg->getProperty("OutputGroupingWorkspace");
    setGroupWSProperty(this, m_workspaceName, wksp);
  }
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void LoadDiffCal::exec() {
  m_filename = getPropertyValue("Filename");
  m_workspaceName = getPropertyValue("WorkspaceName");

  if (endswith(m_filename, ".cal")) {
    runLoadCalFile();
    return;
  }

  // read in everything from the file
  H5File file(m_filename, H5F_ACC_RDONLY);
  H5::Exception::dontPrint();
  getInstrument(file);

  Progress progress(this, 0.1, 0.4, 8);
  Group calibrationGroup;
  try {
    calibrationGroup = file.openGroup("calibration");
  } catch (FileIException &e) {
    e.printError();
    throw std::runtime_error("Did not find group \"/calibration\"");
  }

  progress.report("Reading detid");
  std::vector<int32_t> detids = readInt32Array(calibrationGroup, "detid");
  progress.report("Reading dasid");
  std::vector<int32_t> dasids = readInt32Array(calibrationGroup, "dasid");
  progress.report("Reading group");
  std::vector<int32_t> groups = readInt32Array(calibrationGroup, "group");
  progress.report("Reading use");
  std::vector<int32_t> use = readInt32Array(calibrationGroup, "use");

  progress.report("Reading difc");
  std::vector<double> difc = readDoubleArray(calibrationGroup, "difc");
  progress.report("Reading difa");
  std::vector<double> difa = readDoubleArray(calibrationGroup, "difa");
  progress.report("Reading tzero");
  std::vector<double> tzero = readDoubleArray(calibrationGroup, "tzero");
  progress.report("Reading offset");
  std::vector<double> offset = readDoubleArray(calibrationGroup, "offset");

  file.close();

  // verify the minimum is present
  if (detids.empty()) {
    throw std::runtime_error(
        "File was missing required field \"/calibraion/detid\"");
  }
  if (difc.empty()) {
    throw std::runtime_error(
        "File was missing required field \"/calibraion/difc\"");
  }

  // fix up empty arrays
  if (groups.empty())
    groups.assign(detids.size(), 1); // all go to one spectrum
  if (use.empty())
    use.assign(detids.size(), 1); // use everything
  if (difa.empty())
    difa.assign(detids.size(), 0.); // turn off difa
  if (tzero.empty())
    tzero.assign(detids.size(), 0.); // turn off tzero

  // create the appropriate output workspaces
  makeGroupingWorkspace(detids, groups);
  makeMaskWorkspace(detids, use);
  makeCalWorkspace(detids, difc, difa, tzero, dasids, offset);
}

} // namespace DataHandling
} // namespace Mantid
