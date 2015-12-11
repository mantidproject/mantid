#ifndef MANTID_CRYSTAL_SORTHKL_H_
#define MANTID_CRYSTAL_SORTHKL_H_

#include "MantidKernel/System.h"

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/IPeaksWorkspace_fwd.h"
#include "MantidAPI/ITableWorkspace_fwd.h"

#include "MantidDataObjects/Peak.h"
#include "MantidDataObjects/PeaksWorkspace.h"

#include "MantidGeometry/Crystal/PointGroup.h"
#include "MantidGeometry/Crystal/ReflectionCondition.h"
#include "MantidGeometry/Crystal/UnitCell.h"

#include "MantidKernel/V3D.h"

namespace Mantid {
namespace Crystal {

/**
 * \class UniqueReflection
 *
 * This class is a small helper for SortHKL to hold Peak-objects that
 * belong to the same family of reflections.
 *
 * It has methods to return the intensities and sigmas of the contained
 * Peak-objects as vectors. Furthermore there is a function that removes
 * outliers based on the intensities/sigmas.
 *
 *
 */
class DLLExport UniqueReflection {
public:
  UniqueReflection(const Kernel::V3D &hkl) : m_hkl(hkl), m_peaks() {}

  const Kernel::V3D &getHKL() const { return m_hkl; }

  void addPeak(const DataObjects::Peak &peak) { m_peaks.push_back(peak); }
  const std::vector<DataObjects::Peak> &getPeaks() const { return m_peaks; }
  size_t count() const { return m_peaks.size(); }

  std::vector<double> getIntensities() const;
  std::vector<double> getSigmas() const;

  void removeOutliers(double sigmaCritical = 3.0);
  void setPeaksIntensityAndSigma(double intensity, double sigma);

private:
  Kernel::V3D m_hkl;
  std::vector<DataObjects::Peak> m_peaks;
};

/**
 * \class PeaksStatistics
 *
 * The PeaksStatistics class is a small helper class for SortHKL.
 *
 * During construction, a number of statistical indicators is calculated,
 * using the map passed to the constructor.
 *
 * Please note that the map is modified during the calculation and becomes
 * essentially unusable after that, but that is not a problem since the map
 * is currently not meant to be stored anywhere. This class may eventually
 * disappear and might end up being re-implemented in a more general scope.
 */
class DLLExport PeaksStatistics {
public:
  PeaksStatistics(std::map<Kernel::V3D, UniqueReflection> &uniqueReflections,
                  size_t totalReflectionCount)
      : m_measuredReflections(0), m_uniqueReflections(0), m_completeness(0.0),
        m_redundancy(0.0), m_rMerge(0.0), m_rPim(0.0), m_meanIOverSigma(0.0),
        m_lambdaMin(0.0), m_lambdaMax(0.0), m_chiSquared(0.0), m_peaks() {
    m_peaks.reserve(totalReflectionCount);
    calculatePeaksStatistics(uniqueReflections);
  }

  int m_measuredReflections;
  int m_uniqueReflections;
  double m_completeness;
  double m_redundancy;
  double m_rMerge;
  double m_rPim;
  double m_meanIOverSigma;
  double m_lambdaMin;
  double m_lambdaMax;
  double m_chiSquared;

  std::vector<DataObjects::Peak> m_peaks;

private:
  void calculatePeaksStatistics(
      std::map<Kernel::V3D, UniqueReflection> &uniqueReflections);

  double getIOverSigmaSum(const std::vector<double> &sigmas,
                          const std::vector<double> &intensities) const;
  double getRMS(const std::vector<double> &data) const;

  std::pair<double, double>
  getLambdaLimits(const std::vector<DataObjects::Peak> &peaks) const;
};

/** Save a PeaksWorkspace to a Gsas-style ASCII .hkl file.
 *
 * @author Vickie Lynch, SNS
 * @date 2012-01-20
 */
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

private:
  void init();
  void exec();

  std::vector<DataObjects::Peak>
  getNonZeroPeaks(const std::vector<DataObjects::Peak> &inputPeaks) const;

  std::map<Kernel::V3D, UniqueReflection>
  getUniqueReflections(const std::vector<DataObjects::Peak> &peaks,
                       const Geometry::UnitCell &cell) const;

  Geometry::ReflectionCondition_sptr getCentering() const;
  Geometry::PointGroup_sptr getPointgroup() const;

  std::pair<double, double>
  getDLimits(const std::vector<DataObjects::Peak> &peaks,
             const Geometry::UnitCell &cell) const;

  std::map<Kernel::V3D, UniqueReflection> getPossibleUniqueReflections(
      const Geometry::UnitCell &cell, const std::pair<double, double> &dLimits,
      const Geometry::PointGroup_sptr &pointGroup,
      const Geometry::ReflectionCondition_sptr &centering) const;

  API::ITableWorkspace_sptr getStatisticsTable(const std::string &name) const;
  void insertStatisticsIntoTable(const API::ITableWorkspace_sptr &table,
                                 const PeaksStatistics &statistics) const;

  DataObjects::PeaksWorkspace_sptr getOutputPeaksWorkspace(
      const DataObjects::PeaksWorkspace_sptr &inputPeaksWorkspace) const;

  void sortOutputPeaksByHKL(API::IPeaksWorkspace_sptr outputPeaksWorkspace);

  /// Point Groups possible
  std::vector<Mantid::Geometry::PointGroup_sptr> m_pointGroups;

  /// Reflection conditions
  std::vector<Mantid::Geometry::ReflectionCondition_sptr> m_refConds;
};

} // namespace Crystal
} // namespace Mantid

#endif /* MANTID_CRYSTAL_SORTHKL_H_ */
