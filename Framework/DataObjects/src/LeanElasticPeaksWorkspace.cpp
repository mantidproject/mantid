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
#include "MantidNexusCpp/NeXusFile.hpp"

#include <cmath>

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::Geometry;

namespace Mantid::DataObjects {
/// Register the workspace as a type
DECLARE_WORKSPACE(LeanElasticPeaksWorkspace)

//---------------------------------------------------------------------------------------------
/** Constructor. Create a table with all the required columns.
 */
LeanElasticPeaksWorkspace::LeanElasticPeaksWorkspace()
    : IPeaksWorkspace(), m_peaks(), m_columns(), m_columnNames(), m_coordSystem(None) {
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
LeanElasticPeaksWorkspace::LeanElasticPeaksWorkspace(const LeanElasticPeaksWorkspace &other)
    : IPeaksWorkspace(other), m_peaks(other.m_peaks), m_columns(), m_columnNames(), m_coordSystem(other.m_coordSystem) {
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
  explicit PeakComparator(std::vector<ColumnAndDirection> &criteria) : criteria(criteria) {}

  /** Compare two peaks using the stored criteria */
  inline bool operator()(const LeanElasticPeak &a, const LeanElasticPeak &b) {
    for (const auto &name : criteria) {
      const auto &col = name.first;
      const bool ascending = name.second;
      bool lessThan = false;
      // General double comparison
      const double valA = a.getValueByColName(col);
      const double valB = b.getValueByColName(col);
      // Move on to lesser criterion if equal
      if (valA == valB)
        continue;
      lessThan = (valA < valB);
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
void LeanElasticPeaksWorkspace::sort(std::vector<ColumnAndDirection> &criteria) {
  PeakComparator comparator(criteria);
  std::stable_sort(m_peaks.begin(), m_peaks.end(), comparator);
}

//---------------------------------------------------------------------------------------------
/** @return the number of peaks
 */
int LeanElasticPeaksWorkspace::getNumberPeaks() const { return int(m_peaks.size()); }

//---------------------------------------------------------------------------------------------
/** @return the convention
 */
std::string LeanElasticPeaksWorkspace::getConvention() const { return m_convention; }

//---------------------------------------------------------------------------------------------
/** Removes the indicated peak
 * @param peakNum  the peak to remove. peakNum starts at 0
 */
void LeanElasticPeaksWorkspace::removePeak(const int peakNum) {
  if (peakNum >= static_cast<int>(m_peaks.size()) || peakNum < 0) {
    throw std::invalid_argument("LeanElasticPeaksWorkspace::removePeak(): peakNum is out of range.");
  }
  m_peaks.erase(m_peaks.begin() + peakNum);
}

/** Removes multiple peaks
 * @param badPeaks peaks to be removed
 */
void LeanElasticPeaksWorkspace::removePeaks(std::vector<int> badPeaks) {
  if (badPeaks.empty())
    return;
  // if index of peak is in badPeaks remove
  int ip = -1;
  auto it = std::remove_if(m_peaks.begin(), m_peaks.end(), [&ip, badPeaks](const LeanElasticPeak &pk) {
    (void)pk;
    ip++;
    return std::any_of(badPeaks.cbegin(), badPeaks.cend(), [ip](int badPeak) { return badPeak == ip; });
  });
  m_peaks.erase(it, m_peaks.end());
}

//---------------------------------------------------------------------------------------------
/** Add a peak to the list
 * @param ipeak :: Peak object to add (copy) into this.
 */
void LeanElasticPeaksWorkspace::addPeak(const Geometry::IPeak &ipeak) {
  if (dynamic_cast<const LeanElasticPeak *>(&ipeak)) {
    m_peaks.emplace_back(static_cast<const LeanElasticPeak &>(ipeak));
  } else {
    m_peaks.emplace_back(LeanElasticPeak(ipeak));
  }
}

//---------------------------------------------------------------------------------------------
/** Add a peak to the list
 * @param position :: position on the peak in the specified coordinate frame
 * @param frame :: the coordinate frame that the position is specified in
 */
void LeanElasticPeaksWorkspace::addPeak(const V3D &position, const SpecialCoordinateSystem &frame) {
  auto peak = createPeak(position, frame);
  addPeak(*peak);
}

//---------------------------------------------------------------------------------------------
/** Add a peak to the list
 * @param peak :: Peak object to add (move) into this.
 */
void LeanElasticPeaksWorkspace::addPeak(LeanElasticPeak &&peak) { m_peaks.emplace_back(peak); }

//---------------------------------------------------------------------------------------------
/** Return a reference to the Peak
 * @param peakNum :: index of the peak to get.
 * @return a reference to a Peak object.
 */
LeanElasticPeak &LeanElasticPeaksWorkspace::getPeak(const int peakNum) {
  if (peakNum >= static_cast<int>(m_peaks.size()) || peakNum < 0) {
    throw std::invalid_argument("LeanElasticPeaksWorkspace::getPeak(): peakNum is out of range.");
  }
  return m_peaks[peakNum];
}

//---------------------------------------------------------------------------------------------
/** Return a const reference to the Peak
 * @param peakNum :: index of the peak to get.
 * @return a reference to a Peak object.
 */
const LeanElasticPeak &LeanElasticPeaksWorkspace::getPeak(const int peakNum) const {
  if (peakNum >= static_cast<int>(m_peaks.size()) || peakNum < 0) {
    throw std::invalid_argument("LeanElasticPeaksWorkspace::getPeak(): peakNum is out of range.");
  }
  return m_peaks[peakNum];
}

//---------------------------------------------------------------------------------------------
/** Creates an instance of a Peak BUT DOES NOT ADD IT TO THE WORKSPACE
 * @param QLabFrame :: Q of the center of the peak, in reciprocal space
 * @param detectorDistance :: optional distance between the sample and the
 * detector. You do NOT need to explicitly provide this distance.
 * @return a pointer to a new Peak object.
 */
std::unique_ptr<Geometry::IPeak> LeanElasticPeaksWorkspace::createPeak(const Kernel::V3D &,
                                                                       std::optional<double>) const {
  throw Exception::NotImplementedError("LeanElasticPeak should be create in q sample frame");
}

//---------------------------------------------------------------------------------------------
/** Creates an instance of a Peak BUT DOES NOT ADD IT TO THE WORKSPACE
 * @param position :: position of the center of the peak, in reciprocal space
 * @param frame :: the coordinate system that the position is specified in
 * detector. You do NOT need to explicitly provide this distance.
 * @return a pointer to a new Peak object.
 */
std::unique_ptr<Geometry::IPeak>
LeanElasticPeaksWorkspace::createPeak(const Kernel::V3D &position, const Kernel::SpecialCoordinateSystem &frame) const {
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
std::unique_ptr<IPeak> LeanElasticPeaksWorkspace::createPeakQSample(const V3D &position) const {
  // Create a peak from QSampleFrame
  std::unique_ptr<IPeak> peak = std::make_unique<LeanElasticPeak>(position, run().getGoniometer().getR());
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
std::vector<std::pair<std::string, std::string>> LeanElasticPeaksWorkspace::peakInfo(const Kernel::V3D &, bool) const {
  throw Exception::NotImplementedError("");
}

/**
 * Create a Peak from a HKL value provided by the client.
 *
 *
 * @param HKL : reciprocal lattice vector coefficients
 * @return Fully formed peak.
 */
std::unique_ptr<IPeak> LeanElasticPeaksWorkspace::createPeakHKL(const V3D &HKL) const {
  /*
   The following allows us to add peaks where we have a single UB to work from.
   */
  const auto &lattice = this->sample().getOrientedLattice();
  const auto &goniometer = this->run().getGoniometer();

  // Calculate qSample from q HKL. As per Busing and Levy 1967, q_sample_frame =
  // 2pi * UB * HKL
  const V3D qSampleFrame = lattice.getUB() * HKL * 2 * M_PI;

  // create a peak using the qSample frame
  std::unique_ptr<IPeak> peak = std::make_unique<LeanElasticPeak>(qSampleFrame, goniometer.getR());
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
std::unique_ptr<IPeak> LeanElasticPeaksWorkspace::createPeak() const { return std::make_unique<LeanElasticPeak>(); }

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
std::vector<LeanElasticPeak> &LeanElasticPeaksWorkspace::getPeaks() { return m_peaks; }

/** Return a const reference to the Peaks vector */
const std::vector<LeanElasticPeak> &LeanElasticPeaksWorkspace::getPeaks() const { return m_peaks; }

/** Getter for the integration status.
 @return TRUE if it has been integrated using a peak integration algorithm.
 */
bool LeanElasticPeaksWorkspace::hasIntegratedPeaks() const {
  bool ret = false;
  const std::string peaksIntegrated = "PeaksIntegrated";
  if (this->run().hasProperty(peaksIntegrated)) {
    const auto value = boost::lexical_cast<int>(this->run().getProperty(peaksIntegrated)->value());
    ret = (value != 0);
  }
  return ret;
}

//---------------------------------------------------------------------------------------------
/// Return the memory used in bytes
size_t LeanElasticPeaksWorkspace::getMemorySize() const { return getNumberPeaks() * sizeof(LeanElasticPeak); }

//---------------------------------------------------------------------------------------------
/**
 *  Creates a new TableWorkspace with detailing the contributing Detector IDs.
 * The table
 *  will have 2 columns: Index &  DetectorID, where Index maps into the current
 * index
 *  within the LeanElasticPeaksWorkspace of the peak
 */
API::ITableWorkspace_sptr LeanElasticPeaksWorkspace::createDetectorTable() const {
  throw Exception::NotImplementedError("");
}

//---------------------------------------------------------------------------------------------
/** Initialize all columns */
void LeanElasticPeaksWorkspace::initColumns() {
  // Note: The column types are controlled in PeakColumn.cpp
  addPeakColumn("RunNumber");
  addPeakColumn("h");
  addPeakColumn("k");
  addPeakColumn("l");
  addPeakColumn("Wavelength");
  addPeakColumn("Energy");
  addPeakColumn("DSpacing");
  addPeakColumn("Intens");
  addPeakColumn("SigInt");
  addPeakColumn("Intens/SigInt");
  addPeakColumn("BinCount");
  addPeakColumn("QLab");
  addPeakColumn("QSample");
  addPeakColumn("PeakNumber");
  addPeakColumn("IntHKL");
  addPeakColumn("IntMNP");
}

//---------------------------------------------------------------------------------------------
/**
 * Add a PeakColumn
 * @param name :: The name of the column
 **/
void LeanElasticPeaksWorkspace::addPeakColumn(const std::string &name) {
  // Create the PeakColumn.
  m_columns.emplace_back(std::make_shared<DataObjects::PeakColumn<LeanElasticPeak>>(this->m_peaks, name));
  // Cache the names
  m_columnNames.emplace_back(name);
}

//---------------------------------------------------------------------------------------------
/// @return the index of the column with the given name.
size_t LeanElasticPeaksWorkspace::getColumnIndex(const std::string &name) const {
  for (size_t i = 0; i < m_columns.size(); i++)
    if (m_columns[i]->name() == name)
      return i;
  throw std::invalid_argument("Column named " + name + " was not found in the LeanElasticPeaksWorkspace.");
}

//---------------------------------------------------------------------------------------------
/// Gets the shared pointer to a column by index.
std::shared_ptr<Mantid::API::Column> LeanElasticPeaksWorkspace::getColumn(size_t index) {
  if (index >= m_columns.size())
    throw std::invalid_argument("LeanElasticPeaksWorkspace::getColumn() called with invalid index.");
  return m_columns[index];
}

//---------------------------------------------------------------------------------------------
/// Gets the shared pointer to a column by index.
std::shared_ptr<const Mantid::API::Column> LeanElasticPeaksWorkspace::getColumn(size_t index) const {
  if (index >= m_columns.size())
    throw std::invalid_argument("LeanElasticPeaksWorkspace::getColumn() called with invalid index.");
  return m_columns[index];
}

void LeanElasticPeaksWorkspace::saveNexus(::NeXus::File *file) const {

  // Number of Peaks
  const size_t np(m_peaks.size());

  // Column vectors for peaks table
  std::vector<double> H(np);
  std::vector<double> K(np);
  std::vector<double> L(np);
  std::vector<double> intensity(np);
  std::vector<double> sigmaIntensity(np);
  std::vector<double> binCount(np);
  std::vector<double> waveLength(np);
  std::vector<double> scattering(np);
  std::vector<double> dSpacing(np);
  std::vector<int> runNumber(np);
  std::vector<int> peakNumber(np);
  std::vector<double> tbar(np);
  std::vector<double> intHKL(3 * np);
  std::vector<double> intMNP(3 * np);
  std::vector<double> goniometerMatrix(9 * np);
  std::vector<std::string> shapes(np);
  std::vector<double> qlabs(3 * np);

  // Populate column vectors
  size_t maxShapeJSONLength = 0;
  for (size_t i = 0; i < np; i++) {
    LeanElasticPeak p = m_peaks[i];
    H[i] = p.getH();
    K[i] = p.getK();
    L[i] = p.getL();
    intensity[i] = p.getIntensity();
    sigmaIntensity[i] = p.getSigmaIntensity();
    binCount[i] = p.getBinCount();
    waveLength[i] = p.getWavelength();
    scattering[i] = p.getScattering();
    dSpacing[i] = p.getDSpacing();
    runNumber[i] = p.getRunNumber();
    peakNumber[i] = p.getPeakNumber();
    tbar[i] = p.getAbsorptionWeightedPathLength();
    {
      V3D hkl = p.getIntHKL();
      intHKL[3 * i + 0] = hkl[0];
      intHKL[3 * i + 1] = hkl[1];
      intHKL[3 * i + 2] = hkl[2];
      V3D mnp = p.getIntMNP();
      intMNP[3 * i + 0] = mnp[0];
      intMNP[3 * i + 1] = mnp[1];
      intMNP[3 * i + 2] = mnp[2];
      Matrix<double> gm = p.getGoniometerMatrix();
      goniometerMatrix[9 * i + 0] = gm[0][0];
      goniometerMatrix[9 * i + 1] = gm[1][0];
      goniometerMatrix[9 * i + 2] = gm[2][0];
      goniometerMatrix[9 * i + 3] = gm[0][1];
      goniometerMatrix[9 * i + 4] = gm[1][1];
      goniometerMatrix[9 * i + 5] = gm[2][1];
      goniometerMatrix[9 * i + 6] = gm[0][2];
      goniometerMatrix[9 * i + 7] = gm[1][2];
      goniometerMatrix[9 * i + 8] = gm[2][2];
    }
    const std::string shapeJSON = p.getPeakShape().toJSON();
    shapes[i] = shapeJSON;
    if (shapeJSON.size() > maxShapeJSONLength) {
      maxShapeJSONLength = shapeJSON.size();
    }
    {
      qlabs[3 * i + 0] = p.getQLabFrame().X();
      qlabs[3 * i + 1] = p.getQLabFrame().Y();
      qlabs[3 * i + 2] = p.getQLabFrame().Z();
    }
  }

  // Start Peaks Workspace in Nexus File
  const std::string specifyInteger = "An integer";
  const std::string specifyDouble = "A double";
  const std::string specifyString = "A string";
  file->makeGroup("peaks_workspace", "NXentry",
                  true); // For when peaksWorkspace can be loaded

  // Coordinate system
  file->writeData("coordinate_system", static_cast<uint32_t>(m_coordSystem));

  // Write out the Qconvention
  // ki-kf for Inelastic convention; kf-ki for Crystallography convention
  std::string m_QConvention = this->getConvention();
  file->putAttr("QConvention", m_QConvention);

  // H column
  file->writeData("column_1", H);
  file->openData("column_1");
  file->putAttr("name", "H");
  file->putAttr("interpret_as", specifyDouble);
  file->putAttr("units", "Not known"); // Units may need changing when known
  file->closeData();

  // K column
  file->writeData("column_2", K);
  file->openData("column_2");
  file->putAttr("name", "K");
  file->putAttr("interpret_as", specifyDouble);
  file->putAttr("units", "Not known"); // Units may need changing when known
  file->closeData();

  // L column
  file->writeData("column_3", L);
  file->openData("column_3");
  file->putAttr("name", "L");
  file->putAttr("interpret_as", specifyDouble);
  file->putAttr("units", "Not known"); // Units may need changing when known
  file->closeData();

  // Intensity column
  file->writeData("column_4", intensity);
  file->openData("column_4");
  file->putAttr("name", "Intensity");
  file->putAttr("interpret_as", specifyDouble);
  file->putAttr("units", "Not known"); // Units may need changing when known
  file->closeData();

  // Sigma Intensity column
  file->writeData("column_5", sigmaIntensity);
  file->openData("column_5");
  file->putAttr("name", "Sigma Intensity");
  file->putAttr("interpret_as", specifyDouble);
  file->putAttr("units", "Not known"); // Units may need changing when known
  file->closeData();

  // Bin Count column
  file->writeData("column_6", binCount);
  file->openData("column_6");
  file->putAttr("name", "Bin Count");
  file->putAttr("interpret_as", specifyDouble);
  file->putAttr("units", "Not known"); // Units may need changing when known
  file->closeData();

  // Wave Length Column
  file->writeData("column_7", waveLength);
  file->openData("column_7");
  file->putAttr("name", "Wave Length");
  file->putAttr("interpret_as", specifyDouble);
  file->putAttr("units", "Not known"); // Units may need changing when known
  file->closeData();

  // Scattering Column
  file->writeData("column_8", scattering);
  file->openData("column_8");
  file->putAttr("name", "Scattering");
  file->putAttr("interpret_as", specifyDouble);
  file->putAttr("units", "Not known"); // Units may need changing when known
  file->closeData();

  // D Spacing Column
  file->writeData("column_9", dSpacing);
  file->openData("column_9");
  file->putAttr("name", "D Spacing");
  file->putAttr("interpret_as", specifyDouble);
  file->putAttr("units", "Not known"); // Units may need changing when known
  file->closeData();

  // Run Number column
  file->writeData("column_10", runNumber);
  file->openData("column_10");
  file->putAttr("name", "Run Number");
  file->putAttr("interpret_as", specifyInteger);
  file->putAttr("units", "Not known"); // Units may need changing when known
  file->closeData();

  // Peak Number column
  file->writeData("column_11", peakNumber);
  file->openData("column_11");
  file->putAttr("name", "Peak Number");
  file->putAttr("interpret_as", specifyInteger);
  file->putAttr("units", "Not known"); // Units may need changing when known
  file->closeData();

  // TBar column
  file->writeData("column_12", tbar);
  file->openData("column_12");
  file->putAttr("name", "TBar");
  file->putAttr("interpret_as", specifyDouble);
  file->putAttr("units", "Not known"); // Units may need changing when known
  file->closeData();

  // Goniometer Matrix Column
  std::vector<int> array_dims;
  array_dims.emplace_back(static_cast<int>(m_peaks.size()));
  array_dims.emplace_back(9);
  file->writeData("column_13", goniometerMatrix, array_dims);
  file->openData("column_13");
  file->putAttr("name", "Goniometer Matrix");
  file->putAttr("interpret_as", "A matrix of 3x3 doubles");
  file->putAttr("units", "Not known"); // Units may need changing when known
  file->closeData();

  // Shape
  std::vector<int64_t> dims;
  dims.emplace_back(np);
  dims.emplace_back(static_cast<int>(maxShapeJSONLength));
  const std::string name = "column_14";
  file->makeData(name, NXnumtype::CHAR, dims, false);
  file->openData(name);

  auto toNexus = new char[maxShapeJSONLength * np];
  for (size_t ii = 0; ii < np; ii++) {
    std::string rowStr = shapes[ii];
    for (size_t ic = 0; ic < rowStr.size(); ic++)
      toNexus[ii * maxShapeJSONLength + ic] = rowStr[ic];
    for (size_t ic = rowStr.size(); ic < static_cast<size_t>(maxShapeJSONLength); ic++)
      toNexus[ii * maxShapeJSONLength + ic] = ' ';
  }

  file->putData(static_cast<void *>(toNexus));

  delete[] toNexus;
  file->putAttr("units", "Not known"); // Units may need changing when known
  file->putAttr("name", "Shape");
  file->putAttr("interpret_as", specifyString);
  file->closeData();

  // Qlab
  std::vector<int> qlab_dims;
  qlab_dims.emplace_back(static_cast<int>(m_peaks.size()));
  qlab_dims.emplace_back(3);
  file->writeData("column_15", qlabs, qlab_dims);
  file->openData("column_15");
  file->putAttr("name", "Q LabFrame");
  file->putAttr("interpret_as", "A vector of 3 doubles");
  file->putAttr("units", "angstrom^-1");
  file->closeData();

  // Integer HKL column
  file->writeData("column_16", intHKL, qlab_dims);
  file->openData("column_16");
  file->putAttr("name", "IntHKL");
  file->putAttr("interpret_as", "A vector of 3 doubles");
  file->putAttr("units", "r.l.u.");
  file->closeData();

  // Integer HKL column
  file->writeData("column_17", intMNP, qlab_dims);
  file->openData("column_17");
  file->putAttr("name", "IntMNP");
  file->putAttr("interpret_as", "A vector of 3 doubles");
  file->putAttr("units", "r.l.u.");
  file->closeData();

  file->closeGroup(); // end of peaks workpace
}

/**
 * Set the special Q3D coordinate system.
 * @param coordinateSystem : Option to set.
 */
void LeanElasticPeaksWorkspace::setCoordinateSystem(const Kernel::SpecialCoordinateSystem coordinateSystem) {
  m_coordSystem = coordinateSystem;
}

/**
 * @return the special Q3D coordinate system.
 */
Kernel::SpecialCoordinateSystem LeanElasticPeaksWorkspace::getSpecialCoordinateSystem() const { return m_coordSystem; }

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

ITableWorkspace *LeanElasticPeaksWorkspace::doCloneColumns(const std::vector<std::string> & /*colNames*/) const {
  throw Kernel::Exception::NotImplementedError("LeanElasticPeaksWorkspace cannot clone columns.");
}
} // namespace Mantid::DataObjects

///\cond TEMPLATE

namespace Mantid::Kernel {

template <>
DLLExport Mantid::DataObjects::LeanElasticPeaksWorkspace_sptr
IPropertyManager::getValue<Mantid::DataObjects::LeanElasticPeaksWorkspace_sptr>(const std::string &name) const {
  auto *prop = dynamic_cast<PropertyWithValue<Mantid::DataObjects::LeanElasticPeaksWorkspace_sptr> *>(
      getPointerToProperty(name));
  if (prop) {
    return *prop;
  } else {
    std::string message =
        "Attempt to assign property " + name + " to incorrect type. Expected shared_ptr<LeanElasticPeaksWorkspace>.";
    throw std::runtime_error(message);
  }
}

template <>
DLLExport Mantid::DataObjects::LeanElasticPeaksWorkspace_const_sptr
IPropertyManager::getValue<Mantid::DataObjects::LeanElasticPeaksWorkspace_const_sptr>(const std::string &name) const {
  if (const auto *prop = dynamic_cast<PropertyWithValue<Mantid::DataObjects::LeanElasticPeaksWorkspace_sptr> *>(
          getPointerToProperty(name))) {
    return prop->operator()();
  } else {
    std::string message = "Attempt to assign property " + name +
                          " to incorrect type. Expected const "
                          "shared_ptr<LeanElasticPeaksWorkspace>.";
    throw std::runtime_error(message);
  }
}

} // namespace Mantid::Kernel

///\endcond TEMPLATE
