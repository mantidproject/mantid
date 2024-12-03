// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/BinEdgeAxis.h"
#include "MantidAPI/MatrixWorkspaceMDIterator.h"
#include "MantidAPI/NumericAxis.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/Sample.h"
#include "MantidAPI/SpectraAxis.h"
#include "MantidAPI/SpectrumDetectorMapping.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/Detector.h"
#include "MantidGeometry/Instrument/DetectorGroup.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidGeometry/Instrument/ReferenceFrame.h"
#include "MantidGeometry/MDGeometry/GeneralFrame.h"
#include "MantidGeometry/MDGeometry/MDFrame.h"
#include "MantidIndexing/GlobalSpectrumIndex.h"
#include "MantidIndexing/IndexInfo.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/MDUnit.h"
#include "MantidKernel/MultiThreaded.h"
#include "MantidKernel/Strings.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidKernel/VectorHelper.h"

#include "MantidTypes/SpectrumDefinition.h"

#include <boost/algorithm/string/join.hpp>
#include <boost/algorithm/string/regex.hpp>

#include <cmath>
#include <functional>
#include <numeric>
#include <utility>

using Mantid::Kernel::StringListValidator;
using Mantid::Kernel::TimeSeriesProperty;
using Mantid::Types::Core::DateAndTime;

namespace {
/** Simple method which will accumulate a value as long as it is Finite */
auto accumulate_if_finite = [](const double accumulator, const double newValue) {
  if (std::isfinite(newValue)) {
    return accumulator + newValue;
  } else {
    return accumulator;
  }
};
} // namespace

