#ifndef MANTID_CRYSTAL_SORTHKL_H_
#define MANTID_CRYSTAL_SORTHKL_H_

#include "MantidKernel/System.h"

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/IPeaksWorkspace_fwd.h"
#include "MantidAPI/ITableWorkspace_fwd.h"

#include "MantidDataObjects/Peak.h"
#include "MantidGeometry/Crystal/PointGroup.h"
#include "MantidGeometry/Crystal/ReflectionCondition.h"
#include "MantidKernel/V3D.h"
#include <cmath>
#include "MantidKernel/Statistics.h"

namespace Mantid {
namespace Crystal {

/** Save a PeaksWorkspace to a Gsas-style ASCII .hkl file.
 *
 * @author Vickie Lynch, SNS
 * @date 2012-01-20
 */

class DLLExport UniqueReflection {
public:
  UniqueReflection(const Kernel::V3D &hkl) : m_hkl(hkl), m_peaks() {}

  void addPeak(const DataObjects::Peak &peak) { m_peaks.push_back(peak); }

  void appendPeaksToVector(std::vector<DataObjects::Peak> &peaks) const {
    peaks.insert(peaks.end(), m_peaks.begin(), m_peaks.end());
  }

  size_t count() const { return m_peaks.size(); }

  std::vector<double> getIntensities() {
    std::vector<double> intensities;
    intensities.reserve(m_peaks.size());

    std::transform(
        m_peaks.begin(), m_peaks.end(), std::back_inserter(intensities),
        [](const DataObjects::Peak &peak) { return peak.getIntensity(); });

    return intensities;
  }

  std::vector<double> getSigmas() {
    std::vector<double> sigmas;
    sigmas.reserve(m_peaks.size());

    std::transform(
        m_peaks.begin(), m_peaks.end(), std::back_inserter(sigmas),
        [](const DataObjects::Peak &peak) { return peak.getSigmaIntensity(); });
    return sigmas;
  }

  void removeOutliers(double sigmaCritical = 3.0) {
    if (m_peaks.size() > 2) {

      std::vector<double> intensities = getIntensities();
      std::vector<double> zScores = Kernel::getZscore(intensities);

      std::vector<size_t> outlierIndices;
      for (size_t i = 0; i < zScores.size(); ++i) {
        if (zScores[i] > sigmaCritical) {
          outlierIndices.push_back(i);
        }
      }

      if (outlierIndices.size() > 0) {
        for (auto it = outlierIndices.rbegin(); it != outlierIndices.rend();
             ++it) {
          m_peaks.erase(m_peaks.begin() + (*it));
        }
      }
    }
  }

  void setPeaksIntensityAndSigma(double intensity, double sigma) {
    for (auto &peak : m_peaks) {
      peak.setIntensity(intensity);
      peak.setSigmaIntensity(sigma);
    }
  }

private:
  Kernel::V3D m_hkl;
  std::vector<DataObjects::Peak> m_peaks;
};

class DLLExport SortHKL : public API::Algorithm {
public:
  SortHKL();
  ~SortHKL();

  /// Algorithm's name for identification
  virtual const std::string name() const { return "SortHKL"; };
  /// Summary of algorithms purpose
  virtual const std::string summary() const {
    return "Sorts a PeaksWorkspace by HKL. Averages intensities using point "
           "group.";
  }

  /// Algorithm's version for identification
  virtual int version() const { return 1; };
  /// Algorithm's category for identification
  virtual const std::string category() const {
    return "Crystal\\Peaks;DataHandling\\Text;Utility\\Sorting";
  }

  void sortOutputPeaksByHKL(API::IPeaksWorkspace_sptr outputPeaksWorkspace);

private:
  /// Point Groups possible
  std::vector<Mantid::Geometry::PointGroup_sptr> m_pointGroups;

  /// Reflection conditions
  std::vector<Mantid::Geometry::ReflectionCondition_sptr> m_refConds;

  /// Initialise the properties
  void init();
  /// Run the algorithm
  void exec();

  /// Rounds a double using 0.5 as the cut off for rounding down
  double round(double d);
  /// Rounds the V3D to integer values
  Kernel::V3D round(Kernel::V3D hkl);

  std::map<Kernel::V3D, UniqueReflection> getPossibleUniqueReflections(
      double dMin, double dMax, const Geometry::PointGroup_sptr &pointGroup,
      const Geometry::ReflectionCondition_sptr &centering) const;

  Geometry::ReflectionCondition_sptr getCentering() const;
  Geometry::PointGroup_sptr getPointgroup() const;
  double getIOverSigmaSum(const std::vector<double> &sigmas,
                          const std::vector<double> &intensities) const;
  double getMeanOfSquared(const std::vector<double> &data) const;

  API::ITableWorkspace_sptr getStatisticsTable(const std::string &name) const;

  std::pair<double, double>
  getLambdaLimits(const API::IPeaksWorkspace_sptr &peaksWs,
                      const std::vector<DataObjects::Peak> &peaks) const;
  std::map<Kernel::V3D, UniqueReflection> getUniqueReflections(
      const std::vector<DataObjects::Peak> &peaks,
      const API::IPeaksWorkspace_sptr &outputPeaksWorkspace) const;
};

} // namespace Crystal
} // namespace Mantid

#endif /* MANTID_CRYSTAL_SORTHKL_H_ */
