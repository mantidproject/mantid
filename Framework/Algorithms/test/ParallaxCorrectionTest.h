// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ALGORITHMS_PARALLAXCORRECTIONTEST_H_
#define MANTID_ALGORITHMS_PARALLAXCORRECTIONTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidAlgorithms/CreateSampleWorkspace.h"
#include "MantidAlgorithms/ParallaxCorrection.h"
#include "MantidAlgorithms/SetInstrumentParameter.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidKernel/V3D.h"

using Mantid::Algorithms::CreateSampleWorkspace;
using Mantid::Algorithms::ParallaxCorrection;
using Mantid::Algorithms::SetInstrumentParameter;
using Mantid::API::FrameworkManager;
using Mantid::API::MatrixWorkspace_sptr;
using Mantid::Geometry::DetectorInfo;
using Mantid::Kernel::V3D;

namespace {
MatrixWorkspace_sptr createWorkspace(const int nPixelsPerBank = 3,
                                     const int nBins = 2) {
  CreateSampleWorkspace creator;
  creator.initialize();
  creator.setChild(true);
  creator.setAlwaysStoreInADS(false);
  creator.setProperty("NumBanks", 1);
  creator.setProperty("XMin", 1.);
  creator.setProperty("XMax", 2.);
  creator.setProperty("BinWidth", 1. / nBins);
  creator.setProperty("BankPixelWidth", nPixelsPerBank);
  creator.setProperty("Function", "One Peak");
  creator.setProperty("XUnit", "Wavelength");
  creator.setPropertyValue("OutputWorkspace", "__unused");
  creator.execute();

  MatrixWorkspace_sptr in = creator.getProperty("OutputWorkspace");

  SetInstrumentParameter setter;
  setter.initialize();
  setter.setChild(true);
  setter.setAlwaysStoreInADS(false);

  setter.setProperty("Workspace", in);
  setter.setProperty("ParameterName", "direction");
  setter.setProperty("ParameterType", "String");
  setter.setProperty("ComponentName", "bank1");
  setter.setProperty("Value", "y");
  setter.execute();

  setter.setProperty("Workspace", in);
  setter.setProperty("ParameterName", "parallax");
  setter.setProperty("ParameterType", "String");
  setter.setProperty("ComponentName", "bank1");
  setter.setProperty("Value", "1 + 0.1 * t");
  setter.execute();

  return in;
}
} // namespace

class ParallaxCorrectionTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ParallaxCorrectionTest *createSuite() {
    return new ParallaxCorrectionTest();
  }
  static void destroySuite(ParallaxCorrectionTest *suite) { delete suite; }

  ParallaxCorrectionTest() { FrameworkManager::Instance(); }

  void test_init() {
    ParallaxCorrection alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  void test_exec() {

    MatrixWorkspace_sptr in = createWorkspace();
    const std::vector<std::string> components = {"bank1"};

    ParallaxCorrection alg;
    alg.setChild(true);
    alg.setAlwaysStoreInADS(false);
    alg.initialize();
    alg.isInitialized();
    alg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", in));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("ComponentNames", components));
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("OutputWorkspace", "__unused"));
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    MatrixWorkspace_sptr out = alg.getProperty("OutputWorkspace");
    TS_ASSERT(out);
    // Divide the input with the output to get just the correction
    in /= out;

    auto &detectorInfo = in->detectorInfo();
    for (size_t index = 0; index < in->getNumberHistograms(); ++index) {
      const V3D pos = detectorInfo.position(index);
      const double expectation =
          1. + 0.1 * std::abs(std::atan2(pos.X(), pos.Z()));
      const double reality = in->y(index)[0];
      TS_ASSERT_DELTA(expectation, reality, 1E-5);
    }
  }
};

class ParallaxCorrectionTestPerformance : public CxxTest::TestSuite {
public:
  static ParallaxCorrectionTestPerformance *createSuite() {
    return new ParallaxCorrectionTestPerformance();
  }
  static void destroySuite(ParallaxCorrectionTestPerformance *suite) {
    delete suite;
  }

  void setUp() override {
    FrameworkManager::Instance();
    m_alg.initialize();
    m_alg.setChild(true);
    m_alg.setAlwaysStoreInADS(false);
    m_alg.setRethrows(true);
    MatrixWorkspace_sptr in = createWorkspace(1000, 100);
    const std::vector<std::string> components = {"bank1"};
    m_alg.setProperty("InputWorkspace", in);
    m_alg.setProperty("ComponentNames", components);
    m_alg.setProperty("OutputWorkspace", "__out");
  }

  void test_performance() { TS_ASSERT_THROWS_NOTHING(m_alg.execute()) }

private:
  ParallaxCorrection m_alg;
};

#endif /* MANTID_ALGORITHMS_PARALLAXCORRECTIONTEST_H_ */