namespace Mantid::API {
using std::size_t;
using namespace Geometry;
using Kernel::V3D;

namespace {
/// static logger
Kernel::Logger g_log("MatrixWorkspace");
constexpr const double EPSILON{1.0e-9};

/** Append the x-unit of the workspace to the y-unit label as a denominator
 * E.g. if a workspace has y-unit label "Counts" and x-unit angstrom, the y-unit
 * label becomes "Counts per angstrom". Or if useLatex is true "Counts per
 * $\AA$"
 * @param yLabel :: The y-axis label
 * @param workspace :: The workspace
 * @param useLatex :: Boolean, if true use latex else use ascii
 */
std::string appendUnitDenominatorUsingPer(std::string yLabel, const MatrixWorkspace &workspace, bool useLatex) {
  if (useLatex) {
    std::string xLabel = workspace.getAxis(0)->unit()->label().latex();
    if (!xLabel.empty())
      yLabel += " per $" + workspace.getAxis(0)->unit()->label().latex() + "$";
  } else {
    std::string xLabel = workspace.getAxis(0)->unit()->label().ascii();
    if (!xLabel.empty())
      yLabel += " per " + workspace.getAxis(0)->unit()->label().ascii();
  }
  return yLabel;
}

/** Splits a string on the word "per" and replaces it with a Latex equivalent.
 * E.g. "Counts per meV per hour" becomes "Counts (meV hour)$^{-1}$"
 * If no "per"s are present leave the string as it is.
 * @param yLabel :: The y-axis label
 * @return std::string
 */
std::string replacePerWithLatex(std::string yLabel) {
  std::vector<std::string> splitVec;
  boost::split_regex(splitVec, yLabel, boost::regex(" per "));
  if (splitVec.size() > 1) {
    yLabel = splitVec[0];
    splitVec.erase(splitVec.begin());
    std::string unitString = boost::algorithm::join(splitVec, " ");
    if (!yLabel.empty())
      yLabel += " ";
    yLabel += "(" + unitString + ")$^{-1}$";
  }
  return yLabel;
}

} // namespace
const std::string MatrixWorkspace::xDimensionId = "xDimension";
const std::string MatrixWorkspace::yDimensionId = "yDimension";

/// Default constructor
MatrixWorkspace::MatrixWorkspace()
    : IMDWorkspace(), ExperimentInfo(), m_axes(), m_isInitialized(false), m_YUnit(), m_YUnitLabel(), m_masks() {}

MatrixWorkspace::MatrixWorkspace(const MatrixWorkspace &other)
    : IMDWorkspace(other), ExperimentInfo(other), m_indexInfo(std::make_unique<Indexing::IndexInfo>(other.indexInfo())),
      m_isInitialized(other.m_isInitialized), m_YUnit(other.m_YUnit), m_YUnitLabel(other.m_YUnitLabel),
      m_plotType(other.m_plotType), m_masks(other.m_masks), m_indexInfoNeedsUpdate(false) {
  m_axes.resize(other.m_axes.size());
  for (size_t i = 0; i < m_axes.size(); ++i)
    m_axes[i] = std::unique_ptr<Axis>(other.m_axes[i]->clone(this));
  m_isCommonBinsFlag.store(other.m_isCommonBinsFlag.load());
  m_isCommonBinsFlagValid.store(other.m_isCommonBinsFlagValid.load());
  // TODO: Do we need to init m_monitorWorkspace?
}

/// Destructor
// RJT, 3/10/07: The Analysis Data Service needs to be able to delete
// workspaces, so I moved this from protected to public.
MatrixWorkspace::~MatrixWorkspace() = default;

/** Returns a const reference to the IndexInfo object of the workspace.
 *
 * Used for access to spectrum number and detector ID information of spectra.
 * Writing spectrum number or detector ID groupings of any spectrum in the
 * workspace will invalidate this reference. */
const Indexing::IndexInfo &MatrixWorkspace::indexInfo() const {
  std::lock_guard<std::mutex> lock(m_indexInfoMutex);
  // Individual SpectrumDefinitions in SpectrumInfo may have changed. Due to a
  // copy-on-write mechanism the definitions stored in IndexInfo may then be out
  // of sync (definitions in SpectrumInfo have been updated).
  m_indexInfo->setSpectrumDefinitions(spectrumInfo().sharedSpectrumDefinitions());
  // If spectrum numbers are set in ISpectrum this flag will be true
  if (m_indexInfoNeedsUpdate) {
    std::vector<Indexing::SpectrumNumber> spectrumNumbers;
    for (size_t i = 0; i < getNumberHistograms(); ++i)
      spectrumNumbers.emplace_back(getSpectrum(i).getSpectrumNo());
    m_indexInfo->setSpectrumNumbers(std::move(spectrumNumbers));
    m_indexInfoNeedsUpdate = false;
  }
  return *m_indexInfo;
}

/** Sets the IndexInfo object of the workspace.
 *
 * Used for setting spectrum number and detector ID information of spectra */
void MatrixWorkspace::setIndexInfo(const Indexing::IndexInfo &indexInfo) {
  // Comparing the *local* size of the indexInfo.
  if (indexInfo.size() != getNumberHistograms())
    throw std::invalid_argument("MatrixWorkspace::setIndexInfo: IndexInfo size "
                                "does not match number of histograms in "
                                "workspace");

  m_indexInfo = std::make_unique<Indexing::IndexInfo>(indexInfo);
  m_indexInfoNeedsUpdate = false;
  if (!m_indexInfo->spectrumDefinitions())
    buildDefaultSpectrumDefinitions();
  // Fails if spectrum definitions contain invalid indices.
  rebuildDetectorIDGroupings();
  // This sets the SpectrumDefinitions for the SpectrumInfo, which may seem
  // counterintuitive at first -- why would setting IndexInfo modify internals
  // of SpectrumInfo? However, logically it would not make sense to assign
  // SpectrumDefinitions in an assignment of SpectrumInfo: Changing
  // SpectrumDefinitions requires also changes at a higher level of a workspace
  // (in particular the histograms, which would need to be regrouped as well).
  // Thus, assignment of SpectrumInfo should just check for compatible
  // SpectrumDefinitions and assign other data (such as per-spectrum masking
  // flags, which do not exist yet). Furthermore, since currently detector
  // groupings are still stored in ISpectrum (in addition to the
  // SpectrumDefinitions in SpectrumInfo), an assigment of SpectrumDefinitions
  // in SpectrumInfo would lead to inconsistent workspaces. SpectrumDefinitions
  // are thus assigned by IndexInfo, which acts at a highler level and is
  // typically used at construction time of a workspace, i.e., there is no data
  // in histograms yet which would need to be regrouped.
  setSpectrumDefinitions(m_indexInfo->spectrumDefinitions());
}

/// Variant of setIndexInfo, used by WorkspaceFactoryImpl.
void MatrixWorkspace::setIndexInfoWithoutISpectrumUpdate(const Indexing::IndexInfo &indexInfo) {
  // Comparing the *local* size of the indexInfo.
  if (indexInfo.size() != getNumberHistograms())
    throw std::invalid_argument("MatrixWorkspace::setIndexInfo: IndexInfo size "
                                "does not match number of histograms in "
                                "workspace");
  *m_indexInfo = indexInfo;
  m_indexInfoNeedsUpdate = false;
  setSpectrumDefinitions(m_indexInfo->spectrumDefinitions());
}

/// @returns A human-readable string of the current state
const std::string MatrixWorkspace::toString() const {
  std::ostringstream os;
  os << id() << "\n"
     << "Title: " << getTitle() << "\n"
     << "Histograms: " << getNumberHistograms() << "\n"
     << "Bins: ";

  try {
    os << blocksize() << "\n";
  } catch (std::length_error &) {
    os << "variable\n"; // TODO shouldn't use try/catch
  }

  if (isHistogramData())
    os << "Histogram\n";
  else
    os << "Data points\n";

  os << "X axis: ";
  if (axes() > 0) {
    Axis *ax = getAxis(0);
    if (ax && ax->unit())
      os << ax->unit()->caption() << " / " << ax->unit()->label().ascii();
    else
      os << "Not set";
  } else {
    os << "N/A";
  }
  os << "\n"
     << "Y axis: " << YUnitLabel() << "\n";

  os << "Distribution: " << (isDistribution() ? "True" : "False") << "\n";

  os << ExperimentInfo::toString();
  return os.str();
}

/** Initialize the workspace. Calls the protected init() method, which is
 * implemented in each type of
 *  workspace. Returns immediately if the workspace is already initialized.
 *  @param NVectors :: The number of spectra in the workspace (only relevant for
 * a 2D workspace
 *  @param XLength :: The number of X data points/bin boundaries in each vector
 * (must all be the same)
 *  @param YLength :: The number of data/error points in each vector (must all
 * be the same)
 */
void MatrixWorkspace::initialize(const std::size_t &NVectors, const std::size_t &XLength, const std::size_t &YLength) {
  // Check validity of arguments
  if (NVectors == 0 || XLength == 0 || YLength == 0) {
    throw std::out_of_range("All arguments to init must be positive and non-zero");
  }

  // Bypass the initialization if the workspace has already been initialized.
  if (m_isInitialized)
    return;

  setNumberOfDetectorGroups(NVectors);
  m_indexInfo = std::make_unique<Indexing::IndexInfo>(NVectors);

  // Invoke init() method of the derived class inside a try/catch clause
  try {
    this->init(NVectors, XLength, YLength);
  } catch (std::runtime_error &) {
    throw;
  }

  // Indicate that this workspace has been initialized to prevent duplicate
  // attempts.
  m_isInitialized = true;
}

void MatrixWorkspace::initialize(const std::size_t &NVectors, const HistogramData::Histogram &histogram) {
  Indexing::IndexInfo indices(NVectors);
  // Empty SpectrumDefinitions to indicate no default mapping to detectors.
  indices.setSpectrumDefinitions(std::vector<SpectrumDefinition>(NVectors));
  return initialize(indices, histogram);
}

void MatrixWorkspace::initialize(const Indexing::IndexInfo &indexInfo, const HistogramData::Histogram &histogram) {
  // Check validity of arguments
  if (indexInfo.size() == 0 || histogram.x().empty()) {
    throw std::out_of_range("All arguments to init must be positive and non-zero");
  }

  // Bypass the initialization if the workspace has already been initialized.
  if (m_isInitialized)
    return;
  setNumberOfDetectorGroups(indexInfo.size());
  init(histogram);
  setIndexInfo(indexInfo);

  // Indicate that this workspace has been initialized to prevent duplicate
  // attempts.
  m_isInitialized = true;
}

//---------------------------------------------------------------------------------------
/** Set the title of the workspace
 *
 *  @param t :: The title
 */
void MatrixWorkspace::setTitle(const std::string &t) {
  Workspace::setTitle(t);

  // A MatrixWorkspace contains uniquely one Run object, hence for this
  // workspace
  // keep the Run object run_title property the same as the workspace title
  Run &run = mutableRun();
  run.addProperty("run_title", t, true);
}

/** Get the workspace title
 *
 *  @return The title
 */
const std::string MatrixWorkspace::getTitle() const {
  if (run().hasProperty("run_title")) {
    std::string title = run().getProperty("run_title")->value();
    return title;
  } else
    return Workspace::getTitle();
}

/** Set the plot type of the workspace
 *
 * @param t :: The plot type. Must be one of: ["plot", "scatter", "histogram", "errorbar"]
 */
void MatrixWorkspace::setPlotType(const std::string &t) {

  StringListValidator v(validPlotTypes);

  if (v.isValid(t) == "")
    this->m_plotType = t;
}

/** Get the plot type
 *
 * @return The plot type
 */
std::string MatrixWorkspace::getPlotType() const { return this->m_plotType; }

void MatrixWorkspace::updateSpectraUsing(const SpectrumDetectorMapping &map) {
  for (size_t j = 0; j < getNumberHistograms(); ++j) {
    auto &spec = getSpectrum(j);
    try {
      if (map.indexIsSpecNumber())
        spec.setDetectorIDs(map.getDetectorIDsForSpectrumNo(spec.getSpectrumNo()));
      else
        spec.setDetectorIDs(map.getDetectorIDsForSpectrumIndex(j));
    } catch (std::out_of_range &exception) {
      // Get here if the spectrum number is not in the map.
      spec.clearDetectorIDs();
      g_log.debug(exception.what());
      g_log.debug() << "Spectrum number " << spec.getSpectrumNo() << " not in map.\n";
    }
  }
}

/**
 * Rebuild the default spectra mapping for a workspace. If a non-empty
 * instrument is set then the default maps each detector to a spectra with
 * the same ID. If an empty instrument is set then a 1:1 map from 1->NHistograms
 * is created.
 * @param includeMonitors :: If false the monitors are not included
 * @param specNumOffset :: Constant offset from detector ID used to derive
 *                         spectrum number
 */
void MatrixWorkspace::rebuildSpectraMapping(const bool includeMonitors, const specnum_t specNumOffset) {
  if (sptr_instrument->nelements() == 0) {
    return;
  }

  std::vector<detid_t> pixelIDs = this->getInstrument()->getDetectorIDs(!includeMonitors);

  try {
    size_t index = 0;
    std::vector<detid_t>::const_iterator iend = pixelIDs.end();
    for (std::vector<detid_t>::const_iterator it = pixelIDs.begin(); it != iend; ++it) {
      // The detector ID
      const detid_t detId = *it;
      const auto specNo = specnum_t(index + specNumOffset);

      if (index < this->getNumberHistograms()) {
        auto &spec = getSpectrum(index);
        spec.setSpectrumNo(specNo);
        spec.setDetectorID(detId);
      }

      index++;
    }

  } catch (std::runtime_error &) {
    throw;
  }
}

/** Return a map where:
 *    KEY is the Spectrum #
 *    VALUE is the Workspace Index
 */
spec2index_map MatrixWorkspace::getSpectrumToWorkspaceIndexMap() const {
  auto const *ax = dynamic_cast<SpectraAxis *>(this->m_axes[1].get());
  if (!ax)
    throw std::runtime_error("MatrixWorkspace::getSpectrumToWorkspaceIndexMap: "
                             "axis[1] is not a SpectraAxis, so I cannot "
                             "generate a map.");
  try {
    return ax->getSpectraIndexMap();
  } catch (std::runtime_error &) {
    g_log.error() << "MatrixWorkspace::getSpectrumToWorkspaceIndexMap: no elements!";
    throw;
  }
}

/** Return a vector where:
 *    The index into the vector = spectrum number + offset
 *    The value at that index = the corresponding Workspace Index
 *
 *  @returns :: vector set to above definition
 *  @param offset :: add this to the detector ID to get the index into the
 *vector.
 */
std::vector<size_t> MatrixWorkspace::getSpectrumToWorkspaceIndexVector(specnum_t &offset) const {
  auto const *ax = dynamic_cast<SpectraAxis *>(this->m_axes[1].get());
  if (!ax)
    throw std::runtime_error("MatrixWorkspace::getSpectrumToWorkspaceIndexMap: "
                             "axis[1] is not a SpectraAxis, so I cannot "
                             "generate a map.");

  // Find the min/max spectra IDs
  specnum_t min = std::numeric_limits<specnum_t>::max(); // So that any number
  // will be less than this
  specnum_t max = -std::numeric_limits<specnum_t>::max(); // So that any number
  // will be greater than
  // this
  size_t length = ax->length();
  for (size_t i = 0; i < length; i++) {
    specnum_t spec = ax->spectraNo(i);
    if (spec < min)
      min = spec;
    if (spec > max)
      max = spec;
  }

  // Offset so that the "min" value goes to index 0
  offset = -min;

  // Size correctly
  std::vector<size_t> out(max - min + 1, 0);

  // Make the vector
  for (size_t i = 0; i < length; i++) {
    specnum_t spec = ax->spectraNo(i);
    out[spec + offset] = i;
  }

  return out;
}

/** Does the workspace has any grouped detectors?
 *  @return true if the workspace has any grouped detectors, otherwise false
 */
bool MatrixWorkspace::hasGroupedDetectors() const {
  bool retVal = false;

  // Loop through the workspace index
  for (size_t workspaceIndex = 0; workspaceIndex < this->getNumberHistograms(); workspaceIndex++) {
    auto detList = getSpectrum(workspaceIndex).getDetectorIDs();
    if (detList.size() > 1) {
      retVal = true;
      break;
    }
  }
  return retVal;
}

/** Return a map where:
 *    KEY is the DetectorID (pixel ID)
 *    VALUE is the Workspace Index
 *  @param throwIfMultipleDets :: set to true to make the algorithm throw an
 * error if there is more than one detector for a specific workspace index.
 *  @param ignoreIfNoValidDets :: set to true to exclude spectra that do not
 * include any valid detector IDs. Note that if any valid detector IDs exist
 * then all of the detector IDs (including the invalid ones) will be returned.
 *  @throw runtime_error if there is more than one detector per spectrum (if
 * throwIfMultipleDets is true)
 *  @return Index to Index Map object. THE CALLER TAKES OWNERSHIP OF THE MAP AND
 * IS RESPONSIBLE FOR ITS DELETION.
 */
detid2index_map MatrixWorkspace::getDetectorIDToWorkspaceIndexMap(bool throwIfMultipleDets,
                                                                  bool ignoreIfNoValidDets) const {
  detid2index_map map;
  const auto &specInfo = spectrumInfo();

  // Loop through the workspace index
  for (size_t workspaceIndex = 0; workspaceIndex < this->getNumberHistograms(); ++workspaceIndex) {
    // Workspaces can contain invalid detector IDs. hasDetectors will silently ignore them until this is fixed.
    if (ignoreIfNoValidDets && !specInfo.hasDetectors(workspaceIndex)) {
      continue;
    }
    auto detList = getSpectrum(workspaceIndex).getDetectorIDs();

    if (throwIfMultipleDets) {
      if (detList.size() > 1) {
        throw std::runtime_error("MatrixWorkspace::getDetectorIDToWorkspaceIndexMap(): more than 1 "
                                 "detector for one histogram! I cannot generate a map of detector "
                                 "ID to workspace index.");
      }

      // Set the KEY to the detector ID and the VALUE to the workspace index.
      if (detList.size() == 1)
        map[*detList.begin()] = workspaceIndex;
    } else {
      // Allow multiple detectors per workspace index
      for (auto det : detList)
        map[det] = workspaceIndex;
    }

    // Ignore if the detector list is empty.
  }

  return map;
}

/** Return a vector where:
 *    The index into the vector = DetectorID (pixel ID) + offset
 *    The value at that index = the corresponding Workspace Index
 *
 *  @param offset :: add this to the detector ID to get the index into the
 *vector.
 *  @param throwIfMultipleDets :: set to true to make the algorithm throw an
 *error if there is more than one detector for a specific workspace index.
 *  @throw runtime_error if there is more than one detector per spectrum (if
 *throwIfMultipleDets is true)
 *  @returns :: vector set to above definition
 */
std::vector<size_t> MatrixWorkspace::getDetectorIDToWorkspaceIndexVector(detid_t &offset,
                                                                         bool throwIfMultipleDets) const {
  // Make a correct initial size
  std::vector<size_t> out;
  detid_t minId = 0;
  detid_t maxId = 0;
  this->getInstrument()->getMinMaxDetectorIDs(minId, maxId);
  offset = -minId;
  const int outSize = maxId - minId + 1;
  // Allocate at once
  out.resize(outSize, std::numeric_limits<size_t>::max());

  for (size_t workspaceIndex = 0; workspaceIndex < getNumberHistograms(); ++workspaceIndex) {
    // Get the list of detectors from the WS index
    const auto &detList = this->getSpectrum(workspaceIndex).getDetectorIDs();

    if (throwIfMultipleDets && (detList.size() > 1))
      throw std::runtime_error("MatrixWorkspace::getDetectorIDToWorkspaceIndexVector(): more than 1 "
                               "detector for one histogram! I cannot generate a map of detector ID "
                               "to workspace index.");

    // Allow multiple detectors per workspace index, or,
    // If only one is allowed, then this has thrown already
    for (auto det : detList) {
      int index = det + offset;
      if (index < 0 || index >= outSize) {
        g_log.debug() << "MatrixWorkspace::getDetectorIDToWorkspaceIndexVector("
                         "): detector ID found ("
                      << det << " at workspace index " << workspaceIndex << ") is invalid.\n";
      } else
        // Save it at that point.
        out[index] = workspaceIndex;
    }

  } // (for each workspace index)
  return out;
}

/** Converts a list of spectrum numbers to the corresponding workspace indices.
 *  Not a very efficient operation, but unfortunately it's sometimes required.
 *
 *  @param spectraList :: The list of spectrum numbers required
 *  @returns :: the vector of indices (empty if not a Workspace2D)
 */
std::vector<size_t> MatrixWorkspace::getIndicesFromSpectra(const std::vector<specnum_t> &spectraList) const {
  // Clear the output index list
  std::vector<size_t> indexList;
  indexList.reserve(this->getNumberHistograms());

  auto iter = spectraList.cbegin();
  while (iter != spectraList.cend()) {
    for (size_t i = 0; i < this->getNumberHistograms(); ++i) {
      if (this->getSpectrum(i).getSpectrumNo() == *iter) {
        indexList.emplace_back(i);
        break;
      }
    }
    ++iter;
  }
  return indexList;
}

/** Given a spectrum number, find the corresponding workspace index
 *
 * @param specNo :: spectrum number wanted
 * @return the workspace index
 * @throw runtime_error if not found.
 */
size_t MatrixWorkspace::getIndexFromSpectrumNumber(const specnum_t specNo) const {
  for (size_t i = 0; i < this->getNumberHistograms(); ++i) {
    if (this->getSpectrum(i).getSpectrumNo() == specNo)
      return i;
  }
  throw std::runtime_error("Could not find spectrum number in any spectrum.");
}

/** Converts a list of detector IDs to the corresponding workspace indices.
 *
 *  Note that only known detector IDs are converted (so an empty vector will
 *be returned
 *  if none of the IDs are recognised), and that the returned workspace
 *indices are
 *  effectively a set (i.e. there are no duplicates).
 *
 *  @param detIdList :: The list of detector IDs required
 *  @returns :: a vector of indices
 */
std::vector<size_t> MatrixWorkspace::getIndicesFromDetectorIDs(const std::vector<detid_t> &detIdList) const {
  if (m_indexInfo->size() != m_indexInfo->globalSize())
    throw std::runtime_error("MatrixWorkspace: Using getIndicesFromDetectorIDs "
                             "in a parallel run is most likely incorrect. "
                             "Aborting.");

  // create a set because looking for existence of value is faster than from vector
  std::set<detid_t> detIdSet(detIdList.cbegin(), detIdList.cend());

  // create a mapping of detector number to workspace index
  std::map<detid_t, std::set<size_t>> detectorIDtoWSIndices;
  const size_t NUM_HIST = getNumberHistograms();
  for (size_t i = 0; i < NUM_HIST; ++i) {
    const auto &detIDs = getSpectrum(i).getDetectorIDs();
    for (const auto &detID : detIDs) {
      // only add things to the map that are being asked for
      if (detIdSet.count(detID) > 0) {
        detectorIDtoWSIndices[detID].insert(i);
      }
    }
  }

  // create a vector of workspace indices with the same order as the input list
  std::vector<size_t> indexList;
  indexList.reserve(detIdList.size());
  for (const auto &detId : detIdList) {
    const auto wsIndices = detectorIDtoWSIndices.find(detId);
    if (wsIndices != detectorIDtoWSIndices.end()) {
      std::copy(wsIndices->second.cbegin(), wsIndices->second.cend(), std::back_inserter(indexList));
    }
  }

  return indexList;
}

/** Converts a list of detector IDs to the corresponding spectrum numbers. Might
 *be slow!
 *
 * @param detIdList :: The list of detector IDs required
 * @returns :: a reference to the vector of spectrum numbers.
 *                       0 for not-found detectors
 */
std::vector<specnum_t> MatrixWorkspace::getSpectraFromDetectorIDs(const std::vector<detid_t> &detIdList) const {
  std::vector<specnum_t> spectraList;

  // Try every detector in the list
  for (auto detId : detIdList) {
    bool foundDet = false;
    specnum_t foundSpecNum = 0;

    // Go through every histogram
    for (size_t i = 0; i < this->getNumberHistograms(); i++) {
      if (this->getSpectrum(i).hasDetectorID(detId)) {
        foundDet = true;
        foundSpecNum = this->getSpectrum(i).getSpectrumNo();
        break;
      }
    }

    if (foundDet)
      spectraList.emplace_back(foundSpecNum);
  } // for each detector ID in the list
  return spectraList;
}

double MatrixWorkspace::getXMin() const {
  double xmin;
  double xmax;
  this->getXMinMax(xmin, xmax); // delegate to the proper code
  return xmin;
}

double MatrixWorkspace::getXMax() const {
  double xmin;
  double xmax;
  this->getXMinMax(xmin, xmax); // delegate to the proper code
  return xmax;
}

void MatrixWorkspace::getXMinMax(double &xmin, double &xmax) const {
  // set to crazy values to start
  xmin = std::numeric_limits<double>::max();
  xmax = -1.0 * xmin;
  size_t numberOfSpectra = this->getNumberHistograms();

  // determine the data range
  for (size_t workspaceIndex = 0; workspaceIndex < numberOfSpectra; workspaceIndex++) {
    const auto &xData = this->x(workspaceIndex);
    const double xfront = xData.front();
    const double xback = xData.back();
    if (std::isfinite(xfront) && std::isfinite(xback)) {
      if (xfront < xmin)
        xmin = xfront;
      if (xback > xmax)
        xmax = xback;
    }
  }
}

/** Integrate all the spectra in the matrix workspace within the range given.
 * NaN and Infinite values are ignored.
 * Default implementation, can be overridden by base classes if they know
 *something smarter!
 *
 * @param out :: returns the vector where there is one entry per spectrum in the
 *workspace. Same
 *            order as the workspace indices.
 * @param minX :: minimum X bin to use in integrating.
 * @param maxX :: maximum X bin to use in integrating.
 * @param entireRange :: set to true to use the entire range. minX and maxX are
 *then ignored!
 */
void MatrixWorkspace::getIntegratedSpectra(std::vector<double> &out, const double minX, const double maxX,
                                           const bool entireRange) const {
  out.resize(this->getNumberHistograms(), 0.0);

  // offset for histogram data, because the x axis is not the same size for histogram and point data.
  const size_t histogramOffset = this->isHistogramData() ? 1 : 0;

  // Run in parallel if the implementation is threadsafe
  PARALLEL_FOR_IF(this->threadSafe())
  for (int wksp_index = 0; wksp_index < static_cast<int>(this->getNumberHistograms()); wksp_index++) {
    // Get Handle to data
    const Mantid::MantidVec &xData = this->readX(wksp_index);
    const auto &yData = this->y(wksp_index);
    // If it is a 1D workspace, no need to integrate
    if ((xData.size() <= 1 + histogramOffset) && (!yData.empty())) {
      out[wksp_index] = yData[0];
    } else {
      // Iterators for limits - whole range by default
      Mantid::MantidVec::const_iterator lowit, highit;
      lowit = xData.begin();
      highit = xData.end() - histogramOffset;

      // But maybe we don't want the entire range?
      if (!entireRange) {
        // If the first element is lower that the xmin then search for new lowit
        if ((*lowit) < minX)
          lowit = std::lower_bound(xData.begin(), xData.end(), minX);
        // If the last element is higher that the xmax then search for new highit
        if (*(highit - 1 + histogramOffset) > maxX)
          highit = std::upper_bound(lowit, xData.end(), maxX);
      }

      // Get the range for the y vector
      Mantid::MantidVec::difference_type distmin = std::distance(xData.begin(), lowit);
      Mantid::MantidVec::difference_type distmax = std::distance(xData.begin(), highit);
      double sum(0.0);
      if (distmin <= distmax) {
        // Integrate
        sum = std::accumulate(yData.begin() + distmin, yData.begin() + distmax, 0.0, accumulate_if_finite);
      }
      // Save it in the vector
      out[wksp_index] = sum;
    }
  }
}

/** Get the effective detector for the given spectrum
*  @param  workspaceIndex The workspace index for which the detector is required
*  @return A single detector object representing the detector(s) contributing
*          to the given spectrum number. If more than one detector contributes
then
*          the returned object's concrete type will be DetectorGroup.
*  @throw  Kernel::Exception::NotFoundError If the Instrument is missing or the
requested workspace index does not have any associated detectors
*/
Geometry::IDetector_const_sptr MatrixWorkspace::getDetector(const size_t workspaceIndex) const {
  const auto &dets = getSpectrum(workspaceIndex).getDetectorIDs();
  Instrument_const_sptr localInstrument = getInstrument();
  if (!localInstrument) {
    g_log.debug() << "No instrument defined.\n";
    throw Kernel::Exception::NotFoundError("Instrument not found", "");
  }

  const size_t ndets = dets.size();
  if (ndets == 1) {
    // If only 1 detector for the spectrum number, just return it
    return localInstrument->getDetector(*dets.begin());
  } else if (ndets == 0) {
    throw Kernel::Exception::NotFoundError("MatrixWorkspace::getDetector(): No "
                                           "detectors for this workspace "
                                           "index.",
                                           "");
  }
  // Else need to construct a DetectorGroup and return that
  auto dets_ptr = localInstrument->getDetectors(dets);
  return std::make_shared<Geometry::DetectorGroup>(dets_ptr);
}

/** Returns the signed 2Theta scattering angle for a detector
 *  @param det :: A pointer to the detector object (N.B. might be a
 * DetectorGroup)
 *  @return The scattering angle (0 < theta < pi)
 *  @throws InstrumentDefinitionError if source or sample is missing, or they
 * are in the same place
 */
double MatrixWorkspace::detectorSignedTwoTheta(const Geometry::IDetector &det) const {

  Instrument_const_sptr instrument = getInstrument();
  Geometry::IComponent_const_sptr source = instrument->getSource();
  Geometry::IComponent_const_sptr sample = instrument->getSample();
  if (source == nullptr || sample == nullptr) {
    throw Kernel::Exception::InstrumentDefinitionError(
        "Instrument not sufficiently defined: failed to get source and/or "
        "sample");
  }

  const Kernel::V3D samplePos = sample->getPos();
  const Kernel::V3D beamLine = samplePos - source->getPos();

  if (beamLine.nullVector()) {
    throw Kernel::Exception::InstrumentDefinitionError("Source and sample are at same position!");
  }
  // Get the instrument up axis.
  const V3D &thetaSignAxis = instrument->getReferenceFrame()->vecThetaSign();
  return det.getSignedTwoTheta(samplePos, beamLine, thetaSignAxis);
}

/** Returns the 2Theta scattering angle for a detector
 *  @param det :: A pointer to the detector object (N.B. might be a
 * DetectorGroup)
 *  @return The scattering angle (0 < theta < pi)
 *  @throws InstrumentDefinitionError if source or sample is missing, or they
 * are in the same place
 */
double MatrixWorkspace::detectorTwoTheta(const Geometry::IDetector &det) const {
  Instrument_const_sptr instrument = this->getInstrument();
  Geometry::IComponent_const_sptr source = instrument->getSource();
  Geometry::IComponent_const_sptr sample = instrument->getSample();
  if (source == nullptr || sample == nullptr) {
    throw Kernel::Exception::InstrumentDefinitionError(
        "Instrument not sufficiently defined: failed to get source and/or "
        "sample");
  }

  const Kernel::V3D samplePos = sample->getPos();
  const Kernel::V3D beamLine = samplePos - source->getPos();

  if (beamLine.nullVector()) {
    throw Kernel::Exception::InstrumentDefinitionError("Source and sample are at same position!");
  }
  return det.getTwoTheta(samplePos, beamLine);
}

/// @return The number of axes which this workspace has
int MatrixWorkspace::axes() const { return static_cast<int>(m_axes.size()); }

/** Get a non owning pointer to a workspace axis
 *  @param axisIndex :: The index of the axis required
 *  @throw IndexError If the argument given is outside the range of axes held by
 * this workspace
 *  @return Pointer to Axis object
 */
Axis *MatrixWorkspace::getAxis(const std::size_t &axisIndex) const {
  if (axisIndex >= m_axes.size()) {
    throw Kernel::Exception::IndexError(axisIndex, m_axes.size(), "Argument to getAxis is invalid for this workspace");
  }

  return m_axes[axisIndex].get();
}

/** Replaces one of the workspace's axes with the new one provided.
 *  @param axisIndex :: The index of the axis to replace
 *  @param newAxis :: A Unique_ptr to the new axis. The class will take
 * ownership.
 *  @throw IndexError If the axisIndex given is outside the range of axes held
 * by this workspace
 *  @throw std::runtime_error If the new axis is not of the correct length
 * (within one of the old one)
 */
void MatrixWorkspace::replaceAxis(const std::size_t &axisIndex, std::unique_ptr<Axis> newAxis) {
  // First check that axisIndex is in range
  if (axisIndex >= m_axes.size()) {
    throw Kernel::Exception::IndexError(axisIndex, m_axes.size(), "Value of axisIndex is invalid for this workspace");
  }
  // If we're OK, then delete the old axis and set the pointer to the new one
  m_axes[axisIndex] = std::move(newAxis);
}

/**
 *  Whether the workspace contains common X bins with logarithmic spacing
 *  @return whether the workspace contains common X bins with log spacing
 */
bool MatrixWorkspace::isCommonLogBins() const {
  if (!this->isCommonBins()) {
    return false;
  }

  if (this->getNumberHistograms() == 0) {
    return false;
  }

  const auto &x0 = this->x(0);
  if (x0.size() < 2) {
    return false;
  }

  // guard against all axis elements being equal
  if (x0[1] == x0[0]) {
    return false;
  }

  double diff = x0[1] / x0[0];
  if (!std::isfinite(diff)) {
    return false;
  }
  // ignore final bin, since it may be a different size
  for (size_t i = 1; i < x0.size() - 2; ++i) {
    if (std::isfinite(x0[i + 1]) && std::isfinite(x0[i])) {
      if (std::abs(x0[i + 1] / x0[i] - diff) > EPSILON) {
        return false;
      }
    } else {
      return false;
    }
  }
  return true;
}

/**
 * Return the number of Axis stored by this workspace
 * @return int
 */
size_t MatrixWorkspace::numberOfAxis() const { return m_axes.size(); }

/// Returns the units of the data in the workspace
std::string MatrixWorkspace::YUnit() const { return m_YUnit; }

/// Sets a new unit for the data (Y axis) in the workspace
void MatrixWorkspace::setYUnit(const std::string &newUnit) { m_YUnit = newUnit; }

/**
 * Returns a caption for the units of the data in the workspace.
 * @param useLatex :: Return label using Latex syntax
 * @param plotAsDistribution :: If true, the Y-axis has been divided by bin
 * width
 */
std::string MatrixWorkspace::YUnitLabel(bool useLatex /* = false */, bool plotAsDistribution /* = false */) const {
  std::string retVal;
  if (!m_YUnitLabel.empty()) {
    retVal = m_YUnitLabel;
    // If a custom label has been set and we are dividing by bin width when
    // plotting (i.e. plotAsDistribution = true and the workspace is not a
    // distribution), we must append the x-unit as a divisor. We assume the
    // custom label contains the correct units for the data.
    if (plotAsDistribution && !this->isDistribution())
      retVal = appendUnitDenominatorUsingPer(retVal, *this, useLatex);
  } else {
    retVal = m_YUnit;
    // If no custom label is set and the workspace is a distribution we need to
    // append the divisor's unit to the label. If the workspace is not a
    // distribution, but we are plotting it as a distribution, we must append
    // the divisor's unit.
    if (plotAsDistribution || this->isDistribution())
      retVal = appendUnitDenominatorUsingPer(retVal, *this, useLatex);
  }
  if (useLatex)
    retVal = replacePerWithLatex(retVal);
  return retVal;
}

/// Sets a new caption for the data (Y axis) in the workspace
void MatrixWorkspace::setYUnitLabel(const std::string &newLabel) { m_YUnitLabel = newLabel; }

/** Are the Y-values in this workspace dimensioned?
 * TODO: For example: ????
 * @return whether workspace is a distribution or not
 */
bool MatrixWorkspace::isDistribution() const {
  return getSpectrum(0).yMode() == HistogramData::Histogram::YMode::Frequencies;
}

/** Set the flag for whether the Y-values are dimensioned
 */
void MatrixWorkspace::setDistribution(bool newValue) {
  if (isDistribution() == newValue)
    return;
  HistogramData::Histogram::YMode ymode =
      newValue ? HistogramData::Histogram::YMode::Frequencies : HistogramData::Histogram::YMode::Counts;
  for (size_t i = 0; i < getNumberHistograms(); ++i)
    getSpectrum(i).setYMode(ymode);
}

/**
 *  Whether the workspace contains histogram data
 *  @return whether the workspace contains histogram data
 */
bool MatrixWorkspace::isHistogramData() const {
  // all spectra *should* have the same behavior
  return isHistogramDataByIndex(0);
}

/**
 *  Whether the specified histogram contains histogram data (ie bins)
 *  @param index :: Index of the histogram to be checked
 *  @return whether the histogram contains histogram data
 */
bool MatrixWorkspace::isHistogramDataByIndex(const std::size_t index) const {
  bool isHist = (x(index).size() != y(index).size());

  // TODOHIST temporary sanity check
  if (isHist) {
    if (getSpectrum(index).histogram().xMode() != HistogramData::Histogram::XMode::BinEdges) {
      throw std::logic_error("In MatrixWorkspace::isHistogramData(): "
                             "Histogram::Xmode is not BinEdges");
    }
  } else {
    if (getSpectrum(index).histogram().xMode() != HistogramData::Histogram::XMode::Points) {
      throw std::logic_error("In MatrixWorkspace::isHistogramData(): "
                             "Histogram::Xmode is not Points");
    }
  }
  return isHist;
}

/**
 *  Whether the workspace contains common X bins
 *  @return whether the workspace contains common X bins
 */
bool MatrixWorkspace::isCommonBins() const {
  std::lock_guard<std::mutex> lock{m_isCommonBinsMutex};
  const bool isFlagValid{m_isCommonBinsFlagValid.exchange(true)};
  if (isFlagValid) {
    return m_isCommonBinsFlag;
  }
  m_isCommonBinsFlag = true;
  const size_t numHist = this->getNumberHistograms();
  // there being only one or zero histograms is accepted as not being an error
  if (numHist <= 1) {
    return m_isCommonBinsFlag;
  }

  // First check if the x-axis shares a common ptr.
  const HistogramData::HistogramX *first = &x(0);
  for (size_t i = 1; i < numHist; ++i) {
    if (&x(i) != first) {
      m_isCommonBinsFlag = false;
      break;
    }
  }

  // If true, we may return here.
  if (m_isCommonBinsFlag) {
    return m_isCommonBinsFlag;
  }

  m_isCommonBinsFlag = true;
  // Check that that size of each histogram is identical.
  const size_t numBins = x(0).size();
  for (size_t i = 1; i < numHist; ++i) {
    if (x(i).size() != numBins) {
      m_isCommonBinsFlag = false;
      return m_isCommonBinsFlag;
    }
  }

  const auto &x0 = x(0);
  // Check that the values of each histogram are identical.
  PARALLEL_FOR_IF(this->threadSafe())
  for (int i = 1; i < static_cast<int>(numHist); ++i) {
    if (m_isCommonBinsFlag) {
      const auto specIndex = static_cast<std::size_t>(i);
      const auto &xi = x(specIndex);
      for (size_t j = 0; j < numBins; ++j) {
        const double a = x0[j];
        const double b = xi[j];
        // Check for NaN and infinity before comparing for equality
        if (std::isfinite(a) && std::isfinite(b)) {
          if (std::abs(a - b) > EPSILON) {
            m_isCommonBinsFlag = false;
            break;
          }
          // Otherwise we check that both are NaN or both are infinity
        } else if ((std::isnan(a) != std::isnan(b)) || (std::isinf(a) != std::isinf(b))) {
          m_isCommonBinsFlag = false;
          break;
        }
      }
    }
  }
  return m_isCommonBinsFlag;
}

/**
 * Whether the workspace's bins are integers - and common.
 * @return Whether the workspace's bins are integers - and common.
 **/
bool MatrixWorkspace::isIntegerBins() const {
  if (!this->isCommonBins())
    return false;

  const HistogramData::HistogramX bins = x(0);

  for (size_t i = 0; i < bins.size(); ++i) {
    if (std::trunc(bins[i]) != bins[i])
      return false;
  }
  return true;
}

/** Called by the algorithm MaskBins to mask a single bin for the first time,
 * algorithms that later propagate the
 *  the mask from an input to the output should call flagMasked() instead. Here
 * y-values and errors will be scaled
 *  by (1-weight) as well as the mask flags (m_masks) being updated. This
 * function doesn't protect the writes to the
 *  y and e-value arrays and so is not safe if called by multiple threads
 * working on the same spectrum. Writing to the mask set is marked parrallel
 * critical so different spectra can be analysised in parallel
 *  @param workspaceIndex :: The workspace index of the bin
 *  @param binIndex ::      The index of the bin in the spectrum
 *  @param weight ::        'How heavily' the bin is to be masked. =1 for full
 * masking (the default).
 */
void MatrixWorkspace::maskBin(const size_t &workspaceIndex, const size_t &binIndex, const double &weight) {
  // First check the workspaceIndex is valid
  if (workspaceIndex >= this->getNumberHistograms())
    throw Kernel::Exception::IndexError(workspaceIndex, this->getNumberHistograms(),
                                        "MatrixWorkspace::maskBin,workspaceIndex");
  // Then check the bin index
  if (binIndex >= y(workspaceIndex).size())
    throw Kernel::Exception::IndexError(binIndex, y(workspaceIndex).size(), "MatrixWorkspace::maskBin,binIndex");

  // this function is marked parallel critical
  flagMasked(workspaceIndex, binIndex, weight);

  // this is the actual result of the masking that most algorithms and plotting
  // implementations will see, the bin mask flags defined above are used by only
  // some algorithms
  // If the weight is 0, nothing more needs to be done after flagMasked above
  // (i.e. NaN and Inf will also stay intact)
  // If the weight is not 0, NaN and Inf values are set to 0,
  // whereas other values are scaled by (1 - weight)
  if (weight != 0.) {
    double &yData = this->mutableY(workspaceIndex)[binIndex];
    (std::isnan(yData) || std::isinf(yData)) ? yData = 0. : yData *= (1 - weight);
    double &eData = this->mutableE(workspaceIndex)[binIndex];
    (std::isnan(eData) || std::isinf(eData)) ? eData = 0. : eData *= (1 - weight);
  }
}

/** Writes the masking weight to m_masks (doesn't alter y-values). Contains a
 * parallel critical section
 *  and so is thread safe
 *  @param index :: The workspace index of the spectrum
 *  @param binIndex ::      The index of the bin in the spectrum
 *  @param weight ::        'How heavily' the bin is to be masked. =1 for full
 * masking (the default).
 */
void MatrixWorkspace::flagMasked(const size_t &index, const size_t &binIndex, const double &weight) {
  // Writing to m_masks is not thread-safe, so put in some protection
  PARALLEL_CRITICAL(maskBin) {
    // First get a reference to the list for this spectrum (or create a new
    // list)
    MaskList &binList = m_masks[index];
    binList[binIndex] = weight;
  }
}

/** Does this spectrum contain any masked bins
 *  @param workspaceIndex :: The workspace index to test
 *  @return True if there are masked bins for this spectrum
 */
bool MatrixWorkspace::hasMaskedBins(const size_t &workspaceIndex) const {
  // First check the workspaceIndex is valid. Return false if it isn't (decided
  // against throwing here).
  if (workspaceIndex >= this->getNumberHistograms())
    return false;
  return m_masks.find(workspaceIndex) != m_masks.end();
}

/** Does this workspace contain any masked bins
 *  @return True if there are masked bins somewhere in this workspace
 */
bool MatrixWorkspace::hasAnyMaskedBins() const { return !m_masks.empty(); }

/** Returns the list of masked bins for a spectrum.
 *  @param  workspaceIndex
 *  @return A const reference to the list of masked bins
 *  @throw  Kernel::Exception::IndexError if there are no bins masked for this
 * spectrum (so call hasMaskedBins first!)
 */
const MatrixWorkspace::MaskList &MatrixWorkspace::maskedBins(const size_t &workspaceIndex) const {
  auto it = m_masks.find(workspaceIndex);
  // Throw if there are no masked bins for this spectrum. The caller should
  // check first using hasMaskedBins!
  if (it == m_masks.end()) {
    throw Kernel::Exception::IndexError(workspaceIndex, 0, "MatrixWorkspace::maskedBins");
  }

  return it->second;
}

std::vector<size_t> MatrixWorkspace::maskedBinsIndices(const size_t &workspaceIndex) const {
  auto it = m_masks.find(workspaceIndex);
  // Throw if there are no masked bins for this spectrum. The caller should
  // check first using hasMaskedBins!
  if (it == m_masks.end()) {
    throw Kernel::Exception::IndexError(workspaceIndex, 0, "MatrixWorkspace::maskedBins");
  }

  auto maskedBinsList = it->second;
  std::vector<size_t> maskedIds;
  maskedIds.reserve(maskedBinsList.size());

  std::transform(maskedBinsList.begin(), maskedBinsList.end(), std::back_inserter(maskedIds),
                 [](const auto &mb) { return mb.first; });
  return maskedIds;
}

/** Set the list of masked bins for given workspaceIndex. Not thread safe.
 *
 * No data is masked and previous masking for any bin for this workspace index
 * is overridden, so this should only be used for copying flags into a new
 * workspace, not for performing masking operations. */
void MatrixWorkspace::setMaskedBins(const size_t workspaceIndex, const MaskList &maskedBins) {
  m_masks[workspaceIndex] = maskedBins;
}

/**
 * Removes the mask from an index. Not thread safe.
 *
 * @param workspaceIndex workspace index to be unmasked
 */
void MatrixWorkspace::setUnmaskedBins(const size_t workspaceIndex) { m_masks.erase(workspaceIndex); }

/** Sets the internal monitor workspace to the provided workspace.
 *  This method is intended for use by data-loading algorithms.
 *  Note that no checking is performed as to whether this workspace actually
 * contains data
 *  pertaining to monitors, or that the spectra point to Detector objects marked
 * as monitors.
 *  It simply has to be of the correct type to be accepted.
 *  @param monitorWS The workspace containing the monitor data.
 */
void MatrixWorkspace::setMonitorWorkspace(const std::shared_ptr<MatrixWorkspace> &monitorWS) {
  if (monitorWS.get() == this) {
    throw std::runtime_error("To avoid memory leak, monitor workspace"
                             " can not be the same workspace as the host workspace");
  }
  m_monitorWorkspace = monitorWS;
}

/** Returns a pointer to the internal monitor workspace.
 */
std::shared_ptr<MatrixWorkspace> MatrixWorkspace::monitorWorkspace() const { return m_monitorWorkspace; }

/** Return memory used by the workspace, in bytes.
 * @return bytes used.
 */
size_t MatrixWorkspace::getMemorySize() const {
  // 3 doubles per histogram bin.
  return 3 * size() * sizeof(double) + run().getMemorySize();
}

/** Returns the memory used (in bytes) by the X axes, handling ragged bins.
 * @return bytes used
 */
size_t MatrixWorkspace::getMemorySizeForXAxes() const {
  size_t total = 0;
  auto lastX = this->refX(0);
  for (size_t wi = 0; wi < getNumberHistograms(); wi++) {
    auto X = this->refX(wi);
    // If the pointers are the same
    if (!(X == lastX) || wi == 0)
      total += (*X).size() * sizeof(double);
  }
  return total;
}

/** Return the time of the first pulse received, by accessing the run's
 * sample logs to find the proton_charge.
 *
 * @return the time of the first pulse
 * @throw Exception::NotFoundError if the log is not found; or if it is empty.
 * @throw invalid_argument if the log is not a double TimeSeriesProperty (should
 * be impossible)
 */
Types::Core::DateAndTime MatrixWorkspace::getFirstPulseTime() const { return this->run().getFirstPulseTime(); }

/** Return the time of the last pulse received, by accessing the run's
 * sample logs to find the proton_charge
 *
 * @return the time of the last pulse
 * @throw runtime_error if the log is not found; or if it is empty.
 * @throw invalid_argument if the log is not a double TimeSeriesProperty (should
 * be impossible)
 */
Types::Core::DateAndTime MatrixWorkspace::getLastPulseTime() const { return this->run().getLastPulseTime(); }

/**
 * Returns the y index which corresponds to the X Value provided
 * @param xValue :: The X value to search for
 * @param index :: The index within the workspace to search within (default = 0)
 * @param tolerance :: The tolerance to accept between the passed xValue and the
 *                     stored value (default = 0.0). Used for point data only.
 * @returns The index corresponding to the X value provided
 */
std::size_t MatrixWorkspace::yIndexOfX(const double xValue, const std::size_t &index,
                                       [[maybe_unused]] const double tolerance) const {
  if (index >= getNumberHistograms())
    throw std::out_of_range("MatrixWorkspace::yIndexOfX - Index out of range.");

  const auto &xValues = this->x(index);
  const bool ascendingOrder = xValues.front() < xValues.back();
  const auto minX = ascendingOrder ? xValues.front() : xValues.back();
  const auto maxX = ascendingOrder ? xValues.back() : xValues.front();

  if (isHistogramDataByIndex(index)) {
    if (xValue < minX || xValue > maxX)
      throw std::out_of_range("MatrixWorkspace::yIndexOfX - X value is out of "
                              "the range of the min and max bin edges.");

    return binIndexOfValue(xValues, xValue, ascendingOrder);
  } else {
    if (xValue < minX - tolerance || xValue > maxX + tolerance)
      throw std::out_of_range("MatrixWorkspace::yIndexOfX - X value is out of "
                              "range for this point data.");

    return xIndexOfValue(xValues, xValue, tolerance);
  }
}

/**
 * Returns the bin index of the given X value
 * @param xValues :: The histogram to search
 * @param xValue :: The X value to search for
 * @param ascendingOrder :: True if the order of the xValues is ascending
 * @returns An index to the bin containing X
 */
std::size_t MatrixWorkspace::binIndexOfValue(HistogramData::HistogramX const &xValues, const double xValue,
                                             const bool ascendingOrder) const {
  std::size_t hops;
  if (ascendingOrder) {
    auto lowerIter = std::lower_bound(xValues.cbegin(), xValues.cend(), xValue);

    // If we are pointing at the first value then we want to be in the first bin
    if (lowerIter == xValues.cbegin())
      ++lowerIter;

    hops = std::distance(xValues.cbegin(), lowerIter);
  } else {
    auto lowerIter = std::lower_bound(xValues.crbegin(), xValues.crend(), xValue);

    if (lowerIter == xValues.crbegin())
      ++lowerIter;

    hops = xValues.size() - std::distance(xValues.crbegin(), lowerIter);
  }
  // The bin index is offset by one from the number of hops between iterators as
  // they start at zero (for a histogram workspace)
  return hops - 1;
}

/**
 * Returns the X index of the given X value
 * @param xValues :: The histogram to search
 * @param xValue :: The X value to search for
 * @param tolerance :: The tolerance to accept between the passed xValue and the
 *                     stored value (default = 0.0)
 * @returns The index of the X value
 */
std::size_t MatrixWorkspace::xIndexOfValue(const HistogramData::HistogramX &xValues, const double xValue,
                                           const double tolerance) const {
  auto const iter = std::find_if(xValues.cbegin(), xValues.cend(), [&xValue, &tolerance](double const &value) {
    return std::abs(xValue - value) <= tolerance;
  });
  if (iter != xValues.cend())
    return std::distance(xValues.cbegin(), iter);
  else
    throw std::invalid_argument("MatrixWorkspace::yIndexOfX - the X value provided could not be found "
                                "in the workspace containing point data.");
}

uint64_t MatrixWorkspace::getNPoints() const { return static_cast<uint64_t>(this->size()); }

//================================= FOR MDGEOMETRY
//====================================================

size_t MatrixWorkspace::getNumDims() const { return 2; }

std::string MatrixWorkspace::getDimensionIdFromAxis(const int &axisIndex) const {
  std::string id;
  if (0 == axisIndex) {
    id = xDimensionId;
  } else if (1 == axisIndex) {
    id = yDimensionId;
  } else {
    throw std::invalid_argument("Cannot have an index for a MatrixWorkspace "
                                "axis that is not == 0 or == 1");
  }
  return id;
}

//===============================================================================
class MWDimension : public Mantid::Geometry::IMDDimension {
public:
  MWDimension(const Axis *axis, std::string dimensionId)
      : m_axis(*axis), m_dimensionId(std::move(dimensionId)),
        m_haveEdges(dynamic_cast<const BinEdgeAxis *>(&m_axis) != nullptr),
        m_frame(std::make_unique<Geometry::GeneralFrame>(m_axis.unit()->label(), m_axis.unit()->label())) {}

