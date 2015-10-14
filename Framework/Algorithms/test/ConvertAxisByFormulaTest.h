#ifndef MANTID_ALGORITHMS_CONVERTAXISBYFORMULATEST_H_
#define MANTID_ALGORITHMS_CONVERTAXISBYFORMULATEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/ConvertAxisByFormula.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

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
        inputWs, WorkspaceCreationHelper::Create2DWorkspace123(10, 10));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("InputWorkspace", inputWs))
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", resultWs))
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Formula", "x+3"))
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Axis", "X"))
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("AxisTitle", "My Title"))
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("AxisUnits", "MyUnit"))

    TS_ASSERT_THROWS_NOTHING(alg.execute())
    TS_ASSERT(alg.isExecuted())

    if (!alg.isExecuted()) {
      if (AnalysisDataService::Instance().doesExist(inputWs)) {
        AnalysisDataService::Instance().remove(inputWs);
      }
      return;
    }

    MatrixWorkspace_const_sptr in, result;
    TS_ASSERT_THROWS_NOTHING(
        in = boost::dynamic_pointer_cast<MatrixWorkspace>(
            AnalysisDataService::Instance().retrieve(inputWs)))
    TS_ASSERT_THROWS_NOTHING(
        result = boost::dynamic_pointer_cast<MatrixWorkspace>(
            AnalysisDataService::Instance().retrieve(resultWs)))

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

    if (AnalysisDataService::Instance().doesExist(inputWs)) {
      AnalysisDataService::Instance().remove(inputWs);
    }
    if (AnalysisDataService::Instance().doesExist(resultWs)) {
      AnalysisDataService::Instance().remove(resultWs);
    }
  }

  void testSquareXNumericAxis() {
    using namespace Mantid::API;
    using namespace Mantid::Kernel;

    Mantid::Algorithms::ConvertAxisByFormula alg;
    alg.initialize();

    std::string inputWs = alg.name() + "_testSquareXNumeric_Input";
    std::string resultWs = alg.name() + "_testSquareXNumeric_Result";

    AnalysisDataService::Instance().add(
        inputWs, WorkspaceCreationHelper::Create2DWorkspace123(10, 10));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("InputWorkspace", inputWs))
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", resultWs))
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Formula", "(X+2)*(x+2)"))
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Axis", "X"))
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("AxisTitle", "XTitle"))
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("AxisUnits", "XUnit"))

    TS_ASSERT_THROWS_NOTHING(alg.execute())
    TS_ASSERT(alg.isExecuted())

    if (!alg.isExecuted()) {
      if (AnalysisDataService::Instance().doesExist(inputWs)) {
        AnalysisDataService::Instance().remove(inputWs);
      }
      return;
    }

    MatrixWorkspace_const_sptr in, result;
    TS_ASSERT_THROWS_NOTHING(
        in = boost::dynamic_pointer_cast<MatrixWorkspace>(
            AnalysisDataService::Instance().retrieve(inputWs)))
    TS_ASSERT_THROWS_NOTHING(
        result = boost::dynamic_pointer_cast<MatrixWorkspace>(
            AnalysisDataService::Instance().retrieve(resultWs)))

    Axis *ax = result->getAxis(0);
    TS_ASSERT_EQUALS(ax->unit()->caption(), "XTitle");
    TS_ASSERT_EQUALS(ax->unit()->label(), "XUnit");
    for (size_t i = 0; i < ax->length(); ++i) {
      TS_ASSERT_DELTA(ax->getValue(i), 9.0, 0.0001);
    }

    if (AnalysisDataService::Instance().doesExist(inputWs)) {
      AnalysisDataService::Instance().remove(inputWs);
    }
    if (AnalysisDataService::Instance().doesExist(resultWs)) {
      AnalysisDataService::Instance().remove(resultWs);
    }
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

    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("InputWorkspace", inputWs))
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", resultWs))
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Formula", "(y+2)*(Y+2)"))
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Axis", "Y"))

    TS_ASSERT_THROWS_NOTHING(alg.execute())
    TS_ASSERT(alg.isExecuted())

    if (!alg.isExecuted()) {
      if (AnalysisDataService::Instance().doesExist(inputWs)) {
        AnalysisDataService::Instance().remove(inputWs);
      }
      return;
    }

    MatrixWorkspace_const_sptr in, result;
    TS_ASSERT_THROWS_NOTHING(
        in = boost::dynamic_pointer_cast<MatrixWorkspace>(
            AnalysisDataService::Instance().retrieve(inputWs)))
    TS_ASSERT_THROWS_NOTHING(
        result = boost::dynamic_pointer_cast<MatrixWorkspace>(
            AnalysisDataService::Instance().retrieve(resultWs)))

    Axis *ax = result->getAxis(1);
    TS_ASSERT_EQUALS(ax->unit()->caption(), in->getAxis(1)->unit()->caption());
    TS_ASSERT_EQUALS(ax->unit()->label(), in->getAxis(1)->unit()->label());
    for (size_t i = 0; i < ax->length(); ++i) {
      TS_ASSERT_DELTA(ax->getValue(i), (i + 1 + 2) * (i + 1 + 2), 0.0001);
    }

    if (AnalysisDataService::Instance().doesExist(inputWs)) {
      AnalysisDataService::Instance().remove(inputWs);
    }
    if (AnalysisDataService::Instance().doesExist(resultWs)) {
      AnalysisDataService::Instance().remove(resultWs);
    }
  }
};

#endif /* MANTID_ALGORITHMS_CONVERTAXISBYFORMULATEST_H_ */
