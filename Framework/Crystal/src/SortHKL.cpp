#include "MantidAPI/FileProperty.h"
#include "MantidCrystal/SortHKL.h"
#include "MantidDataObjects/Peak.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidGeometry/Instrument/RectangularDetector.h"
#include "MantidGeometry/Crystal/PointGroupFactory.h"
#include "MantidGeometry/Crystal/HKLGenerator.h"
#include "MantidGeometry/Crystal/BasicHKLFilters.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"
#include "MantidKernel/Utils.h"
#include "MantidKernel/ListValidator.h"
#include <fstream>

using namespace Mantid::Geometry;
using namespace Mantid::DataObjects;
using namespace Mantid::Kernel;
using namespace Mantid::API;

namespace Mantid {
namespace Crystal {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(SortHKL)

//----------------------------------------------------------------------------------------------
/** Constructor
 */
SortHKL::SortHKL() {
  m_pointGroups = getAllPointGroups();
  m_refConds = getAllReflectionConditions();
}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
SortHKL::~SortHKL() {}

//----------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void SortHKL::init() {
  declareProperty(new WorkspaceProperty<PeaksWorkspace>("InputWorkspace", "",
                                                        Direction::Input),
                  "An input PeaksWorkspace with an instrument.");
  std::vector<std::string> pgOptions;
  for (size_t i = 0; i < m_pointGroups.size(); ++i)
    pgOptions.push_back(m_pointGroups[i]->getName());
  declareProperty("PointGroup", pgOptions[0],
                  boost::make_shared<StringListValidator>(pgOptions),
                  "Which point group applies to this crystal?");

  std::vector<std::string> centeringOptions;
  for (size_t i = 0; i < m_refConds.size(); ++i)
    centeringOptions.push_back(m_refConds[i]->getName());
  declareProperty("LatticeCentering", centeringOptions[0],
                  boost::make_shared<StringListValidator>(centeringOptions),
                  "Appropriate lattice centering for the peaks.");