  /// the name of the dimennlsion as can be displayed along the axis
  std::string getName() const override {
    const auto &unit = m_axis.unit();
    if (unit && unit->unitID() != "Empty")
      return unit->caption();
    else
      return m_axis.title();
  }

  /// @return the units of the dimension as a string
  const Kernel::UnitLabel getUnits() const override { return m_axis.unit()->label(); }

  /// short name which identify the dimension among other dimension. A dimension
  /// can be usually find by its ID and various
  /// various method exist to manipulate set of dimensions by their names.
  const std::string &getDimensionId() const override { return m_dimensionId; }

  /// if the dimension is integrated (e.g. have single bin)
  bool getIsIntegrated() const override { return m_axis.length() == 1; }

  /// @return the minimum extent of this dimension
  coord_t getMinimum() const override { return coord_t(m_axis.getMin()); }

  /// @return the maximum extent of this dimension
  coord_t getMaximum() const override { return coord_t(m_axis.getMax()); }

  /// number of bins dimension have (an integrated has one). A axis directed
  /// along dimension would have getNBins+1 axis points.
  size_t getNBins() const override {
    if (m_haveEdges)
      return m_axis.length() - 1;
    else
      return m_axis.length();
  }

  /// number of bin boundaries (axis points)
  size_t getNBoundaries() const override { return m_axis.length(); }

