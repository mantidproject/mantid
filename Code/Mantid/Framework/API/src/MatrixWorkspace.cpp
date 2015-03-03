#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/BinEdgeAxis.h"
#include "MantidAPI/NumericAxis.h"
#include "MantidAPI/SpectraAxis.h"
#include "MantidAPI/MatrixWorkspaceMDIterator.h"
#include "MantidAPI/SpectrumDetectorMapping.h"
#include "MantidGeometry/Instrument/Detector.h"
#include "MantidGeometry/Instrument/DetectorGroup.h"
#include "MantidGeometry/Instrument/NearestNeighboursFactory.h"
#include "MantidGeometry/Instrument/ReferenceFrame.h"
#include "MantidKernel/TimeSeriesProperty.h"

#include <numeric>
#include <boost/math/special_functions/fpclassify.hpp>

using Mantid::Kernel::DateAndTime;
using Mantid::Kernel::TimeSeriesProperty;
using NeXus::NXcompression;
using Mantid::Kernel::Strings::toString;

namespace Mantid {
namespace API {
using std::size_t;
using namespace Geometry;
using Kernel::V3D;

namespace {
/// static logger
Kernel::Logger g_log("MatrixWorkspace");
}

const std::string MatrixWorkspace::xDimensionId = "xDimension";
const std::string MatrixWorkspace::yDimensionId = "yDimension";

/// Default constructor
MatrixWorkspace::MatrixWorkspace(
    Mantid::Geometry::INearestNeighboursFactory *nnFactory)
    : IMDWorkspace(), ExperimentInfo(), m_axes(), m_isInitialized(false),
      m_YUnit(), m_YUnitLabel(), m_isDistribution(false),
      m_isCommonBinsFlagSet(false), m_isCommonBinsFlag(false), m_masks(),
      m_indexCalculator(),
      m_nearestNeighboursFactory(
          (nnFactory == NULL) ? new NearestNeighboursFactory : nnFactory),
      m_nearestNeighbours() {}

/// Destructor
// RJT, 3/10/07: The Analysis Data Service needs to be able to delete
// workspaces, so I moved this from protected to public.
MatrixWorkspace::~MatrixWorkspace() {
  for (unsigned int i = 0; i < m_axes.size(); ++i) {
    delete m_axes[i];
  }
}

/// @returns A human-readable string of the current state
const std::string MatrixWorkspace::toString() const {
  std::ostringstream os;
  os << id() << "\n"
     << "Title: " << getTitle() << "\n"
     << "Histograms: " << getNumberHistograms() << "\n"
     << "Bins: " << blocksize() << "\n";

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
*  @param YLength :: The number of data/error points in each vector (must all be
* the same)
*/
void MatrixWorkspace::initialize(const std::size_t &NVectors,
                                 const std::size_t &XLength,
                                 const std::size_t &YLength) {
  // Check validity of arguments
  if (NVectors == 0 || XLength == 0 || YLength == 0) {
    throw std::out_of_range(
        "All arguments to init must be positive and non-zero");
  }

  // Bypass the initialization if the workspace has already been initialized.
  if (m_isInitialized)
    return;

  // Invoke init() method of the derived class inside a try/catch clause
  try {
    this->init(NVectors, XLength, YLength);
  } catch (std::runtime_error &) {
    throw;
  }

  m_indexCalculator = MatrixWSIndexCalculator(this->blocksize());
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

//---------------------------------------------------------------------------------------
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

void MatrixWorkspace::updateSpectraUsing(const SpectrumDetectorMapping &map) {
  for (size_t j = 0; j < getNumberHistograms(); ++j) {
    auto spec = getSpectrum(j);
    try {
      if(map.indexIsSpecNumber())
        spec->setDetectorIDs(
            map.getDetectorIDsForSpectrumNo(spec->getSpectrumNo()));
      else
        spec->setDetectorIDs(
            map.getDetectorIDsForSpectrumIndex(j));
    } catch (std::out_of_range &e) {
      // Get here if the spectrum number is not in the map.
      spec->clearDetectorIDs();
      g_log.debug(e.what());
      g_log.debug() << "Spectrum number " << spec->getSpectrumNo()
                    << " not in map.\n";
    }
  }
}

//---------------------------------------------------------------------------------------
/**
* Rebuild the default spectra mapping for a workspace. If a non-empty
* instrument is set then the default maps each detector to a spectra with
* the same ID. If an empty instrument is set then a 1:1 map from 1->NHistograms
* is created.
* @param includeMonitors :: If false the monitors are not included
*/
void MatrixWorkspace::rebuildSpectraMapping(const bool includeMonitors) {
  if (sptr_instrument->nelements() == 0) {
    return;
  }

  std::vector<detid_t> pixelIDs =
      this->getInstrument()->getDetectorIDs(!includeMonitors);

  try {
    size_t index = 0;
    std::vector<detid_t>::const_iterator iend = pixelIDs.end();
    for (std::vector<detid_t>::const_iterator it = pixelIDs.begin(); it != iend;
         ++it) {
      // The detector ID
      const detid_t detId = *it;
      // By default: Spectrum number = index +  1
      const specid_t specNo = specid_t(index + 1);

      if (index < this->getNumberHistograms()) {
        ISpectrum *spec = getSpectrum(index);
        spec->setSpectrumNo(specNo);
        spec->setDetectorID(detId);
      }

      index++;
    }

    m_nearestNeighbours.reset();

  } catch (std::runtime_error &) {
    throw;
  }
}

//---------------------------------------------------------------------------------------
/**
* Handles the building of the NearestNeighbours object, if it has not already
* been
* populated for this parameter map.
* @param ignoreMaskedDetectors :: flag indicating that masked detectors should
* be ignored. True to ignore detectors.
*/
void MatrixWorkspace::buildNearestNeighbours(
    const bool ignoreMaskedDetectors) const {
  if (!m_nearestNeighbours) {
    boost::shared_ptr<const Instrument> inst = this->getInstrument();
    if (inst) {
      SpectrumDetectorMapping spectraMap(this);
      m_nearestNeighbours.reset(m_nearestNeighboursFactory->create(
          inst, spectraMap.getMapping(), ignoreMaskedDetectors));
    } else {
      throw Mantid::Kernel::Exception::NullPointerException(
          "ParameterMap: buildNearestNeighbours. Can't obtain instrument.",
          "instrument");
    }
  }
}

/*
Allow the NearestNeighbours list to be cleaned and rebuilt. Certain algorithms
require this in order to exclude/include
detectors from previously being considered.
*/
void MatrixWorkspace::rebuildNearestNeighbours() {
  /*m_nearestNeighbours should now be NULL. This will trigger rebuilding on
  subsequent first call to getNeighbours
  ,which peforms a lazy evaluation on the nearest neighbours map */
  m_nearestNeighbours.reset();
}

//---------------------------------------------------------------------------------------
/** Queries the NearestNeighbours object for the selected detector.
* NOTE! getNeighbours(spectrumNumber, radius) is MUCH faster.
*
* @param comp :: pointer to the querying detector
* @param radius :: distance from detector on which to filter results
* @param ignoreMaskedDetectors :: flag indicating that masked detectors should
*be ignored. True to ignore detectors.
* @return map of DetectorID to distance for the nearest neighbours
*/
std::map<specid_t, V3D>
MatrixWorkspace::getNeighbours(const Geometry::IDetector *comp,
                               const double radius,
                               const bool ignoreMaskedDetectors) const {
  if (!m_nearestNeighbours) {
    buildNearestNeighbours(ignoreMaskedDetectors);
  }
  // Find the spectrum number
  std::vector<specid_t> spectra;
  this->getSpectraFromDetectorIDs(std::vector<detid_t>(1, comp->getID()),
                                  spectra);
  if (spectra.empty()) {
    throw Kernel::Exception::NotFoundError("MatrixWorkspace::getNeighbours - "
                                           "Cannot find spectrum number for "
                                           "detector",
                                           comp->getID());
  }
  std::map<specid_t, V3D> neighbours =
      m_nearestNeighbours->neighboursInRadius(spectra[0], radius);
  return neighbours;
}

//---------------------------------------------------------------------------------------
/** Queries the NearestNeighbours object for the selected spectrum number.
*
* @param spec :: spectrum number of the detector you are looking at
* @param radius :: distance from detector on which to filter results
* @param ignoreMaskedDetectors :: flag indicating that masked detectors should
*be ignored. True to ignore detectors.
* @return map of DetectorID to distance for the nearest neighbours
*/
std::map<specid_t, V3D>
MatrixWorkspace::getNeighbours(specid_t spec, const double radius,
                               bool ignoreMaskedDetectors) const {
  if (!m_nearestNeighbours) {
    buildNearestNeighbours(ignoreMaskedDetectors);
  }
  std::map<specid_t, V3D> neighbours =
      m_nearestNeighbours->neighboursInRadius(spec, radius);
  return neighbours;
}

//---------------------------------------------------------------------------------------
/** Queries the NearestNeighbours object for the selected spectrum number.
*
* @param spec :: spectrum number of the detector you are looking at
* @param nNeighbours :: unsigned int, number of neighbours to include.
* @param ignoreMaskedDetectors :: flag indicating that masked detectors should
*be ignored. True to ignore detectors.
* @return map of DetectorID to distance for the nearest neighbours
*/
std::map<specid_t, V3D>
MatrixWorkspace::getNeighboursExact(specid_t spec, const int nNeighbours,
                                    bool ignoreMaskedDetectors) const {
  if (!m_nearestNeighbours) {
    SpectrumDetectorMapping spectraMap(this);
    m_nearestNeighbours.reset(m_nearestNeighboursFactory->create(
        nNeighbours, this->getInstrument(), spectraMap.getMapping(),
        ignoreMaskedDetectors));
  }
  std::map<specid_t, V3D> neighbours = m_nearestNeighbours->neighbours(spec);
  return neighbours;
}

//---------------------------------------------------------------------------------------
/** Return a map where:
*    KEY is the Spectrum #
*    VALUE is the Workspace Index
*/
spec2index_map MatrixWorkspace::getSpectrumToWorkspaceIndexMap() const {
  SpectraAxis *ax = dynamic_cast<SpectraAxis *>(this->m_axes[1]);
  if (!ax)
    throw std::runtime_error("MatrixWorkspace::getSpectrumToWorkspaceIndexMap: "
                             "axis[1] is not a SpectraAxis, so I cannot "
                             "generate a map.");
  spec2index_map map;
  try {
    ax->getSpectraIndexMap(map);
  } catch (std::runtime_error &) {
    throw std::runtime_error(
        "MatrixWorkspace::getSpectrumToWorkspaceIndexMap: no elements!");
  }
  return map;
}

//---------------------------------------------------------------------------------------
/** Return a vector where:
*    The index into the vector = spectrum number + offset
*    The value at that index = the corresponding Workspace Index
*
*  @param out :: vector set to above definition
*  @param offset :: add this to the detector ID to get the index into the
*vector.
*/
void
MatrixWorkspace::getSpectrumToWorkspaceIndexVector(std::vector<size_t> &out,
                                                   specid_t &offset) const {
  SpectraAxis *ax = dynamic_cast<SpectraAxis *>(this->m_axes[1]);
  if (!ax)
    throw std::runtime_error("MatrixWorkspace::getSpectrumToWorkspaceIndexMap: "
                             "axis[1] is not a SpectraAxis, so I cannot "
                             "generate a map.");

  // Find the min/max spectra IDs
  specid_t min = std::numeric_limits<
      specid_t>::max(); // So that any number will be less than this
  specid_t max =
      -std::numeric_limits<
          specid_t>::max(); // So that any number will be greater than this
  size_t length = ax->length();
  for (size_t i = 0; i < length; i++) {
    specid_t spec = ax->spectraNo(i);
    if (spec < min)
      min = spec;
    if (spec > max)
      max = spec;
  }

  // Offset so that the "min" value goes to index 0
  offset = -min;

  // Resize correctly
  out.resize(max - min + 1, 0);

  // Make the vector
  for (size_t i = 0; i < length; i++) {
    specid_t spec = ax->spectraNo(i);
    out[spec + offset] = i;
  }
}

//---------------------------------------------------------------------------------------
/** Does the workspace has any grouped detectors?
*  @return true if the workspace has any grouped detectors, otherwise false
*/
bool MatrixWorkspace::hasGroupedDetectors() const {
  bool retVal = false;

  // Loop through the workspace index
  for (size_t workspaceIndex = 0; workspaceIndex < this->getNumberHistograms();
       workspaceIndex++) {
    auto detList = getSpectrum(workspaceIndex)->getDetectorIDs();
    if (detList.size() > 1) {
      retVal = true;
      break;
    }
  }
  return retVal;
}

//---------------------------------------------------------------------------------------
/** Return a map where:
*    KEY is the DetectorID (pixel ID)
*    VALUE is the Workspace Index
*  @param throwIfMultipleDets :: set to true to make the algorithm throw an
* error
*         if there is more than one detector for a specific workspace index.
*  @throw runtime_error if there is more than one detector per spectrum (if
* throwIfMultipleDets is true)
*  @return Index to Index Map object. THE CALLER TAKES OWNERSHIP OF THE MAP AND
* IS RESPONSIBLE FOR ITS DELETION.
*/
detid2index_map MatrixWorkspace::getDetectorIDToWorkspaceIndexMap(
    bool throwIfMultipleDets) const {
  detid2index_map map;

  // Loop through the workspace index
  for (size_t workspaceIndex = 0; workspaceIndex < this->getNumberHistograms();
       ++workspaceIndex) {
    auto detList = getSpectrum(workspaceIndex)->getDetectorIDs();

    if (throwIfMultipleDets) {
      if (detList.size() > 1) {
        throw std::runtime_error(
            "MatrixWorkspace::getDetectorIDToWorkspaceIndexMap(): more than 1 "
            "detector for one histogram! I cannot generate a map of detector "
            "ID to workspace index.");
      }

      // Set the KEY to the detector ID and the VALUE to the workspace index.
      if (detList.size() == 1)
        map[*detList.begin()] = workspaceIndex;
    } else {
      // Allow multiple detectors per workspace index
      for (auto it = detList.begin(); it != detList.end(); ++it)
        map[*it] = workspaceIndex;
    }

    // Ignore if the detector list is empty.
  }

  return map;
}

//---------------------------------------------------------------------------------------
/** Return a vector where:
*    The index into the vector = DetectorID (pixel ID) + offset
*    The value at that index = the corresponding Workspace Index
*
*  @param out :: vector set to above definition
*  @param offset :: add this to the detector ID to get the index into the
*vector.
*  @param throwIfMultipleDets :: set to true to make the algorithm throw an
*error
*         if there is more than one detector for a specific workspace index.
*  @throw runtime_error if there is more than one detector per spectrum (if
*throwIfMultipleDets is true)
*/
void MatrixWorkspace::getDetectorIDToWorkspaceIndexVector(
    std::vector<size_t> &out, detid_t &offset, bool throwIfMultipleDets) const {
  // Make a correct initial size
  out.clear();
  detid_t minId = 0;
  detid_t maxId = 0;
  this->getInstrument()->getMinMaxDetectorIDs(minId, maxId);
  offset = -minId;
  const int outSize = maxId - minId + 1;
  // Allocate at once
  out.resize(outSize, std::numeric_limits<size_t>::max());

  for (size_t workspaceIndex = 0; workspaceIndex < getNumberHistograms();
       ++workspaceIndex) {
    // Get the list of detectors from the WS index
    const std::set<detid_t> &detList =
        this->getSpectrum(workspaceIndex)->getDetectorIDs();

    if (throwIfMultipleDets && (detList.size() > 1))
      throw std::runtime_error(
          "MatrixWorkspace::getDetectorIDToWorkspaceIndexVector(): more than 1 "
          "detector for one histogram! I cannot generate a map of detector ID "
          "to workspace index.");

    // Allow multiple detectors per workspace index, or,
    // If only one is allowed, then this has thrown already
    for (std::set<detid_t>::const_iterator it = detList.begin();
         it != detList.end(); ++it) {
      int index = *it + offset;
      if (index < 0 || index >= outSize) {
        g_log.debug() << "MatrixWorkspace::getDetectorIDToWorkspaceIndexVector("
                         "): detector ID found (" << *it
                      << " at workspace index " << workspaceIndex
                      << ") is invalid." << std::endl;
      } else
        // Save it at that point.
        out[index] = workspaceIndex;
    }

  } // (for each workspace index)
}

//---------------------------------------------------------------------------------------
/** Converts a list of spectrum numbers to the corresponding workspace indices.
*  Not a very efficient operation, but unfortunately it's sometimes required.
*
*  @param spectraList :: The list of spectrum numbers required
*  @param indexList ::   Returns a reference to the vector of indices (empty if
*not a Workspace2D)
*/
void
MatrixWorkspace::getIndicesFromSpectra(const std::vector<specid_t> &spectraList,
                                       std::vector<size_t> &indexList) const {
  // Clear the output index list
  indexList.clear();
  indexList.reserve(this->getNumberHistograms());

  std::vector<specid_t>::const_iterator iter = spectraList.begin();
  while (iter != spectraList.end()) {
    for (size_t i = 0; i < this->getNumberHistograms(); ++i) {
      if (this->getSpectrum(i)->getSpectrumNo() == *iter) {
        indexList.push_back(i);
        break;
      }
    }
    ++iter;
  }
}

//---------------------------------------------------------------------------------------
/** Given a spectrum number, find the corresponding workspace index
*
* @param specNo :: spectrum number wanted
* @return the workspace index
* @throw runtime_error if not found.
*/
size_t
MatrixWorkspace::getIndexFromSpectrumNumber(const specid_t specNo) const {
  for (size_t i = 0; i < this->getNumberHistograms(); ++i) {
    if (this->getSpectrum(i)->getSpectrumNo() == specNo)
      return i;
  }
  throw std::runtime_error("Could not find spectrum number in any spectrum.");
}

//---------------------------------------------------------------------------------------
/** Converts a list of detector IDs to the corresponding workspace indices.
*
*  @param detIdList :: The list of detector IDs required
*  @param indexList :: Returns a reference to the vector of indices
*/
void MatrixWorkspace::getIndicesFromDetectorIDs(
    const std::vector<detid_t> &detIdList,
    std::vector<size_t> &indexList) const {
  std::map<detid_t, std::set<size_t>> detectorIDtoWSIndices;
  for (size_t i = 0; i < getNumberHistograms(); ++i) {
    auto detIDs = getSpectrum(i)->getDetectorIDs();
    for (auto it = detIDs.begin(); it != detIDs.end(); ++it) {
      detectorIDtoWSIndices[*it].insert(i);
    }
  }

  indexList.clear();
  indexList.reserve(detIdList.size());
  for (size_t j = 0; j < detIdList.size(); ++j) {
    auto wsIndices = detectorIDtoWSIndices.find(detIdList[j]);
    if (wsIndices != detectorIDtoWSIndices.end()) {
      for (auto it = wsIndices->second.begin(); it != wsIndices->second.end();
           ++it) {
        indexList.push_back(*it);
      }
    }
  }
}

//---------------------------------------------------------------------------------------
/** Converts a list of detector IDs to the corresponding spectrum numbers. Might
*be slow!
*
* @param detIdList :: The list of detector IDs required
* @param spectraList :: Returns a reference to the vector of spectrum numbers.
*                       0 for not-found detectors
*/
void MatrixWorkspace::getSpectraFromDetectorIDs(
    const std::vector<detid_t> &detIdList,
    std::vector<specid_t> &spectraList) const {
  std::vector<detid_t>::const_iterator it_start = detIdList.begin();
  std::vector<detid_t>::const_iterator it_end = detIdList.end();

  spectraList.clear();

  // Try every detector in the list
  std::vector<detid_t>::const_iterator it;
  for (it = it_start; it != it_end; ++it) {
    bool foundDet = false;
    specid_t foundSpecNum = 0;

    // Go through every histogram
    for (size_t i = 0; i < this->getNumberHistograms(); i++) {
      if (this->getSpectrum(i)->hasDetectorID(*it)) {
        foundDet = true;
        foundSpecNum = this->getSpectrum(i)->getSpectrumNo();
        break;
      }
    }

    if (foundDet)
      spectraList.push_back(foundSpecNum);
  } // for each detector ID in the list
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
  for (size_t workspaceIndex = 0; workspaceIndex < numberOfSpectra;
       workspaceIndex++) {
    const MantidVec &dataX = this->readX(workspaceIndex);
    const double xfront = dataX.front();
    const double xback = dataX.back();
    if (boost::math::isfinite(xfront) && boost::math::isfinite(xback)) {
      if (xfront < xmin)
        xmin = xfront;
      if (xback > xmax)
        xmax = xback;
    }
  }
}

//---------------------------------------------------------------------------------------
/** Integrate all the spectra in the matrix workspace within the range given.
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
void MatrixWorkspace::getIntegratedSpectra(std::vector<double> &out,
                                           const double minX, const double maxX,
                                           const bool entireRange) const {
  out.resize(this->getNumberHistograms(), 0.0);

  // Run in parallel if the implementation is threadsafe
  PARALLEL_FOR_IF(this->threadSafe())
  for (int wksp_index = 0;
       wksp_index < static_cast<int>(this->getNumberHistograms());
       wksp_index++) {
    // Get Handle to data
    const Mantid::MantidVec &x = this->readX(wksp_index);
    const Mantid::MantidVec &y = this->readY(wksp_index);
    // If it is a 1D workspace, no need to integrate
    if ((x.size() <= 2) && (y.size() >= 1)) {
      out[wksp_index] = y[0];
    } else {
      // Iterators for limits - whole range by default
      Mantid::MantidVec::const_iterator lowit, highit;
      lowit = x.begin();
      highit = x.end() - 1;

      // But maybe we don't want the entire range?
      if (!entireRange) {
        // If the first element is lower that the xmin then search for new lowit
        if ((*lowit) < minX)
          lowit = std::lower_bound(x.begin(), x.end(), minX);
        // If the last element is higher that the xmax then search for new lowit
        if ((*highit) > maxX)
          highit = std::upper_bound(lowit, x.end(), maxX);
      }

      // Get the range for the y vector
      Mantid::MantidVec::difference_type distmin =
          std::distance(x.begin(), lowit);
      Mantid::MantidVec::difference_type distmax =
          std::distance(x.begin(), highit);
      double sum(0.0);
      if (distmin <= distmax) {
        // Integrate
        sum = std::accumulate(y.begin() + distmin, y.begin() + distmax, 0.0);
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
Geometry::IDetector_const_sptr
MatrixWorkspace::getDetector(const size_t workspaceIndex) const {
  const ISpectrum *spec = this->getSpectrum(workspaceIndex);
  if (!spec)
    throw Kernel::Exception::NotFoundError("MatrixWorkspace::getDetector(): "
                                           "NULL spectrum found at the given "
                                           "workspace index.",
                                           "");

  const std::set<detid_t> &dets = spec->getDetectorIDs();
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
  std::vector<Geometry::IDetector_const_sptr> dets_ptr =
      localInstrument->getDetectors(dets);
  return Geometry::IDetector_const_sptr(
      new Geometry::DetectorGroup(dets_ptr, false));
}

/** Returns the signed 2Theta scattering angle for a detector
*  @param det :: A pointer to the detector object (N.B. might be a
* DetectorGroup)
*  @return The scattering angle (0 < theta < pi)
*  @throws InstrumentDefinitionError if source or sample is missing, or they are
* in the same place
*/
double MatrixWorkspace::detectorSignedTwoTheta(
    Geometry::IDetector_const_sptr det) const {
  Instrument_const_sptr instrument = getInstrument();

  Geometry::IComponent_const_sptr source = instrument->getSource();
  Geometry::IComponent_const_sptr sample = instrument->getSample();
  if (source == NULL || sample == NULL) {
    throw Kernel::Exception::InstrumentDefinitionError(
        "Instrument not sufficiently defined: failed to get source and/or "
        "sample");
  }

  const Kernel::V3D samplePos = sample->getPos();
  const Kernel::V3D beamLine = samplePos - source->getPos();

  if (beamLine.nullVector()) {
    throw Kernel::Exception::InstrumentDefinitionError(
        "Source and sample are at same position!");
  }
  // Get the instrument up axis.
  const V3D &instrumentUpAxis =
      instrument->getReferenceFrame()->vecPointingUp();
  return det->getSignedTwoTheta(samplePos, beamLine, instrumentUpAxis);
}

/** Returns the 2Theta scattering angle for a detector
*  @param det :: A pointer to the detector object (N.B. might be a
* DetectorGroup)
*  @return The scattering angle (0 < theta < pi)
*  @throws InstrumentDefinitionError if source or sample is missing, or they are
* in the same place
*/
double
MatrixWorkspace::detectorTwoTheta(Geometry::IDetector_const_sptr det) const {
  Geometry::IComponent_const_sptr source = getInstrument()->getSource();
  Geometry::IComponent_const_sptr sample = getInstrument()->getSample();
  if (source == NULL || sample == NULL) {
    throw Kernel::Exception::InstrumentDefinitionError(
        "Instrument not sufficiently defined: failed to get source and/or "
        "sample");
  }

  const Kernel::V3D samplePos = sample->getPos();
  const Kernel::V3D beamLine = samplePos - source->getPos();

  if (beamLine.nullVector()) {
    throw Kernel::Exception::InstrumentDefinitionError(
        "Source and sample are at same position!");
  }

  return det->getTwoTheta(samplePos, beamLine);
}

/**Calculates the distance a neutron coming from the sample will have deviated
* from a
*  straight tragetory before hitting a detector. If calling this function many
* times
*  for the same detector you can call this function once, with waveLength=1, and
* use
*  the fact drop is proportional to wave length squared .This function has no
* knowledge
*  of which axis is vertical for a given instrument
*  @param det :: the detector that the neutron entered
*  @param waveLength :: the neutrons wave length in meters
*  @return the deviation in meters
*/
double MatrixWorkspace::gravitationalDrop(Geometry::IDetector_const_sptr det,
                                          const double waveLength) const {
  using namespace PhysicalConstants;
  /// Pre-factor in gravity calculation: gm^2/2h^2
  static const double gm2_OVER_2h2 =
      g * NeutronMass * NeutronMass / (2.0 * h * h);

  const V3D samplePos = getInstrument()->getSample()->getPos();
  const double pathLength = det->getPos().distance(samplePos);
  // Want L2 (sample-pixel distance) squared, times the prefactor g^2/h^2
  const double L2 = gm2_OVER_2h2 * std::pow(pathLength, 2);

  return waveLength * waveLength * L2;
}

//---------------------------------------------------------------------------------------
/** Add parameters to the instrument parameter map that are defined in
* instrument
*   definition file and for which logfile data are available. Logs must be
* loaded
*   before running this method.
*/
void MatrixWorkspace::populateInstrumentParameters() {
  ExperimentInfo::populateInstrumentParameters();
  // Clear out the nearestNeighbors so that it gets recalculated
  this->m_nearestNeighbours.reset();
}

//----------------------------------------------------------------------------------------------------
/// @return The number of axes which this workspace has
int MatrixWorkspace::axes() const { return static_cast<int>(m_axes.size()); }

//----------------------------------------------------------------------------------------------------
/** Get a pointer to a workspace axis
*  @param axisIndex :: The index of the axis required
*  @throw IndexError If the argument given is outside the range of axes held by
* this workspace
*  @return Pointer to Axis object
*/
Axis *MatrixWorkspace::getAxis(const std::size_t &axisIndex) const {
  if (axisIndex >= m_axes.size()) {
    throw Kernel::Exception::IndexError(
        axisIndex, m_axes.size(),
        "Argument to getAxis is invalid for this workspace");
  }

  return m_axes[axisIndex];
}

/** Replaces one of the workspace's axes with the new one provided.
*  @param axisIndex :: The index of the axis to replace
*  @param newAxis :: A pointer to the new axis. The class will take ownership.
*  @throw IndexError If the axisIndex given is outside the range of axes held by
* this workspace
*  @throw std::runtime_error If the new axis is not of the correct length
* (within one of the old one)
*/
void MatrixWorkspace::replaceAxis(const std::size_t &axisIndex,
                                  Axis *const newAxis) {
  // First check that axisIndex is in range
  if (axisIndex >= m_axes.size()) {
    throw Kernel::Exception::IndexError(
        axisIndex, m_axes.size(),
        "Value of axisIndex is invalid for this workspace");
  }
  // If we're OK, then delete the old axis and set the pointer to the new one
  delete m_axes[axisIndex];
  m_axes[axisIndex] = newAxis;
}

//----------------------------------------------------------------------------------------------------
/// Returns the units of the data in the workspace
std::string MatrixWorkspace::YUnit() const { return m_YUnit; }

/// Sets a new unit for the data (Y axis) in the workspace
void MatrixWorkspace::setYUnit(const std::string &newUnit) {
  m_YUnit = newUnit;
}

/// Returns a caption for the units of the data in the workspace
std::string MatrixWorkspace::YUnitLabel() const {
  std::string retVal;
  if (!m_YUnitLabel.empty())
    retVal = m_YUnitLabel;
  else {
    retVal = m_YUnit;
    // If this workspace a distribution & has at least one axis & this axis has
    // its unit set
    // then append that unit to the string to be returned
    if (!retVal.empty() && this->isDistribution() && this->axes() &&
        this->getAxis(0)->unit()) {
      retVal = retVal + " per " + this->getAxis(0)->unit()->label().ascii();
    }
  }

  return retVal;
}

/// Sets a new caption for the data (Y axis) in the workspace
void MatrixWorkspace::setYUnitLabel(const std::string &newLabel) {
  m_YUnitLabel = newLabel;
}

//----------------------------------------------------------------------------------------------------
/** Are the Y-values in this workspace dimensioned?
* TODO: For example: ????
* @return whether workspace is a distribution or not
*/
const bool &MatrixWorkspace::isDistribution() const { return m_isDistribution; }

/** Set the flag for whether the Y-values are dimensioned
*  @return whether workspace is now a distribution
*/
bool &MatrixWorkspace::isDistribution(bool newValue) {
  m_isDistribution = newValue;
  return m_isDistribution;
}

/**
*  Whether the workspace contains histogram data
*  @return whether the workspace contains histogram data
*/
bool MatrixWorkspace::isHistogramData() const {
  return (readX(0).size() == blocksize() ? false : true);
}

/**
*  Whether the workspace contains common X bins
*  @return whether the workspace contains common X bins
*/
bool MatrixWorkspace::isCommonBins() const {
  if (!m_isCommonBinsFlagSet) {
    m_isCommonBinsFlag = true;

    // there being only one or zero histograms is accepted as not being an error
    if (blocksize() || getNumberHistograms() > 1) {
      // otherwise will compare some of the data, to save time just check two
      // the first and the last
      const size_t lastSpec = getNumberHistograms() - 1;
      // Quickest check is to see if they are actually the same vector
      if (&(readX(0)[0]) != &(readX(lastSpec)[0])) {
        // Now check numerically
        const double first =
            std::accumulate(readX(0).begin(), readX(0).end(), 0.);
        const double last =
            std::accumulate(readX(lastSpec).begin(), readX(lastSpec).end(), 0.);
        if (std::abs(first - last) / std::abs(first + last) > 1.0E-9) {
          m_isCommonBinsFlag = false;
        }

        // handle Nan's and inf's
        if ((boost::math::isinf(first) != boost::math::isinf(last)) ||
            (boost::math::isnan(first) != boost::math::isnan(last))) {
          m_isCommonBinsFlag = false;
        }
      }
    }
    m_isCommonBinsFlagSet = true;
  }

  return m_isCommonBinsFlag;
}
//----------------------------------------------------------------------------------------------------
/**
* Mask a given workspace index, setting the data and error values to zero
* @param index :: The index within the workspace to mask
*/
void MatrixWorkspace::maskWorkspaceIndex(const std::size_t index) {
  if (index >= this->getNumberHistograms()) {
    throw Kernel::Exception::IndexError(
        index, this->getNumberHistograms(),
        "MatrixWorkspace::maskWorkspaceIndex,index");
  }

  ISpectrum *spec = this->getSpectrum(index);
  if (!spec)
    throw std::invalid_argument(
        "MatrixWorkspace::maskWorkspaceIndex() got a null Spectrum.");

  // Virtual method clears the spectrum as appropriate
  spec->clearData();

  const std::set<detid_t> dets = spec->getDetectorIDs();
  for (std::set<detid_t>::const_iterator iter = dets.begin();
       iter != dets.end(); ++iter) {
    try {
      if (const Geometry::Detector *det =
              dynamic_cast<const Geometry::Detector *>(
                  sptr_instrument->getDetector(*iter).get())) {
        m_parmap->addBool(det, "masked", true); // Thread-safe method
      }
    } catch (Kernel::Exception::NotFoundError &) {
    }
  }
  // If masking has occured, the NearestNeighbours map will be out of date must
  // be rebuilt.
  this->rebuildNearestNeighbours();
}

//----------------------------------------------------------------------------------------------------
/** Called by the algorithm MaskBins to mask a single bin for the first time,
* algorithms that later propagate the
*  the mask from an input to the output should call flagMasked() instead. Here
* y-values and errors will be scaled
*  by (1-weight) as well as the mask flags (m_masks) being updated. This
* function doesn't protect the writes to the
*  y and e-value arrays and so is not safe if called by multiple threads working
* on the same spectrum. Writing to
*  the mask set is marked parrallel critical so different spectra can be
* analysised in parallel
*  @param workspaceIndex :: The workspace spectrum index of the bin
*  @param binIndex ::      The index of the bin in the spectrum
*  @param weight ::        'How heavily' the bin is to be masked. =1 for full
* masking (the default).
*/
void MatrixWorkspace::maskBin(const size_t &workspaceIndex,
                              const size_t &binIndex, const double &weight) {
  // First check the workspaceIndex is valid
  if (workspaceIndex >= this->getNumberHistograms())
    throw Kernel::Exception::IndexError(
        workspaceIndex, this->getNumberHistograms(),
        "MatrixWorkspace::maskBin,workspaceIndex");
  // Then check the bin index
  if (binIndex >= this->blocksize())
    throw Kernel::Exception::IndexError(binIndex, this->blocksize(),
                                        "MatrixWorkspace::maskBin,binIndex");

  // this function is marked parallel critical
  flagMasked(workspaceIndex, binIndex, weight);

  // this is the actual result of the masking that most algorithms and plotting
  // implementations will see, the bin mask flags defined above are used by only
  // some algorithms
  this->dataY(workspaceIndex)[binIndex] *= (1 - weight);
  this->dataE(workspaceIndex)[binIndex] *= (1 - weight);
}

/** Writes the masking weight to m_masks (doesn't alter y-values). Contains a
* parrallel critical section
*  and so is thread safe
*  @param spectrumIndex :: The workspace spectrum index of the bin
*  @param binIndex ::      The index of the bin in the spectrum
*  @param weight ::        'How heavily' the bin is to be masked. =1 for full
* masking (the default).
*/
void MatrixWorkspace::flagMasked(const size_t &spectrumIndex,
                                 const size_t &binIndex, const double &weight) {
  // Writing to m_masks is not thread-safe, so put in some protection
  PARALLEL_CRITICAL(maskBin) {
    // First get a reference to the list for this spectrum (or create a new
    // list)
    MaskList &binList = m_masks[spectrumIndex];
    MaskList::iterator it = binList.find(binIndex);
    if (it != binList.end()) {
      binList.erase(it);
    }
    binList.insert(std::make_pair(binIndex, weight));
  }
}

/** Does this spectrum contain any masked bins
*  @param workspaceIndex :: The workspace spectrum index to test
*  @return True if there are masked bins for this spectrum
*/
bool MatrixWorkspace::hasMaskedBins(const size_t &workspaceIndex) const {
  // First check the workspaceIndex is valid. Return false if it isn't (decided
  // against throwing here).
  if (workspaceIndex >= this->getNumberHistograms())
    return false;
  return (m_masks.find(workspaceIndex) == m_masks.end()) ? false : true;
}

/** Returns the list of masked bins for a spectrum.
*  @param  workspaceIndex
*  @return A const reference to the list of masked bins
*  @throw  Kernel::Exception::IndexError if there are no bins masked for this
* spectrum (so call hasMaskedBins first!)
*/
const MatrixWorkspace::MaskList &
MatrixWorkspace::maskedBins(const size_t &workspaceIndex) const {
  std::map<int64_t, MaskList>::const_iterator it = m_masks.find(workspaceIndex);
  // Throw if there are no masked bins for this spectrum. The caller should
  // check first using hasMaskedBins!
  if (it == m_masks.end()) {
    throw Kernel::Exception::IndexError(workspaceIndex, 0,
                                        "MatrixWorkspace::maskedBins");
  }

  return it->second;
}

/** Sets the internal monitor workspace to the provided workspace.
*  This method is intended for use by data-loading algorithms.
*  Note that no checking is performed as to whether this workspace actually
* contains data
*  pertaining to monitors, or that the spectra point to Detector objects marked
* as monitors.
*  It simply has to be of the correct type to be accepted.
*  @param monitorWS The workspace containing the monitor data.
*/
void MatrixWorkspace::setMonitorWorkspace(
    const boost::shared_ptr<MatrixWorkspace> &monitorWS) {
  m_monitorWorkspace = monitorWS;
}

/** Returns a pointer to the internal monitor workspace.
*/
boost::shared_ptr<MatrixWorkspace> MatrixWorkspace::monitorWorkspace() const {
  return m_monitorWorkspace;
}

//---------------------------------------------------------------------------------------------
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
  MantidVecPtr lastX = this->refX(0);
  for (size_t wi = 0; wi < getNumberHistograms(); wi++) {
    MantidVecPtr X = this->refX(wi);
    // If the pointers are the same
    if (!(X == lastX) || wi == 0)
      total += (*X).size() * sizeof(double);
  }
  return total;
}

//-----------------------------------------------------------------------------
/** Return the time of the first pulse received, by accessing the run's
* sample logs to find the proton_charge.
*
* NOTE, JZ: Pulse times before 1991 (up to 100) are skipped. This is to avoid
* a DAS bug at SNS around Mar 2011 where the first pulse time is Jan 1, 1990.
*
* @return the time of the first pulse
* @throw runtime_error if the log is not found; or if it is empty.
* @throw invalid_argument if the log is not a double TimeSeriesProperty (should
*be impossible)
*/
Kernel::DateAndTime MatrixWorkspace::getFirstPulseTime() const {
  TimeSeriesProperty<double> *log =
      this->run().getTimeSeriesProperty<double>("proton_charge");

  DateAndTime startDate = log->firstTime();
  DateAndTime reference("1991-01-01T00:00:00");

  int i = 0;
  // Find the first pulse after 1991
  while (startDate < reference && i < 100) {
    i++;
    startDate = log->nthTime(i);
  }

  // Return as DateAndTime.
  return startDate;
}

//-----------------------------------------------------------------------------
/** Return the time of the last pulse received, by accessing the run's
* sample logs to find the proton_charge
*
* @return the time of the first pulse
* @throw runtime_error if the log is not found; or if it is empty.
* @throw invalid_argument if the log is not a double TimeSeriesProperty (should
*be impossible)
*/
Kernel::DateAndTime MatrixWorkspace::getLastPulseTime() const {
  TimeSeriesProperty<double> *log =
      this->run().getTimeSeriesProperty<double>("proton_charge");
  return log->lastTime();
}

//----------------------------------------------------------------------------------------------------
/**
* Returns the bin index of the given X value
* @param xValue :: The X value to search for
* @param index :: The index within the workspace to search within (default = 0)
* @returns An index that
*/
size_t MatrixWorkspace::binIndexOf(const double xValue,
                                   const std::size_t index) const {
  if (index >= getNumberHistograms()) {
    throw std::out_of_range(
        "MatrixWorkspace::binIndexOf - Index out of range.");
  }
  const MantidVec &xValues = this->readX(index);
  // Lower bound will test if the value is greater than the last but we need to
  // see if X is valid at the start
  if (xValue < xValues.front()) {
    throw std::out_of_range("MatrixWorkspace::binIndexOf - X value lower than "
                            "lowest in current range.");
  }
  MantidVec::const_iterator lowit =
      std::lower_bound(xValues.begin(), xValues.end(), xValue);
  if (lowit == xValues.end()) {
    throw std::out_of_range("MatrixWorkspace::binIndexOf - X value greater "
                            "than highest in current range.");
  }
  // If we are pointing at the first value then that means we still want to be
  // in the first bin
  if (lowit == xValues.begin()) {
    ++lowit;
  }
  size_t hops = std::distance(xValues.begin(), lowit);
  // The bin index is offset by one from the number of hops between iterators as
  // they start at zero
  return hops - 1;
}

uint64_t MatrixWorkspace::getNPoints() const {
  return (uint64_t)(this->size());
}

//================================= FOR MDGEOMETRY
//====================================================

size_t MatrixWorkspace::getNumDims() const { return 2; }

std::string
MatrixWorkspace::getDimensionIdFromAxis(const int &axisIndex) const {
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
  MWDimension(const Axis *axis, const std::string &dimensionId)
      : m_axis(*axis), m_dimensionId(dimensionId),
        m_haveEdges(dynamic_cast<const BinEdgeAxis *>(&m_axis) != NULL) {}

  /// the name of the dimennlsion as can be displayed along the axis
  virtual std::string getName() const {
    const auto &unit = m_axis.unit();
    if (unit && unit->unitID() != "Empty")
      return unit->caption();
    else
      return m_axis.title();
  }

  /// @return the units of the dimension as a string
  virtual const Kernel::UnitLabel getUnits() const {
    return m_axis.unit()->label();
  }

  /// short name which identify the dimension among other dimension. A dimension
  /// can be usually find by its ID and various
  /// various method exist to manipulate set of dimensions by their names.
  virtual std::string getDimensionId() const { return m_dimensionId; }

  /// if the dimension is integrated (e.g. have single bin)
  virtual bool getIsIntegrated() const { return m_axis.length() == 1; }

  /// @return the minimum extent of this dimension
  virtual coord_t getMinimum() const { return coord_t(m_axis.getMin()); }

  /// @return the maximum extent of this dimension
  virtual coord_t getMaximum() const { return coord_t(m_axis.getMax()); }

  /// number of bins dimension have (an integrated has one). A axis directed
  /// along dimension would have getNBins+1 axis points.
  virtual size_t getNBins() const {
    if (m_haveEdges)
      return m_axis.length() - 1;
    else
      return m_axis.length();
  }

  /// Change the extents and number of bins
  virtual void setRange(size_t /*nBins*/, coord_t /*min*/, coord_t /*max*/) {
    throw std::runtime_error("Not implemented");
  }

  ///  Get coordinate for index;
  virtual coord_t getX(size_t ind) const { return coord_t(m_axis(ind)); }

  /**
  * Return the bin width taking into account if the stored values are actually
  * bin centres or not
  * @return A single value for the uniform bin width
  */
  virtual coord_t getBinWidth() const {
    size_t nsteps = (m_haveEdges) ? this->getNBins() : this->getNBins() - 1;
    return (getMaximum() - getMinimum()) / static_cast<coord_t>(nsteps);
  }

  // Dimensions must be xml serializable.
  virtual std::string toXMLString() const {
    throw std::runtime_error("Not implemented");
  }

  virtual ~MWDimension() {}

private:
  const Axis &m_axis;
  const std::string m_dimensionId;
  const bool m_haveEdges;
};

//===============================================================================
/** An implementation of IMDDimension for MatrixWorkspace that
* points to the X vector of the first spectrum.
*/
class MWXDimension : public Mantid::Geometry::IMDDimension {
public:
  MWXDimension(const MatrixWorkspace *ws, const std::string &dimensionId)
      : m_ws(ws), m_dimensionId(dimensionId) {
    m_X = ws->readX(0);
  }

  virtual ~MWXDimension() {}

  /// the name of the dimennlsion as can be displayed along the axis
  virtual std::string getName() const {
    const auto *axis = m_ws->getAxis(0);
    const auto &unit = axis->unit();
    if (unit && unit->unitID() != "Empty")
      return unit->caption();
    else
      return axis->title();
  }

  /// @return the units of the dimension as a string
  virtual const Kernel::UnitLabel getUnits() const {
    return m_ws->getAxis(0)->unit()->label();
  }

  /// short name which identify the dimension among other dimension. A dimension
  /// can be usually find by its ID and various
  /// various method exist to manipulate set of dimensions by their names.
  virtual std::string getDimensionId() const { return m_dimensionId; }

  /// if the dimension is integrated (e.g. have single bin)
  virtual bool getIsIntegrated() const { return m_X.size() == 1; }

  /// coord_t the minimum extent of this dimension
  virtual coord_t getMinimum() const { return coord_t(m_X.front()); }

  /// @return the maximum extent of this dimension
  virtual coord_t getMaximum() const { return coord_t(m_X.back()); }

  /// number of bins dimension have (an integrated has one). A axis directed
  /// along dimension would have getNBins+1 axis points.
  virtual size_t getNBins() const {
    return (m_ws->isHistogramData()) ? m_X.size() - 1 : m_X.size();
  }

  /// Change the extents and number of bins
  virtual void setRange(size_t /*nBins*/, coord_t /*min*/, coord_t /*max*/) {
    throw std::runtime_error("Not implemented");
  }

  ///  Get coordinate for index;
  virtual coord_t getX(size_t ind) const { return coord_t(m_X[ind]); }

  // Dimensions must be xml serializable.
  virtual std::string toXMLString() const {
    throw std::runtime_error("Not implemented");
  }

private:
  /// Workspace we refer to
  const MatrixWorkspace *m_ws;
  /// Cached X vector
  MantidVec m_X;
  /// Dimension ID string
  const std::string m_dimensionId;
};

boost::shared_ptr<const Mantid::Geometry::IMDDimension>
MatrixWorkspace::getDimension(size_t index) const {
  if (index == 0) {
    MWXDimension *dimension = new MWXDimension(this, xDimensionId);
    return boost::shared_ptr<const Mantid::Geometry::IMDDimension>(dimension);
  } else if (index == 1) {
    Axis *yAxis = this->getAxis(1);
    MWDimension *dimension = new MWDimension(yAxis, yDimensionId);
    return boost::shared_ptr<const Mantid::Geometry::IMDDimension>(dimension);
  } else
    throw std::invalid_argument("MatrixWorkspace only has 2 dimensions.");
}

boost::shared_ptr<const Mantid::Geometry::IMDDimension>
MatrixWorkspace::getDimensionWithId(std::string id) const {
  int nAxes = this->axes();
  IMDDimension *dim = NULL;
  for (int i = 0; i < nAxes; i++) {
    Axis *xAxis = this->getAxis(i);
    const std::string &knownId = getDimensionIdFromAxis(i);
    if (knownId == id) {
      dim = new MWDimension(xAxis, id);
      break;
    }
  }
  if (NULL == dim) {
    std::string message = "Cannot find id : " + id;
    throw std::overflow_error(message);
  }
  return boost::shared_ptr<const Mantid::Geometry::IMDDimension>(dim);
}

//--------------------------------------------------------------------------------------------
/** Create IMDIterators from this 2D workspace
*
* @param suggestedNumCores :: split the iterators into this many cores (if
*threadsafe)
* @param function :: implicit function to limit range
* @return MatrixWorkspaceMDIterator vector
*/
std::vector<IMDIterator *> MatrixWorkspace::createIterators(
    size_t suggestedNumCores,
    Mantid::Geometry::MDImplicitFunction *function) const {
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
  std::vector<IMDIterator *> out;
  for (size_t i = 0; i < numCores; i++) {
    size_t begin = (i * numElements) / numCores;
    size_t end = ((i + 1) * numElements) / numCores;
    if (end > numElements)
      end = numElements;
    out.push_back(new MatrixWorkspaceMDIterator(this, function, begin, end));
  }
  return out;
}

//------------------------------------------------------------------------------------
/** Obtain coordinates for a line plot through a MDWorkspace.
* Cross the workspace from start to end points, recording the signal along the
*line.
* Sets the x,y vectors to the histogram bin boundaries and counts
*
* @param start :: coordinates of the start point of the line
* @param end :: coordinates of the end point of the line
* @param normalize :: how to normalize the signal
* @param x :: is set to the boundaries of the bins, relative to start of the
*line.
* @param y :: is set to the normalized signal for each bin. Length = length(x) -
*1
* @param e :: is set to the normalized errors for each bin. Length = length(x) -
*1
*/
void MatrixWorkspace::getLinePlot(const Mantid::Kernel::VMD &start,
                                  const Mantid::Kernel::VMD &end,
                                  Mantid::API::MDNormalization normalize,
                                  std::vector<coord_t> &x,
                                  std::vector<signal_t> &y,
                                  std::vector<signal_t> &e) const {
  IMDWorkspace::getLinePlot(start, end, normalize, x, y, e);
}

//------------------------------------------------------------------------------------
/** Returns the (normalized) signal at a given coordinates
*
* @param coords :: bare array, size 2, of coordinates. X, Y
* @param normalization :: how to normalize the signal
* @return normalized signal.
*/
signal_t MatrixWorkspace::getSignalAtCoord(
    const coord_t *coords,
    const Mantid::API::MDNormalization &normalization) const {
  if (this->axes() != 2)
    throw std::invalid_argument("MatrixWorkspace::getSignalAtCoord() - "
                                "Workspace can only have 2 axes, found " +
                                boost::lexical_cast<std::string>(this->axes()));

  coord_t x = coords[0];
  coord_t y = coords[1];
  // First, find the workspace index
  Axis *ax1 = this->getAxis(1);
  size_t wi(-1);
  try {
    wi = ax1->indexOfValue(y);
  } catch (std::out_of_range &) {
    return std::numeric_limits<double>::quiet_NaN();
  }

  const size_t nhist = this->getNumberHistograms();
  const auto &yVals = this->readY(wi);
  double yBinSize(1.0); // only applies for volume normalization & numeric axis
  if (normalization == VolumeNormalization && ax1->isNumeric()) {
    size_t uVI; // unused vertical index.
    double currentVertical = ax1->operator()(wi, uVI);
    if (wi + 1 == nhist && nhist > 1) // On the boundary, look back to get diff
    {
      yBinSize = currentVertical - ax1->operator()(wi - 1, uVI);
    } else {
      yBinSize = ax1->operator()(wi + 1, uVI) - currentVertical;
    }
  }

  if (wi < nhist) {
    const MantidVec &X = this->readX(wi);
    MantidVec::const_iterator it = std::lower_bound(X.begin(), X.end(), x);
    if (it == X.end()) {
      // Out of range
      return std::numeric_limits<double>::quiet_NaN();
    } else {
      size_t i = (it - X.begin());
      if (i > 0) {
        double y = yVals[i - 1];
        // What is our normalization factor?
        switch (normalization) {
        case NoNormalization:
          return y;
        case VolumeNormalization:
          // Divide the signal by the area
          return y / (yBinSize * (X[i] - X[i - 1]));
        case NumEventsNormalization:
          // Not yet implemented, may not make sense
          return y;
        }
        // This won't happen
        return y;
      } else
        return std::numeric_limits<double>::quiet_NaN();
    }
  } else
    // Out of range
    return std::numeric_limits<double>::quiet_NaN();
}

//--------------------------------------------------------------------------------------------
/** Save the spectra detector map to an open NeXus file.
* @param file :: open NeXus file
* @param spec :: list of the Workspace Indices to save.
* @param compression :: NXcompression int to indicate how to compress
*/
void MatrixWorkspace::saveSpectraMapNexus(
    ::NeXus::File *file, const std::vector<int> &spec,
    const ::NeXus::NXcompression compression) const {
  // Count the total number of detectors
  std::size_t nDetectors = 0;
  for (size_t i = 0; i < spec.size(); i++) {
    size_t wi = size_t(spec[i]); // Workspace index
    nDetectors += this->getSpectrum(wi)->getDetectorIDs().size();
  }

  if (nDetectors < 1) {
    // No data in spectraMap to write
    g_log.warning("No spectramap data to write");
    return;
  }

  // Start the detector group
  file->makeGroup("detector", "NXdetector", 1);
  file->putAttr("version", 1);

  int numberSpec = int(spec.size());
  // allocate space for the Nexus Muon format of spctra-detector mapping
  std::vector<int32_t> detector_index(
      numberSpec + 1, 0); // allow for writing one more than required
  std::vector<int32_t> detector_count(numberSpec, 0);
  std::vector<int32_t> detector_list(nDetectors, 0);
  std::vector<int32_t> spectra(numberSpec, 0);
  std::vector<double> detPos(nDetectors * 3);
  detector_index[0] = 0;
  int id = 0;

  int ndet = 0;
  // get data from map into Nexus Muon format
  for (int i = 0; i < numberSpec; i++) {
    // Workspace index
    int si = spec[i];
    // Spectrum there
    const ISpectrum *spectrum = this->getSpectrum(si);
    spectra[i] = int32_t(spectrum->getSpectrumNo());

    // The detectors in this spectrum
    const std::set<detid_t> &detectorgroup = spectrum->getDetectorIDs();
    const int ndet1 = static_cast<int>(detectorgroup.size());

    detector_index[i + 1] = int32_t(
        detector_index[i] +
        ndet1); // points to start of detector list for the next spectrum
    detector_count[i] = int32_t(ndet1);
    ndet += ndet1;

    std::set<detid_t>::const_iterator it;
    for (it = detectorgroup.begin(); it != detectorgroup.end(); ++it) {
      detector_list[id++] = int32_t(*it);
    }
  }
  // Cut the extra entry at the end of detector_index
  detector_index.resize(numberSpec);

  // write data as Nexus sections detector{index,count,list}
  std::vector<int> dims(1, numberSpec);
  file->writeCompData("detector_index", detector_index, dims, compression,
                      dims);
  file->writeCompData("detector_count", detector_count, dims, compression,
                      dims);
  dims[0] = ndet;
  file->writeCompData("detector_list", detector_list, dims, compression, dims);
  dims[0] = numberSpec;
  file->writeCompData("spectra", spectra, dims, compression, dims);

  // Get all the positions
  try {
    Geometry::Instrument_const_sptr inst = this->getInstrument();
    Geometry::IComponent_const_sptr sample = inst->getSample();
    if (sample) {
      Kernel::V3D sample_pos = sample->getPos();
      for (int i = 0; i < ndet; i++) {
        double R, Theta, Phi;
        try {
          Geometry::IDetector_const_sptr det =
              inst->getDetector(detector_list[i]);
          Kernel::V3D pos = det->getPos() - sample_pos;
          pos.getSpherical(R, Theta, Phi);
          R = det->getDistance(*sample);
          Theta = this->detectorTwoTheta(det) * 180.0 / M_PI;
        } catch (...) {
          R = 0.;
          Theta = 0.;
          Phi = 0.;
        }
        // Need to get R & Theta through these methods to be correct for grouped
        // detectors
        detPos[3 * i] = R;
        detPos[3 * i + 1] = Theta;
        detPos[3 * i + 2] = Phi;
      }
    } else
      for (int i = 0; i < 3 * ndet; i++)
        detPos[i] = 0.;

    dims[0] = ndet;
    dims.push_back(3);
    dims[1] = 3;
    file->writeCompData("detector_positions", detPos, dims, compression, dims);
  } catch (...) {
    g_log.error("Unknown error caught when saving detector positions.");
  }

  file->closeGroup();
}

/*
MDMasking for a Matrix Workspace has not been implemented.
@param :
*/
void MatrixWorkspace::setMDMasking(Mantid::Geometry::MDImplicitFunction *) {
  throw std::runtime_error(
      "MatrixWorkspace::setMDMasking has no implementation");
}

/*
Clear MDMasking for a Matrix Workspace has not been implemented.
*/
void MatrixWorkspace::clearMDMasking() {
  throw std::runtime_error(
      "MatrixWorkspace::clearMDMasking has no implementation");
}

/**
@return the special coordinate system used if any.
*/
Mantid::Kernel::SpecialCoordinateSystem
MatrixWorkspace::getSpecialCoordinateSystem() const {
  return Mantid::Kernel::None;
}

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
MantidImage_sptr MatrixWorkspace::getImage(
    const MantidVec &(MatrixWorkspace::*read)(std::size_t const) const,
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
    throw std::runtime_error(
        "Cannot create image: start index is out of range");
  }

  if (stop >= nHist) {
    throw std::runtime_error("Cannot create image: stop index is out of range");
  }

  // calculate image geometry
  size_t dataSize = stop - start + 1;
  size_t height = dataSize / width;

  // and check that the data fits exactly into this geometry
  if (height * width != dataSize) {
    throw std::runtime_error(
        "Cannot create image: the data set cannot form a rectangle.");
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

  if (indexStart >= nBins || indexEnd > nBins ||
      (!isHisto && indexEnd == nBins)) {
    throw std::runtime_error(
        "Cannot create image: integration interval is out of range.");
  }

  // initialize the image
  auto image = boost::make_shared<MantidImage>(height);
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
        row[j] =
            std::accumulate(V.begin() + indexStart, V.begin() + indexEnd, 0.0);
      }
    }
  }

