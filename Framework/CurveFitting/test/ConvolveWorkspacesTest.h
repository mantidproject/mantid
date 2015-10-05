#ifndef ConvolveWorkspacesTEST_H_
#define ConvolveWorkspacesTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidCurveFitting/ConvolveWorkspaces.h"
#include "MantidCurveFitting/Fit.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidAPI/IPeakFunction.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include "MantidTestHelpers/FakeObjects.h"
#include "MantidCurveFitting/Gaussian.h"

using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::CurveFitting;

class ConvolveWorkspacesTest : public CxxTest::TestSuite {
public:
  // Functor to generate spline values
  struct NormGaussianFunc1 {
    double operator()(double x, int) {
      double sig = 0.1;
      return exp(-pow(x, 2) / (2 * pow(sig, 2))) / (sqrt(2 * M_PI) * sig);
    }
  };
  // Functor to generate spline values
  struct NormGaussianFunc2 {
    double operator()(double x, int) {
      double sig = sqrt(0.1 * 0.1 + 0.1 * 0.1);
      return exp(-pow(x, 2) / (2 * pow(sig, 2))) / (sqrt(2 * M_PI) * sig);
    }
  };
  void testFunction() {
    ConvolveWorkspaces alg;

    // Convolution of normalized Gaussians should have sigma =
    // sqrt(sig1^2+sig2^2)
    Workspace2D_sptr ws1 =
        WorkspaceCreationHelper::Create2DWorkspaceFromFunction(
            NormGaussianFunc1(), 1, -2.0, 2.0, 0.01, false);
    Workspace2D_sptr ws2 =
        WorkspaceCreationHelper::Create2DWorkspaceFromFunction(
            NormGaussianFunc2(), 1, -2.0, 2.0, 0.01, false);
    TS_ASSERT_THROWS_NOTHING(
        AnalysisDataService::Instance().addOrReplace("wksp1", ws1));
    TS_ASSERT_THROWS_NOTHING(
        AnalysisDataService::Instance().addOrReplace("wksp2", ws2));

    alg.initialize();
    alg.isInitialized();
    alg.setChild(true);
    alg.setPropertyValue("OutputWorkspace", "Conv");
    alg.setProperty("Workspace1", "wksp1");
    alg.setProperty("Workspace2", "wksp1");

    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    Workspace2D_sptr ows = alg.getProperty("OutputWorkspace");

    for (size_t i = 0; i < ows->getNumberHistograms(); ++i) {
      const auto &xs2 = ws2->readX(i);
      const auto &xs = ows->readX(i);
      const auto &ys2 = ws2->readY(i);
      const auto &ys = ows->readY(i);

      // check output for consistency
      for (size_t j = 0; j < ys.size(); ++j) {
        TS_ASSERT_DELTA(xs[j], xs2[j], 1e-15);
        TS_ASSERT_DELTA(ys[j], ys2[j], 1e-8);
      }
    }
    AnalysisDataService::Instance().remove("wksp1");
    AnalysisDataService::Instance().remove("wksp2");
  }
};

#endif /*ConvolveWorkspacesTEST_H_*/