  /// Change the extents and number of bins
  void setRange(size_t /*nBins*/, coord_t /*min*/, coord_t /*max*/) override {
    throw std::runtime_error("Not implemented");
  }

  ///  Get coordinate for index;
  coord_t getX(size_t ind) const override { return coord_t(m_axis(ind)); }

  /**
   * Return the bin width taking into account if the stored values are actually
   * bin centres or not
   * @return A single value for the uniform bin width
   */
  coord_t getBinWidth() const override {
    size_t nsteps = (m_haveEdges) ? this->getNBins() : this->getNBins() - 1;
    return (getMaximum() - getMinimum()) / static_cast<coord_t>(nsteps);
  }

  // Dimensions must be xml serializable.
  std::string toXMLString() const override { throw std::runtime_error("Not implemented"); }

  const Kernel::MDUnit &getMDUnits() const override { return m_frame->getMDUnit(); }
  const Geometry::MDFrame &getMDFrame() const override { return *m_frame; }

private:
  const Axis &m_axis;
  const std::string m_dimensionId;
  const bool m_haveEdges;
  const Geometry::MDFrame_const_uptr m_frame;
};

//===============================================================================
/** An implementation of IMDDimension for MatrixWorkspace that
 * points to the X vector of the first spectrum.
 */
class MWXDimension : public Mantid::Geometry::IMDDimension {
public:
  MWXDimension(const MatrixWorkspace *ws, std::string dimensionId)
      : m_ws(ws), m_X(ws->readX(0)), m_dimensionId(std::move(dimensionId)),
        m_frame(std::make_unique<Geometry::GeneralFrame>(m_ws->getAxis(0)->unit()->label(),
                                                         m_ws->getAxis(0)->unit()->label())) {}

