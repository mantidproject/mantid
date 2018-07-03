#ifndef MANTID_CRYSTAL_PEAKSTATISTICSTOOLS_H_
#define MANTID_CRYSTAL_PEAKSTATISTICSTOOLS_H_

#include "MantidDataObjects/Peak.h"

#include "MantidGeometry/Crystal/PointGroup.h"
#include "MantidGeometry/Crystal/ReflectionCondition.h"
#include "MantidGeometry/Crystal/UnitCell.h"

#include "MantidKernel/V3D.h"

namespace Mantid {
namespace Crystal {
namespace PeakStatisticsTools {
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

  std::vector<double> getWavelengths() const;
  std::vector<double> getIntensities() const;
  std::vector<double> getSigmas() const;

  UniqueReflection removeOutliers(double sigmaCritical = 3.0,
                                  bool weightedZ = false) const;
  void setPeaksIntensityAndSigma(double intensity, double sigma);

private:
  Kernel::V3D m_hkl;
  std::vector<DataObjects::Peak> m_peaks;
};

/**
 * \class UniqueReflectionCollection
 *
 * This class computes all possible unique reflections within the
 * specified d-limits, given a certain unit cell, lattice centering
 * and point group. The cost of this computation depends directly
 * on the size of the unit cell (larger cells result in more
 * reflections) and to some extent also on the symmetry (higher symmetry
 * results in more matrix operations).
 *
 * After adding observations using addObservations, various reflection-
 * counts can be obtained, for example to calculate redundancy or
 * completeness of the observations.
 *
 */
class DLLExport UniqueReflectionCollection {
public:
  UniqueReflectionCollection(
      const Geometry::UnitCell &cell, const std::pair<double, double> &dLimits,
      const Geometry::PointGroup_sptr &pointGroup,
      const Geometry::ReflectionCondition_sptr &centering);

  ~UniqueReflectionCollection() = default;

  void addObservations(const std::vector<DataObjects::Peak> &peaks);
  UniqueReflection getReflection(const Kernel::V3D &hkl) const;

  size_t getUniqueReflectionCount() const;
  size_t getObservedUniqueReflectionCount(size_t moreThan = 0) const;
  std::vector<Kernel::V3D> getUnobservedUniqueReflections() const;

  size_t getObservedReflectionCount() const;

  const std::map<Kernel::V3D, UniqueReflection> &getReflections() const;

protected:
  /// Alternative constructor for testing purposes, no validation is performed.
  UniqueReflectionCollection(
      const std::map<Kernel::V3D, UniqueReflection> &reflections,
      const Geometry::PointGroup_sptr &pointGroup)
      : m_reflections(reflections), m_pointgroup(pointGroup) {}

private:
  std::map<Kernel::V3D, UniqueReflection> m_reflections;
  Geometry::PointGroup_sptr m_pointgroup;
};

/**
 * \class PeaksStatistics
 *
 * The PeaksStatistics class is a small helper class that is used
 * in SortHKL. It takes a UniqueReflectionCollection and calculates
 * a few data set quality indicators such as Rmerge and Rpim.
 *
 * Do not rely on this class to exist forever, parts of it may change
 * or the entire class may disappear over time.
 */
class DLLExport PeaksStatistics {
public:
  explicit PeaksStatistics(const UniqueReflectionCollection &reflections)
      : m_measuredReflections(0), m_uniqueReflections(0), m_completeness(0.0),
        m_redundancy(0.0), m_rMerge(0.0), m_rPim(0.0), m_meanIOverSigma(0.0),
        m_dspacingMin(0.0), m_dspacingMax(0.0), m_chiSquared(0.0), m_peaks() {
    m_peaks.reserve(reflections.getObservedReflectionCount());
    std::string equivalentIntensities = "Mean";
    double sigmaCritical = 3.0;
    bool weightedZ = false;
    calculatePeaksStatistics(reflections.getReflections(),
                             equivalentIntensities, sigmaCritical, weightedZ);
  }
  explicit PeaksStatistics(const UniqueReflectionCollection &reflections,
                           std::string &equivalentIntensities,
                           double &sigmaCritical, bool &weightedZ)
      : m_measuredReflections(0), m_uniqueReflections(0), m_completeness(0.0),
        m_redundancy(0.0), m_rMerge(0.0), m_rPim(0.0), m_meanIOverSigma(0.0),
        m_dspacingMin(0.0), m_dspacingMax(0.0), m_chiSquared(0.0), m_peaks() {
    m_peaks.reserve(reflections.getObservedReflectionCount());
    calculatePeaksStatistics(reflections.getReflections(),
                             equivalentIntensities, sigmaCritical, weightedZ);
  }

  /// Total number of observed reflections - no symmetry is taken into
  /// account for this.
  int m_measuredReflections;

  /// Number of unique reflections. This counts each reflection family once,
  /// according to the point group.
  int m_uniqueReflections;

  /// Fraction of observed unique reflections in the resolution range defined
  /// by d_min and d_max.
  double m_completeness;

  /// Average number of observations for a unique reflection.
  double m_redundancy;

  /// Merging R-factor, R_merge, sometimes also called R_sym. This is a basic
  /// measure for how well the intensities of symmetry equivalent reflections
  /// agree with each other.
  double m_rMerge;

  /// Precision indicating R-factor (R_{p.i.m}). Also a measurement of agreement
  /// between equivalent reflections, but without some of the weeknesses of
  /// R_merge.
  double m_rPim;

  /// Average signal to noise ratio in the reflections.
  double m_meanIOverSigma;

  /// Lower d-spacing limit in the data set, sometimes referred to as upper
  /// resolution limit.
  double m_dspacingMin;

  /// Upper d-spacing limit in the data set.
  double m_dspacingMax;

  double m_chiSquared;
  std::vector<DataObjects::Peak> m_peaks;

private:
  void calculatePeaksStatistics(
      const std::map<Kernel::V3D, UniqueReflection> &uniqueReflections,
      std::string &equivalentIntensities, double &sigmaCritical,
      bool &weightedZ);

  double getIOverSigmaSum(const std::vector<double> &sigmas,
                          const std::vector<double> &intensities) const;
  double getRMS(const std::vector<double> &data) const;

  std::pair<double, double>
  getDSpacingLimits(const std::vector<DataObjects::Peak> &peaks) const;
};

} // namespace PeakStatisticsTools
} // namespace Crystal
} // namespace Mantid

#endif /* MANTID_CRYSTAL_PEAKSTATISTICSTOOLS_H_ */
