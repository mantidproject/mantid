// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataHandling/LoadVulcanCalFile.h"
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidDataObjects/GroupingWorkspace.h"
#include "MantidDataObjects/MaskWorkspace.h"
#include "MantidDataObjects/OffsetsWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/OptionalBool.h"
#include "MantidKernel/System.h"
#include <Poco/Path.h>
#include <fstream>

#include <fstream>
#include <sstream>

#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/trim.hpp>

using Mantid::Geometry::Instrument_const_sptr;
using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::DataObjects;

namespace Mantid {
namespace DataHandling {
// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(LoadVulcanCalFile)

// Number of detectors per module/bank
const size_t NUMBERDETECTORPERMODULE = 1232;
// Number of reserved detectors per module/bank
const size_t NUMBERRESERVEDPERMODULE = 1250;

/** Initialize the algorithm's properties.
 */
void LoadVulcanCalFile::init() {
  // LoadVulcanCalFile::getInstrument3WaysInit(this);

  declareProperty(std::make_unique<FileProperty>("OffsetFilename", "",
                                                    FileProperty::Load, ".dat"),
                  "Path to the VULCAN offset file. ");

  std::array<std::string, 3> groupoptions = {{"6Modules", "2Banks", "1Bank"}};

  declareProperty(
      "Grouping", "6Modules",
      boost::make_shared<ListValidator<std::string>>(groupoptions),
      "Choices to output group workspace for 1 bank, 2 banks or 6 modules. ");

  declareProperty(std::make_unique<FileProperty>("BadPixelFilename", "",
                                                    FileProperty::OptionalLoad,
                                                    ".dat"),
                  "Path to the VULCAN bad pixel file. ");

  declareProperty(
      std::make_unique<PropertyWithValue<std::string>>("WorkspaceName", "",
                                                          Direction::Input),
      "The base of the output workspace names. Names will have '_group', "
      "'_offsets', '_mask' appended to them.");

  // Effective geometry: bank IDs
  declareProperty(std::make_unique<ArrayProperty<int>>("BankIDs"),
                  "Bank IDs for the effective detectors. "
                  "Must cover all banks in the definition. ");

  // Effective geometry: DIFCs
  declareProperty(std::make_unique<ArrayProperty<double>>("EffectiveDIFCs"),
                  "DIFCs for effective detectors. ");

  // Effective geometry: 2theta
  declareProperty(
      std::make_unique<ArrayProperty<double>>("Effective2Thetas"),
      "2 thetas for effective detectors. ");

  // This is the property for testing purpose only!
  declareProperty(
      std::make_unique<WorkspaceProperty<EventWorkspace>>(
          "EventWorkspace", "", Direction::InOut, PropertyMode::Optional),
      "Optional input/output EventWorkspace to get aligned by offset file. "
      "It serves as a verifying tool, and will be removed after test. ");
}

/** Execute the algorithm.
 */
void LoadVulcanCalFile::exec() {
  // Process input properties
  processInOutProperites();

  // Grouping workspace
  setupGroupingWorkspace();

  // Mask workspace
  setupMaskWorkspace();

  // Genreate Offset workspace
  generateOffsetsWorkspace();

  // Output
  if (m_doAlignEventWS)
    setProperty("EventWorkspace", m_eventWS);
}

/** Process input and output
 */
void LoadVulcanCalFile::processInOutProperites() {
  // Input
  m_offsetFilename = getPropertyValue("OffsetFilename");
  m_badPixFilename = getPropertyValue("BadPixelFilename");

  std::string WorkspaceName = getPropertyValue("WorkspaceName");
  if (WorkspaceName.empty())
    throw std::invalid_argument("Must specify WorkspaceName.");

  // Get intrument
  m_instrument = getInstrument();

  // Grouping
  std::string grouptypestr = getPropertyValue("Grouping");
  size_t numeffbanks = 6;
  if (grouptypestr == "6Modules") {
    m_groupingType = VULCAN_OFFSET_BANK;
  } else if (grouptypestr == "2Banks") {
    m_groupingType = VULCAN_OFFSET_MODULE;
  } else if (grouptypestr == "1Bank") {
    m_groupingType = VULCAN_OFFSET_STACK;
  } else {
    std::stringstream ess;
    ess << "Group type " << grouptypestr << " is not supported. ";
    throw std::runtime_error(ess.str());
  }

  // Effective L and 2thetas
  std::vector<int> vec_bankids = getProperty("BankIDs");
  std::vector<double> vec_difcs = getProperty("EffectiveDIFCs");
  std::vector<double> vec_2thetas = getProperty("Effective2Thetas");
  if (vec_bankids.size() != numeffbanks || vec_difcs.size() != numeffbanks ||
      vec_2thetas.size() != numeffbanks) {
    std::stringstream ess;
    ess << "Number of items of BankIDs (" << vec_bankids.size()
        << "), EffectiveDIFCs (" << vec_difcs.size()
        << ") and Effective2Thetas (" << vec_2thetas.size() << ") must be "
        << numeffbanks << " in mode '" << grouptypestr << "'! ";
    throw std::runtime_error(ess.str());
  }

  for (size_t i = 0; i < numeffbanks; ++i) {
    int bankid = vec_bankids[i];
    double difc = vec_difcs[i];
    double theta = 0.5 * vec_2thetas[i];
    double effl = difc / (252.777 * 2.0 * sin(theta / 180. * M_PI));

    m_effLTheta.emplace(bankid, std::make_pair(effl, theta));
  }

  // Create offset workspace
  std::string title = Poco::Path(m_offsetFilename).getFileName();
  m_tofOffsetsWS = OffsetsWorkspace_sptr(new OffsetsWorkspace(m_instrument));
  m_offsetsWS = OffsetsWorkspace_sptr(new OffsetsWorkspace(m_instrument));
  m_offsetsWS->setTitle(title);

  // Create mask workspace/bad pixel
  std::string masktitle = Poco::Path(m_badPixFilename).getFileName();
  m_maskWS = boost::make_shared<MaskWorkspace>(m_instrument);
  m_maskWS->setTitle(masktitle);

  // Set properties for these file
  m_offsetsWS->mutableRun().addProperty("Filename", m_offsetFilename);

  declareProperty(std::make_unique<WorkspaceProperty<OffsetsWorkspace>>(
                      "OutputOffsetsWorkspace", WorkspaceName + "_offsets",
                      Direction::Output),
                  "Set the the output OffsetsWorkspace. ");
  setProperty("OutputOffsetsWorkspace", m_offsetsWS);

  m_tofOffsetsWS->mutableRun().addProperty("Filename", m_offsetFilename);
  declareProperty(std::make_unique<WorkspaceProperty<OffsetsWorkspace>>(
                      "OutputTOFOffsetsWorkspace",
                      WorkspaceName + "_TOF_offsets", Direction::Output),
                  "Set the the (TOF) output OffsetsWorkspace. ");
  setProperty("OutputTOFOffsetsWorkspace", m_tofOffsetsWS);

  // mask workspace
  declareProperty(
      std::make_unique<WorkspaceProperty<MaskWorkspace>>(
          "OutputMaskWorkspace", WorkspaceName + "_mask", Direction::Output),
      "Set the output MaskWorkspace. ");
  m_maskWS->mutableRun().addProperty("Filename", m_badPixFilename);
  setProperty("OutputMaskWorkspace", m_maskWS);

  // Extra event workspace as a verification tool
  m_eventWS = getProperty("EventWorkspace");
  if (m_eventWS)
    m_doAlignEventWS = true;
  else
    m_doAlignEventWS = false;
}

/** Set up grouping workspace
 */
void LoadVulcanCalFile::setupGroupingWorkspace() {
  // Get the right group option for CreateGroupingWorkspace
  std::string groupdetby;
  switch (m_groupingType) {
  case VULCAN_OFFSET_BANK:
    groupdetby = "bank";
    break;

  case VULCAN_OFFSET_MODULE:
    groupdetby = "Group";
    break;

  case VULCAN_OFFSET_STACK:
    groupdetby = "All";
    break;

  default:
    throw std::runtime_error("Grouping type is not supported. ");
    break;
  }

  // Calling algorithm CreateGroupingWorkspace
  IAlgorithm_sptr creategroupws =
      createChildAlgorithm("CreateGroupingWorkspace", -1, -1, true);
  creategroupws->initialize();

  creategroupws->setProperty("InstrumentName", "VULCAN");
  creategroupws->setProperty("GroupDetectorsBy", groupdetby);

  creategroupws->execute();
  if (!creategroupws->isExecuted())
    throw std::runtime_error("Unable to create grouping workspace.");

  m_groupWS = creategroupws->getProperty("OutputWorkspace");

  // Set title
  m_groupWS->setTitle(groupdetby);

  // Output
  std::string WorkspaceName = getPropertyValue("WorkspaceName");
  declareProperty(std::make_unique<WorkspaceProperty<GroupingWorkspace>>(
                      "OutputGroupingWorkspace", WorkspaceName + "_group",
                      Direction::Output),
                  "Set the output GroupingWorkspace. ");
  m_groupWS->mutableRun().addProperty("Filename", m_offsetFilename);
  setProperty("OutputGroupingWorkspace", m_groupWS);
}

/** Set up masking workspace
 */
void LoadVulcanCalFile::setupMaskWorkspace() {

  // Skip if bad pixel file is not given
  if (m_badPixFilename.empty())
    return;

  // Open file
  std::ifstream maskss(m_badPixFilename.c_str());
  if (!maskss.is_open()) {
    g_log.warning("Bad pixel file cannot be read.");
    return;
  }

  std::string line;
  while (std::getline(maskss, line)) {
    boost::algorithm::trim(line);
    if (!line.empty()) {
      // Get the bad pixel's detector ID.  One per line
      std::stringstream liness(line);
      try {
        int pixelid;
        liness >> pixelid;

        // Set mask
        m_maskWS->setValue(pixelid, 1.0);
      } catch (const std::invalid_argument &e) {
        g_log.debug() << "Unable to parse line " << line
                      << ".  Error message: " << e.what() << "\n";
        continue;
      }
    }
  }
  maskss.close();

  // Mask workspace index
  std::ostringstream msg;
  auto &spectrumInfo = m_maskWS->mutableSpectrumInfo();
  for (size_t i = 0; i < m_maskWS->getNumberHistograms(); ++i) {
    if (m_maskWS->y(i)[0] > 0.5) {
      m_maskWS->getSpectrum(i).clearData();
      spectrumInfo.setMasked(i, true);
      m_maskWS->mutableY(i)[0] = 1.0;
      msg << "Spectrum " << i << " is masked. DataY = " << m_maskWS->y(i)[0]
          << "\n";
    }
  }
  g_log.information(msg.str());
}

/** Generate offset workspace
 */
void LoadVulcanCalFile::generateOffsetsWorkspace() {
  std::map<detid_t, double> map_detoffset;

  // Read offset file
  readOffsetFile(map_detoffset);

  // Generate map among PIDs, workspace indexes and offsets
  processOffsets(map_detoffset);

  // Convert the input EventWorkspace if necessary
  if (m_doAlignEventWS)
    alignEventWorkspace();

  // Convert to Mantid offset values
  convertOffsets();
}

/** Read VULCAN's offset file
 */
void LoadVulcanCalFile::readOffsetFile(
    std::map<detid_t, double> &map_detoffset) {
  // Read file
  std::ifstream infile(m_offsetFilename.c_str());
  if (!infile.is_open()) {
    std::stringstream errss;
    errss << "Input offset file " << m_offsetFilename << " cannot be opened.";
    throw std::runtime_error(errss.str());
  }

  std::string line;
  while (std::getline(infile, line)) {
    std::istringstream iss(line);
    int pid;
    double offset;
    if (!(iss >> pid >> offset))
      continue;
    detid_t detid = static_cast<detid_t>(pid);
    map_detoffset.emplace(detid, offset);
  }
}

/** Process offsets by generating maps
 * Output: Offset workspace : 10^(xi_0 + xi_1 + xi_2)
 */
void LoadVulcanCalFile::processOffsets(
    std::map<detid_t, double> map_detoffset) {
  size_t numspec = m_tofOffsetsWS->getNumberHistograms();

  const auto &spectrumInfo = m_tofOffsetsWS->spectrumInfo();

  // Map from Mantid instrument to VULCAN offset
  std::map<detid_t, size_t> map_det2index;
  for (size_t i = 0; i < numspec; ++i) {
    detid_t tmpid = spectrumInfo.detector(i).getID();

    // Map between detector ID and workspace index
    map_det2index.emplace(tmpid, i);
  }

  // Map from VULCAN offset to Mantid instrument: Validate
  std::set<int> set_bankID;
  std::map<detid_t, std::pair<bool, int>>
      map_verify; // key: detector ID, value: flag to have a match, bank ID
  for (auto &miter : map_detoffset) {
    detid_t pid = miter.first;
    auto fiter = map_det2index.find(pid);
    if (fiter == map_det2index.end()) {
      map_verify.emplace(pid, std::make_pair(false, -1));
    } else {
      size_t wsindex = fiter->second;
      // Get bank ID from instrument tree
      const auto &det = spectrumInfo.detector(wsindex);
      Geometry::IComponent_const_sptr parent = det.getParent();
      std::string pname = parent->getName();

      std::vector<std::string> terms;
      boost::split(terms, pname, boost::is_any_of("("));
      std::vector<std::string> terms2;
      boost::split(terms2, terms[0], boost::is_any_of("bank"));
      int bank = std::stoi(terms2.back());
      set_bankID.insert(bank);

      map_verify.emplace(pid, std::make_pair(true, bank));
    }
  }

  // Verify
  static const size_t arr[] = {21, 22, 23, 26, 27, 28};
  std::vector<size_t> vec_banks(arr, arr + sizeof(arr) / sizeof(arr[0]));

  for (auto bankindex : vec_banks) {
    for (size_t j = 0; j < NUMBERDETECTORPERMODULE; ++j) {
      detid_t detindex =
          static_cast<detid_t>(bankindex * NUMBERRESERVEDPERMODULE + j);
      auto miter = map_verify.find(detindex);
      if (miter == map_verify.end())
        throw std::runtime_error("It cannot happen!");
      bool exist = miter->second.first;
      int tmpbank = miter->second.second;
      if (!exist)
        throw std::runtime_error(
            "Define VULCAN offset pixel is not defined in Mantid.");
      if (tmpbank != static_cast<int>(bankindex))
        throw std::runtime_error("Bank ID does not match!");
    }
  }

  // Get the global correction
  g_log.information() << "Number of bankds to process = " << set_bankID.size()
                      << "\n";
  std::map<int, double> map_bankLogCorr;
  for (const auto bankid : set_bankID) {
    // Locate inter bank and inter pack correction (log)
    double globalfactor = 0.;

    // Inter-bank correction
    if (m_groupingType != VULCAN_OFFSET_BANK) {
      detid_t interbank_detid =
          static_cast<detid_t>((bankid + 1) * NUMBERRESERVEDPERMODULE) - 2;

      g_log.information() << "Find inter-bank correction for bank " << bankid
                          << " for special detid " << interbank_detid << ".\n";

      const auto offsetiter = map_detoffset.find(interbank_detid);
      if (offsetiter == map_detoffset.end())
        throw std::runtime_error("It cannot happen!");
      double interbanklogcorr = offsetiter->second;

      globalfactor += interbanklogcorr;
    }

    // Inter-module correction
    if (m_groupingType == VULCAN_OFFSET_STACK) {
      g_log.information() << "Find inter-module correction for bank " << bankid
                          << ".\n";

      detid_t intermodule_detid =
          static_cast<detid_t>((bankid + 1) * NUMBERRESERVEDPERMODULE) - 1;
      const auto offsetiter = map_detoffset.find(intermodule_detid);
      if (offsetiter == map_detoffset.end())
        throw std::runtime_error("It cannot happen!");
      double intermodulelogcorr = offsetiter->second;

      globalfactor += intermodulelogcorr;
    }

    map_bankLogCorr.emplace(bankid, globalfactor);
  }

  // Calcualte the offset for each detector (log now still)
  std::map<detid_t, double>::iterator offsetiter;
  std::map<int, double>::iterator bankcorriter;
  for (size_t iws = 0; iws < numspec; ++iws) {
    detid_t detid = spectrumInfo.detector(iws).getID();
    offsetiter = map_detoffset.find(detid);
    if (offsetiter == map_detoffset.end())
      throw std::runtime_error("It cannot happen!");

    int bankid =
        static_cast<int>(detid) / static_cast<int>(NUMBERRESERVEDPERMODULE);
    bankcorriter = map_bankLogCorr.find(bankid);
    if (bankcorriter == map_bankLogCorr.end())
      throw std::runtime_error("It cannot happen!");

    double offset = offsetiter->second + bankcorriter->second;
    m_tofOffsetsWS->mutableY(iws)[0] = pow(10., offset);
  }
}

/** Align the input EventWorkspace
 */
void LoadVulcanCalFile::alignEventWorkspace() {
  g_log.notice("Align input EventWorkspace.");

  size_t numberOfSpectra = m_eventWS->getNumberHistograms();
  if (numberOfSpectra != m_tofOffsetsWS->getNumberHistograms())
    throw std::runtime_error("Number of histograms are different!");

  PARALLEL_FOR_NO_WSP_CHECK()
  for (int64_t i = 0; i < int64_t(numberOfSpectra); ++i) {
    PARALLEL_START_INTERUPT_REGION

    // Compute the conversion factor
    double factor = m_tofOffsetsWS->y(i)[0];

    // Perform the multiplication on all events
    m_eventWS->getSpectrum(i).convertTof(1. / factor);

    PARALLEL_END_INTERUPT_REGION
  }
  PARALLEL_CHECK_INTERUPT_REGION
}

/** Translate the VULCAN's offset to Mantid
 * Input Offset workspace : 10^(xi_0 + xi_1 + xi_2)
 *
 * This is the rigorous way to calcualte 2theta
 * detPos -= samplePos;
 * l2 = detPos.norm();
 * halfcosTwoTheta = detPos.scalar_prod(beamline)/(l2*beamline_norm);
 * sinTheta=sqrt(0.5-halfcosTwoTheta);
 *
 * 6 pixels in each bank to be focussed on
 * // (detid == 26870 || detid == 28120 || detid == 29370 ||
 * //  detid == 33120 || detid == 34370 || detid == 35620)
 *
 */
void LoadVulcanCalFile::convertOffsets() {
  size_t numspec = m_tofOffsetsWS->getNumberHistograms();

  // Instrument parameters
  const auto &spectrumInfo = m_tofOffsetsWS->spectrumInfo();
  double l1 = spectrumInfo.l1();

  std::map<int, std::pair<double, double>>::iterator mfiter;
  for (size_t iws = 0; iws < numspec; ++iws) {
    // Get detector's information including bank belonged to and geometry
    // parameters

    detid_t detid = spectrumInfo.detector(iws).getID();
    int bankid = detid / static_cast<int>(NUMBERRESERVEDPERMODULE);

    double l2 = spectrumInfo.l2(iws);
    double twotheta = spectrumInfo.twoTheta(iws);

    // Get effective
    mfiter = m_effLTheta.find(bankid);
    if (mfiter == m_effLTheta.end())
      throw std::runtime_error(
          "Effective DIFC and 2theta information is missed. ");

    double effL = mfiter->second.first;
    double effTheta = mfiter->second.second;
    double totL = l1 + l2;

    // Calcualte converted offset
    double vuloffset = m_tofOffsetsWS->y(iws)[0];
    double manoffset = (totL * sin(twotheta * 0.5)) /
                           (effL * sin(effTheta * M_PI / 180.)) / vuloffset -
                       1.;
    m_offsetsWS->mutableY(iws)[0] = manoffset;
  }
}

/** Get a pointer to an instrument in one of 3 ways: InputWorkspace,
 * InstrumentName, InstrumentFilename
 */
Geometry::Instrument_const_sptr LoadVulcanCalFile::getInstrument() {
  // Set up name
  std::string InstrumentName("VULCAN");

  // Get the instrument
  Instrument_const_sptr inst;

  Algorithm_sptr childAlg = createChildAlgorithm("LoadInstrument", 0.0, 0.2);
  MatrixWorkspace_sptr tempWS = boost::make_shared<Workspace2D>();
  childAlg->setProperty<MatrixWorkspace_sptr>("Workspace", tempWS);
  childAlg->setPropertyValue("InstrumentName", InstrumentName);
  childAlg->setProperty("RewriteSpectraMap",
                        Mantid::Kernel::OptionalBool(false));
  childAlg->executeAsChildAlg();
  inst = tempWS->getInstrument();

  return inst;
}

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
void LoadVulcanCalFile::readCalFile(const std::string &calFileName,
                                    GroupingWorkspace_sptr groupWS,
                                    OffsetsWorkspace_sptr offsetsWS,
                                    MaskWorkspace_sptr maskWS) {
  bool doGroup = bool(groupWS);
  bool doOffsets = bool(offsetsWS);
  bool doMask = bool(maskWS);

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
        msg << "Encountered offset = " << offset << " at index " << n
            << " for udet = " << udet << ". Offsets must be greater than -1.";
        throw std::runtime_error(msg.str());
      }

      try {
        offsetsWS->setValue(udet, offset);
      } catch (std::invalid_argument &) {
        numErrors++;
      }
    }

    if (doGroup) {
      try {
        groupWS->setValue(udet, double(group));
        if ((!hasGrouped) && (group > 0))
          hasGrouped = true;
      } catch (std::invalid_argument &) {
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
        // Could not find the UDET.
        numErrors++;
      }
    }
  }

  // Warn about any errors

  if (numErrors > 0)
    Logger("LoadVulcanCalFile").warning()
        << numErrors
        << " errors (invalid Detector ID's) found when reading .cal file '"
        << calFileName << "'.\n";
  if (doGroup && (!hasGrouped))
    Logger("LoadVulcanCalFile").warning()
        << "'" << calFileName << "' has no spectra grouped\n";
  if (doMask && (!hasUnmasked))
    Logger("LoadVulcanCalFile").warning()
        << "'" << calFileName << "' masks all spectra\n";
}

} // namespace DataHandling
} // namespace Mantid