  /// the name of the dimennlsion as can be displayed along the axis
  std::string getName() const override {
    const auto *axis = m_ws->getAxis(0);
    const auto &unit = axis->unit();
    if (unit && unit->unitID() != "Empty")
      return unit->caption();
    else
      return axis->title();
  }

  /// @return the units of the dimension as a string
  const Kernel::UnitLabel getUnits() const override { return m_ws->getAxis(0)->unit()->label(); }

  /// short name which identify the dimension among other dimension. A dimension
  /// can be usually find by its ID and various
  /// various method exist to manipulate set of dimensions by their names.
  const std::string &getDimensionId() const override { return m_dimensionId; }

  /// if the dimension is integrated (e.g. have single bin)
  bool getIsIntegrated() const override { return m_X.size() == 1; }

  /// coord_t the minimum extent of this dimension
  coord_t getMinimum() const override { return coord_t(m_X.front()); }

  /// @return the maximum extent of this dimension
  coord_t getMaximum() const override { return coord_t(m_X.back()); }

  /// number of bins dimension have (an integrated has one). A axis directed
  /// along dimension would have getNBins+1 axis points.
  size_t getNBins() const override { return (m_ws->isHistogramData()) ? m_X.size() - 1 : m_X.size(); }

  /// number of axis points (bin boundaries)
  size_t getNBoundaries() const override { return m_X.size(); }

