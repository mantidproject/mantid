// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataObjects/LeanElasticPeaksWorkspace.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/Sample.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"
#include "MantidGeometry/Instrument/Goniometer.h"
#include "MantidKernel/IPropertyManager.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/UnitConversion.h"

// clang-format off
#include <nexus/NeXusFile.hpp>
#include <nexus/NeXusException.hpp>
// clang-format on

#include <cmath>

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::Geometry;

namespace Mantid {
namespace DataObjects {
/// Register the workspace as a type
DECLARE_WORKSPACE(LeanElasticPeaksWorkspace)

//---------------------------------------------------------------------------------------------
/** Constructor. Create a table with all the required columns.
 *
 * @return LeanElasticPeaksWorkspace object
 */
LeanElasticPeaksWorkspace::LeanElasticPeaksWorkspace()
    : IPeaksWorkspace(), peaks(), columns(), columnNames(),
      m_coordSystem(None) {
  initColumns();
  // LeanElasticPeaksWorkspace does not use the grouping mechanism of
  // ExperimentInfo.
  setNumberOfDetectorGroups(0);
}

//---------------------------------------------------------------------------------------------
/** Copy constructor
 *
 * @param other :: other LeanElasticPeaksWorkspace to copy from
 */
LeanElasticPeaksWorkspace::LeanElasticPeaksWorkspace(
    const LeanElasticPeaksWorkspace &other)
    : IPeaksWorkspace(other), peaks(other.peaks), columns(), columnNames(),
      m_coordSystem(other.m_coordSystem) {
  initColumns();
  // LeanElasticPeaksWorkspace does not use the grouping mechanism of
  // ExperimentInfo.
  setNumberOfDetectorGroups(0);
}

/** Comparator class for sorting peaks by one or more criteria
 */
class PeakComparator {
public:
  using ColumnAndDirection = LeanElasticPeaksWorkspace::ColumnAndDirection;
  std::vector<ColumnAndDirection> &criteria;

  /** Constructor for the comparator for sorting peaks
   * @param criteria : a vector with a list of pairs: column name, bool;
   *        where bool = true for ascending, false for descending sort.
   */
  explicit PeakComparator(std::vector<ColumnAndDirection> &criteria)
      : criteria(criteria) {}

