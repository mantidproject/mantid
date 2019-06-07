// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataHandling/LoadDiffCal.h"

#include "MantidAPI/FileProperty.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Progress.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/TableRow.h"
#include "MantidDataHandling/H5Util.h"
#include "MantidDataHandling/LoadCalFile.h"
#include "MantidDataObjects/GroupingWorkspace.h"
#include "MantidDataObjects/MaskWorkspace.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/Diffraction.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/OptionalBool.h"

#include <H5Cpp.h>
#include <cmath>

namespace Mantid {
namespace DataHandling {

using Mantid::API::FileProperty;
using Mantid::API::ITableWorkspace;
using Mantid::API::ITableWorkspace_sptr;
using Mantid::API::MatrixWorkspace_sptr;
using Mantid::API::Progress;
using Mantid::API::WorkspaceProperty;
using Mantid::DataObjects::GroupingWorkspace_sptr;
using Mantid::DataObjects::MaskWorkspace_sptr;
using Mantid::DataObjects::Workspace2D;
using Mantid::Kernel::Direction;
using Mantid::Kernel::Exception::FileError;
using Mantid::Kernel::PropertyWithValue;

using namespace H5;

namespace {
namespace PropertyNames {
const std::string CAL_FILE("Filename");
const std::string GROUP_FILE("GroupFilename");
const std::string MAKE_CAL("MakeCalWorkspace");
const std::string MAKE_GRP("MakeGroupingWorkspace");
const std::string MAKE_MSK("MakeMaskWorkspace");
} // namespace PropertyNames
} // namespace

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(LoadDiffCal)

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

/** Initialize the algorithm's properties.
 */
void LoadDiffCal::init() {
  // 3 properties for getting the right instrument
  LoadCalFile::getInstrument3WaysInit(this);

  const std::vector<std::string> exts{".h5", ".hd5", ".hdf", ".cal"};
  declareProperty(std::make_unique<FileProperty>(PropertyNames::CAL_FILE, "",
                                                 FileProperty::Load, exts),
                  "Path to the .h5 file.");
  declareProperty(std::make_unique<FileProperty>(
                      PropertyNames::GROUP_FILE, "", FileProperty::OptionalLoad,
                      std::vector<std::string>{".xml", ".cal"}),
                  "Overrides grouping from CalFileName");

  declareProperty(std::make_unique<PropertyWithValue<bool>>(
                      PropertyNames::MAKE_GRP, true, Direction::Input),
                  "Set to true to create a GroupingWorkspace with called "
                  "WorkspaceName_group.");

  declareProperty(std::make_unique<PropertyWithValue<bool>>(
                      PropertyNames::MAKE_CAL, true, Direction::Input),
                  "Set to true to create a CalibrationWorkspace with called "
                  "WorkspaceName_cal.");

  declareProperty(
      std::make_unique<PropertyWithValue<bool>>(PropertyNames::MAKE_MSK, true,
                                                Direction::Input),
      "Set to true to create a MaskWorkspace with called WorkspaceName_mask.");

  declareProperty(
      std::make_unique<PropertyWithValue<std::string>>("WorkspaceName", "",
                                                       Direction::Input),
      "The base of the output workspace names. Names will have '_group', "
      "'_cal', '_mask' appended to them.");

  std::string grpName("Calibration Validation");
  declareProperty("TofMin", 0., "Minimum for TOF axis. Defaults to 0.");
  declareProperty("TofMax", EMPTY_DBL(),
                  "Maximum for TOF axis. Defaults to Unused.");
  declareProperty(std::make_unique<PropertyWithValue<bool>>(
                      "FixConversionIssues", true, Direction::Input),
                  "Set DIFA and TZERO to zero if there is an error and the "
                  "pixel is masked");
  setPropertyGroup("TofMin", grpName);
  setPropertyGroup("TofMax", grpName);
  setPropertyGroup("FixConversionIssues", grpName);
}

namespace { // anonymous

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
      std::make_unique<WorkspaceProperty<DataObjects::GroupingWorkspace>>(
          "OutputGroupingWorkspace", prefix + "_group", Direction::Output),
      "Set the the output GroupingWorkspace, if any.");
  alg->setProperty("OutputGroupingWorkspace", wksp);
}

void setMaskWSProperty(API::Algorithm *alg, const std::string &prefix,
                       MaskWorkspace_sptr wksp) {
  alg->declareProperty(
      std::make_unique<WorkspaceProperty<DataObjects::MaskWorkspace>>(
          "OutputMaskWorkspace", prefix + "_mask", Direction::Output),
      "Set the the output MaskWorkspace, if any.");
  alg->setProperty("OutputMaskWorkspace", wksp);
}

void setCalWSProperty(API::Algorithm *alg, const std::string &prefix,
                      ITableWorkspace_sptr wksp) {
  alg->declareProperty(
      std::make_unique<WorkspaceProperty<ITableWorkspace>>(
          "OutputCalWorkspace", prefix + "_cal", Direction::Output),
      "Set the output Diffraction Calibration workspace, if any.");
  alg->setProperty("OutputCalWorkspace", wksp);
}

} // anonymous namespace

