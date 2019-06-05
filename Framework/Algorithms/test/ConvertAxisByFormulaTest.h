// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ALGORITHMS_CONVERTAXISBYFORMULATEST_H_
#define MANTID_ALGORITHMS_CONVERTAXISBYFORMULATEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/Axis.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidAlgorithms/ConvertAxisByFormula.h"
#include "MantidKernel/PhysicalConstants.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include <math.h>

using Mantid::Algorithms::ConvertAxisByFormula;

class ConvertAxisByFormulaTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ConvertAxisByFormulaTest *createSuite() {
    return new ConvertAxisByFormulaTest();
  }
  static void destroySuite(ConvertAxisByFormulaTest *suite) { delete suite; }

  void testPlusRefAxis() {
    using namespace Mantid::API;
    using namespace Mantid::Kernel;

    Mantid::Algorithms::ConvertAxisByFormula alg;
    alg.initialize();

    std::string inputWs = alg.name() + "_testPlusRefAxis_Input";
    std::string resultWs = alg.name() + "_testPlusRefAxis_Result";

    AnalysisDataService::Instance().add(
        inputWs, WorkspaceCreationHelper::create2DWorkspace123(10, 10));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("InputWorkspace", inputWs));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", resultWs));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Formula", "x+3"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Axis", "X"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("AxisTitle", "My Title"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("AxisUnits", "MyUnit"));

    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    if (!alg.isExecuted()) {
      cleanupWorkspaces(std::vector<std::string>{inputWs});
      return;
    }

    MatrixWorkspace_const_sptr in, result;
    TS_ASSERT_THROWS_NOTHING(
        in = boost::dynamic_pointer_cast<MatrixWorkspace>(
            AnalysisDataService::Instance().retrieve(inputWs)));
    TS_ASSERT_THROWS_NOTHING(
        result = boost::dynamic_pointer_cast<MatrixWorkspace>(
            AnalysisDataService::Instance().retrieve(resultWs)));

    Axis *ax = result->getAxis(0);
    TS_ASSERT_EQUALS(ax->unit()->caption(), "My Title");
    TS_ASSERT_EQUALS(ax->unit()->label(), "MyUnit");
    const size_t xsize = result->blocksize();
    for (size_t i = 0; i < result->getNumberHistograms(); ++i) {
      const auto &outX = result->readX(i);
      const auto &outY = result->readY(i);
      const auto &outE = result->readE(i);
      const auto &inX = in->readX(i);
      const auto &inY = in->readY(i);
      const auto &inE = in->readE(i);

      for (size_t j = 0; j < xsize; ++j) {
        TS_ASSERT_EQUALS(outX[j], inX[j] + 3);
        TS_ASSERT_EQUALS(outY[j], inY[j]);
        TS_ASSERT_EQUALS(outE[j], inE[j]);
      }
    }

    cleanupWorkspaces(std::vector<std::string>{inputWs, resultWs});
  }

  void testSquareXNumericAxis() {
    using namespace Mantid::API;
    using namespace Mantid::Kernel;

    Mantid::Algorithms::ConvertAxisByFormula alg;
    alg.initialize();

    std::string inputWs = alg.name() + "_testSquareXNumeric_Input";
    std::string resultWs = alg.name() + "_testSquareXNumeric_Result";

    AnalysisDataService::Instance().add(
        inputWs, WorkspaceCreationHelper::create2DWorkspace123(10, 10));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("InputWorkspace", inputWs));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", resultWs));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Formula", "(X+2)*(x+2)"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Axis", "X"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("AxisTitle", "XTitle"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("AxisUnits", "XUnit"));

    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    if (!alg.isExecuted()) {
      cleanupWorkspaces(std::vector<std::string>{inputWs});
      return;
    }

    MatrixWorkspace_const_sptr in, result;
    TS_ASSERT_THROWS_NOTHING(
        in = boost::dynamic_pointer_cast<MatrixWorkspace>(
            AnalysisDataService::Instance().retrieve(inputWs)));
    TS_ASSERT_THROWS_NOTHING(
        result = boost::dynamic_pointer_cast<MatrixWorkspace>(
            AnalysisDataService::Instance().retrieve(resultWs)));

    Axis *ax = result->getAxis(0);
    TS_ASSERT_EQUALS(ax->unit()->caption(), "XTitle");
    TS_ASSERT_EQUALS(ax->unit()->label(), "XUnit");
    TS_ASSERT_EQUALS(ax->length(), 10);
    TS_ASSERT_DELTA(ax->getValue(0), 9.0, 1e-14);
    TS_ASSERT_DELTA(ax->getValue(9), 144.0, 1e-14);

    cleanupWorkspaces(std::vector<std::string>{inputWs, resultWs});
  }

  void testSquareYNumericAxisDefaultUnits() {
    using namespace Mantid::API;
    using namespace Mantid::Kernel;

    Mantid::Algorithms::ConvertAxisByFormula alg;
    alg.initialize();

    std::string inputWs = alg.name() + "_testSquareXNumeric_Input";
    std::string resultWs = alg.name() + "_testSquareXNumeric_Result";

    AnalysisDataService::Instance().add(
        inputWs, WorkspaceCreationHelper::create2DWorkspaceThetaVsTOF(10, 10));

    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("InputWorkspace", inputWs));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", resultWs));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Formula", "(y+2)*(Y+2)"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Axis", "Y"));

    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    if (!alg.isExecuted()) {
      cleanupWorkspaces(std::vector<std::string>{inputWs});
      return;
    }

    MatrixWorkspace_const_sptr in, result;
    TS_ASSERT_THROWS_NOTHING(
        in = boost::dynamic_pointer_cast<MatrixWorkspace>(
            AnalysisDataService::Instance().retrieve(inputWs)));
    TS_ASSERT_THROWS_NOTHING(
        result = boost::dynamic_pointer_cast<MatrixWorkspace>(
            AnalysisDataService::Instance().retrieve(resultWs)));

    Axis *ax = result->getAxis(1);
    TS_ASSERT_EQUALS(ax->unit()->caption(), in->getAxis(1)->unit()->caption());
    TS_ASSERT_EQUALS(ax->unit()->label(), in->getAxis(1)->unit()->label());
    for (size_t i = 0; i < ax->length(); ++i) {
      TS_ASSERT_DELTA(ax->getValue(i), (i + 1 + 2) * (i + 1 + 2), 0.0001);
    }

    cleanupWorkspaces(std::vector<std::string>{inputWs, resultWs});
  }

  void testYNumericAxisDisalowsGeometricOperators() {
    using namespace Mantid::API;
    using namespace Mantid::Kernel;

    Mantid::Algorithms::ConvertAxisByFormula alg;
    alg.initialize();

    std::string inputWs =
        alg.name() + "_testYNumericAxisDisalowsGeometricOperators_Input";
    std::string resultWs =
        alg.name() + "_testYNumericAxisDisalowsGeometricOperators_Result";

    AnalysisDataService::Instance().add(
        inputWs, WorkspaceCreationHelper::create2DWorkspaceThetaVsTOF(10, 10));

    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("InputWorkspace", inputWs));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", resultWs));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Formula", "y*twotheta"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Axis", "Y"));

    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(!alg.isExecuted());
    cleanupWorkspaces(std::vector<std::string>{inputWs});
  }

  bool runConvertAxisByFormula(std::string testName, std::string formula,
                               std::string axis, std::string &inputWs,
                               std::string &resultWs) {
    Mantid::Algorithms::ConvertAxisByFormula alg;
    alg.initialize();
    inputWs = alg.name() + "_" + testName + "_Input";
    resultWs = alg.name() + "_" + testName + "_Result";
    Mantid::API::AnalysisDataService::Instance().add(
        inputWs,
        WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(10, 10));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("InputWorkspace", inputWs));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", resultWs));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Formula", formula));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Axis", axis));

    TS_ASSERT_THROWS_NOTHING(alg.execute());

    if (!alg.isExecuted()) {
      cleanupWorkspaces(std::vector<std::string>{inputWs});
    }
    return alg.isExecuted();
  }

  void cleanupWorkspaces(const std::vector<std::string> &wsList) {
    for (const auto &wsName : wsList) {
      if (Mantid::API::AnalysisDataService::Instance().doesExist(wsName)) {
        Mantid::API::AnalysisDataService::Instance().remove(wsName);
      }
    }
  }

  void testGeomteryOperatorl1() {
    using namespace Mantid::API;
    using namespace Mantid::Kernel;

    std::string testName = "testGeomteryOperatorl1";
    std::string formula = "x+l1";
    std::string axis = "X";
    std::string inputWs;
    std::string resultWs;
    TS_ASSERT(
        runConvertAxisByFormula(testName, formula, axis, inputWs, resultWs));

    MatrixWorkspace_const_sptr in, result;
    TS_ASSERT_THROWS_NOTHING(
        in = boost::dynamic_pointer_cast<MatrixWorkspace>(
            AnalysisDataService::Instance().retrieve(inputWs)));
    TS_ASSERT_THROWS_NOTHING(
        result = boost::dynamic_pointer_cast<MatrixWorkspace>(
            AnalysisDataService::Instance().retrieve(resultWs)));

    double l1 = in->spectrumInfo().l1();
    Axis *axIn = in->getAxis(0);
    Axis *ax = result->getAxis(0);
    for (size_t i = 0; i < ax->length(); ++i) {
      TS_ASSERT_DELTA(ax->getValue(i), axIn->getValue(i) + l1, 0.0001);
    }
    cleanupWorkspaces(std::vector<std::string>{inputWs, resultWs});
  }

  void testGeomteryOperatorl2() {
    using namespace Mantid::API;
    using namespace Mantid::Kernel;

    std::string testName = "testGeomteryOperatorl2";
    std::string formula = "x/l2";
    std::string axis = "X";
    std::string inputWs;
    std::string resultWs;
    TS_ASSERT(
        runConvertAxisByFormula(testName, formula, axis, inputWs, resultWs));

    MatrixWorkspace_const_sptr in, result;
    TS_ASSERT_THROWS_NOTHING(
        in = boost::dynamic_pointer_cast<MatrixWorkspace>(
            AnalysisDataService::Instance().retrieve(inputWs)));
    TS_ASSERT_THROWS_NOTHING(
        result = boost::dynamic_pointer_cast<MatrixWorkspace>(
            AnalysisDataService::Instance().retrieve(resultWs)));

    const auto &spectrumInfo = in->spectrumInfo();
    const size_t xsize = result->blocksize();
    for (size_t i = 0; i < result->getNumberHistograms(); ++i) {
      const auto &outX = result->readX(i);
      const auto &outY = result->readY(i);
      const auto &outE = result->readE(i);
      const auto &inX = in->readX(i);
      const auto &inY = in->readY(i);
      const auto &inE = in->readE(i);

      double l2 = spectrumInfo.l2(i);
      for (size_t j = 0; j < xsize; ++j) {
        TS_ASSERT_DELTA(outX[j], inX[j] / l2, 0.0001);
        TS_ASSERT_EQUALS(outY[j], inY[j]);
        TS_ASSERT_EQUALS(outE[j], inE[j]);
      }
    }

    cleanupWorkspaces(std::vector<std::string>{inputWs, resultWs});
  }

  void testGeomteryOperatortwotheta() {
    using namespace Mantid::API;
    using namespace Mantid::Kernel;

    std::string testName = "testGeomteryOperatortwotheta";
    std::string formula = "x*(1+twotheta)";
    std::string axis = "X";
    std::string inputWs;
    std::string resultWs;
    TS_ASSERT(
        runConvertAxisByFormula(testName, formula, axis, inputWs, resultWs));

    MatrixWorkspace_const_sptr in, result;
    TS_ASSERT_THROWS_NOTHING(
        in = boost::dynamic_pointer_cast<MatrixWorkspace>(
            AnalysisDataService::Instance().retrieve(inputWs)));
    TS_ASSERT_THROWS_NOTHING(
        result = boost::dynamic_pointer_cast<MatrixWorkspace>(
            AnalysisDataService::Instance().retrieve(resultWs)));

    const auto &spectrumInfo = in->spectrumInfo();
    const size_t xsize = result->blocksize();
    for (size_t i = 0; i < result->getNumberHistograms(); ++i) {
      const auto &outX = result->readX(i);
      const auto &outY = result->readY(i);
      const auto &outE = result->readE(i);
      const auto &inX = in->readX(i);
      const auto &inY = in->readY(i);
      const auto &inE = in->readE(i);

      double twoTheta = spectrumInfo.twoTheta(i);
      for (size_t j = 0; j < xsize; ++j) {

        TS_ASSERT_DELTA(outX[j], inX[j] * (1 + twoTheta), 0.001);
        TS_ASSERT_EQUALS(outY[j], inY[j]);
        TS_ASSERT_EQUALS(outE[j], inE[j]);
      }
    }

    cleanupWorkspaces(std::vector<std::string>{inputWs, resultWs});
  }

  void testGeomteryOperatorsignedtwotheta() {
    using namespace Mantid::API;
    using namespace Mantid::Kernel;

    std::string testName = "testGeomteryOperatorsignedtwotheta";
    std::string formula = "x-signedtwotheta";
    std::string axis = "X";
    std::string inputWs;
    std::string resultWs;
    TS_ASSERT(
        runConvertAxisByFormula(testName, formula, axis, inputWs, resultWs));

    MatrixWorkspace_const_sptr in, result;
    TS_ASSERT_THROWS_NOTHING(
        in = boost::dynamic_pointer_cast<MatrixWorkspace>(
            AnalysisDataService::Instance().retrieve(inputWs)));
    TS_ASSERT_THROWS_NOTHING(
        result = boost::dynamic_pointer_cast<MatrixWorkspace>(
            AnalysisDataService::Instance().retrieve(resultWs)));

    const auto &spectrumInfo = in->spectrumInfo();
    const size_t xsize = result->blocksize();
    for (size_t i = 0; i < result->getNumberHistograms(); ++i) {
      const auto &outX = result->readX(i);
      const auto &outY = result->readY(i);
      const auto &outE = result->readE(i);
      const auto &inX = in->readX(i);
      const auto &inY = in->readY(i);
      const auto &inE = in->readE(i);

      double signedTwoTheta = spectrumInfo.signedTwoTheta(i);
      for (size_t j = 0; j < xsize; ++j) {
        TS_ASSERT_DELTA(outX[j], inX[j] - signedTwoTheta, 0.0001);
        TS_ASSERT_EQUALS(outY[j], inY[j]);
        TS_ASSERT_EQUALS(outE[j], inE[j]);
      }
    }

    cleanupWorkspaces(std::vector<std::string>{inputWs, resultWs});
  }

  void testWorkspaceReversedIfNeeded() {
    using namespace Mantid::API;
    using namespace Mantid::Kernel;

    std::string testName = "testGeomteryOperatorsignedtwotheta";
    std::string formula = "-x";
    std::string axis = "X";
    std::string inputWs;
    std::string resultWs;
    TS_ASSERT(
        runConvertAxisByFormula(testName, formula, axis, inputWs, resultWs));

    MatrixWorkspace_const_sptr in, result;
    TS_ASSERT_THROWS_NOTHING(
        in = boost::dynamic_pointer_cast<MatrixWorkspace>(
            AnalysisDataService::Instance().retrieve(inputWs)));
    TS_ASSERT_THROWS_NOTHING(
        result = boost::dynamic_pointer_cast<MatrixWorkspace>(
            AnalysisDataService::Instance().retrieve(resultWs)));

    const size_t xsize = result->blocksize();
    for (size_t i = 0; i < result->getNumberHistograms(); ++i) {
      const auto &outX = result->readX(i);
      const auto &outY = result->readY(i);
      const auto &outE = result->readE(i);
      const auto &inX = in->readX(i);
      const auto &inY = in->readY(i);
      const auto &inE = in->readE(i);

      for (size_t j = 0; j < xsize; ++j) {
        TS_ASSERT_DELTA(outX[j], -inX[xsize - j], 0.0001);
        TS_ASSERT_EQUALS(outY[j], inY[xsize - j - 1]);
        TS_ASSERT_EQUALS(outE[j], inE[xsize - j - 1]);
      }
    }

    cleanupWorkspaces(std::vector<std::string>{inputWs, resultWs});
  }

  void testConstant() {
    using namespace Mantid::API;
    using namespace Mantid::Kernel;

    std::string testName = "testConstant";
    std::string formula = "pi";
    std::string axis = "X";
    std::string inputWs;
    std::string resultWs;
    TS_ASSERT(
        runConvertAxisByFormula(testName, formula, axis, inputWs, resultWs));

    MatrixWorkspace_const_sptr in, result;
    TS_ASSERT_THROWS_NOTHING(
        in = boost::dynamic_pointer_cast<MatrixWorkspace>(
            AnalysisDataService::Instance().retrieve(inputWs)));
    TS_ASSERT_THROWS_NOTHING(
        result = boost::dynamic_pointer_cast<MatrixWorkspace>(
            AnalysisDataService::Instance().retrieve(resultWs)));

    const size_t xsize = result->blocksize();
    for (size_t i = 0; i < result->getNumberHistograms(); ++i) {
      const auto &outX = result->readX(i);
      const auto &outY = result->readY(i);
      const auto &outE = result->readE(i);
      const auto &inY = in->readY(i);
      const auto &inE = in->readE(i);

      for (size_t j = 0; j < xsize; ++j) {
        TS_ASSERT_DELTA(outX[j], M_PI, 0.0001);
        TS_ASSERT_EQUALS(outY[j], inY[j]);
        TS_ASSERT_EQUALS(outE[j], inE[j]);
      }
    }

    cleanupWorkspaces(std::vector<std::string>{inputWs, resultWs});
  }
};

#endif /* MANTID_ALGORITHMS_CONVERTAXISBYFORMULATEST_H_ */