  return image;
}

/**
 * Get start and end x indices for images
 * @param i :: Histogram index.
 * @param startX :: Lower bound of the x integration range.
 * @param endX :: Upper bound of the x integration range.
 */
std::pair<size_t, size_t>
MatrixWorkspace::getImageStartEndXIndices(size_t i, double startX,
                                          double endX) const {
  if (startX == EMPTY_DBL())
    startX = readX(i).front();
  auto pStart = getXIndex(i, startX, true);
  if (pStart.second != 0.0) {
    throw std::runtime_error(
        "Start X value is required to be on bin boundary.");
  }
  if (endX == EMPTY_DBL())
    endX = readX(i).back();
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
MantidImage_sptr MatrixWorkspace::getImageY(size_t start, size_t stop,
                                            size_t width, double startX,
                                            double endX) const {
  auto p = getImageStartEndXIndices(0, startX, endX);
  return getImage(&MatrixWorkspace::readY, start, stop, width, p.first,
                  p.second);
}

/**
 * Creates a 2D image of the error values in this workspace.
 * @param start :: First workspace index for the image.
 * @param stop :: Last workspace index for the image.
 * @param width :: Image width. Must divide (stop - start + 1) exactly.
 * @param startX :: Lower bound of the x integration range.
 * @param endX :: Upper bound of the x integration range.
 */
MantidImage_sptr MatrixWorkspace::getImageE(size_t start, size_t stop,
                                            size_t width, double startX,
                                            double endX) const {
  auto p = getImageStartEndXIndices(0, startX, endX);
  return getImage(&MatrixWorkspace::readE, start, stop, width, p.first,
                  p.second);
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
std::pair<size_t, double> MatrixWorkspace::getXIndex(size_t i, double x,
                                                     bool isLeft,
                                                     size_t start) const {
  auto &X = readX(i);
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
      return x <= X[start] ? std::make_pair(start, 0.0)
                           : std::make_pair(nx, 0.0);
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
      return std::make_pair(index, fabs((X[index] - x) / (*ix - *(ix - 1))));
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
void MatrixWorkspace::setImage(
    MantidVec &(MatrixWorkspace::*dataVec)(const std::size_t),
    const MantidImage &image, size_t start, bool parallelExecution) {

  if (image.empty())
    return;
  if (image[0].empty())
    return;

  if (blocksize() != 1) {
    throw std::runtime_error(
        "Cannot set image: a single bin workspace is expected.");
  }

  size_t height = image.size();
  size_t width = image.front().size();
  size_t dataSize = width * height;

  if (start + dataSize > getNumberHistograms()) {
    throw std::runtime_error(
        "Cannot set image: image is bigger than workspace.");
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
void MatrixWorkspace::setImageY(const MantidImage &image, size_t start,
                                bool parallelExecution) {
  setImage(&MatrixWorkspace::dataY, image, start, parallelExecution);
}

/**
 * Copy the data from an image to this workspace's errors.
 * @param image :: An image to copy the data from.
 * @param start :: Startinf workspace indx to copy data to.
 * @param parallelExecution :: Should inner loop run as parallel operation
 */
void MatrixWorkspace::setImageE(const MantidImage &image, size_t start,
                                bool parallelExecution) {
  setImage(&MatrixWorkspace::dataE, image, start, parallelExecution);
}

} // namespace API
} // Namespace Mantid

///\cond TEMPLATE
namespace Mantid {
namespace Kernel {

template <>
MANTID_API_DLL Mantid::API::MatrixWorkspace_sptr
IPropertyManager::getValue<Mantid::API::MatrixWorkspace_sptr>(
    const std::string &name) const {
  PropertyWithValue<Mantid::API::MatrixWorkspace_sptr> *prop =
      dynamic_cast<PropertyWithValue<Mantid::API::MatrixWorkspace_sptr> *>(
          getPointerToProperty(name));
  if (prop) {
    return *prop;
  } else {
    std::string message = "Attempt to assign property " + name +
                          " to incorrect type. Expected MatrixWorkspace.";
    throw std::runtime_error(message);
  }
}

template <>
MANTID_API_DLL Mantid::API::MatrixWorkspace_const_sptr
IPropertyManager::getValue<Mantid::API::MatrixWorkspace_const_sptr>(
    const std::string &name) const {
  PropertyWithValue<Mantid::API::MatrixWorkspace_sptr> *prop =
      dynamic_cast<PropertyWithValue<Mantid::API::MatrixWorkspace_sptr> *>(
          getPointerToProperty(name));
  if (prop) {
    return prop->operator()();
  } else {
    std::string message = "Attempt to assign property " + name +
                          " to incorrect type. Expected const MatrixWorkspace.";
    throw std::runtime_error(message);
  }
}

} // namespace Kernel
} // namespace Mantid

///\endcond TEMPLATE
