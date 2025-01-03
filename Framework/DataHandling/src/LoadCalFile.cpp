// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataHandling/LoadCalFile.h"
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidDataObjects/GroupingWorkspace.h"
#include "MantidDataObjects/MaskWorkspace.h"
#include "MantidDataObjects/OffsetsWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/OptionalBool.h"

#include <Poco/Path.h>
#include <fstream>

using Mantid::Geometry::Instrument_const_sptr;
using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::DataObjects;

namespace Mantid::DataHandling {
// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(LoadCalFile)

/** For use by getInstrument3Ways, initializes the properties
 * @param alg :: algorithm to which to add the properties.
 * */
void LoadCalFile::getInstrument3WaysInit(Algorithm *alg) {
  std::string grpName("Specify the Instrument");

  alg->declareProperty(
      std::make_unique<WorkspaceProperty<>>("InputWorkspace", "", Direction::Input, PropertyMode::Optional),
      "Optional: An input workspace with the instrument we want to use.");

  alg->declareProperty(std::make_unique<PropertyWithValue<std::string>>("InstrumentName", "", Direction::Input),
                       "Optional: Name of the instrument on which to base the GroupingWorkspace.");

  const std::vector<std::string> validFilenameExtensions{".xml", ".hdf5", ".nxs", ".nxs.h5"};
  alg->declareProperty(
      std::make_unique<FileProperty>("InstrumentFilename", "", FileProperty::OptionalLoad, validFilenameExtensions),
      "Optional: Path to a file (full or relative) defining the instrument on which to base the GroupingWorkspace. "
      "The file could be an IDF or a NeXus Geometry file. Note, InstrumentFilename or InstrumentName must be "
      "specified, but not both.");

  alg->setPropertyGroup("InputWorkspace", grpName);
  alg->setPropertyGroup("InstrumentName", grpName);
  alg->setPropertyGroup("InstrumentFilename", grpName);
}

bool LoadCalFile::instrumentIsSpecified(API::Algorithm *alg) {
  MatrixWorkspace_sptr inWS = alg->getProperty("InputWorkspace");
  if (bool(inWS))
    return true;

  std::string InstrumentName = alg->getPropertyValue("InstrumentName");
  if (!InstrumentName.empty())
    return true;

  std::string InstrumentFilename = alg->getPropertyValue("InstrumentFilename");
  return !InstrumentFilename.empty();
}

/** Get a pointer to an instrument in one of 3 ways: InputWorkspace,
 * InstrumentName, InstrumentFilename
 * @param alg :: algorithm from which to get the property values.
 * */
Geometry::Instrument_const_sptr LoadCalFile::getInstrument3Ways(Algorithm *alg) {
  MatrixWorkspace_sptr inWS = alg->getProperty("InputWorkspace");
  std::string InstrumentName = alg->getPropertyValue("InstrumentName");
  std::string InstrumentFilename = alg->getPropertyValue("InstrumentFilename");

  // Some validation
  int numParams = 0;
  if (inWS)
    numParams++;
  if (!InstrumentName.empty())
    numParams++;
  if (!InstrumentFilename.empty())
    numParams++;

  if (numParams > 1)
    throw std::invalid_argument("You must specify exactly ONE way to get an "
                                "instrument (workspace, instrument name, or "
                                "IDF file). You specified more than one.");
  if (numParams == 0)
    throw std::invalid_argument("You must specify exactly ONE way to get an "
                                "instrument (workspace, instrument name, or "
                                "IDF file). You specified none.");

  // ---------- Get the instrument one of 3 ways ---------------------------
  Instrument_const_sptr inst;
  if (inWS) {
    inst = inWS->getInstrument();
  } else {
    Algorithm_sptr childAlg = alg->createChildAlgorithm("LoadEmptyInstrument", 0.0, 0.2);
    childAlg->setPropertyValue("Filename", InstrumentFilename);
    childAlg->setPropertyValue("InstrumentName", InstrumentName);
    childAlg->executeAsChildAlg();
    MatrixWorkspace_sptr tempWS = childAlg->getProperty("OutputWorkspace");
    inst = tempWS->getInstrument();
  }

  return inst;
}

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void LoadCalFile::init() {
  LoadCalFile::getInstrument3WaysInit(this);

  declareProperty(std::make_unique<FileProperty>("CalFilename", "", FileProperty::Load, ".cal"),
                  "Path to the old-style .cal grouping/calibration file (multi-column "
                  "ASCII). You must also specify the instrument.");

  declareProperty(std::make_unique<PropertyWithValue<bool>>("MakeGroupingWorkspace", true, Direction::Input),
                  "Set to true to create a GroupingWorkspace with called "
                  "WorkspaceName_group.");

  declareProperty(std::make_unique<PropertyWithValue<bool>>("MakeOffsetsWorkspace", true, Direction::Input),
                  "Set to true to create a OffsetsWorkspace with called "
                  "WorkspaceName_offsets.");

  declareProperty(std::make_unique<PropertyWithValue<bool>>("MakeMaskWorkspace", true, Direction::Input),
                  "Set to true to create a MaskWorkspace with called WorkspaceName_mask.");

  declareProperty(std::make_unique<PropertyWithValue<std::string>>("WorkspaceName", "", Direction::Input),
                  "The base of the output workspace names. Names will have '_group', "
                  "'_cal', '_offsets', '_mask' appended to them.");
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void LoadCalFile::exec() {
  std::string CalFilename = getPropertyValue("CalFilename");
  std::string WorkspaceName = getPropertyValue("WorkspaceName");
  bool MakeGroupingWorkspace = getProperty("MakeGroupingWorkspace");
  bool MakeOffsetsWorkspace = getProperty("MakeOffsetsWorkspace");
  bool MakeMaskWorkspace = getProperty("MakeMaskWorkspace");

  if (WorkspaceName.empty())
    throw std::invalid_argument("Must specify WorkspaceName.");

  Instrument_const_sptr inst = LoadCalFile::getInstrument3Ways(this);

  GroupingWorkspace_sptr groupWS;
  OffsetsWorkspace_sptr offsetsWS;
  MaskWorkspace_sptr maskWS;

  // Title of all workspaces = the file without path
  std::string title = Poco::Path(CalFilename).getFileName();

  // Initialize all required workspaces.
  if (MakeGroupingWorkspace) {
    groupWS = GroupingWorkspace_sptr(new GroupingWorkspace(inst));
    groupWS->setTitle(title);
    declareProperty(std::make_unique<WorkspaceProperty<GroupingWorkspace>>("OutputGroupingWorkspace",
                                                                           WorkspaceName + "_group", Direction::Output),
                    "Set the output GroupingWorkspace, if any.");
    groupWS->mutableRun().addProperty("Filename", CalFilename);
    setProperty("OutputGroupingWorkspace", groupWS);
  }

  if (MakeOffsetsWorkspace) {
    offsetsWS = OffsetsWorkspace_sptr(new OffsetsWorkspace(inst));
    offsetsWS->setTitle(title);
    declareProperty(std::make_unique<WorkspaceProperty<OffsetsWorkspace>>(
                        "OutputOffsetsWorkspace", WorkspaceName + "_offsets", Direction::Output),
                    "Set the output OffsetsWorkspace, if any.");
    offsetsWS->mutableRun().addProperty("Filename", CalFilename);
    setProperty("OutputOffsetsWorkspace", offsetsWS);
  }

  if (MakeMaskWorkspace) {
    maskWS = MaskWorkspace_sptr(new MaskWorkspace(inst));
    maskWS->setTitle(title);
    declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>("OutputMaskWorkspace", WorkspaceName + "_mask",
                                                                         Direction::Output),
                    "Set the output MaskWorkspace, if any.");
    maskWS->mutableRun().addProperty("Filename", CalFilename);
    setProperty("OutputMaskWorkspace", maskWS);
  }

  LoadCalFile::readCalFile(CalFilename, groupWS, offsetsWS, maskWS);

  if (MakeOffsetsWorkspace) {
    auto alg = createChildAlgorithm("ConvertDiffCal");
    alg->setProperty("OffsetsWorkspace", offsetsWS);
    alg->executeAsChildAlg();
    ITableWorkspace_sptr calWS = alg->getProperty("OutputWorkspace");
    calWS->setTitle(title);
    declareProperty(std::make_unique<WorkspaceProperty<ITableWorkspace>>("OutputCalWorkspace", WorkspaceName + "_cal",
                                                                         Direction::Output),
                    "Set the output Diffraction Calibration workspace, if any.");
    setProperty("OutputCalWorkspace", calWS);
  }
}

//-----------------------------------------------------------------------
/** Reads the calibration file.
 *
 * @param calFileName :: path to the old .cal file
 * @param groupWS :: optional, GroupingWorkspace to fill. Must be initialized to
 *the right instrument.
 * @param offsetsWS :: optional, OffsetsWorkspace to fill. Must be initialized
 *to the right instrument.
 * @param maskWS :: optional, masking-type workspace to fill. Must be
 *initialized to the right instrument.
 */
void LoadCalFile::readCalFile(const std::string &calFileName, const GroupingWorkspace_sptr &groupWS,
                              const OffsetsWorkspace_sptr &offsetsWS, const MaskWorkspace_sptr &maskWS) {
  auto doGroup = bool(groupWS);
  auto doOffsets = bool(offsetsWS);
  auto doMask = bool(maskWS);

  bool hasUnmasked(false);
  bool hasGrouped(false);

  if (!doOffsets && !doGroup && !doMask)
    throw std::invalid_argument("You must give at least one of the grouping, "
                                "offsets or masking workspaces.");

  std::ifstream grFile(calFileName.c_str());
  if (!grFile) {
    throw std::runtime_error("Unable to open calibration file " + calFileName);
  }

  size_t numErrors = 0;

  detid2index_map detID_to_wi;
  if (doMask) {
    detID_to_wi = maskWS->getDetectorIDToWorkspaceIndexMap();
  }

  // not all of these should be doubles, but to make reading work read as double
  // then recast to int
  int n, udet, select, group;
  double n_d, udet_d, offset, select_d, group_d;

  SpectrumInfo *maskSpectrumInfo{nullptr};
  if (maskWS)
    maskSpectrumInfo = &maskWS->mutableSpectrumInfo();
  std::string str;
  while (getline(grFile, str)) {
    if (str.empty() || str[0] == '#')
      continue;
    std::istringstream istr(str);

    // read in everything as double then cast as appropriate
    istr >> n_d >> udet_d >> offset >> select_d >> group_d;
    n = static_cast<int>(n_d);
    udet = static_cast<int>(udet_d);
    select = static_cast<int>(select_d);
    group = static_cast<int>(group_d);

    if (doOffsets) {
      if (offset <= -1.) // should never happen
      {
        std::stringstream msg;
        msg << "Encountered offset = " << offset << " at index " << n << " for udet = " << udet
            << ". Offsets must be greater than -1.";
        throw std::runtime_error(msg.str());
      }

      try {
        offsetsWS->setValue(udet, offset);
      } catch (std::invalid_argument &) {
        // Ignore the error if the IS is actually for a monitor
        if (!idIsMonitor(offsetsWS->getInstrument(), udet))
          numErrors++;
      }
    }

    if (doGroup) {
      try {
        groupWS->setValue(udet, double(group));
        if ((!hasGrouped) && (group > 0))
          hasGrouped = true;
      } catch (std::invalid_argument &) {
        // Ignore the error if the IS is actually for a monitor
        if (!idIsMonitor(groupWS->getInstrument(), udet))
          numErrors++;
      }
    }

    if (doMask) {
      detid2index_map::const_iterator it = detID_to_wi.find(udet);
      if (it != detID_to_wi.end()) {
        size_t wi = it->second;

        if (select <= 0) {
          // Not selected, then mask this detector
          maskWS->getSpectrum(wi).clearData();
          maskSpectrumInfo->setMasked(wi, true);
          maskWS->mutableY(wi)[0] = 1.0;
        } else {
          // Selected, set the value to be 0
          maskWS->mutableY(wi)[0] = 0.0;
          if (!hasUnmasked)
            hasUnmasked = true;
        }
      } else {
        // Ignore the error if the IS is actually for a monitor
        if (!idIsMonitor(maskWS->getInstrument(), udet))
          numErrors++;
      }
    }
  }

  // Warn about any errors

  if (numErrors > 0)
    Logger("LoadCalFile").warning() << numErrors << " errors (invalid Detector ID's) found when reading .cal file '"
                                    << calFileName << "'.\n";
  if (doGroup && (!hasGrouped))
    Logger("LoadCalFile").warning() << "'" << calFileName << "' has no spectra grouped\n";
  if (doMask && (!hasUnmasked))
    Logger("LoadCalFile").warning() << "'" << calFileName << "' masks all spectra\n";
}

/**
 * Used to determine if a given detector ID is for a monitor.
 *
 * @param inst Pointer to the instrument
 * @param detID Detector ID to check
 * @return True if a monitor, false otherwise
 */
bool LoadCalFile::idIsMonitor(const Instrument_const_sptr &inst, int detID) {
  auto monitorList = inst->getMonitors();
  auto it = std::find(monitorList.begin(), monitorList.end(), detID);
  return (it != monitorList.end());
}
} // namespace Mantid::DataHandling