  /// Change the extents and number of bins
  void setRange(size_t /*nBins*/, coord_t /*min*/, coord_t /*max*/) override {
    throw std::runtime_error("Not implemented");
  }

  ///  Get coordinate for index;
  coord_t getX(size_t ind) const override { return coord_t(m_X[ind]); }

  // Dimensions must be xml serializable.
  std::string toXMLString() const override { throw std::runtime_error("Not implemented"); }
  const Kernel::MDUnit &getMDUnits() const override { return m_frame->getMDUnit(); }
  const Geometry::MDFrame &getMDFrame() const override { return *m_frame; }

private:
  /// Workspace we refer to
  const MatrixWorkspace *m_ws;
  /// Cached X vector
  MantidVec m_X;
  /// Dimension ID string
  const std::string m_dimensionId;
  /// Unit
  const Geometry::MDFrame_const_uptr m_frame;
};

std::shared_ptr<const Mantid::Geometry::IMDDimension> MatrixWorkspace::getDimension(size_t index) const {
  if (index == 0) {
    return std::make_shared<MWXDimension>(this, xDimensionId);
  } else if (index == 1) {
    Axis *yAxis = this->getAxis(1);
    return std::make_shared<MWDimension>(yAxis, yDimensionId);
  } else
    throw std::invalid_argument("MatrixWorkspace only has 2 dimensions.");
}

std::shared_ptr<const Mantid::Geometry::IMDDimension> MatrixWorkspace::getDimensionWithId(std::string id) const {
  int nAxes = this->axes();
  std::shared_ptr<IMDDimension> dim;
  for (int i = 0; i < nAxes; i++) {
    const std::string knownId = getDimensionIdFromAxis(i);
    if (knownId == id) {
      dim = std::make_shared<MWDimension>(this->getAxis(i), id);
      break;
    }
  }

  if (nullptr == dim) {
    std::string message = "Cannot find id : " + id;
    throw std::overflow_error(message);
  }
  return dim;
}

/** Create IMDIterators from this 2D workspace
 *
 * @param suggestedNumCores :: split the iterators into this many cores (if
 *threadsafe)
 * @param function :: implicit function to limit range
 * @return MatrixWorkspaceMDIterator vector
 */
std::vector<std::unique_ptr<IMDIterator>>
MatrixWorkspace::createIterators(size_t suggestedNumCores, Mantid::Geometry::MDImplicitFunction *function) const {
  // Find the right number of cores to use
  size_t numCores = suggestedNumCores;
  if (!this->threadSafe())
    numCores = 1;
  size_t numElements = this->getNumberHistograms();
  if (numCores > numElements)
    numCores = numElements;
  if (numCores < 1)
    numCores = 1;

  // Create one iterator per core, splitting evenly amongst spectra
  std::vector<std::unique_ptr<IMDIterator>> out;
  for (size_t i = 0; i < numCores; i++) {
    size_t begin = (i * numElements) / numCores;
    size_t end = ((i + 1) * numElements) / numCores;
    if (end > numElements)
      end = numElements;
    out.emplace_back(std::make_unique<MatrixWorkspaceMDIterator>(this, function, begin, end));
  }
  return out;
}

/** Obtain coordinates for a line plot through a MDWorkspace.
 * Cross the workspace from start to end points, recording the signal along the
 *line.
 * Sets the x,y vectors to the histogram bin boundaries and counts
 *
 * @param start :: coordinates of the start point of the line
 * @param end :: coordinates of the end point of the line
 * @param normalize :: how to normalize the signal
 * @returns :: a LinePlot in which x is set to the boundaries of the bins,
 * relative to start of the line, y is set to the normalized signal for
 * each bin with Length = length(x) - 1 and e is set to the normalized
 * errors for each bin with Length = length(x) - 1.
 */
IMDWorkspace::LinePlot MatrixWorkspace::getLinePlot(const Mantid::Kernel::VMD &start, const Mantid::Kernel::VMD &end,
                                                    Mantid::API::MDNormalization normalize) const {
  return IMDWorkspace::getLinePlot(start, end, normalize);
}

/** Returns the (normalized) signal at a given coordinates
 *
 * @param coords :: bare array, size 2, of coordinates. X, Y
 * @param normalization :: how to normalize the signal
 * @return normalized signal.
 */
signal_t MatrixWorkspace::getSignalAtCoord(const coord_t *coords,
                                           const Mantid::API::MDNormalization &normalization) const {
  if (this->axes() != 2)
    throw std::invalid_argument("MatrixWorkspace::getSignalAtCoord() - "
                                "Workspace can only have 2 axes, found " +
                                std::to_string(this->axes()));

  // First, find the workspace index
  Axis const *ax1 = this->getAxis(1);
  size_t wi(-1);
  try {
    wi = ax1->indexOfValue(coords[1]);
  } catch (std::out_of_range &) {
    return std::numeric_limits<double>::quiet_NaN();
  }

  const size_t nhist = this->getNumberHistograms();
  const auto &yVals = this->y(wi);
  double yBinSize(1.0); // only applies for volume normalization & numeric axis
  if (normalization == VolumeNormalization && ax1->isNumeric()) {
    size_t uVI = 0; // unused vertical index.
    double currentVertical = ax1->operator()(wi, uVI);
    if (wi + 1 == nhist && nhist > 1) // On the boundary, look back to get diff
    {
      yBinSize = currentVertical - ax1->operator()(wi - 1, uVI);
    } else {
      yBinSize = ax1->operator()(wi + 1, uVI) - currentVertical;
    }
  }

  if (wi < nhist) {
    const auto &xVals = x(wi);
    size_t i;
    try {
      coord_t xCoord = coords[0];
      if (isHistogramData())
        i = Kernel::VectorHelper::indexOfValueFromEdges(xVals.rawData(), xCoord);
      else
        i = Kernel::VectorHelper::indexOfValueFromCenters(xVals.rawData(), xCoord);
    } catch (std::out_of_range &) {
      return std::numeric_limits<double>::quiet_NaN();
    }

    double yVal = yVals[i];
    // What is our normalization factor?
    switch (normalization) {
    case NoNormalization:
      return yVal;
    case VolumeNormalization: {
      // Divide the signal by the area
      auto volume = yBinSize * (xVals[i + 1] - xVals[i]);
      if (volume == 0.0) {
        return std::numeric_limits<double>::quiet_NaN();
      }
      return yVal / volume;
    }
    case NumEventsNormalization:
      // Not yet implemented, may not make sense
      return yVal;
    }
    // This won't happen
    return yVal;
  } else {
    return std::numeric_limits<double>::quiet_NaN();
  }
}

/** Returns the (normalized) signal at a given coordinates
 * Implementation differs from getSignalAtCoord for MD workspaces
 *
 * @param coords :: bare array, size 2, of coordinates. X, Y
 * @param normalization :: how to normalize the signal
 * @return normalized signal.
 */
signal_t MatrixWorkspace::getSignalWithMaskAtCoord(const coord_t *coords,
                                                   const Mantid::API::MDNormalization &normalization) const {
  return getSignalAtCoord(coords, normalization);
}

/*
MDMasking for a Matrix Workspace has not been implemented.
@param :
*/
void MatrixWorkspace::setMDMasking(std::unique_ptr<Mantid::Geometry::MDImplicitFunction> /*maskingRegion*/) {
  throw std::runtime_error("MatrixWorkspace::setMDMasking has no implementation");
}

/*
Clear MDMasking for a Matrix Workspace has not been implemented.
*/
void MatrixWorkspace::clearMDMasking() {
  throw std::runtime_error("MatrixWorkspace::clearMDMasking has no implementation");
}

/**
@return the special coordinate system used if any.
*/
Mantid::Kernel::SpecialCoordinateSystem MatrixWorkspace::getSpecialCoordinateSystem() const {
  return Mantid::Kernel::None;
}

// Check if this class has an oriented lattice on a sample object
bool MatrixWorkspace::hasOrientedLattice() const { return Mantid::API::ExperimentInfo::sample().hasOrientedLattice(); }

/**
 * Creates a 2D image.
 * @param read :: Pointer to a method returning a MantidVec to provide data for
 * the image.
 * @param start :: First workspace index for the image.
 * @param stop :: Last workspace index for the image.
 * @param width :: Image width. Must divide (stop - start + 1) exactly.
 * @param indexStart :: First index of the x integration range.
 * @param indexEnd :: Last index of the x integration range.
 */
MantidImage_sptr MatrixWorkspace::getImage(const MantidVec &(MatrixWorkspace::*read)(std::size_t const) const,
                                           size_t start, size_t stop, size_t width, size_t indexStart,
                                           size_t indexEnd) const {
  // width must be provided (for now)
  if (width == 0) {
    throw std::runtime_error("Cannot create image with width 0");
  }

  size_t nHist = getNumberHistograms();
  // use all spectra by default
  if (stop == 0) {
    stop = nHist;
  }

  // check start and stop
  if (stop < start) {
    throw std::runtime_error("Cannot create image for an empty data set.");
  }

  if (start >= nHist) {
    throw std::runtime_error("Cannot create image: start index is out of range");
  }

  if (stop >= nHist) {
    throw std::runtime_error("Cannot create image: stop index is out of range");
  }

  // calculate image geometry
  size_t dataSize = stop - start + 1;
  size_t height = dataSize / width;

  // and check that the data fits exactly into this geometry
  if (height * width != dataSize) {
    throw std::runtime_error("Cannot create image: the data set cannot form a rectangle.");
  }

  size_t nBins = blocksize();
  bool isHisto = isHistogramData();

  // default indexEnd is the last index of the X vector
  if (indexEnd == 0) {
    indexEnd = nBins;
    if (!isHisto && indexEnd > 0)
      --indexEnd;
  }

  // check the x-range indices
  if (indexEnd < indexStart) {
    throw std::runtime_error("Cannot create image for an empty data set.");
  }

  if (indexStart >= nBins || indexEnd > nBins || (!isHisto && indexEnd == nBins)) {
    throw std::runtime_error("Cannot create image: integration interval is out of range.");
  }

  // initialize the image
  auto image = std::make_shared<MantidImage>(height);
  if (!isHisto)
    ++indexEnd;

  // deal separately with single-binned workspaces: no integration is required
  if (isHisto && indexEnd == indexStart + 1) {
    PARALLEL_FOR_NO_WSP_CHECK()
    for (int i = 0; i < static_cast<int>(height); ++i) {
      auto &row = (*image)[i];
      row.resize(width);
      size_t spec = start + static_cast<size_t>(i) * width;
      for (size_t j = 0; j < width; ++j, ++spec) {
        row[j] = (this->*read)(spec)[indexStart];
      }
    }
  } else {
    // each image pixel is integrated over the x-range [indexStart,indexEnd)
    PARALLEL_FOR_NO_WSP_CHECK()
    for (int i = 0; i < static_cast<int>(height); ++i) {
      auto &row = (*image)[i];
      row.resize(width);
      size_t spec = start + static_cast<size_t>(i) * width;
      for (size_t j = 0; j < width; ++j, ++spec) {
        auto &V = (this->*read)(spec);
        row[j] = std::accumulate(V.begin() + indexStart, V.begin() + indexEnd, 0.0);
      }
    }
  }

  return image;
}

std::pair<int64_t, int64_t> MatrixWorkspace::findY(double value, const std::pair<int64_t, int64_t> &idx) const {
  std::pair<int64_t, int64_t> out(-1, -1);
  const int64_t numHists = static_cast<int64_t>(this->getNumberHistograms());
  if (std::isnan(value)) {
    for (int64_t i = idx.first; i < numHists; ++i) {
      const auto &Y = this->y(i);
      if (auto it = std::find_if(std::next(Y.begin(), idx.second), Y.end(), [](double v) { return std::isnan(v); });
          it != Y.end()) {
        out = {i, std::distance(Y.begin(), it)};
        break;
      }
    }
  } else {
    for (int64_t i = idx.first; i < numHists; ++i) {
      const auto &Y = this->y(i);
      if (auto it = std::find(std::next(Y.begin(), idx.second), Y.end(), value); it != Y.end()) {
        out = {i, std::distance(Y.begin(), it)};
        break;
      }
    }
  }
  return out;
}

/**
 * Get start and end x indices for images
 * @param i :: Histogram index.
 * @param startX :: Lower bound of the x integration range.
 * @param endX :: Upper bound of the x integration range.
 */
std::pair<size_t, size_t> MatrixWorkspace::getImageStartEndXIndices(size_t i, double startX, double endX) const {
  if (startX == EMPTY_DBL())
    startX = x(i).front();
  auto pStart = getXIndex(i, startX, true);
  if (pStart.second != 0.0) {
    throw std::runtime_error("Start X value is required to be on bin boundary.");
  }
  if (endX == EMPTY_DBL())
    endX = x(i).back();
  auto pEnd = getXIndex(i, endX, false, pStart.first);
  if (pEnd.second != 0.0) {
    throw std::runtime_error("End X value is required to be on bin boundary.");
  }
  return std::make_pair(pStart.first, pEnd.first);
}

/**
 * Creates a 2D image of the y values in this workspace.
 * @param start :: First workspace index for the image.
 * @param stop :: Last workspace index for the image.
 * @param width :: Image width. Must divide (stop - start + 1) exactly.
 * @param startX :: Lower bound of the x integration range.
 * @param endX :: Upper bound of the x integration range.
 */
MantidImage_sptr MatrixWorkspace::getImageY(size_t start, size_t stop, size_t width, double startX, double endX) const {
  auto p = getImageStartEndXIndices(0, startX, endX);
  return getImage(&MatrixWorkspace::readY, start, stop, width, p.first, p.second);
}

/**
 * Creates a 2D image of the error values in this workspace.
 * @param start :: First workspace index for the image.
 * @param stop :: Last workspace index for the image.
 * @param width :: Image width. Must divide (stop - start + 1) exactly.
 * @param startX :: Lower bound of the x integration range.
 * @param endX :: Upper bound of the x integration range.
 */
MantidImage_sptr MatrixWorkspace::getImageE(size_t start, size_t stop, size_t width, double startX, double endX) const {
  auto p = getImageStartEndXIndices(0, startX, endX);
  return getImage(&MatrixWorkspace::readE, start, stop, width, p.first, p.second);
}

/**
 * Find an index in the X vector for an x-value close to a given value. It is
 * returned as the first
 * member of the pair. The second member is the fraction [0,1] of bin width cut
 * off by the search value.
 * If the first member == size of X vector then search failed.
 * @param i :: Histogram index.
 * @param x :: The value to find the index for.
 * @param isLeft :: If true the left bin boundary is returned, if false - the
 * right one.
 * @param start :: Index to start the search from.
 */
std::pair<size_t, double> MatrixWorkspace::getXIndex(size_t i, double x, bool isLeft, size_t start) const {
  auto &X = this->x(i);
  auto nx = X.size();

  // if start out of range - search failed
  if (start >= nx)
    return std::make_pair(nx, 0.0);
  if (start > 0 && start == nx - 1) {
    // starting with the last index is allowed for right boundary search
    if (!isLeft)
      return std::make_pair(start, 0.0);
    return std::make_pair(nx, 0.0);
  }

  // consider point data with single value
  if (nx == 1) {
    assert(start == 0);
    if (isLeft)
      return x <= X[start] ? std::make_pair(start, 0.0) : std::make_pair(nx, 0.0);
    return x >= X[start] ? std::make_pair(start, 0.0) : std::make_pair(nx, 0.0);
  }

  // left boundaries below start value map to the start value
  if (x <= X[start]) {
    return isLeft ? std::make_pair(start, 0.0) : std::make_pair(nx, 0.0);
  }
  // right boundary search returns last x value for all values above it
  if (x >= X.back()) {
    return !isLeft ? std::make_pair(nx - 1, 0.0) : std::make_pair(nx, 0.0);
  }

  // general case: find the boundary index and bin fraction
  auto end = X.end();
  for (auto ix = X.begin() + start + 1; ix != end; ++ix) {
    if (*ix >= x) {
      auto index = static_cast<size_t>(std::distance(X.begin(), ix));
      if (isLeft)
        --index;
      return std::make_pair(index, std::abs((X[index] - x) / (*ix - *(ix - 1))));
    }
  }
  // I don't think we can ever get here
  return std::make_pair(nx, 0.0);
}

/**
 * Copy data from an image.
 * @param dataVec :: A method returning non-const references to data vectors to
 * copy the image to.
 * @param image :: An image to copy the data from.
 * @param start :: Startinf workspace indx to copy data to.
 * @param parallelExecution :: Should inner loop run as parallel operation
 */
void MatrixWorkspace::setImage(MantidVec &(MatrixWorkspace::*dataVec)(const std::size_t), const MantidImage &image,
                               size_t start, [[maybe_unused]] bool parallelExecution) {

  if (image.empty())
    return;
  if (image[0].empty())
    return;

  if (blocksize() != 1) {
    throw std::runtime_error("Cannot set image: a single bin workspace is expected.");
  }

  size_t height = image.size();
  size_t width = image.front().size();
  size_t dataSize = width * height;

  if (start + dataSize > getNumberHistograms()) {
    throw std::runtime_error("Cannot set image: image is bigger than workspace.");
  }

  PARALLEL_FOR_IF(parallelExecution)
  for (int i = 0; i < static_cast<int>(height); ++i) {
    auto &row = image[i];
    if (row.size() != width) {
      throw std::runtime_error("Canot set image: image is corrupted.");
    }
    size_t spec = start + static_cast<size_t>(i) * width;
    auto rowEnd = row.end();
    for (auto pixel = row.begin(); pixel != rowEnd; ++pixel, ++spec) {
      (this->*dataVec)(spec)[0] = *pixel;
    }
  }
}

/**
 * Copy the data (Y's) from an image to this workspace.
 * @param image :: An image to copy the data from.
 * @param start :: Startinf workspace indx to copy data to.
 * @param parallelExecution :: Should inner loop run as parallel operation
 */
void MatrixWorkspace::setImageY(const MantidImage &image, size_t start, bool parallelExecution) {
  setImage(&MatrixWorkspace::dataY, image, start, parallelExecution);
}

/**
 * Copy the data from an image to this workspace's errors.
 * @param image :: An image to copy the data from.
 * @param start :: Startinf workspace indx to copy data to.
 * @param parallelExecution :: Should inner loop run as parallel operation
 */
void MatrixWorkspace::setImageE(const MantidImage &image, size_t start, bool parallelExecution) {
  setImage(&MatrixWorkspace::dataE, image, start, parallelExecution);
}

void MatrixWorkspace::invalidateCachedSpectrumNumbers() { m_indexInfoNeedsUpdate = true; }

/** Update detector grouping for spectrum with given index.
 *
 * This method is called when the detector grouping stored in SpectrumDefinition
 * at `index` in Beamline::SpectrumInfo is not initialized or outdated. Detector
 * IDs are currently stored in ISpectrum, but grouping information needs to be
 * available and updated in Beamline::SpectrumInfo. */
void MatrixWorkspace::updateCachedDetectorGrouping(const size_t index) const {
  setDetectorGrouping(index, getSpectrum(index).getDetectorIDs());
}

void MatrixWorkspace::buildDefaultSpectrumDefinitions() {
  const auto &detInfo = detectorInfo();
  size_t numberOfDetectors{detInfo.size()};
  if (numberOfDetectors == 0) {
    // Default to empty spectrum definitions if there is no instrument.
    m_indexInfo->setSpectrumDefinitions(std::vector<SpectrumDefinition>(m_indexInfo->size()));
    return;
  }
  size_t numberOfSpectra = numberOfDetectors * detInfo.scanCount();
  if (numberOfSpectra != m_indexInfo->globalSize())
    throw std::invalid_argument("MatrixWorkspace: IndexInfo does not contain spectrum definitions so "
                                "building a 1:1 mapping from spectra to detectors was attempted, but "
                                "the number of spectra in the workspace is not equal to the number of "
                                "detectors in the instrument.");
  std::vector<SpectrumDefinition> specDefs(m_indexInfo->size());
  if (!detInfo.isScanning() && (numberOfSpectra == m_indexInfo->size())) {
    for (size_t i = 0; i < numberOfSpectra; ++i)
      specDefs[i].add(i);
  } else {
    size_t specIndex = 0;
    size_t globalSpecIndex = 0;
    for (size_t detIndex = 0; detIndex < detInfo.size(); ++detIndex) {
      for (size_t time = 0; time < detInfo.scanCount(); ++time) {
        if (m_indexInfo->isOnThisPartition(Indexing::GlobalSpectrumIndex(globalSpecIndex++)))
          specDefs[specIndex++].add(detIndex, time);
      }
    }
  }
  m_indexInfo->setSpectrumDefinitions(std::move(specDefs));
}

void MatrixWorkspace::rebuildDetectorIDGroupings() {
  const auto &detInfo = detectorInfo();
  const auto &allDetIDs = detInfo.detectorIDs();
  const auto &specDefs = m_indexInfo->spectrumDefinitions();
  const auto indexInfoSize = static_cast<int64_t>(m_indexInfo->size());
  enum class ErrorCode { None, InvalidDetIndex, InvalidTimeIndex };
  std::atomic<ErrorCode> errorValue(ErrorCode::None);
#pragma omp parallel for
  for (int64_t i = 0; i < indexInfoSize; ++i) {
    auto &spec = getSpectrum(i);
    // Prevent setting flags that require spectrum definition updates
    spec.setMatrixWorkspace(nullptr, i);
    spec.setSpectrumNo(static_cast<specnum_t>(m_indexInfo->spectrumNumber(i)));
    std::set<detid_t> detIDs;
    for (const auto &index : (*specDefs)[i]) {
      const size_t detIndex = index.first;
      const size_t timeIndex = index.second;
      if (detIndex >= allDetIDs.size()) {
        errorValue = ErrorCode::InvalidDetIndex;
      } else if (timeIndex >= detInfo.scanCount()) {
        errorValue = ErrorCode::InvalidTimeIndex;
      } else {
        detIDs.insert(allDetIDs[detIndex]);
      }
    }
    spec.setDetectorIDs(std::move(detIDs));
  }
  switch (errorValue) {
  case ErrorCode::InvalidDetIndex:
    throw std::invalid_argument("MatrixWorkspace: SpectrumDefinition contains an out-of-range "
                                "detector index, i.e., the spectrum definition does not match "
                                "the instrument in the workspace.");
  case ErrorCode::InvalidTimeIndex:
    throw std::invalid_argument("MatrixWorkspace: SpectrumDefinition contains an out-of-range "
                                "time index for a detector, i.e., the spectrum definition does "
                                "not match the instrument in the workspace.");
  case ErrorCode::None:; // nothing to do
  }
}

} // namespace Mantid::API

