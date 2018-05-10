#ifndef FOREGROUNDMODELTEST_H_
#define FOREGROUNDMODELTEST_H_

#include "MDFittingTestHelpers.h"
#include "MantidMDAlgorithms/Quantification/ForegroundModel.h"
#include <cxxtest/TestSuite.h>

class ForegroundModelTest : public CxxTest::TestSuite {
public:
  void test_getInitial_Always_Returns_Default_Value() {
    Fake1DFunction fitFunction;
    double dummy[1] = {0};

    TS_ASSERT_DELTA(fitFunction.fgModel.getInitialParameterValue(0),
                    fitFunction.fgModel.start1, 1e-12);
    TS_ASSERT_DELTA(fitFunction.fgModel.getInitialParameterValue(1),
                    fitFunction.fgModel.start2, 1e-12);

    // Run fit
    fitFunction.function1D(dummy, dummy, 1);

    TS_ASSERT_DELTA(fitFunction.fgModel.getInitialParameterValue(0),
                    fitFunction.fgModel.start1, 1e-12);
    TS_ASSERT_DELTA(fitFunction.fgModel.getInitialParameterValue(1),
                    fitFunction.fgModel.start2, 1e-12);
  }

  void
  test_getCurrent_returns_correct_fitting_parameters_when_fitfunction_itself_has_other_parameters() {
    Fake1DFunction fitFunction;
    double dummy[1] = {0};

    TS_ASSERT_DELTA(fitFunction.fgModel.getCurrentParameterValue(0),
                    fitFunction.fgModel.start1, 1e-12);
    TS_ASSERT_DELTA(fitFunction.fgModel.getCurrentParameterValue(1),
                    fitFunction.fgModel.start2, 1e-12);

    // Run fit
    fitFunction.function1D(dummy, dummy, 1);

    TS_ASSERT_DELTA(fitFunction.fgModel.getCurrentParameterValue(0),
                    fitFunction.fgModel.a0, 1e-12);
    TS_ASSERT_DELTA(fitFunction.fgModel.getCurrentParameterValue(1),
                    fitFunction.fgModel.a1, 1e-12);
  }
};

#endif // FOREGROUNDMODELTEST_H_
