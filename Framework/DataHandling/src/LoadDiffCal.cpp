// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataHandling/LoadDiffCal.h"

#include "MantidAPI/FileProperty.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Progress.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/TableRow.h"
#include "MantidDataHandling/LoadCalFile.h"
#include "MantidDataObjects/GroupingWorkspace.h"
#include "MantidDataObjects/MaskWorkspace.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/EnumeratedString.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/OptionalBool.h"
#include "MantidKernel/Unit.h"
#include "MantidNexus/H5Util.h"

#include <H5Cpp.h>
#include <cmath>
#include <filesystem>

namespace Mantid::DataHandling {

using Mantid::API::FileProperty;
using Mantid::API::ITableWorkspace;
using Mantid::API::ITableWorkspace_sptr;
using Mantid::API::MatrixWorkspace_sptr;
using Mantid::API::Progress;
using Mantid::API::WorkspaceProperty;
using Mantid::DataObjects::GroupingWorkspace_sptr;
using Mantid::DataObjects::MaskWorkspace_sptr;
using Mantid::DataObjects::Workspace2D;
using Mantid::Kernel::compareStringsCaseInsensitive;
using Mantid::Kernel::Direction;
using Mantid::Kernel::EnumeratedString;
using Mantid::Kernel::PropertyWithValue;
using Mantid::Kernel::Exception::FileError;

using namespace H5;
using namespace NeXus;

namespace {
enum class CalibFilenameExtensionEnum { H5, HD5, HDF, CAL, enum_count };
std::vector<std::string> const calibFilenameExtensions{".h5", ".hd5", ".hdf", ".cal"};
typedef EnumeratedString<CalibFilenameExtensionEnum, &calibFilenameExtensions, &compareStringsCaseInsensitive>
    CalibFilenameExtension;

enum class GroupingFilenameExtensionEnum { XML, H5, HD5, HDF, CAL, enum_count };
std::vector<std::string> const groupingFilenameExtensions{".xml", ".h5", ".hd5", ".hdf", ".cal"};
typedef EnumeratedString<GroupingFilenameExtensionEnum, &groupingFilenameExtensions, &compareStringsCaseInsensitive>
    GroupingFilenameExtension;

namespace PropertyNames {
std::string const CAL_FILE("Filename");
std::string const GROUP_FILE("GroupFilename");
std::string const MAKE_CAL("MakeCalWorkspace");
std::string const MAKE_GRP("MakeGroupingWorkspace");
std::string const MAKE_MSK("MakeMaskWorkspace");
} // namespace PropertyNames
} // namespace

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(LoadDiffCal)

/// Algorithms name for identification. @see Algorithm::name
std::string const LoadDiffCal::name() const { return "LoadDiffCal"; }

/// Algorithm's version for identification. @see Algorithm::version
int LoadDiffCal::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
std::string const LoadDiffCal::category() const { return "DataHandling\\Instrument;Diffraction\\DataHandling"; }

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
std::string const LoadDiffCal::summary() const { return "Loads a calibration file for powder diffraction"; }

/** Initialize the algorithm's properties.
 */
void LoadDiffCal::init() {
  // 3 properties for getting the right instrument
  LoadCalFile::getInstrument3WaysInit(this);

  declareProperty(
      std::make_unique<FileProperty>(PropertyNames::CAL_FILE, "", FileProperty::Load, calibFilenameExtensions),
      "Path to the input calibration file.");

  declareProperty(std::make_unique<FileProperty>(PropertyNames::GROUP_FILE, "", FileProperty::OptionalLoad,
                                                 groupingFilenameExtensions),
                  "Overrides grouping from CalFileName");

  declareProperty(std::make_unique<PropertyWithValue<bool>>(PropertyNames::MAKE_GRP, true, Direction::Input),
                  "Set to true to create a GroupingWorkspace with called "
                  "WorkspaceName_group.");

  declareProperty(std::make_unique<PropertyWithValue<bool>>(PropertyNames::MAKE_CAL, true, Direction::Input),
                  "Set to true to create a CalibrationWorkspace with called "
                  "WorkspaceName_cal.");

  declareProperty(std::make_unique<PropertyWithValue<bool>>(PropertyNames::MAKE_MSK, true, Direction::Input),
                  "Set to true to create a MaskWorkspace with called WorkspaceName_mask.");

  declareProperty(std::make_unique<PropertyWithValue<std::string>>("WorkspaceName", "", Direction::Input),
                  "The base of the output workspace names. Names will have '_group', "
                  "'_cal', '_mask' appended to them.");

  std::string grpName("Calibration Validation");
  declareProperty("TofMin", 0., "Minimum for TOF axis. Defaults to 0.");
  declareProperty("TofMax", EMPTY_DBL(), "Maximum for TOF axis. Defaults to Unused.");
  declareProperty(std::make_unique<PropertyWithValue<bool>>("FixConversionIssues", true, Direction::Input),
                  "Set DIFA and TZERO to zero if there is an error and the "
                  "pixel is masked");
  setPropertyGroup("TofMin", grpName);
  setPropertyGroup("TofMax", grpName);
  setPropertyGroup("FixConversionIssues", grpName);
}

namespace { // anonymous

void setGroupWSProperty(API::Algorithm *alg, std::string const &prefix, GroupingWorkspace_sptr const &wksp) {
  alg->declareProperty(std::make_unique<WorkspaceProperty<DataObjects::GroupingWorkspace>>(
                           "OutputGroupingWorkspace", prefix + "_group", Direction::Output),
                       "Set the output GroupingWorkspace, if any.");
  alg->setProperty("OutputGroupingWorkspace", wksp);
}

void setMaskWSProperty(API::Algorithm *alg, std::string const &prefix, MaskWorkspace_sptr const &wksp) {
  alg->declareProperty(std::make_unique<WorkspaceProperty<DataObjects::MaskWorkspace>>(
                           "OutputMaskWorkspace", prefix + "_mask", Direction::Output),
                       "Set the output MaskWorkspace, if any.");
  alg->setProperty("OutputMaskWorkspace", wksp);
}

void setCalWSProperty(API::Algorithm *alg, std::string const &prefix, ITableWorkspace_sptr const &wksp) {
  alg->declareProperty(
      std::make_unique<WorkspaceProperty<ITableWorkspace>>("OutputCalWorkspace", prefix + "_cal", Direction::Output),
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

  std::string idf = H5Util::readString(file, "/calibration/instrument/instrument_source");
  std::string instrumentName = H5Util::readString(file, "/calibration/instrument/name");

  g_log.debug() << "IDF : " << idf << "\n"
                << "NAME: " << instrumentName << "\n";

  API::Algorithm_sptr childAlg = this->createChildAlgorithm("LoadInstrument", 0.0, 0.1);
  MatrixWorkspace_sptr tempWS(new Workspace2D());
  childAlg->setProperty<MatrixWorkspace_sptr>("Workspace", tempWS);
  if (idf.empty()) {
    childAlg->setPropertyValue("InstrumentName", instrumentName);
  } else {
    childAlg->setPropertyValue("Filename", idf);
  }
  childAlg->setProperty("RewriteSpectraMap", Mantid::Kernel::OptionalBool(false));
  childAlg->executeAsChildAlg();

  m_instrument = tempWS->getInstrument();

  g_log.information() << "Loaded instrument \"" << m_instrument->getName() << "\" from \""
                      << m_instrument->getFilename() << "\"\n";
}

void LoadDiffCal::makeGroupingWorkspace(std::vector<int32_t> const &detids, std::vector<int32_t> const &groups) {
  bool makeWS = getProperty(PropertyNames::MAKE_GRP);
  if (!makeWS) {
    g_log.information("Not loading GroupingWorkspace from the calibration file");
    return;
  }

  // load grouping from a separate file if supplied
  if (!isDefault(PropertyNames::GROUP_FILE)) {
    loadGroupingFromAlternateFile();
    return;
  }

  size_t numDet = detids.size();
  Progress progress(this, .4, .6, numDet);

  GroupingWorkspace_sptr wksp = std::make_shared<DataObjects::GroupingWorkspace>(m_instrument);
  wksp->setTitle(m_filename);
  wksp->mutableRun().addProperty("Filename", m_filename);

  for (size_t i = 0; i < numDet; ++i) {
    auto detid = static_cast<detid_t>(detids[i]);
    wksp->setValue(detid, groups[i]);
    progress.report();
  }

  setGroupWSProperty(this, m_workspaceName, wksp);
}

void LoadDiffCal::makeMaskWorkspace(std::vector<int32_t> const &detids, std::vector<int32_t> const &use) {
  bool makeWS = getProperty(PropertyNames::MAKE_MSK);
  if (!makeWS) {
    g_log.information("Not making a MaskWorkspace");
    return;
  }

  size_t numDet = detids.size();
  Progress progress(this, .6, .8, numDet);

  MaskWorkspace_sptr wksp = std::make_shared<DataObjects::MaskWorkspace>(m_instrument);
  wksp->setTitle(m_filename);
  wksp->mutableRun().addProperty("Filename", m_filename);

  for (size_t i = 0; i < numDet; ++i) {
    bool shouldUse = (use[i] > 0); // true if detector is calibrated
    auto detid = static_cast<detid_t>(detids[i]);
    // in maskworkspace 0=use, 1=dontuse
    wksp->setMasked(detid, !shouldUse);
    // The mask value is 0 if the detector is good for use
    wksp->setValue(detid, (shouldUse ? 0. : 1.));
    progress.report();
  }

  setMaskWSProperty(this, m_workspaceName, wksp);
}

void LoadDiffCal::makeCalWorkspace(std::vector<int32_t> const &detids, std::vector<double> const &difc,
                                   std::vector<double> const &difa, std::vector<double> const &tzero,
                                   std::vector<int32_t> const &dasids, std::vector<double> const &offsets,
                                   std::vector<int32_t> const &use) {
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

  ITableWorkspace_sptr wksp = std::make_shared<DataObjects::TableWorkspace>();
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
    Kernel::Units::dSpacing dspacingUnit;
    double const tofMinRow = dspacingUnit.calcTofMin(difc[i], difa[i], tzero[i], tofMin);
    std::stringstream msg;
    if (tofMinRow != tofMin) {
      msg << "TofMin shifted from " << tofMin << " to " << tofMinRow << " ";
    }
    newrow << tofMinRow;
    if (useTofMax) {
      double const tofMaxRow = dspacingUnit.calcTofMax(difc[i], difa[i], tzero[i], tofMax);
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
    this->g_log.warning() << badCount << " rows have reduced time-of-flight range\n";
  }

  setCalWSProperty(this, m_workspaceName, wksp);
}

void LoadDiffCal::loadGroupingFromAlternateFile() {
  bool makeWS = getProperty(PropertyNames::MAKE_GRP);
  if (!makeWS)
    return; // input property says not to load grouping

  if (isDefault(PropertyNames::GROUP_FILE))
    return; // a separate grouping file was not specified

  // Check that the instrument is defined
  if (!m_instrument) {
    throw std::runtime_error("Cannot load alternate grouping: the instrument is not defined.");
  }
  // Create a grouping workspace with this instrument
  GroupingWorkspace_sptr groupingWorkspace = std::make_shared<DataObjects::GroupingWorkspace>(m_instrument);

  // Get the alternate grouping file name
  std::string filename = getPropertyValue(PropertyNames::GROUP_FILE);
  g_log.information() << "Override grouping with information from \"" << filename << "\"\n";

  // Determine file format by file name extension
  std::string filenameExtension = std::filesystem::path(filename).extension().string();
  GroupingFilenameExtension enFilenameExtension(
      filenameExtension); // this will throw a runtime error if the extension is invalid
  switch (enFilenameExtension) {
  case GroupingFilenameExtensionEnum::XML: {
    auto alg = createChildAlgorithm("LoadDetectorsGroupingFile");
    alg->setProperty("InputWorkspace", groupingWorkspace);
    alg->setProperty("InputFile", filename);
    alg->executeAsChildAlg();
    groupingWorkspace = alg->getProperty("OutputWorkspace");
  } break;
  case GroupingFilenameExtensionEnum::H5:
  case GroupingFilenameExtensionEnum::HD5:
  case GroupingFilenameExtensionEnum::HDF:
  case GroupingFilenameExtensionEnum::CAL: {
    auto alg = createChildAlgorithm("LoadDiffCal");
    alg->setPropertyValue(PropertyNames::CAL_FILE, filename); // the alternate grouping file
    alg->setProperty("InputWorkspace", groupingWorkspace);    // a workspace to get the instrument from
    alg->setProperty<bool>(PropertyNames::MAKE_CAL, false);
    alg->setProperty<bool>(PropertyNames::MAKE_GRP, true);
    alg->setProperty<bool>(PropertyNames::MAKE_MSK, false);
    alg->setPropertyValue("WorkspaceName", m_workspaceName);
    alg->executeAsChildAlg();
    groupingWorkspace = alg->getProperty("OutputGroupingWorkspace");
  } break;
  default:
    std::ostringstream os;
    os << "Alternate grouping file has an invalid extension: "
       << "\"" << filenameExtension << "\"";
    throw std::runtime_error(os.str());
  }

  setGroupWSProperty(this, m_workspaceName, groupingWorkspace);
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
  alg->setPropertyValue("InstrumentFilename", getPropertyValue("InstrumentFilename"));
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
    setMaskWSProperty(this, m_workspaceName, std::dynamic_pointer_cast<DataObjects::MaskWorkspace>(wksp));
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

  // Determine file format by file name extension
  std::string filenameExtension = std::filesystem::path(m_filename).extension().string();
  CalibFilenameExtension enFilenameExtension(
      filenameExtension); // this will throw a runtime error if the extension is invalid
  if (enFilenameExtension == CalibFilenameExtensionEnum::CAL) {
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
  std::vector<int32_t> detids = H5Util::readArray1DCoerce<int32_t>(calibrationGroup, "detid");
  progress.report("Reading dasid");
  std::vector<int32_t> dasids = H5Util::readArray1DCoerce<int32_t>(calibrationGroup, "dasid");
  progress.report("Reading group");
  std::vector<int32_t> groups = H5Util::readArray1DCoerce<int32_t>(calibrationGroup, "group");
  progress.report("Reading use");
  std::vector<int32_t> use = H5Util::readArray1DCoerce<int32_t>(calibrationGroup, "use");

  progress.report("Reading difc");
  std::vector<double> difc = H5Util::readArray1DCoerce<double>(calibrationGroup, "difc");
  progress.report("Reading difa");
  std::vector<double> difa = H5Util::readArray1DCoerce<double>(calibrationGroup, "difa");
  progress.report("Reading tzero");
  std::vector<double> tzero = H5Util::readArray1DCoerce<double>(calibrationGroup, "tzero");
  progress.report("Reading offset");
  std::vector<double> offset = H5Util::readArray1DCoerce<double>(calibrationGroup, "offset");

  file.close();

  // verify the minimum is present
  if (detids.empty()) {
    throw std::runtime_error("File was missing required field \"/calibraion/detid\"");
  }
  if (difc.empty()) {
    throw std::runtime_error("File was missing required field \"/calibraion/difc\"");
  }

  // fix up empty arrays
  if (groups.empty())
    groups.assign(detids.size(), 1); // all go to one spectrum
  if (use.empty())
    use.assign(detids.size(), 1); // all detectors are good, use them
  if (difa.empty())
    difa.assign(detids.size(), 0.); // turn off difa
  if (tzero.empty())
    tzero.assign(detids.size(), 0.); // turn off tzero

  // create the appropriate output workspaces
  makeGroupingWorkspace(detids, groups);
  makeMaskWorkspace(detids, use);
  makeCalWorkspace(detids, difc, difa, tzero, dasids, offset, use);
}
} // namespace Mantid::DataHandling