///\cond TEMPLATE
namespace Mantid::Kernel {

template <>
MANTID_API_DLL Mantid::API::MatrixWorkspace_sptr
IPropertyManager::getValue<Mantid::API::MatrixWorkspace_sptr>(const std::string &name) const {
  auto *prop = dynamic_cast<PropertyWithValue<Mantid::API::MatrixWorkspace_sptr> *>(getPointerToProperty(name));
  if (prop) {
    return *prop;
  } else {
    std::string message =
        "Attempt to assign property " + name + " to incorrect type. Expected shared_ptr<MatrixWorkspace>.";
    throw std::runtime_error(message);
  }
}

template <>
MANTID_API_DLL Mantid::API::MatrixWorkspace_const_sptr
IPropertyManager::getValue<Mantid::API::MatrixWorkspace_const_sptr>(const std::string &name) const {
  auto const *prop = dynamic_cast<PropertyWithValue<Mantid::API::MatrixWorkspace_sptr> *>(getPointerToProperty(name));
  if (prop) {
    return prop->operator()();
  } else {
    std::string message =
        "Attempt to assign property " + name + " to incorrect type. Expected const shared_ptr<MatrixWorkspace>.";
    throw std::runtime_error(message);
  }
}

} // namespace Mantid::Kernel

///\endcond TEMPLATE
