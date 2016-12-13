#ifndef MANTID_CRYSTAL_SORTHKLTEST_H_
#define MANTID_CRYSTAL_SORTHKLTEST_H_

#include "MantidCrystal/SortHKL.h"
#include "MantidDataObjects/Peak.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidGeometry/IDTypes.h"
#include "MantidKernel/Material.h"
#include "MantidKernel/System.h"
#include "MantidKernel/Timer.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/Sample.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"
#include <cxxtest/TestSuite.h>
#include <fstream>
#include <Poco/File.h>

using namespace Mantid;
using namespace Mantid::Crystal;
using namespace Mantid::API;
using namespace Mantid::Geometry;
using namespace Mantid::Kernel;
using namespace Mantid::DataObjects;
using namespace Mantid::PhysicalConstants;

class SortHKLTest : public CxxTest::TestSuite {
public:
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
    TS_ASSERT_THROWS(reflection.removeOutliers(0.0), std::invalid_argument);
    TS_ASSERT_THROWS(reflection.removeOutliers(-10.0), std::invalid_argument);
  }

  void test_UniqueReflectionRemoveOutliersFewPeaks() {
    std::vector<Peak> peaks = getPeaksWithIandSigma({30.0, 34.0}, {4.5, 6.5});

    UniqueReflection reflection(V3D(2, 3, 4));
    reflection.addPeak(peaks[0]);

    TS_ASSERT_THROWS_NOTHING(reflection.removeOutliers());
    TSM_ASSERT_EQUALS("Peak was removed as outlier although there's only 1.",
                      reflection.count(), 1);

    reflection.addPeak(peaks[1]);

    TS_ASSERT_THROWS_NOTHING(reflection.removeOutliers());
    TSM_ASSERT_EQUALS("Peak was removed as outlier although there's only 2.",
                      reflection.count(), 2);
  }

  void test_UniqueReflectionRemoveOutliers() {
    UniqueReflection reflection =
        getReflectionWithPeaks({30.0, 34.0, 32.0, 31.0}, {4.5, 6.5, 10.0, 2.3});

    // standard deviation is 1.70782512765993
    reflection.removeOutliers();
    TSM_ASSERT_EQUALS(
        "UniqueReflection removed outlier although it should not.",
        reflection.count(), 4);

    reflection.removeOutliers(2.0);
    TSM_ASSERT_EQUALS(
        "UniqueReflection removed outlier although it should not.",
        reflection.count(), 4);

    reflection.removeOutliers(1.0);
    TSM_ASSERT_EQUALS(
        "UniqueReflection did not remove outliers although it should have.",
        reflection.count(), 2);

    std::vector<double> cleanIntensities = reflection.getIntensities();
    TS_ASSERT_EQUALS(cleanIntensities[0], 32.0);
    TS_ASSERT_EQUALS(cleanIntensities[1], 31.0);
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

  void test_PeaksStatisticsNoObservation() {
    std::map<V3D, UniqueReflection> uniques;
    uniques.insert(
        std::make_pair(V3D(1, 1, 1), UniqueReflection(V3D(1, 1, 1))));

    PeaksStatistics statistics(uniques, 0);
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

    PeaksStatistics statistics(uniques, 1);
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

    PeaksStatistics statistics(uniques, 1);
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

    PeaksStatistics statistics(uniques, 2);
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

    PeaksStatistics statistics(uniques, 2);
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

    PeaksStatistics statistics(uniques, 3);
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

  void test_Init() {
    SortHKL alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
  }

  void do_test(int numRuns, size_t numBanks, size_t numPeaksPerBank) {
    Instrument_sptr inst =
        ComponentCreationHelper::createTestInstrumentRectangular(4, 10, 1.0);
    PeaksWorkspace_sptr ws(new PeaksWorkspace());
    ws->setInstrument(inst);

    auto lattice = new Mantid::Geometry::OrientedLattice;
    Mantid::Kernel::DblMatrix UB(3, 3, true);
    UB.identityMatrix();
    lattice->setUB(UB);
    ws->mutableSample().setOrientedLattice(lattice);

    double smu = 0.357;
    double amu = 0.011;
    NeutronAtom neutron(static_cast<uint16_t>(EMPTY_DBL()),
                        static_cast<uint16_t>(0), 0.0, 0.0, smu, 0.0, smu, amu);
    Object sampleShape;
    sampleShape.setMaterial(Material("SetInSaveHKLTest", neutron, 1.0));
    ws->mutableSample().setShape(sampleShape);

    API::Run &mrun = ws->mutableRun();
    mrun.addProperty<double>("Radius", 0.1, true);

    for (int run = 1000; run < numRuns + 1000; run++)
      for (size_t b = 1; b <= numBanks; b++)
        for (size_t i = 0; i < numPeaksPerBank; i++) {
          V3D hkl(static_cast<double>(i), static_cast<double>(i),
                  static_cast<double>(i));
          DblMatrix gon(3, 3, true);
          Peak p(inst, static_cast<detid_t>(b * 100 + i + 1 + i * 10),
                 static_cast<double>(i) * 1.0 + 0.5, hkl, gon);
          p.setRunNumber(run);
          p.setBankName("bank1");
          p.setIntensity(static_cast<double>(i) + 0.1);
          p.setSigmaIntensity(sqrt(static_cast<double>(i) + 0.1));
          p.setBinCount(static_cast<double>(i));
          ws->addPeak(p);
        }
    AnalysisDataService::Instance().addOrReplace("TOPAZ_peaks", ws);

    SortHKL alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", "TOPAZ_peaks"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("OutputWorkspace", "TOPAZ_peaks"));
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    PeaksWorkspace_sptr wsout;
    TS_ASSERT_THROWS_NOTHING(
        wsout = boost::dynamic_pointer_cast<PeaksWorkspace>(
            AnalysisDataService::Instance().retrieve("TOPAZ_peaks")));
    TS_ASSERT(wsout);
    if (!wsout)
      return;
    TS_ASSERT_EQUALS(wsout->getNumberPeaks(), 24);

    Peak p = wsout->getPeaks()[0];
    TS_ASSERT_EQUALS(p.getH(), 1);
    TS_ASSERT_EQUALS(p.getK(), 1);
    TS_ASSERT_EQUALS(p.getL(), 1);
    TS_ASSERT_DELTA(p.getIntensity(), 1.1, 1e-4);
    TS_ASSERT_DELTA(p.getSigmaIntensity(), 1.0488, 1e-4);
    TS_ASSERT_DELTA(p.getWavelength(), 1.5, 1e-4);
    TS_ASSERT_EQUALS(p.getRunNumber(), 1000.);
    TS_ASSERT_DELTA(p.getDSpacing(), 3.5933, 1e-4);
    const auto sampleMaterial = wsout->sample().getMaterial();
    if (sampleMaterial.totalScatterXSection(NeutronAtom::ReferenceLambda) !=
        0.0) {
      double rho = sampleMaterial.numberDensity();
      smu = sampleMaterial.totalScatterXSection(NeutronAtom::ReferenceLambda) *
            rho;
      amu = sampleMaterial.absorbXSection(NeutronAtom::ReferenceLambda) * rho;
    } else {
      throw std::invalid_argument(
          "Could not retrieve LinearScatteringCoef from material");
    }
    const API::Run &run = wsout->run();
    double radius;
    if (run.hasProperty("Radius")) {
      Kernel::Property *prop = run.getProperty("Radius");
      radius = boost::lexical_cast<double, std::string>(prop->value());
    } else {
      throw std::invalid_argument("Could not retrieve Radius from run object");
    }

    TS_ASSERT_DELTA(smu, 0.357, 1e-3);
    TS_ASSERT_DELTA(amu, 0.011, 1e-3);
    TS_ASSERT_DELTA(radius, 0.1, 1e-3);
  }

  /// Test with a few peaks
  void test_exec() { do_test(2, 4, 4); }

private:
  std::vector<Peak>
  getPeaksWithIandSigma(const std::vector<double> &intensity,
                        const std::vector<double> &sigma) const {
    std::vector<Peak> peaks;
    std::transform(intensity.begin(), intensity.end(), sigma.begin(),
                   std::back_inserter(peaks),
                   [](double intensity, double sigma) {
                     Peak peak;
                     peak.setIntensity(intensity);
                     peak.setSigmaIntensity(sigma);
                     return peak;
                   });

    return peaks;
  }

  UniqueReflection
  getReflectionWithPeaks(const std::vector<double> &intensities,
                         const std::vector<double> &sigmas,
                         double wavelength = 0.0) const {
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
};

#endif /* MANTID_CRYSTAL_SORTHKLTEST_H_ */