  /** Compare two peaks using the stored criteria */
  inline bool operator()(const LeanElasticPeak &a, const LeanElasticPeak &b) {
    for (const auto &name : criteria) {
      const auto &col = name.first;
      const bool ascending = name.second;
      bool lessThan = false;
      if (col == "BankName") {
        // If this criterion is equal, move on to the next one
        const std::string valA = a.getBankName();
        const std::string valB = b.getBankName();
        // Move on to lesser criterion if equal
        if (valA == valB)
          continue;
        lessThan = (valA < valB);
      } else {
        // General double comparison
        const double valA = a.getValueByColName(col);
        const double valB = b.getValueByColName(col);
        // Move on to lesser criterion if equal
        if (valA == valB)
          continue;
        lessThan = (valA < valB);
      }
      // Flip the sign of comparison if descending.
      if (ascending)
        return lessThan;
      else
        return !lessThan;
    }
    // If you reach here, all criteria were ==; so not <, so return false
    return false;
  }
};

//---------------------------------------------------------------------------------------------
/** Sort the peaks by one or more criteria
 *
 * @param criteria : a vector with a list of pairs: column name, bool;
 *        where bool = true for ascending, false for descending sort.
 *        The peaks are sorted by the first criterion first, then the 2nd if
 *equal, etc.
 */
void LeanElasticPeaksWorkspace::sort(
    std::vector<ColumnAndDirection> &criteria) {
  PeakComparator comparator(criteria);
  std::stable_sort(peaks.begin(), peaks.end(), comparator);
}

//---------------------------------------------------------------------------------------------
/** @return the number of peaks
 */
int LeanElasticPeaksWorkspace::getNumberPeaks() const {
  return int(peaks.size());
}

//---------------------------------------------------------------------------------------------
/** @return the convention
 */
std::string LeanElasticPeaksWorkspace::getConvention() const {
  return convention;
}

//---------------------------------------------------------------------------------------------
/** Removes the indicated peak
 * @param peakNum  the peak to remove. peakNum starts at 0
 */
void LeanElasticPeaksWorkspace::removePeak(const int peakNum) {
  if (peakNum >= static_cast<int>(peaks.size()) || peakNum < 0) {
    throw std::invalid_argument(
        "LeanElasticPeaksWorkspace::removePeak(): peakNum is out of range.");
  }
  peaks.erase(peaks.begin() + peakNum);
}

/** Removes multiple peaks
 * @param badPeaks peaks to be removed
 */
void LeanElasticPeaksWorkspace::removePeaks(std::vector<int> badPeaks) {
  if (badPeaks.empty())
    return;
  // if index of peak is in badPeaks remove
  int ip = -1;
  auto it = std::remove_if(
      peaks.begin(), peaks.end(), [&ip, badPeaks](LeanElasticPeak &pk) {
        (void)pk;
        ip++;
        return std::any_of(badPeaks.cbegin(), badPeaks.cend(),
                           [ip](int badPeak) { return badPeak == ip; });
      });
  peaks.erase(it, peaks.end());
}

//---------------------------------------------------------------------------------------------
/** Add a peak to the list
 * @param ipeak :: Peak object to add (copy) into this.
 */
void LeanElasticPeaksWorkspace::addPeak(const Geometry::IPeak &ipeak) {
  if (dynamic_cast<const LeanElasticPeak *>(&ipeak)) {
    peaks.emplace_back((const LeanElasticPeak &)ipeak);
  } else {
    peaks.emplace_back(LeanElasticPeak(ipeak));
  }
}

//---------------------------------------------------------------------------------------------
/** Add a peak to the list
 * @param position :: position on the peak in the specified coordinate frame
 * @param frame :: the coordinate frame that the position is specified in
 */
void LeanElasticPeaksWorkspace::addPeak(const V3D &position,
                                        const SpecialCoordinateSystem &frame) {
  auto peak = createPeak(position, frame);
  addPeak(*peak);
}

//---------------------------------------------------------------------------------------------
/** Add a peak to the list
 * @param peak :: Peak object to add (move) into this.
 */
void LeanElasticPeaksWorkspace::addPeak(LeanElasticPeak &&peak) {
  peaks.emplace_back(peak);
}

//---------------------------------------------------------------------------------------------
/** Return a reference to the Peak
 * @param peakNum :: index of the peak to get.
 * @return a reference to a Peak object.
 */
LeanElasticPeak &LeanElasticPeaksWorkspace::getPeak(const int peakNum) {
  if (peakNum >= static_cast<int>(peaks.size()) || peakNum < 0) {
    throw std::invalid_argument(
        "LeanElasticPeaksWorkspace::getPeak(): peakNum is out of range.");
  }
  return peaks[peakNum];
}

//---------------------------------------------------------------------------------------------
/** Return a const reference to the Peak
 * @param peakNum :: index of the peak to get.
 * @return a reference to a Peak object.
 */
const LeanElasticPeak &
LeanElasticPeaksWorkspace::getPeak(const int peakNum) const {
  if (peakNum >= static_cast<int>(peaks.size()) || peakNum < 0) {
    throw std::invalid_argument(
        "LeanElasticPeaksWorkspace::getPeak(): peakNum is out of range.");
  }
  return peaks[peakNum];
}

//---------------------------------------------------------------------------------------------
/** Creates an instance of a Peak BUT DOES NOT ADD IT TO THE WORKSPACE
 * @param QLabFrame :: Q of the center of the peak, in reciprocal space
 * @param detectorDistance :: optional distance between the sample and the
 * detector. You do NOT need to explicitly provide this distance.
 * @return a pointer to a new Peak object.
 */
std::unique_ptr<Geometry::IPeak>
LeanElasticPeaksWorkspace::createPeak(const Kernel::V3D &,
                                      boost::optional<double>) const {
  throw Exception::NotImplementedError(
      "LeanElasticPeak should be create in q sample frame");
}

//---------------------------------------------------------------------------------------------
/** Creates an instance of a Peak BUT DOES NOT ADD IT TO THE WORKSPACE
 * @param position :: position of the center of the peak, in reciprocal space
 * @param frame :: the coordinate system that the position is specified in
 * detector. You do NOT need to explicitly provide this distance.
 * @return a pointer to a new Peak object.
 */
std::unique_ptr<Geometry::IPeak> LeanElasticPeaksWorkspace::createPeak(
    const Kernel::V3D &position,
    const Kernel::SpecialCoordinateSystem &frame) const {
  if (frame == Mantid::Kernel::HKL) {
    return createPeakHKL(position);
  } else if (frame == Mantid::Kernel::QLab) {
    return createPeak(position);
  } else {
    return createPeakQSample(position);
  }
}

//---------------------------------------------------------------------------------------------
/** Creates an instance of a Peak BUT DOES NOT ADD IT TO THE WORKSPACE
 * @param position :: QSample position of the center of the peak, in reciprocal
 * space
 * detector. You do NOT need to explicitly provide this distance.
 * @return a pointer to a new Peak object.
 */
std::unique_ptr<IPeak>
LeanElasticPeaksWorkspace::createPeakQSample(const V3D &position) const {
  // Create a peak from QSampleFrame
  std::unique_ptr<IPeak> peak =
      std::make_unique<LeanElasticPeak>(position, run().getGoniometer().getR());
  // Take the run number from this
  peak->setRunNumber(getRunNumber());
  return peak;
}

/**
 * Returns selected information for a "peak" at QLabFrame.
 *
 * @param qFrame      An arbitrary position in Q-space.  This does not have to
 *be the
 *                    position of a peak.
 * @param labCoords   Set true if the position is in the lab coordinate system,
 *false if
 *                    it is in the sample coordinate system.
 * @return a vector whose elements contain different information about the
 *"peak" at that position.
 *         each element is a pair of description of information and the string
 *form for the corresponding
 *         value.
 */
std::vector<std::pair<std::string, std::string>>
LeanElasticPeaksWorkspace::peakInfo(const Kernel::V3D &, bool) const {
  throw Exception::NotImplementedError("");
}

/**
 * Create a Peak from a HKL value provided by the client.
 *
 *
 * @param HKL : reciprocal lattice vector coefficients
 * @return Fully formed peak.
 */
std::unique_ptr<IPeak>
LeanElasticPeaksWorkspace::createPeakHKL(const V3D &HKL) const {
  /*
   The following allows us to add peaks where we have a single UB to work from.
   */
  const auto &lattice = this->sample().getOrientedLattice();
  const auto &goniometer = this->run().getGoniometer();

  // Calculate qLab from q HKL. As per Busing and Levy 1967, q_lab_frame = 2pi *
  // Goniometer * UB * HKL
  const V3D qSampleFrame = lattice.getUB() * HKL * 2 * M_PI;

  // create a peak using the qLab frame
  // This should calculate the detector positions too
  std::unique_ptr<IPeak> peak =
      std::make_unique<LeanElasticPeak>(qSampleFrame, goniometer.getR());
  // We need to set HKL separately to keep things consistent.
  peak->setHKL(HKL[0], HKL[1], HKL[2]);
  peak->setIntHKL(peak->getHKL());
  // Take the run number from this
  peak->setRunNumber(this->getRunNumber());

  return peak;
}

/**
 * Create a Peak using default values
 *
 * @return a point to a new peak object
 */
std::unique_ptr<IPeak> LeanElasticPeaksWorkspace::createPeak() const {
  return std::make_unique<LeanElasticPeak>();
}

/**
 * Returns selected information for a "peak" at QLabFrame.
 *
 * @param qFrame      An arbitrary position in Q-space.  This does not have to
 *be the
 *                    position of a peak.
 * @param labCoords  Set true if the position is in the lab coordinate system,
 *false if
 *                    it is in the sample coordinate system.
 * @return a vector whose elements contain different information about the
 *"peak" at that position.
 *         each element is a pair of description of information and the string
 *form for the corresponding
 *         value.
 */
int LeanElasticPeaksWorkspace::peakInfoNumber(const Kernel::V3D &, bool) const {
  throw Exception::NotImplementedError("");
}

//---------------------------------------------------------------------------------------------
/** Return a reference to the Peaks vector */
std::vector<LeanElasticPeak> &LeanElasticPeaksWorkspace::getPeaks() {
  return peaks;
}

/** Return a const reference to the Peaks vector */
const std::vector<LeanElasticPeak> &
LeanElasticPeaksWorkspace::getPeaks() const {
  return peaks;
}

/** Getter for the integration status.
 @return TRUE if it has been integrated using a peak integration algorithm.
 */
bool LeanElasticPeaksWorkspace::hasIntegratedPeaks() const {
  bool ret = false;
  const std::string peaksIntegrated = "PeaksIntegrated";
  if (this->run().hasProperty(peaksIntegrated)) {
    const auto value = boost::lexical_cast<int>(
        this->run().getProperty(peaksIntegrated)->value());
    ret = (value != 0);
  }
  return ret;
}

//---------------------------------------------------------------------------------------------
/// Return the memory used in bytes
size_t LeanElasticPeaksWorkspace::getMemorySize() const {
  return getNumberPeaks() * sizeof(LeanElasticPeak);
}

//---------------------------------------------------------------------------------------------
/**
 *  Creates a new TableWorkspace with detailing the contributing Detector IDs.
 * The table
 *  will have 2 columns: Index &  DetectorID, where Index maps into the current
 * index
 *  within the LeanElasticPeaksWorkspace of the peak
 */
API::ITableWorkspace_sptr
LeanElasticPeaksWorkspace::createDetectorTable() const {
  throw Exception::NotImplementedError("");
}

//---------------------------------------------------------------------------------------------
/** Initialize all columns */
void LeanElasticPeaksWorkspace::initColumns() {
  // Note: The column types are controlled in PeakColumn.cpp
  addPeakColumn("RunNumber");
  addPeakColumn("DetID");
  addPeakColumn("h");
  addPeakColumn("k");
  addPeakColumn("l");
  addPeakColumn("Wavelength");
  addPeakColumn("Energy");
  addPeakColumn("TOF");
  addPeakColumn("DSpacing");
  addPeakColumn("Intens");
  addPeakColumn("SigInt");
  addPeakColumn("Intens/SigInt");
  addPeakColumn("BinCount");
  addPeakColumn("BankName");
  addPeakColumn("Row");
  addPeakColumn("Col");
  addPeakColumn("QLab");
  addPeakColumn("QSample");
  addPeakColumn("PeakNumber");
  addPeakColumn("TBar");
}

//---------------------------------------------------------------------------------------------
/**
 * Add a PeakColumn
 * @param name :: The name of the column
 **/
void LeanElasticPeaksWorkspace::addPeakColumn(const std::string &name) {
  // Create the PeakColumn.
  columns.emplace_back(
      std::make_shared<DataObjects::LeanElasticPeakColumn>(this->peaks, name));
  // Cache the names
  columnNames.emplace_back(name);
}

//---------------------------------------------------------------------------------------------
/// @return the index of the column with the given name.
size_t
LeanElasticPeaksWorkspace::getColumnIndex(const std::string &name) const {
  for (size_t i = 0; i < columns.size(); i++)
    if (columns[i]->name() == name)
      return i;
  throw std::invalid_argument(
      "Column named " + name +
      " was not found in the LeanElasticPeaksWorkspace.");
}

//---------------------------------------------------------------------------------------------
/// Gets the shared pointer to a column by index.
std::shared_ptr<Mantid::API::Column>
LeanElasticPeaksWorkspace::getColumn(size_t index) {
  if (index >= columns.size())
    throw std::invalid_argument(
        "LeanElasticPeaksWorkspace::getColumn() called with invalid index.");
  return columns[index];
}

//---------------------------------------------------------------------------------------------
/// Gets the shared pointer to a column by index.
std::shared_ptr<const Mantid::API::Column>
LeanElasticPeaksWorkspace::getColumn(size_t index) const {
  if (index >= columns.size())
    throw std::invalid_argument(
        "LeanElasticPeaksWorkspace::getColumn() called with invalid index.");
  return columns[index];
}

void LeanElasticPeaksWorkspace::saveNexus(::NeXus::File *) const {
  throw Exception::NotImplementedError("");
}

/**
 * Set the special Q3D coordinate system.
 * @param coordinateSystem : Option to set.
 */
void LeanElasticPeaksWorkspace::setCoordinateSystem(
    const Kernel::SpecialCoordinateSystem coordinateSystem) {
  m_coordSystem = coordinateSystem;
}

/**
 * @return the special Q3D coordinate system.
 */
Kernel::SpecialCoordinateSystem
LeanElasticPeaksWorkspace::getSpecialCoordinateSystem() const {
  return m_coordSystem;
}

// prevent shared pointer from deleting this
struct NullDeleter {
  template <typename T> void operator()(T * /*unused*/) {}
};
/**Get access to shared pointer containing workspace porperties */
API::LogManager_sptr LeanElasticPeaksWorkspace::logs() {
  return API::LogManager_sptr(&(this->mutableRun()), NullDeleter());
}

/** Get constant access to shared pointer containing workspace porperties;
 * Copies logs into new LogManager variable Meaningfull only for some
 * multithereaded methods when a thread wants to have its own copy of logs */
API::LogManager_const_sptr LeanElasticPeaksWorkspace::getLogs() const {
  return API::LogManager_const_sptr(new API::LogManager(this->run()));
}

ITableWorkspace *LeanElasticPeaksWorkspace::doCloneColumns(
    const std::vector<std::string> & /*colNames*/) const {
  throw Kernel::Exception::NotImplementedError(
      "LeanElasticPeaksWorkspace cannot clone columns.");
}
} // namespace DataObjects
} // namespace Mantid