  declareProperty(new WorkspaceProperty<PeaksWorkspace>("OutputWorkspace", "",
                                                        Direction::Output),
                  "Output PeaksWorkspace");
  declareProperty("OutputChi2", 0.0, "Chi-square is available as output",
                  Direction::Output);
  declareProperty(new WorkspaceProperty<ITableWorkspace>(
                      "StatisticsTable", "StatisticsTable", Direction::Output),
                  "An output table workspace for the statistics of the peaks.");
  declareProperty(new PropertyWithValue<std::string>("RowName", "Overall",
                                                     Direction::Input),
                  "name of row");
  declareProperty("Append", false,
                  "Append to output table workspace if true.\n"
                  "If false, new output table workspace (default).");
}

ReflectionCondition_sptr SortHKL::getCentering() const {
  ReflectionCondition_sptr centering =
      boost::make_shared<ReflectionConditionPrimitive>();
  // Get it from the property
  std::string refCondName = getPropertyValue("LatticeCentering");
  for (size_t i = 0; i < m_refConds.size(); ++i)
    if (m_refConds[i]->getName() == refCondName)
      centering = m_refConds[i];

  return centering;
}

PointGroup_sptr SortHKL::getPointgroup() const {
  PointGroup_sptr pointGroup =
      PointGroupFactory::Instance().createPointGroup("-1");
  // Get it from the property
  std::string pointGroupName = getPropertyValue("PointGroup");
  for (size_t i = 0; i < m_pointGroups.size(); ++i)
    if (m_pointGroups[i]->getName() == pointGroupName)
      pointGroup = m_pointGroups[i];

  return pointGroup;
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void SortHKL::exec() {

  PeaksWorkspace_sptr InPeaksW = getProperty("InputWorkspace");
  // HKL will be overwritten by equivalent HKL but never seen by user
  PeaksWorkspace_sptr peaksW = getProperty("OutputWorkspace");
  if (peaksW != InPeaksW)
    peaksW.reset(InPeaksW->clone().release());

  // Init or append to a table workspace
  bool append = getProperty("Append");
  TableWorkspace_sptr tablews;
  const std::string tableName = getProperty("StatisticsTable");
  if (append && AnalysisDataService::Instance().doesExist(tableName)) {
    tablews =
        AnalysisDataService::Instance().retrieveWS<TableWorkspace>(tableName);
  } else {
    tablews = boost::shared_ptr<TableWorkspace>(new TableWorkspace());
    tablews->addColumn("str", "Resolution Shell");
    tablews->addColumn("int", "No. of Unique Reflections");
    tablews->addColumn("double", "Resolution Min");
    tablews->addColumn("double", "Resolution Max");
    tablews->addColumn("double", "Multiplicity");
    tablews->addColumn("double", "Mean ((I)/sd(I))");
    tablews->addColumn("double", "Rmerge");
    tablews->addColumn("double", "Rpim");
    tablews->addColumn("double", "Data Completeness");
  }

  // append to the table workspace
  API::TableRow newrow = tablews->appendRow();
  std::string name = getProperty("RowName");
  newrow << name;

  std::vector<Peak> &peaks = peaksW->getPeaks();
  int NumberPeaks = peaksW->getNumberPeaks();
  for (int i = 0; i < NumberPeaks; i++) {
    V3D hkl1 = round(peaks[i].getHKL());
    peaks[i].setHKL(hkl1);
  }

  double Chisq = 0.0;
  for (int i = int(NumberPeaks) - 1; i >= 0; --i) {
    if (peaks[i].getIntensity() == 0.0 || peaks[i].getHKL() == V3D(0, 0, 0))
      peaksW->removePeak(i);
  }

  NumberPeaks = peaksW->getNumberPeaks();
  if (NumberPeaks == 0) {
    g_log.error() << "Number of peaks should not be 0 for SortHKL.\n";
    return;
  }

  ReflectionCondition_sptr centering = getCentering();
  PointGroup_sptr pointGroup = getPointgroup();

  std::vector<V3D> possibleUniqueReflections =
      getPossibleUniqueReflections(peaksW, pointGroup, centering);

  std::vector<V3D> measuredUniqueReflections;
  measuredUniqueReflections.reserve(possibleUniqueReflections.size());

  int equivalent = 0;
  for (int i = 0; i < NumberPeaks; i++) {
    V3D hkl1 = peaks[i].getHKL();
    measuredUniqueReflections.push_back(pointGroup->getReflectionFamily(hkl1));

    bool found = false;
    for (int j = i + 1; j < NumberPeaks; j++) {
      V3D hkl2 = peaks[j].getHKL();
      if (pointGroup->isEquivalent(hkl1, hkl2)) {
        peaks[j].setHKL(hkl1);
        found = true;
      }
    }
    if (found)
      equivalent++;
  }

  std::sort(measuredUniqueReflections.begin(), measuredUniqueReflections.end());
  measuredUniqueReflections.erase(std::unique(measuredUniqueReflections.begin(),
                                              measuredUniqueReflections.end()),
                                  measuredUniqueReflections.end());

  std::vector<std::pair<std::string, bool>> criteria;
  // Sort by wavelength
  criteria.push_back(std::pair<std::string, bool>("wavelength", true));
  peaksW->sort(criteria);
  int unique = static_cast<int>(measuredUniqueReflections.size());
  // table workspace output
  newrow << unique << peaks[0].getWavelength()
         << peaks[NumberPeaks - 1].getWavelength();

  criteria.clear();
  // Sort by HKL
  criteria.push_back(std::pair<std::string, bool>("H", true));
  criteria.push_back(std::pair<std::string, bool>("K", true));
  criteria.push_back(std::pair<std::string, bool>("L", true));
  peaksW->sort(criteria);

  std::vector<size_t> multiplicity;
  std::vector<double> IsigI;
  for (int i = 0; i < NumberPeaks; i++) {
    IsigI.push_back(peaks[i].getIntensity() / peaks[i].getSigmaIntensity());
  }
  Statistics statsIsigI = getStatistics(IsigI);
  IsigI.clear();

  std::vector<double> data, sig2;
  std::vector<int> peakno;
  double rSum = 0, rpSum = 0, f2Sum = 0;
  V3D hkl1;
  for (int i = 1; i < NumberPeaks; i++) {
    hkl1 = peaks[i - 1].getHKL();
    f2Sum += peaks[i - 1].getIntensity();
    if (i == 1) {
      peakno.push_back(0);
      data.push_back(peaks[i - 1].getIntensity());
      sig2.push_back(std::pow(peaks[i - 1].getSigmaIntensity(), 2));
    }
    V3D hkl2 = peaks[i].getHKL();
    if (hkl1 == hkl2) {
      peakno.push_back(i);
      data.push_back(peaks[i].getIntensity());
      sig2.push_back(std::pow(peaks[i].getSigmaIntensity(), 2));
      if (i == NumberPeaks - 1) {
        f2Sum += peaks[i].getIntensity();
        Outliers(data, sig2);
        if (static_cast<int>(data.size()) > 1) {
          Statistics stats = getStatistics(data);
          Chisq += stats.standard_deviation / stats.mean;
          Statistics stats2 = getStatistics(sig2);
          std::vector<int>::iterator itpk;
          for (itpk = peakno.begin(); itpk != peakno.end(); ++itpk) {
            double F2 = peaks[*itpk].getIntensity();
            rSum += std::fabs(F2 - stats.mean);
            rpSum += std::sqrt(1.0 / double(data.size() - 1)) *
                     std::fabs(F2 - stats.mean);
            peaks[*itpk].setIntensity(stats.mean);
            peaks[*itpk].setSigmaIntensity(std::sqrt(stats2.mean));
          }
        }
        multiplicity.push_back(data.size());
        peakno.clear();
        data.clear();
        sig2.clear();
      }
    } else {
      Outliers(data, sig2);
      if (static_cast<int>(data.size()) > 1) {
        Statistics stats = getStatistics(data);
        Chisq += stats.standard_deviation / stats.mean;
        Statistics stats2 = getStatistics(sig2);
        std::vector<int>::iterator itpk;
        for (itpk = peakno.begin(); itpk != peakno.end(); ++itpk) {
          double F2 = peaks[*itpk].getIntensity();
          rSum += std::fabs(F2 - stats.mean);
          rpSum += std::sqrt(1.0 / double(data.size() - 1)) *
                   std::fabs(F2 - stats.mean);
          peaks[*itpk].setIntensity(stats.mean);
          peaks[*itpk].setSigmaIntensity(std::sqrt(stats2.mean));
        }
      }
      multiplicity.push_back(data.size());
      peakno.clear();
      data.clear();
      sig2.clear();
      hkl1 = hkl2;
      peakno.push_back(i);
      data.push_back(peaks[i].getIntensity());
      sig2.push_back(std::pow(peaks[i].getSigmaIntensity(), 2));
    }
  }
  Statistics statsMult = getStatistics(multiplicity);
  multiplicity.clear();
  // statistics to output table workspace

  double uniqueReflectionsCount = 0.0;
  if (name.substr(0, 4) != "bank") {
    uniqueReflectionsCount =
        static_cast<double>(possibleUniqueReflections.size());
  }

  newrow << statsMult.mean << statsIsigI.mean << 100.0 * rSum / f2Sum
         << 100.0 * rpSum / f2Sum
         << 100.0 * double(unique) / double(uniqueReflectionsCount);
  data.clear();
  sig2.clear();
  // Reset hkl of equivalent peaks to original value
  for (int i = 0; i < NumberPeaks; i++) {
    peaks[i].resetHKL();
  }
  setProperty("OutputWorkspace", peaksW);
  setProperty("OutputChi2", Chisq);
  setProperty("StatisticsTable", tablews);
  AnalysisDataService::Instance().addOrReplace(tableName, tablews);
}
void SortHKL::Outliers(std::vector<double> &data, std::vector<double> &sig2) {
  if (data.size() < 3)
    return;
  std::vector<double> Zscore = getZscore(data);
  std::vector<size_t> banned;
  for (size_t i = 0; i < data.size(); ++i) {
    if (Zscore[i] > 3.0) {
      banned.push_back(i);
      g_log.notice() << "Data (I):";
      for (size_t j = 0; j < data.size(); ++j)
        g_log.notice() << data[j] << "  ";
      g_log.notice() << "\nData (sigI^2):";
      for (size_t j = 0; j < data.size(); ++j)
        g_log.notice() << data[j] << "  " << sig2[j];
      g_log.notice() << "\nOutlier removed (I and sigI^2):" << data[i] << "  "
                     << sig2[i] << "\n";
    }
  }
  // delete banned peaks
  for (std::vector<size_t>::const_reverse_iterator it = banned.rbegin();
       it != banned.rend(); ++it) {
    data.erase(data.begin() + (*it));
    sig2.erase(sig2.begin() + (*it));
  }
}

/** Rounds the V3D to integer values
* @param hkl the input vector
* @returns The output V3D
*/
V3D SortHKL::round(V3D hkl) {
  V3D hkl1;
  hkl1.setX(round(hkl.X()));
  hkl1.setY(round(hkl.Y()));
  hkl1.setZ(round(hkl.Z()));
  return hkl1;
}

std::vector<V3D> SortHKL::getPossibleUniqueReflections(
    const IPeaksWorkspace_sptr &peaks, const PointGroup_sptr &pointGroup,
    const ReflectionCondition_sptr &centering) const {

  UnitCell cell = peaks->sample().getOrientedLattice();

  std::vector<double> dValues;
  dValues.reserve(peaks->getNumberPeaks());

  for (int i = 0; i < peaks->getNumberPeaks(); ++i) {
    dValues.push_back(cell.d(peaks->getPeak(i).getHKL()));
  }
  std::sort(dValues.begin(), dValues.end());

  double dMin = dValues.front();
  double dMax = dValues.back();

  // Get unit cell from sample

  // Generate HKLS, transform to
  HKLGenerator generator(cell, dMin);
  HKLFilter_const_sptr dFilter =
      boost::make_shared<const HKLFilterDRange>(cell, dMin, dMax);
  HKLFilter_const_sptr centeringFilter =
      boost::make_shared<const HKLFilterCentering>(centering);
  HKLFilter_const_sptr filter = dFilter & centeringFilter;

  std::vector<V3D> uniqueHKLs;
  uniqueHKLs.reserve(generator.size());
  for (auto hkl : generator) {
    if (filter->isAllowed(hkl)) {
      uniqueHKLs.push_back(pointGroup->getReflectionFamily(hkl));
    }
  }

  std::sort(uniqueHKLs.begin(), uniqueHKLs.end());
  uniqueHKLs.erase(std::unique(uniqueHKLs.begin(), uniqueHKLs.end()),
                   uniqueHKLs.end());

  return uniqueHKLs;
}

/** Rounds a double using 0.5 as the cut off for rounding down
* @param d the input value
* @returns The output value
*/
double SortHKL::round(double d) { return floor(d + 0.5); }

} // namespace Mantid
} // namespace Crystal
