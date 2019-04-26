// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_WORKFLOWALGORITHMS_SOFTWOTHETATOFTEST_H_
#define MANTID_WORKFLOWALGORITHMS_SOFTWOTHETATOFTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidWorkflowAlgorithms/SofTwoThetaTOF.h"

#include "MantidAPI/Axis.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidGeometry/Crystal/AngleUnits.h"
#include "MantidGeometry/Instrument.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

#include <boost/filesystem.hpp>

using namespace Mantid;
using Mantid::Geometry::deg2rad;
using Mantid::Geometry::rad2deg;

class SofTwoThetaTOFTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static SofTwoThetaTOFTest *createSuite() { return new SofTwoThetaTOFTest(); }
  static void destroySuite(SofTwoThetaTOFTest *suite) { delete suite; }

  void test_init() {
    WorkflowAlgorithms::SofTwoThetaTOF alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  void test_twoThetaGrouping() {
    constexpr int numBanks{1};
    constexpr int bankSize{6};
    constexpr int numBins{13};
    constexpr double angleStep{0.1};
    API::MatrixWorkspace_sptr inputWS =
        WorkspaceCreationHelper::create2DWorkspaceWithRectangularInstrument(
            numBanks, bankSize, numBins);
    inputWS->getAxis(0)->setUnit("TOF");
    inputWS->mutableRun().addProperty("wavelength", 1.0);
    auto &paramMap = inputWS->instrumentParameters();
    paramMap.addString(inputWS->getInstrument().get(), "l2",
                       5.);
    WorkflowAlgorithms::SofTwoThetaTOF alg;
    alg.setChild(true);
    alg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", inputWS));
    TS_ASSERT_THROWS_NOTHING(
        alg.setProperty("OutputWorkspace", "_unused_for_child"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("AngleStep", angleStep))
    TS_ASSERT_THROWS_NOTHING(alg.execute())
    TS_ASSERT(alg.isExecuted())
    API::MatrixWorkspace_const_sptr outputWS =
        alg.getProperty("OutputWorkspace");
    TS_ASSERT(outputWS)
    auto const &spectrumInfo = outputWS->spectrumInfo();
    auto const nHist = spectrumInfo.size();
    TS_ASSERT_LESS_THAN(1, nHist)
    auto angleBinEdge =
        std::floor(spectrumInfo.twoTheta(0) / (angleStep * deg2rad));
    for (size_t i = 0; i < nHist; ++i) {
      auto const twoTheta = spectrumInfo.twoTheta(i);
      TS_ASSERT_LESS_THAN(angleBinEdge, twoTheta)
      angleBinEdge += angleStep * deg2rad;
      TS_ASSERT_LESS_THAN(twoTheta, angleBinEdge)
    }
  }

  void test_grouping_file_and_par_file_creation() {
    constexpr int numBanks{1};
    constexpr int bankSize{6};
    constexpr int numBins{13};
    constexpr double angleStep{0.1};
    API::MatrixWorkspace_sptr inputWS =
        WorkspaceCreationHelper::create2DWorkspaceWithRectangularInstrument(
            numBanks, bankSize, numBins);
    inputWS->getAxis(0)->setUnit("TOF");
    inputWS->mutableRun().addProperty("wavelength", 1.0);
    auto &paramMap = inputWS->instrumentParameters();
    paramMap.addDouble(inputWS->getInstrument().get(), "l2",
                       5.);
    WorkflowAlgorithms::SofTwoThetaTOF alg;
    alg.setChild(true);
    alg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", inputWS));
    TS_ASSERT_THROWS_NOTHING(
        alg.setProperty("OutputWorkspace", "_unused_for_child"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("AngleStep", angleStep))
    auto tempXml = boost::filesystem::temp_directory_path();
    tempXml /= boost::filesystem::unique_path("SofTwoThetaTest-%%%%%%%%.xml");
#if _WIN32
    auto const wfilename = tempXml.native();
    std::string const filename{wfilename.cbegin(), wfilename.cend()};
#else
    std::string const filename{tempXml.native()};
#endif
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("GroupingFilename", filename))
    TS_ASSERT_THROWS_NOTHING(alg.execute())
    TS_ASSERT(alg.isExecuted())
    auto const xmlExists = boost::filesystem::exists(tempXml);
    TS_ASSERT(xmlExists)
    if (xmlExists) {
      boost::filesystem::remove(tempXml);
    }
    auto tempPar = tempXml;
    tempPar.replace_extension(".par");
    auto const parExists = boost::filesystem::exists(tempPar);
    TS_ASSERT(parExists)
    if (parExists) {
      boost::filesystem::remove(tempPar);
    }
  }

  void test_averaging() {
    constexpr int numBanks{1};
    constexpr int bankSize{6};
    constexpr int numBins{13};
    constexpr double angleStep{0.1};
    API::MatrixWorkspace_sptr inputWS =
        WorkspaceCreationHelper::create2DWorkspaceWithRectangularInstrument(
            numBanks, bankSize, numBins);
    inputWS->getAxis(0)->setUnit("TOF");
    inputWS->mutableRun().addProperty("wavelength", 1.0);
    auto &paramMap = inputWS->instrumentParameters();
    paramMap.addDouble(inputWS->getInstrument().get(), "l2",
                       5.);
    WorkflowAlgorithms::SofTwoThetaTOF alg;
    alg.setChild(true);
    alg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", inputWS));
    TS_ASSERT_THROWS_NOTHING(
        alg.setProperty("OutputWorkspace", "_unused_for_child"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("AngleStep", angleStep))
    TS_ASSERT_THROWS_NOTHING(alg.execute())
    TS_ASSERT(alg.isExecuted())
    API::MatrixWorkspace_const_sptr outputWS =
        alg.getProperty("OutputWorkspace");
    TS_ASSERT(outputWS)
    for (size_t i = 0; i < outputWS->getNumberHistograms(); ++i) {
      auto const &Ys = outputWS->y(i);
      auto const &Es = outputWS->e(i);
      for (size_t j = 0; j < Ys.size(); ++j) {
        if (Ys[j] == 0.)
          std::cout << "i: " << i << " j: " << j << '\n';
        if (j == 0 && i != 6) {
          // These bins are known to be zero.
          TS_ASSERT_EQUALS(Ys[j], 0.)
          TS_ASSERT_EQUALS(Es[j], 0.)
        } else {
          TS_ASSERT_DELTA(Ys[j], 2., 1e-12)
          TS_ASSERT_LESS_THAN(0., Es[j])
          TS_ASSERT_LESS_THAN_EQUALS(Es[j], std::sqrt(2))
        }
      }
    }
  }
};

#endif /* MANTID_WORKFLOWALGORITHMS_SOFTWOTHETATOFTEST_H_ */