void LoadDiffCal::getInstrument(H5File &file) {
  // don't bother if there isn't a mask or grouping requested
  bool makeMask = getProperty(PropertyNames::MAKE_MSK);
  bool makeGrouping = getProperty(PropertyNames::MAKE_GRP);
  if ((!makeMask) & (!makeGrouping))
    return;

  // see if the user specified the instrument independently
  if (LoadCalFile::instrumentIsSpecified(this)) {
    m_instrument = LoadCalFile::getInstrument3Ways(this);
    return;
  }

  std::string idf =
      H5Util::readString(file, "/calibration/instrument/instrument_source");
  std::string instrumentName =
      H5Util::readString(file, "/calibration/instrument/name");

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
  bool makeWS = getProperty(PropertyNames::MAKE_GRP);
  if (!makeWS) {
    g_log.information(
        "Not loading GroupingWorkspace from the calibration file");
    return;
  }

  // load grouping from a separate file if supplied
  if (!isDefault(PropertyNames::GROUP_FILE)) {
    loadGroupingFromAlternateFile();
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
  bool makeWS = getProperty(PropertyNames::MAKE_MSK);
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

void LoadDiffCal::makeCalWorkspace(const std::vector<int32_t> &detids,
                                   const std::vector<double> &difc,
                                   const std::vector<double> &difa,
                                   const std::vector<double> &tzero,
                                   const std::vector<int32_t> &dasids,
                                   const std::vector<double> &offsets,
                                   const std::vector<int32_t> &use) {
  bool makeWS = getProperty(PropertyNames::MAKE_CAL);
  if (!makeWS) {
    g_log.information("Not making a calibration workspace");
    return;
  }

  size_t numDet = detids.size();
  Progress progress(this, .8, 1., numDet);

  bool haveDasids = !dasids.empty();
  bool haveOffsets = !offsets.empty();
  bool fixIssues = getProperty("FixConversionIssues");

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
    const double tofMinRow =
        Kernel::Diffraction::calcTofMin(difc[i], difa[i], tzero[i], tofMin);
    std::stringstream msg;
    if (tofMinRow != tofMin) {
      msg << "TofMin shifted from " << tofMin << " to " << tofMinRow << " ";
    }
    newrow << tofMinRow;
    if (useTofMax) {
      const double tofMaxRow =
          Kernel::Diffraction::calcTofMax(difc[i], difa[i], tzero[i], tofMax);
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

      // to fix issues for masked pixels, just zero difa and tzero
      if (fixIssues && (!use[i])) {
        longMsg << " pixel is masked, ";
        longMsg << " changing difa (" << wksp->cell<double>(i, 2) << " to 0.)";
        wksp->cell<double>(i, 2) = 0.;

        longMsg << " and tzero (" << wksp->cell<double>(i, 3) << " to 0.)";
        wksp->cell<double>(i, 3) = 0.;

        // restore valid tof range
        size_t index = 4; // where tofmin natively is
        if (haveDasids)
          index += 1;
        if (haveOffsets)
          index += 1;
        wksp->cell<double>(i, index) = tofMin;
        if (useTofMax)
          wksp->cell<double>(i, index + 1) = tofMax;
      }

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

/// @return true if the grouping information should be taken from the
/// calibration file
void LoadDiffCal::loadGroupingFromAlternateFile() {
  bool makeWS = getProperty(PropertyNames::MAKE_GRP);
  if (!makeWS)
    return; // input property says not to load grouping

  if (isDefault(PropertyNames::GROUP_FILE))
    return; // a separate grouping file was not specified

  std::string filename = getPropertyValue(PropertyNames::GROUP_FILE);
  g_log.information() << "Override grouping with information from \""
                      << filename << "\"\n";
  if (!m_instrument) {
    throw std::runtime_error(
        "Do not have an instrument defined before loading separate grouping");
  }
  GroupingWorkspace_sptr wksp =
      boost::make_shared<DataObjects::GroupingWorkspace>(m_instrument);

  if (filename.find(".cal") != std::string::npos) {
    auto alg = createChildAlgorithm("LoadDiffCal");
    alg->setProperty("InputWorkspace", wksp);
    alg->setPropertyValue(PropertyNames::CAL_FILE, filename);
    alg->setProperty<bool>(PropertyNames::MAKE_CAL, false);
    alg->setProperty<bool>(PropertyNames::MAKE_GRP, true);
    alg->setProperty<bool>(PropertyNames::MAKE_MSK, false);
    alg->setPropertyValue("WorkspaceName", m_workspaceName);
    alg->executeAsChildAlg();

    // get the workspace
    wksp = alg->getProperty("OutputGroupingWorkspace");
  } else {
    auto alg = createChildAlgorithm("LoadDetectorsGroupingFile");
    alg->setProperty("InputWorkspace", wksp);
    alg->setProperty("InputFile", filename);
    alg->executeAsChildAlg();

    // get the workspace
    wksp = alg->getProperty("OutputWorkspace");
  }
  setGroupWSProperty(this, m_workspaceName, wksp);
}

void LoadDiffCal::runLoadCalFile() {
  bool makeCalWS = getProperty(PropertyNames::MAKE_CAL);
  bool makeMaskWS = getProperty(PropertyNames::MAKE_MSK);
  bool makeGroupWS = getProperty(PropertyNames::MAKE_GRP);
  API::MatrixWorkspace_sptr inputWs = getProperty("InputWorkspace");

  bool haveGroupingFile = !isDefault(PropertyNames::GROUP_FILE);

  auto alg = createChildAlgorithm("LoadCalFile", 0., 1.);
  alg->setPropertyValue("CalFilename", m_filename);
  alg->setProperty("InputWorkspace", inputWs);
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
    if (haveGroupingFile) {
      // steal the instrument from what was loaded already
      if (!m_instrument)
        m_instrument = wksp->getInstrument();
      loadGroupingFromAlternateFile();
    } else {
      setGroupWSProperty(this, m_workspaceName, wksp);
    }
  }
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void LoadDiffCal::exec() {
  m_filename = getPropertyValue(PropertyNames::CAL_FILE);
  m_workspaceName = getPropertyValue("WorkspaceName");

  if (endswith(m_filename, ".cal")) {
    runLoadCalFile();
    return;
  }

  // read in everything from the file
  H5::Exception::dontPrint();
  H5File file;
  try {
    file = H5File(m_filename, H5F_ACC_RDONLY);
  } catch (FileIException &) {
    throw FileError("Failed to open file using HDF5", m_filename);
  }
  getInstrument(file);

  Progress progress(this, 0.1, 0.4, 8);
  Group calibrationGroup;
  try {
    calibrationGroup = file.openGroup("calibration");
  } catch (FileIException &e) {
#if H5_VERSION_GE(1, 8, 13)
    UNUSED_ARG(e);
    H5::Exception::printErrorStack();
#else
    e.printError(stderr);
#endif
    file.close();
    throw FileError("Did not find group \"/calibration\"", m_filename);
  }

  progress.report("Reading detid");
  std::vector<int32_t> detids =
      H5Util::readArray1DCoerce<int32_t>(calibrationGroup, "detid");
  progress.report("Reading dasid");
  std::vector<int32_t> dasids =
      H5Util::readArray1DCoerce<int32_t>(calibrationGroup, "dasid");
  progress.report("Reading group");
  std::vector<int32_t> groups =
      H5Util::readArray1DCoerce<int32_t>(calibrationGroup, "group");
  progress.report("Reading use");
  std::vector<int32_t> use =
      H5Util::readArray1DCoerce<int32_t>(calibrationGroup, "use");

  progress.report("Reading difc");
  std::vector<double> difc =
      H5Util::readArray1DCoerce<double>(calibrationGroup, "difc");
  progress.report("Reading difa");
  std::vector<double> difa =
      H5Util::readArray1DCoerce<double>(calibrationGroup, "difa");
  progress.report("Reading tzero");
  std::vector<double> tzero =
      H5Util::readArray1DCoerce<double>(calibrationGroup, "tzero");
  progress.report("Reading offset");
  std::vector<double> offset =
      H5Util::readArray1DCoerce<double>(calibrationGroup, "offset");

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
  makeCalWorkspace(detids, difc, difa, tzero, dasids, offset, use);
}

Parallel::ExecutionMode LoadDiffCal::getParallelExecutionMode(
    const std::map<std::string, Parallel::StorageMode> &storageModes) const {
  // There is an optional input workspace which may have
  // StorageMode::Distributed but it is merely used for passing an instrument.
  // Output should always have StorageMode::Cloned, so we run with
  // ExecutionMode::Identical.
  static_cast<void>(storageModes);
  return Parallel::ExecutionMode::Identical;
}

} // namespace DataHandling
} // namespace Mantid