///\cond TEMPLATE

namespace Mantid {
namespace Kernel {

template <>
DLLExport Mantid::DataObjects::LeanElasticPeaksWorkspace_sptr
IPropertyManager::getValue<Mantid::DataObjects::LeanElasticPeaksWorkspace_sptr>(
    const std::string &name) const {
  auto *prop = dynamic_cast<
      PropertyWithValue<Mantid::DataObjects::LeanElasticPeaksWorkspace_sptr> *>(
      getPointerToProperty(name));
  if (prop) {
    return *prop;
  } else {
    std::string message =
        "Attempt to assign property " + name +
        " to incorrect type. Expected shared_ptr<LeanElasticPeaksWorkspace>.";
    throw std::runtime_error(message);
  }
}

template <>
DLLExport Mantid::DataObjects::LeanElasticPeaksWorkspace_const_sptr
IPropertyManager::getValue<
    Mantid::DataObjects::LeanElasticPeaksWorkspace_const_sptr>(
    const std::string &name) const {
  auto *prop = dynamic_cast<
      PropertyWithValue<Mantid::DataObjects::LeanElasticPeaksWorkspace_sptr> *>(
      getPointerToProperty(name));
  if (prop) {
    return prop->operator()();
  } else {
    std::string message = "Attempt to assign property " + name +
                          " to incorrect type. Expected const "
                          "shared_ptr<LeanElasticPeaksWorkspace>.";
    throw std::runtime_error(message);
  }
}

} // namespace Kernel
} // namespace Mantid

///\endcond TEMPLATE
