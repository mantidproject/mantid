// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidAPI/AlgorithmFactory.h"
#include "MantidAPI/Column.h"
#include "MantidAPI/ColumnFactory.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/Sample.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidDataObjects/Peak.h"
#include "MantidDataObjects/TableColumn.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"
#include "MantidGeometry/Instrument/Goniometer.h"
#include "MantidKernel/DateAndTime.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/PhysicalConstants.h"
#include "MantidKernel/Quat.h"
#include "MantidKernel/Unit.h"
#include "MantidKernel/UnitConversion.h"
#include "MantidKernel/V3D.h"

#include <algorithm>
#include <boost/shared_ptr.hpp>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <fstream>
// clang-format off
#include <nexus/NeXusFile.hpp>
#include <nexus/NeXusException.hpp>
// clang-format on
#include <ostream>
#include <string>

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::Geometry;

namespace Mantid {
namespace DataObjects {
/// Register the workspace as a type
DECLARE_WORKSPACE(PeaksWorkspace)

Mantid::Kernel::Logger g_log("PeaksWorkspace");

//---------------------------------------------------------------------------------------------
/** Constructor. Create a table with all the required columns.
 *
 * @return PeaksWorkspace object
 */
PeaksWorkspace::PeaksWorkspace()
    : IPeaksWorkspace(), peaks(), columns(), columnNames(),
      m_coordSystem(None) {
  initColumns();
  // PeaksWorkspace does not use the grouping mechanism of ExperimentInfo.
  setNumberOfDetectorGroups(0);
}

//---------------------------------------------------------------------------------------------
/** Copy constructor
 *
 * @param other :: other PeaksWorkspace to copy from
 * @return
 */
PeaksWorkspace::PeaksWorkspace(const PeaksWorkspace &other)
    : IPeaksWorkspace(other), peaks(other.peaks), columns(), columnNames(),
      m_coordSystem(other.m_coordSystem) {
  initColumns();
  // PeaksWorkspace does not use the grouping mechanism of ExperimentInfo.
  setNumberOfDetectorGroups(0);
}

//=====================================================================================
//=====================================================================================
/** Comparator class for sorting peaks by one or more criteria
 */
class PeakComparator : public std::binary_function<Peak, Peak, bool> {
public:
  std::vector<std::pair<std::string, bool>> &criteria;

  /** Constructor for the comparator for sorting peaks
   * @param criteria : a vector with a list of pairs: column name, bool;
   *        where bool = true for ascending, false for descending sort.
   */
  explicit PeakComparator(std::vector<std::pair<std::string, bool>> &criteria)
      : criteria(criteria) {}

