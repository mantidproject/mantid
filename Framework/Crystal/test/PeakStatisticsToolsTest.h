// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_CRYSTAL_PEAKSTATISTICSTOOLSTEST_H_
#define MANTID_CRYSTAL_PEAKSTATISTICSTOOLSTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidCrystal/PeakStatisticsTools.h"
#include "MantidDataObjects/Peak.h"
#include "MantidGeometry/Crystal/PointGroupFactory.h"

using namespace Mantid::Crystal;
using namespace Mantid::Crystal::PeakStatisticsTools;
using namespace Mantid::DataObjects;
using namespace Mantid::Geometry;
using namespace Mantid::Kernel;

namespace {
std::vector<Peak> getPeaksWithIandSigma(const std::vector<double> &intensity,
                                        const std::vector<double> &sigma,
                                        const V3D &hkl = V3D(0, 0, 1)) {
  std::vector<Peak> peaks;
  std::transform(intensity.begin(), intensity.end(), sigma.begin(),
                 std::back_inserter(peaks),
                 [hkl](double intensity, double sigma) {
                   Peak peak;
                   peak.setIntensity(intensity);
                   peak.setSigmaIntensity(sigma);
                   peak.setHKL(hkl);
                   return peak;
                 });

  return peaks;
}

UniqueReflection getReflectionWithPeaks(const std::vector<double> &intensities,
                                        const std::vector<double> &sigmas,
                                        double wavelength = 0.0) {
  std::vector<Peak> peaks = getPeaksWithIandSigma(intensities, sigmas);

  if (wavelength > 0) {
    for (auto &peak : peaks) {
      peak.setWavelength(wavelength);
    }
  }

  UniqueReflection reflection(V3D(2, 3, 4));
  for (auto peak : peaks) {
    reflection.addPeak(peak);
  }

  return reflection;
}

UniqueReflectionCollection
getUniqueReflectionCollection(double a, const std::string &centering,
                              const std::string &pointGroup, double dMin) {
  UnitCell cell(a, a, a);
  PointGroup_sptr pg =
      PointGroupFactory::Instance().createPointGroup(pointGroup);
  ReflectionCondition_sptr cent = getReflectionConditionBySymbol(centering);

  return UniqueReflectionCollection(cell, std::make_pair(dMin, 100.0), pg,
                                    cent);
}

class MockUniqueReflectionCollection : public UniqueReflectionCollection {
public:
  explicit MockUniqueReflectionCollection(
      const std::map<V3D, UniqueReflection> &reflections,
      const PointGroup_sptr &pointGroup =
          PointGroupFactory::Instance().createPointGroup("1"))
      : UniqueReflectionCollection(reflections, pointGroup) {}
};
} // namespace

class PeakStatisticsToolsTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static PeakStatisticsToolsTest *createSuite() {
    return new PeakStatisticsToolsTest();
  }
  static void destroySuite(PeakStatisticsToolsTest *suite) { delete suite; }

  void test_UniqueReflectionsConstructor() {
    V3D hkl(1, 1, 1);
    UniqueReflection reflection(hkl);

    TSM_ASSERT_EQUALS("Constructed UniqueReflection does not have 0 peaks.",
                      reflection.count(), 0);
    TSM_ASSERT_EQUALS(
        "HKL is not equal to constructor argument in UniqueReflection",
        reflection.getHKL(), hkl);
  }

  void test_UniqueReflectionsPeaks() {
    UniqueReflection reflection(V3D(2, 3, 4));

    Peak peak;
    TS_ASSERT_THROWS_NOTHING(reflection.addPeak(peak));
    TSM_ASSERT_EQUALS("UniqueReflection count is not 1 after adding peak.",
                      reflection.count(), 1);
    TSM_ASSERT_EQUALS(
        "UniqueReflection peaks vector size is not 1 after adding peak.",
        reflection.getPeaks().size(), 1);
  }

  void test_UniqueReflectionsGetIntensitiesAndSigmas() {
    UniqueReflection reflection(V3D(2, 3, 4));

    std::vector<Peak> peaks = getPeaksWithIandSigma({30.0, 34.0}, {4.5, 6.5});
    for (auto peak : peaks) {
      reflection.addPeak(peak);
    }

    std::vector<double> intensities = reflection.getIntensities();
    TSM_ASSERT_EQUALS("Intensity vector from UniqueReflection has wrong size.",
                      intensities.size(), 2);
    TS_ASSERT_EQUALS(intensities[0], 30.0);
    TS_ASSERT_EQUALS(intensities[1], 34.0);

    std::vector<double> sigmas = reflection.getSigmas();
    TSM_ASSERT_EQUALS("Sigma vector from UniqueReflection has wrong size.",
                      sigmas.size(), 2);
    TS_ASSERT_EQUALS(sigmas[0], 4.5);
    TS_ASSERT_EQUALS(sigmas[1], 6.5);
  }

  void test_UniqueReflectionRemoveOutliersSigmaCrit() {
    UniqueReflection reflection(V3D(2, 3, 4));
    TS_ASSERT_THROWS_NOTHING(reflection.removeOutliers(3.0));
    TS_ASSERT_THROWS(reflection.removeOutliers(0.0),
                     const std::invalid_argument &);
    TS_ASSERT_THROWS(reflection.removeOutliers(-10.0),
                     const std::invalid_argument &);
  }

  void test_UniqueReflectionRemoveOutliersFewPeaks() {
    std::vector<Peak> peaks = getPeaksWithIandSigma({30.0, 34.0}, {4.5, 6.5});

    UniqueReflection reflection(V3D(2, 3, 4));
    reflection.addPeak(peaks[0]);

    TS_ASSERT_THROWS_NOTHING(reflection.removeOutliers());

    auto outliersRemoved = reflection.removeOutliers();
    TSM_ASSERT_EQUALS("Peak was removed as outlier although there's only 1.",
                      outliersRemoved.count(), 1);

    reflection.addPeak(peaks[1]);

    TS_ASSERT_THROWS_NOTHING(reflection.removeOutliers());

    outliersRemoved = reflection.removeOutliers();
    TSM_ASSERT_EQUALS("Peak was removed as outlier although there's only 2.",
                      outliersRemoved.count(), 2);
  }

  void test_UniqueReflectionRemoveOutliers() {
    UniqueReflection reflection =
        getReflectionWithPeaks({30.0, 34.0, 32.0, 31.0}, {4.5, 6.5, 10.0, 2.3});

    // standard deviation is 1.70782512765993
    auto cleanReflection = reflection.removeOutliers();
    TSM_ASSERT_EQUALS(
        "UniqueReflection removed outlier although it should not.",
        cleanReflection.count(), 4);

    cleanReflection = reflection.removeOutliers(2.0);
    TSM_ASSERT_EQUALS(
        "UniqueReflection removed outlier although it should not.",
        cleanReflection.count(), 4);

    cleanReflection = reflection.removeOutliers(1.0);
    TSM_ASSERT_EQUALS(
        "UniqueReflection did not remove outliers although it should have.",
        cleanReflection.count(), 2);

    std::vector<double> cleanIntensities = cleanReflection.getIntensities();
    TS_ASSERT_EQUALS(cleanIntensities[0], 32.0);
    TS_ASSERT_EQUALS(cleanIntensities[1], 31.0);
  }

  void test_UniqueReflectionRemoveOutliersWeighted() {
    UniqueReflection reflection =
        getReflectionWithPeaks({30.0, 34.0, 32.0, 31.0}, {4.5, 6.5, 10.0, 2.3});

    // standard deviation is 1.70782512765993
    auto cleanReflection = reflection.removeOutliers(3.0, true);
    TSM_ASSERT_EQUALS(
        "UniqueReflection removed outlier although it should not.",
        cleanReflection.count(), 3);

    cleanReflection = reflection.removeOutliers(2.0, true);
    TSM_ASSERT_EQUALS(
        "UniqueReflection removed outlier although it should not.",
        cleanReflection.count(), 2);

    cleanReflection = reflection.removeOutliers(1.0, true);
    TSM_ASSERT_EQUALS(
        "UniqueReflection did not remove outliers although it should have.",
        cleanReflection.count(), 1);

    std::vector<double> cleanIntensities = cleanReflection.getIntensities();
    TS_ASSERT_EQUALS(cleanIntensities[0], 32.0);
  }

  void test_UniqueReflectionSetIntensityAndSigma() {
    UniqueReflection reflection =
        getReflectionWithPeaks({30.0, 34.0, 32.0, 31.0}, {4.5, 6.5, 10.0, 2.3});

    reflection.setPeaksIntensityAndSigma(10.0, 0.1);

    for (auto peak : reflection.getPeaks()) {
      TSM_ASSERT_EQUALS(
          "Incorrect peak intensity after set in UniqueReflection.",
          peak.getIntensity(), 10.0);
      TSM_ASSERT_EQUALS("Incorrect peak sigma after set in UniqueReflection.",
                        peak.getSigmaIntensity(), 0.1);
    }
  }

  void test_UniqueReflectionCollectionEmpty() {
    UniqueReflectionCollection reflections =
        getUniqueReflectionCollection(3.0, "P", "m-3m", 1.5);

    // There should be 4 reflections: 001, 011, 111, 002
    TS_ASSERT_EQUALS(reflections.getUniqueReflectionCount(), 4);

    // Uses point group to retrieve UniqueReflections
    TS_ASSERT_THROWS_NOTHING(reflections.getReflection(V3D(0, 0, 1)));
    TS_ASSERT_THROWS_NOTHING(reflections.getReflection(V3D(0, 0, -1)));

    TS_ASSERT_THROWS_NOTHING(reflections.getReflection(V3D(0, 1, 1)));
    TS_ASSERT_THROWS_NOTHING(reflections.getReflection(V3D(1, 1, 1)));
    TS_ASSERT_THROWS_NOTHING(reflections.getReflection(V3D(0, 0, 2)));

    // Reflections that do not exist throw some exception
    TS_ASSERT_THROWS_ANYTHING(reflections.getReflection(V3D(0, 0, 3)));
    TS_ASSERT_THROWS_ANYTHING(reflections.getReflection(V3D(2, -1, 0)));

    // No observations
    TS_ASSERT_EQUALS(reflections.getObservedReflectionCount(), 0);
    TS_ASSERT_EQUALS(reflections.getObservedUniqueReflectionCount(), 0);
  }

  void test_UniqueReflectionCollectionAddObservations() {
    UniqueReflectionCollection reflections =
        getUniqueReflectionCollection(3.0, "P", "m-3m", 1.5);

    TS_ASSERT_EQUALS(reflections.getObservedReflectionCount(), 0);
    TS_ASSERT_EQUALS(reflections.getObservedUniqueReflectionCount(), 0);
    TS_ASSERT_EQUALS(reflections.getUnobservedUniqueReflections().size(), 4);

    reflections.addObservations(
        getPeaksWithIandSigma({1.0, 1.0}, {2.0, 2.0}, V3D(1, 0, 0)));

    TS_ASSERT_EQUALS(reflections.getObservedReflectionCount(), 2);
    TS_ASSERT_EQUALS(reflections.getObservedUniqueReflectionCount(), 1);
    TS_ASSERT_EQUALS(reflections.getUnobservedUniqueReflections().size(), 3);

    // out-of-range peaks are ignored, so the reflection counts do not change
    reflections.addObservations(
        getPeaksWithIandSigma({1.0, 1.0}, {2.0, 2.0}, V3D(0, 5, 0)));

    TS_ASSERT_EQUALS(reflections.getObservedReflectionCount(), 2);
    TS_ASSERT_EQUALS(reflections.getObservedUniqueReflectionCount(), 1);
  }

  void test_UniqueReflectionCollectionReflectionCounts() {
    UniqueReflectionCollection reflections =
        getUniqueReflectionCollection(3.0, "P", "m-3m", 1.5);

    reflections.addObservations(
        getPeaksWithIandSigma({1.0, 1.0}, {2.0, 2.0}, V3D(1, 0, 0)));
    reflections.addObservations(
        getPeaksWithIandSigma({1.0, 1.0, 2.0}, {2.0, 2.0, 3.0}, V3D(1, 1, 0)));

    TS_ASSERT_EQUALS(reflections.getObservedReflectionCount(), 5);
    TS_ASSERT_EQUALS(reflections.getObservedUniqueReflectionCount(), 2);
    TS_ASSERT_EQUALS(reflections.getObservedUniqueReflectionCount(2), 1);
    TS_ASSERT_EQUALS(reflections.getObservedUniqueReflectionCount(3), 0);

    TS_ASSERT_EQUALS(reflections.getUnobservedUniqueReflections().size(), 2);
  }

  void test_PeaksStatisticsNoObservation() {
    std::map<V3D, UniqueReflection> uniques;
    uniques.insert(
        std::make_pair(V3D(1, 1, 1), UniqueReflection(V3D(1, 1, 1))));
    MockUniqueReflectionCollection reflections(uniques);

    PeaksStatistics statistics(reflections);
    TS_ASSERT_EQUALS(statistics.m_peaks.size(), 0);
    TS_ASSERT_EQUALS(statistics.m_uniqueReflections, 0);
    TS_ASSERT_EQUALS(statistics.m_redundancy, 0.0);
    TS_ASSERT_EQUALS(statistics.m_completeness, 0.0);
    TS_ASSERT_EQUALS(statistics.m_rMerge, 0.0);
    TS_ASSERT_EQUALS(statistics.m_rPim, 0.0);
    TS_ASSERT_EQUALS(statistics.m_meanIOverSigma, 0.0);
  }

  void test_PeaksStatisticsOneObservation() {
    std::map<V3D, UniqueReflection> uniques{
        {{1, 1, 1}, getReflectionWithPeaks({56.0}, {4.5}, 1.0)}};
    MockUniqueReflectionCollection reflections(uniques);

    PeaksStatistics statistics(reflections);
    TS_ASSERT_EQUALS(statistics.m_peaks.size(), 1);
    TS_ASSERT_EQUALS(statistics.m_uniqueReflections, 1);
    TS_ASSERT_EQUALS(statistics.m_redundancy, 1.0);
    TS_ASSERT_EQUALS(statistics.m_completeness, 1.0);
    TS_ASSERT_EQUALS(statistics.m_rMerge, 0.0);
    TS_ASSERT_EQUALS(statistics.m_rPim, 0.0);
    TS_ASSERT_EQUALS(statistics.m_meanIOverSigma, 56.0 / 4.5);
  }

  void test_PeaksStatisticsOneObservationTwoUnique() {
    std::map<V3D, UniqueReflection> uniques{
        {{1, 1, 1}, getReflectionWithPeaks({56.0}, {4.5}, 1.0)},
        {{1, 1, 2}, UniqueReflection(V3D(1, 1, 2))}};
    MockUniqueReflectionCollection reflections(uniques);

    PeaksStatistics statistics(reflections);
    TS_ASSERT_EQUALS(statistics.m_peaks.size(), 1);
    TS_ASSERT_EQUALS(statistics.m_uniqueReflections, 1);
    TS_ASSERT_EQUALS(statistics.m_redundancy, 1.0);
    TS_ASSERT_EQUALS(statistics.m_completeness, 0.5);
    TS_ASSERT_EQUALS(statistics.m_rMerge, 0.0);
    TS_ASSERT_EQUALS(statistics.m_rPim, 0.0);
    TS_ASSERT_EQUALS(statistics.m_meanIOverSigma, 56.0 / 4.5);
  }

  void test_PeaksStatisticsTwoObservationTwoUnique() {

    std::map<V3D, UniqueReflection> uniques{
        {{1, 1, 1}, getReflectionWithPeaks({10.0}, {1.0}, 1.0)},
        {{1, 1, 2}, getReflectionWithPeaks({20.0}, {1.0}, 2.0)}};
    MockUniqueReflectionCollection reflections(uniques);

    PeaksStatistics statistics(reflections);
    TS_ASSERT_EQUALS(statistics.m_peaks.size(), 2);
    TS_ASSERT_EQUALS(statistics.m_uniqueReflections, 2);
    TS_ASSERT_EQUALS(statistics.m_redundancy, 1.0);
    TS_ASSERT_EQUALS(statistics.m_completeness, 1.0);
    TS_ASSERT_EQUALS(statistics.m_rMerge, 0.0);
    TS_ASSERT_EQUALS(statistics.m_rPim, 0.0);
    TS_ASSERT_EQUALS(statistics.m_meanIOverSigma, 15.0);
  }

  void test_PeaksStatisticsTwoObservationOneUnique() {
    std::map<V3D, UniqueReflection> uniques{
        {{1, 1, 1}, getReflectionWithPeaks({10.0, 20.0}, {0.1, 0.1}, 1.0)}};
    MockUniqueReflectionCollection reflections(uniques);

    PeaksStatistics statistics(reflections);
    TS_ASSERT_EQUALS(statistics.m_peaks.size(), 2);
    TS_ASSERT_EQUALS(statistics.m_uniqueReflections, 1);
    TS_ASSERT_EQUALS(statistics.m_redundancy, 2.0);
    TS_ASSERT_EQUALS(statistics.m_completeness, 1.0);
    // <I> = 15, sum(I) = 30, sum(|I - <I>|) = 10, rMerge = 10 / 30 = 0.33
    TS_ASSERT_EQUALS(statistics.m_rMerge, 1.0 / 3.0);
    // For 2 observations this is the same since sqrt(1 / (2 - 1)) = 1
    TS_ASSERT_EQUALS(statistics.m_rPim, 1.0 / 3.0);
    TS_ASSERT_EQUALS(statistics.m_meanIOverSigma, 150.0);
  }

  void test_PeaksStatisticsThreeObservationOneUnique() {
    std::map<V3D, UniqueReflection> uniques{
        {{1, 1, 1},
         getReflectionWithPeaks({10.0, 20.0, 15.0}, {0.1, 0.1, 0.1}, 1.0)}};
    MockUniqueReflectionCollection reflections(uniques);

    PeaksStatistics statistics(reflections);
    TS_ASSERT_EQUALS(statistics.m_peaks.size(), 3);
    TS_ASSERT_EQUALS(statistics.m_uniqueReflections, 1);
    TS_ASSERT_EQUALS(statistics.m_redundancy, 3.0);
    TS_ASSERT_EQUALS(statistics.m_completeness, 1.0);
    // <I> = 15, sum(I) = 45, sum(|I - <I>|) = 10, rMerge = 10 / 45 = 0.222
    TS_ASSERT_EQUALS(statistics.m_rMerge, 1.0 / 4.5);
    // For rpim the factor is  sqrt(1 / (3 - 1)) = sqrt(0.5)
    TS_ASSERT_EQUALS(statistics.m_rPim, sqrt(0.5) / 4.5);
    TS_ASSERT_EQUALS(statistics.m_meanIOverSigma, 150.0);
  }
};

#endif /* MANTID_CRYSTAL_PEAKSTATISTICSTOOLSTEST_H_ */