  /** Compare two peaks using the stored criteria */
  inline bool operator()(const Peak &a, const Peak &b) {
    for (auto &name : criteria) {
      std::string &col = name.first;
      bool ascending = name.second;
      bool lessThan = false;
      if (col == "BankName") {
        // If this criterion is equal, move on to the next one
        std::string valA = a.getBankName();
        std::string valB = b.getBankName();
        // Move on to lesser criterion if equal
        if (valA == valB)
          continue;
        lessThan = (valA < valB);
      } else {
        // General double comparison
        double valA = a.getValueByColName(col);
        double valB = b.getValueByColName(col);
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
void PeaksWorkspace::sort(std::vector<std::pair<std::string, bool>> &criteria) {
  PeakComparator comparator(criteria);
  std::stable_sort(peaks.begin(), peaks.end(), comparator);
}

//---------------------------------------------------------------------------------------------
/** @return the number of peaks
 */
int PeaksWorkspace::getNumberPeaks() const { return int(peaks.size()); }

//---------------------------------------------------------------------------------------------
/** @return the convention
 */
std::string PeaksWorkspace::getConvention() const { return convention; }

//---------------------------------------------------------------------------------------------
/** Removes the indicated peak
 * @param peakNum  the peak to remove. peakNum starts at 0
 */
void PeaksWorkspace::removePeak(const int peakNum) {
  if (peakNum >= static_cast<int>(peaks.size()) || peakNum < 0) {
    throw std::invalid_argument(
        "PeaksWorkspace::removePeak(): peakNum is out of range.");
  }
  peaks.erase(peaks.begin() + peakNum);
}

/** Removes multiple peaks
 * @param badPeaks peaks to be removed
 */
void PeaksWorkspace::removePeaks(std::vector<int> badPeaks) {
  if (badPeaks.empty())
    return;
  // if index of peak is in badPeaks remove
  int ip = -1;
  auto it =
      std::remove_if(peaks.begin(), peaks.end(), [&ip, badPeaks](Peak &pk) {
        (void)pk;
        ip++;
        for (int badPeak : badPeaks) {
          if (badPeak == ip)
            return true;
        }
        return false;
      });
  peaks.erase(it, peaks.end());
}

//---------------------------------------------------------------------------------------------
/** Add a peak to the list
 * @param ipeak :: Peak object to add (copy) into this.
 */
void PeaksWorkspace::addPeak(const Geometry::IPeak &ipeak) {
  if (dynamic_cast<const Peak *>(&ipeak)) {
    peaks.push_back((const Peak &)ipeak);
  } else {
    peaks.push_back(Peak(ipeak));
  }
}

//---------------------------------------------------------------------------------------------
/** Add a peak to the list
 * @param position :: position on the peak in the specified coordinate frame
 * @param frame :: the coordinate frame that the position is specified in
 */
void PeaksWorkspace::addPeak(const V3D &position,
                             const SpecialCoordinateSystem &frame) {
  auto peak = createPeak(position, frame);
  addPeak(*peak);
}

//---------------------------------------------------------------------------------------------
/** Add a peak to the list
 * @param peak :: Peak object to add (move) into this.
 */
void PeaksWorkspace::addPeak(Peak &&peak) { peaks.push_back(peak); }

//---------------------------------------------------------------------------------------------
/** Return a reference to the Peak
 * @param peakNum :: index of the peak to get.
 * @return a reference to a Peak object.
 */
Peak &PeaksWorkspace::getPeak(const int peakNum) {
  if (peakNum >= static_cast<int>(peaks.size()) || peakNum < 0) {
    throw std::invalid_argument(
        "PeaksWorkspace::getPeak(): peakNum is out of range.");
  }
  return peaks[peakNum];
}

//---------------------------------------------------------------------------------------------
/** Return a const reference to the Peak
 * @param peakNum :: index of the peak to get.
 * @return a reference to a Peak object.
 */
const Peak &PeaksWorkspace::getPeak(const int peakNum) const {
  if (peakNum >= static_cast<int>(peaks.size()) || peakNum < 0) {
    throw std::invalid_argument(
        "PeaksWorkspace::getPeak(): peakNum is out of range.");
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
Geometry::IPeak *
PeaksWorkspace::createPeak(const Kernel::V3D &QLabFrame,
                           boost::optional<double> detectorDistance) const {
  Geometry::Goniometer goniometer = this->run().getGoniometer();

  // create a peak using the qLab frame
  auto peak = new Peak(this->getInstrument(), QLabFrame, detectorDistance);

  // Set the goniometer
  peak->setGoniometerMatrix(goniometer.getR());

  // Take the run number from this
  peak->setRunNumber(this->getRunNumber());

  return peak;
}

//---------------------------------------------------------------------------------------------
/** Creates an instance of a Peak BUT DOES NOT ADD IT TO THE WORKSPACE
 * @param position :: position of the center of the peak, in reciprocal space
 * @param frame :: the coordinate system that the position is specified in
 * detector. You do NOT need to explicitly provide this distance.
 * @return a pointer to a new Peak object.
 */
std::unique_ptr<Geometry::IPeak>
PeaksWorkspace::createPeak(const Kernel::V3D &position,
                           const Kernel::SpecialCoordinateSystem &frame) const {
  if (frame == Mantid::Kernel::HKL) {
    return std::unique_ptr<Geometry::IPeak>(createPeakHKL(position));
  } else if (frame == Mantid::Kernel::QLab) {
    return std::unique_ptr<Geometry::IPeak>(createPeak(position));
  } else {
    return std::unique_ptr<Geometry::IPeak>(createPeakQSample(position));
  }
}

//---------------------------------------------------------------------------------------------
/** Creates an instance of a Peak BUT DOES NOT ADD IT TO THE WORKSPACE
 * @param position :: QSample position of the center of the peak, in reciprocal
 * space
 * detector. You do NOT need to explicitly provide this distance.
 * @return a pointer to a new Peak object.
 */
Peak *PeaksWorkspace::createPeakQSample(const V3D &position) const {
  // Create a peak from QSampleFrame

  Geometry::Goniometer goniometer;

  LogManager_const_sptr props = getLogs();
  // See if we can get a wavelength/energy
  // Then assume constant wavelenth
  double wavelength(0);
  if (props->hasProperty("wavelength")) {
    wavelength = props->getPropertyValueAsType<double>("wavelength");
  } else if (props->hasProperty("energy")) {
    wavelength = Kernel::UnitConversion::run(
        "Energy", "Wavelength", props->getPropertyValueAsType<double>("energy"),
        0, 0, 0, Kernel::DeltaEMode::Elastic, 0);
  } else if (getInstrument()->hasParameter("wavelength")) {
    wavelength = getInstrument()->getNumberParameter("wavelength").at(0);
  }

  if (wavelength > 0) {
    goniometer.calcFromQSampleAndWavelength(position, wavelength);
    g_log.information() << "Found goniometer rotation to be "
                        << goniometer.getEulerAngles()[0]
                        << " degrees for Q sample = " << position << "\n";
  } else {
    goniometer = run().getGoniometer();
  }
  // create a peak using the qLab frame
  auto peak = new Peak(getInstrument(), position, goniometer.getR());
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
PeaksWorkspace::peakInfo(const Kernel::V3D &qFrame, bool labCoords) const {
  std::vector<std::pair<std::string, std::string>> Result;
  std::ostringstream oss;
  oss << std::setw(12) << std::fixed << std::setprecision(3) << (qFrame.norm());
  std::pair<std::string, std::string> QMag("|Q|", oss.str());
  Result.push_back(QMag);

  oss.str("");
  oss.clear();
  oss << std::setw(12) << std::fixed << std::setprecision(3)
      << (2.0 * M_PI / qFrame.norm());

  std::pair<std::string, std::string> dspc("d-spacing", oss.str());
  oss.str("");
  oss.clear();
  Result.push_back(dspc);

  int seqNum = -1;
  bool hasOneRunNumber = true;
  int runNum = -1;
  int NPeaks = getNumberPeaks();
  try {
    double minDist = 10000000;
    for (int i = 0; i < NPeaks; i++) {
      Peak pk = getPeak(i);
      V3D Q = pk.getQLabFrame();
      if (!labCoords)
        Q = pk.getQSampleFrame();
      double D = qFrame.distance(Q);
      if (D < minDist) {
        minDist = D;
        seqNum = i;
      }

      int run = pk.getRunNumber();
      if (runNum < 0)
        runNum = run;
      else if (runNum != run)
        hasOneRunNumber = false;
    }
  } catch (...) {
    seqNum = -1; // peak could have been removed
  }
  V3D Qlab = qFrame;
  V3D Qsamp;
  Kernel::Matrix<double> Gon(3, 3, true);

  if (seqNum >= 0 && NPeaks == getNumberPeaks())
    Gon = getPeak(seqNum).getGoniometerMatrix();
  if (labCoords) {

    Kernel::Matrix<double> InvGon(Gon);
    InvGon.Invert();
    Qsamp = InvGon * Qlab;

  } else {
    Qsamp = qFrame;
    Qlab = Gon * Qsamp;
  }

  if (labCoords || seqNum >= 0)

  {
    std::pair<std::string, std::string> QlabStr(
        "Qlab", boost::lexical_cast<std::string>(Qlab));
    Result.push_back(QlabStr);
  }

  if (!labCoords || seqNum >= 0) {

    std::pair<std::string, std::string> QsampStr(
        "QSample", boost::lexical_cast<std::string>(Qsamp));
    Result.push_back(QsampStr);
  }

  try {

    Geometry::IPeak *peak = createPeak(Qlab);

    if (sample().hasOrientedLattice()) {

      peak->setGoniometerMatrix(Gon);
      const Geometry::OrientedLattice &lat = (sample().getOrientedLattice());

      const Kernel::Matrix<double> &UB0 = lat.getUB();
      Kernel::Matrix<double> UB(UB0);
      UB.Invert();
      V3D hkl = UB * Qsamp / 2 / M_PI;

      std::pair<std::string, std::string> HKL(
          "HKL", boost::lexical_cast<std::string>(hkl));
      Result.push_back(HKL);
    }

    if (hasOneRunNumber) {
      std::pair<std::string, std::string> runn("RunNumber",
                                               "   " + std::to_string(runNum));
      Result.push_back(runn);
    }

    //------- Now get phi, chi and omega ----------------
    Geometry::Goniometer GonG(Gon);
    std::vector<double> OmegaChiPhi = GonG.getEulerAngles("YZY");
    Kernel::V3D PhiChiOmega(OmegaChiPhi[2], OmegaChiPhi[1], OmegaChiPhi[0]);

    std::pair<std::string, std::string> GRead(
        "Goniometer Angles", boost::lexical_cast<std::string>(PhiChiOmega));
    Result.push_back(GRead);

    std::pair<std::string, std::string> SeqNum(
        "Seq Num,1st=1", "    " + std::to_string(seqNum + 1));
    Result.push_back(SeqNum);

    oss << std::setw(12) << std::fixed << std::setprecision(3)
        << (peak->getWavelength());
    std::pair<std::string, std::string> wl("Wavelength", oss.str());
    Result.push_back(wl);
    oss.str("");
    oss.clear();

    if (peak->findDetector()) {
      std::pair<std::string, std::string> detpos(
          "Position(x,y,z)",
          boost::lexical_cast<std::string>(peak->getDetPos()));
      Result.push_back(detpos);

      oss << std::setw(15) << std::fixed << std::setprecision(3)
          << (peak->getTOF());
      std::pair<std::string, std::string> tof("TOF", oss.str());
      Result.push_back(tof);
      oss.str("");
      oss.clear();

      oss << std::setw(12) << std::fixed << std::setprecision(3)
          << (peak->getFinalEnergy());
      std::pair<std::string, std::string> Energy("Energy", oss.str());
      Result.push_back(Energy);
      oss.str("");
      oss.clear();

      std::pair<std::string, std::string> row(
          "Row", "    " + std::to_string(peak->getRow()));
      Result.push_back(row);

      std::pair<std::string, std::string> col(
          "Col", "    " + std::to_string(peak->getCol()));
      Result.push_back(col);

      std::pair<std::string, std::string> bank("Bank",
                                               "    " + peak->getBankName());
      Result.push_back(bank);

      oss << std::setw(12) << std::fixed << std::setprecision(3)
          << (peak->getScattering());
      std::pair<std::string, std::string> scat("Scattering Angle", oss.str());
      Result.push_back(scat);
    }

  } catch (...) // Impossible position
  {
  }
  return Result;
}

/**
 * Create a Peak from a HKL value provided by the client.
 *
 *
 * @param HKL : reciprocal lattice vector coefficients
 * @return Fully formed peak.
 */
Peak *PeaksWorkspace::createPeakHKL(const V3D &HKL) const {
  /*
   The following allows us to add peaks where we have a single UB to work from.
   */

  Geometry::OrientedLattice lattice = this->sample().getOrientedLattice();
  Geometry::Goniometer goniometer = this->run().getGoniometer();

  // Calculate qLab from q HKL. As per Busing and Levy 1967, q_lab_frame = 2pi *
  // Goniometer * UB * HKL
  V3D qLabFrame = goniometer.getR() * lattice.getUB() * HKL * 2 * M_PI;

  // create a peak using the qLab frame
  auto peak =
      new Peak(this->getInstrument(),
               qLabFrame); // This should calculate the detector positions too.

  // We need to set HKL separately to keep things consistent.
  peak->setHKL(HKL[0], HKL[1], HKL[2]);

  // Set the goniometer
  peak->setGoniometerMatrix(goniometer.getR());

  // Take the run number from this
  peak->setRunNumber(this->getRunNumber());

  return peak;
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
int PeaksWorkspace::peakInfoNumber(const Kernel::V3D &qFrame,
                                   bool labCoords) const {
  std::vector<std::pair<std::string, std::string>> Result;
  std::ostringstream oss;
  oss << std::setw(12) << std::fixed << std::setprecision(3) << (qFrame.norm());
  std::pair<std::string, std::string> QMag("|Q|", oss.str());
  Result.push_back(QMag);

  oss.str("");
  oss.clear();
  oss << std::setw(12) << std::fixed << std::setprecision(3)
      << (2.0 * M_PI / qFrame.norm());

  std::pair<std::string, std::string> dspc("d-spacing", oss.str());
  oss.str("");
  oss.clear();
  Result.push_back(dspc);

  int seqNum = -1;
  double minDist = 10000000;

  for (int i = 0; i < getNumberPeaks(); i++) {
    Peak pk = getPeak(i);
    V3D Q = pk.getQLabFrame();
    if (!labCoords)
      Q = pk.getQSampleFrame();
    double D = qFrame.distance(Q);
    if (D < minDist) {
      minDist = D;
      seqNum = i + 1;
    }
  }
  return seqNum;
}

//---------------------------------------------------------------------------------------------
/** Return a reference to the Peaks vector */
std::vector<Peak> &PeaksWorkspace::getPeaks() { return peaks; }

/** Return a const reference to the Peaks vector */
const std::vector<Peak> &PeaksWorkspace::getPeaks() const { return peaks; }

/** Getter for the integration status.
 @return TRUE if it has been integrated using a peak integration algorithm.
 */
bool PeaksWorkspace::hasIntegratedPeaks() const {
  bool ret = false;
  const std::string peaksIntegrated = "PeaksIntegrated";
  if (this->run().hasProperty(peaksIntegrated)) {
    const int value = boost::lexical_cast<int>(
        this->run().getProperty(peaksIntegrated)->value());
    ret = (value != 0);
  }
  return ret;
}

//---------------------------------------------------------------------------------------------
/// Return the memory used in bytes
size_t PeaksWorkspace::getMemorySize() const {
  return getNumberPeaks() * sizeof(Peak);
}

//---------------------------------------------------------------------------------------------
/**
 *  Creates a new TableWorkspace with detailing the contributing Detector IDs.
 * The table
 *  will have 2 columns: Index &  DetectorID, where Index maps into the current
 * index
 *  within the PeaksWorkspace of the peak
 */
API::ITableWorkspace_sptr PeaksWorkspace::createDetectorTable() const {
  auto table = API::WorkspaceFactory::Instance().createTable("TableWorkspace");
  table->addColumn("int", "Index");
  table->addColumn("int", "DetectorID");

  const int npeaks(static_cast<int>(this->rowCount()));
  int nrows(0);
  for (int i = 0; i < npeaks; ++i) {
    const Peak &peak = this->peaks[i];
    auto detIDs = peak.getContributingDetIDs();
    auto itEnd = detIDs.end();
    for (auto it = detIDs.begin(); it != itEnd; ++it) {
      table->appendRow();
      table->cell<int>(nrows, 0) = i;
      table->cell<int>(nrows, 1) = *it;
      ++nrows;
    }
  }

  return table;
}

//---------------------------------------------------------------------------------------------
/** Initialize all columns */
void PeaksWorkspace::initColumns() {
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
  addPeakColumn("BinCount");
  addPeakColumn("BankName");
  addPeakColumn("Row");
  addPeakColumn("Col");
  addPeakColumn("QLab");
  addPeakColumn("QSample");
  addPeakColumn("PeakNumber");
}

//---------------------------------------------------------------------------------------------
/**
 * Add a PeakColumn
 * @param name :: The name of the column
 **/
void PeaksWorkspace::addPeakColumn(const std::string &name) {
  // Create the PeakColumn.
  columns.push_back(boost::shared_ptr<DataObjects::PeakColumn>(
      new DataObjects::PeakColumn(this->peaks, name)));
  // Cache the names
  columnNames.push_back(name);
}

//---------------------------------------------------------------------------------------------
/// @return the index of the column with the given name.
size_t PeaksWorkspace::getColumnIndex(const std::string &name) const {
  for (size_t i = 0; i < columns.size(); i++)
    if (columns[i]->name() == name)
      return i;
  throw std::invalid_argument("Column named " + name +
                              " was not found in the PeaksWorkspace.");
}

//---------------------------------------------------------------------------------------------
/// Gets the shared pointer to a column by index.
boost::shared_ptr<Mantid::API::Column> PeaksWorkspace::getColumn(size_t index) {
  if (index >= columns.size())
    throw std::invalid_argument(
        "PeaksWorkspace::getColumn() called with invalid index.");
  return columns[index];
}

//---------------------------------------------------------------------------------------------
/// Gets the shared pointer to a column by index.
boost::shared_ptr<const Mantid::API::Column>
PeaksWorkspace::getColumn(size_t index) const {
  if (index >= columns.size())
    throw std::invalid_argument(
        "PeaksWorkspace::getColumn() called with invalid index.");
  return columns[index];
}

void PeaksWorkspace::saveNexus(::NeXus::File *file) const {

  // Number of Peaks
  const size_t np(peaks.size());

  // Column vectors for peaks table
  std::vector<int> detectorID(np);
  std::vector<double> H(np);
  std::vector<double> K(np);
  std::vector<double> L(np);
  std::vector<double> intensity(np);
  std::vector<double> sigmaIntensity(np);
  std::vector<double> binCount(np);
  std::vector<double> initialEnergy(np);
  std::vector<double> finalEnergy(np);
  std::vector<double> waveLength(np);
  std::vector<double> scattering(np);
  std::vector<double> dSpacing(np);
  std::vector<double> TOF(np);
  std::vector<int> runNumber(np);
  std::vector<int> peakNumber(np);
  std::vector<double> goniometerMatrix(9 * np);
  std::vector<std::string> shapes(np);

  // Populate column vectors from Peak Workspace
  size_t maxShapeJSONLength = 0;
  for (size_t i = 0; i < np; i++) {
    Peak p = peaks[i];
    detectorID[i] = p.getDetectorID();
    H[i] = p.getH();
    K[i] = p.getK();
    L[i] = p.getL();
    intensity[i] = p.getIntensity();
    sigmaIntensity[i] = p.getSigmaIntensity();
    binCount[i] = p.getBinCount();
    initialEnergy[i] = p.getInitialEnergy();
    finalEnergy[i] = p.getFinalEnergy();
    waveLength[i] = p.getWavelength();
    scattering[i] = p.getScattering();
    dSpacing[i] = p.getDSpacing();
    TOF[i] = p.getTOF();
    runNumber[i] = p.getRunNumber();
    peakNumber[i] = p.getPeakNumber();
    {
      Matrix<double> gm = p.getGoniometerMatrix();
      goniometerMatrix[9 * i] = gm[0][0];
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

  // Detectors column
  file->writeData("column_1", detectorID);
  file->openData("column_1");
  file->putAttr("name", "Detector ID");
  file->putAttr("interpret_as", specifyInteger);
  file->putAttr("units", "Not known");
  file->closeData();

  // H column
  file->writeData("column_2", H);
  file->openData("column_2");
  file->putAttr("name", "H");
  file->putAttr("interpret_as", specifyDouble);
  file->putAttr("units", "Not known"); // Units may need changing when known
  file->closeData();

  // K column
  file->writeData("column_3", K);
  file->openData("column_3");
  file->putAttr("name", "K");
  file->putAttr("interpret_as", specifyDouble);
  file->putAttr("units", "Not known"); // Units may need changing when known
  file->closeData();

  // L column
  file->writeData("column_4", L);
  file->openData("column_4");
  file->putAttr("name", "L");
  file->putAttr("interpret_as", specifyDouble);
  file->putAttr("units", "Not known"); // Units may need changing when known
  file->closeData();

  // Intensity column
  file->writeData("column_5", intensity);
  file->openData("column_5");
  file->putAttr("name", "Intensity");
  file->putAttr("interpret_as", specifyDouble);
  file->putAttr("units", "Not known"); // Units may need changing when known
  file->closeData();

  // Sigma Intensity column
  file->writeData("column_6", sigmaIntensity);
  file->openData("column_6");
  file->putAttr("name", "Sigma Intensity");
  file->putAttr("interpret_as", specifyDouble);
  file->putAttr("units", "Not known"); // Units may need changing when known
  file->closeData();

  // Bin Count column
  file->writeData("column_7", binCount);
  file->openData("column_7");
  file->putAttr("name", "Bin Count");
  file->putAttr("interpret_as", specifyDouble);
  file->putAttr("units", "Not known"); // Units may need changing when known
  file->closeData();

  // Initial Energy column
  file->writeData("column_8", initialEnergy);
  file->openData("column_8");
  file->putAttr("name", "Initial Energy");
  file->putAttr("interpret_as", specifyDouble);
  file->putAttr("units", "Not known"); // Units may need changing when known
  file->closeData();

  // Final Energy column
  file->writeData("column_9", finalEnergy);
  file->openData("column_9");
  file->putAttr("name", "Final Energy");
  file->putAttr("interpret_as", specifyDouble);
  file->putAttr("units", "Not known"); // Units may need changing when known
  file->closeData();

  // Wave Length Column
  file->writeData("column_10", waveLength);
  file->openData("column_10");
  file->putAttr("name", "Wave Length");
  file->putAttr("interpret_as", specifyDouble);
  file->putAttr("units", "Not known"); // Units may need changing when known
  file->closeData();

  // Scattering Column
  file->writeData("column_11", scattering);
  file->openData("column_11");
  file->putAttr("name", "Scattering");
  file->putAttr("interpret_as", specifyDouble);
  file->putAttr("units", "Not known"); // Units may need changing when known
  file->closeData();

  // D Spacing Column
  file->writeData("column_12", dSpacing);
  file->openData("column_12");
  file->putAttr("name", "D Spacing");
  file->putAttr("interpret_as", specifyDouble);
  file->putAttr("units", "Not known"); // Units may need changing when known
  file->closeData();

  // TOF Column
  file->writeData("column_13", TOF);
  file->openData("column_13");
  file->putAttr("name", "TOF");
  file->putAttr("interpret_as", specifyDouble);
  file->putAttr("units", "Not known"); // Units may need changing when known
  file->closeData();

  // Run Number column
  file->writeData("column_14", runNumber);
  file->openData("column_14");
  file->putAttr("name", "Run Number");
  file->putAttr("interpret_as", specifyInteger);
  file->putAttr("units", "Not known"); // Units may need changing when known
  file->closeData();

  // Peak Number column
  file->writeData("column_17", peakNumber);
  file->openData("column_17");
  file->putAttr("name", "Peak Number");
  file->putAttr("interpret_as", specifyInteger);
  file->putAttr("units", "Not known"); // Units may need changing when known
  file->closeData();

  // Goniometer Matrix Column
  std::vector<int> array_dims;
  array_dims.push_back(static_cast<int>(peaks.size()));
  array_dims.push_back(9);
  file->writeData("column_15", goniometerMatrix, array_dims);
  file->openData("column_15");
  file->putAttr("name", "Goniometer Matrix");
  file->putAttr("interpret_as", "A matrix of 3x3 doubles");
  file->putAttr("units", "Not known"); // Units may need changing when known
  file->closeData();

  // Shape
  std::vector<int64_t> dims;
  dims.push_back(np);
  dims.push_back(static_cast<int>(maxShapeJSONLength));
  const std::string name = "column_16";
  file->makeData(name, NeXus::CHAR, dims, false);
  file->openData(name);

  auto toNexus = new char[maxShapeJSONLength * np];
  for (size_t ii = 0; ii < np; ii++) {
    std::string rowStr = shapes[ii];
    for (size_t ic = 0; ic < rowStr.size(); ic++)
      toNexus[ii * maxShapeJSONLength + ic] = rowStr[ic];
    for (size_t ic = rowStr.size();
         ic < static_cast<size_t>(maxShapeJSONLength); ic++)
      toNexus[ii * maxShapeJSONLength + ic] = ' ';
  }

  file->putData((void *)(toNexus));

  delete[] toNexus;
  file->putAttr("units", "Not known"); // Units may need changing when known
  file->putAttr("name", "Shape");
  file->putAttr("interpret_as", specifyString);
  file->closeData();

  // QLab & QSample are calculated and do not need to be saved

  file->closeGroup(); // end of peaks workpace
}

/**
 * Set the special Q3D coordinate system.
 * @param coordinateSystem : Option to set.
 */
void PeaksWorkspace::setCoordinateSystem(
    const Kernel::SpecialCoordinateSystem coordinateSystem) {
  m_coordSystem = coordinateSystem;
}

/**
 * @return the special Q3D coordinate system.
 */
Kernel::SpecialCoordinateSystem
PeaksWorkspace::getSpecialCoordinateSystem() const {
  return m_coordSystem;
}

// prevent shared pointer from deleting this
struct NullDeleter {
  template <typename T> void operator()(T *) {}
};
/**Get access to shared pointer containing workspace porperties */
API::LogManager_sptr PeaksWorkspace::logs() {
  return API::LogManager_sptr(&(this->mutableRun()), NullDeleter());
}

/** Get constant access to shared pointer containing workspace porperties;
 * Copies logs into new LogManager variable Meaningfull only for some
 * multithereaded methods when a thread wants to have its own copy of logs */
API::LogManager_const_sptr PeaksWorkspace::getLogs() const {
  return API::LogManager_const_sptr(new API::LogManager(this->run()));
}

ITableWorkspace *
PeaksWorkspace::doCloneColumns(const std::vector<std::string> &) const {
  throw Kernel::Exception::NotImplementedError(
      "PeaksWorkspace cannot clone columns.");
}
} // namespace DataObjects
} // namespace Mantid

///\cond TEMPLATE

namespace Mantid {
namespace Kernel {

template <>
DLLExport Mantid::DataObjects::PeaksWorkspace_sptr
IPropertyManager::getValue<Mantid::DataObjects::PeaksWorkspace_sptr>(
    const std::string &name) const {
  PropertyWithValue<Mantid::DataObjects::PeaksWorkspace_sptr> *prop =
      dynamic_cast<
          PropertyWithValue<Mantid::DataObjects::PeaksWorkspace_sptr> *>(
          getPointerToProperty(name));
  if (prop) {
    return *prop;
  } else {
    std::string message =
        "Attempt to assign property " + name +
        " to incorrect type. Expected shared_ptr<PeaksWorkspace>.";
    throw std::runtime_error(message);
  }
}

template <>
DLLExport Mantid::DataObjects::PeaksWorkspace_const_sptr
IPropertyManager::getValue<Mantid::DataObjects::PeaksWorkspace_const_sptr>(
    const std::string &name) const {
  PropertyWithValue<Mantid::DataObjects::PeaksWorkspace_sptr> *prop =
      dynamic_cast<
          PropertyWithValue<Mantid::DataObjects::PeaksWorkspace_sptr> *>(
          getPointerToProperty(name));
  if (prop) {
    return prop->operator()();
  } else {
    std::string message =
        "Attempt to assign property " + name +
        " to incorrect type. Expected const shared_ptr<PeaksWorkspace>.";
    throw std::runtime_error(message);
  }
}

} // namespace Kernel
} // namespace Mantid

///\endcond TEMPLATE
