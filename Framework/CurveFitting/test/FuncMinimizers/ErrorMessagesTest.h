// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef CURVEFITTING_MINIMIZER_ERROR_MESSAGES_TEST_H_
#define CURVEFITTING_MINIMIZER_ERROR_MESSAGES_TEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidCurveFitting/Algorithms/Fit.h"

#include "MantidTestHelpers/WorkspaceCreationHelper.h"

using namespace Mantid::API;
using namespace Mantid::CurveFitting::Algorithms;

class ErrorMessagesTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ErrorMessagesTest *createSuite() { return new ErrorMessagesTest(); }
  static void destroySuite(ErrorMessagesTest *suite) { delete suite; }

  ErrorMessagesTest() {
    // need to have DataObjects loaded
    FrameworkManager::Instance();
  }

  void test_too_few_iterations() {
    auto ws = WorkspaceCreationHelper::create2DWorkspaceFromFunction(
        [](double x, int) { return 2.0 * (x * x + 4.0); }, 1, -10.0, 10.0, 0.1);
    Fit fit;
    fit.initialize();
    fit.setProperty("Function", "name=Lorentzian,Amplitude=5,FWHM=1");
    fit.setProperty("InputWorkspace", ws);
    fit.setProperty("MaxIterations", 10);
    fit.execute();
    std::string status = fit.getProperty("OutputStatus");
    TS_ASSERT_EQUALS(status, "Failed to converge after 10 iterations.");
  }

  void test_bad_parameter() {
    auto ws = WorkspaceCreationHelper::create2DWorkspaceFromFunction(
        [](double x, int) { return 2.0 + x; }, 1, -10.0, 10.0, 0.1);
    // Parameter (b) that cannot be determined.
    {
      Fit fit;
      fit.initialize();
      fit.setProperty("Function", "name=UserFunction,Formula=a+b*x*0");
      fit.setProperty("InputWorkspace", ws);
      fit.setProperty("Minimizer", "Levenberg-Marquardt");
      fit.setProperty("MaxIterations", 10);
      fit.execute();
      std::string status = fit.getProperty("OutputStatus");
      TS_ASSERT_EQUALS(status, "Changes in function value are too small");
    }
    {
      Fit fit;
      fit.initialize();
      fit.setProperty("Function", "name=UserFunction,Formula=a+b*x*0");
      fit.setProperty("InputWorkspace", ws);
      fit.setProperty("Minimizer", "Levenberg-MarquardtMD");
      fit.setProperty("MaxIterations", 10);
      fit.execute();
      std::string status = fit.getProperty("OutputStatus");
      TS_ASSERT_EQUALS(status, "Function doesn\'t depend on parameter b");
    }
    {
      Fit fit;
      fit.initialize();
      fit.setProperty("Function", "name=UserFunction,Formula=a+b*x*0");
      fit.setProperty("InputWorkspace", ws);
      fit.setProperty("Minimizer", "Simplex");
      fit.setProperty("MaxIterations", 10);
      fit.execute();
      std::string status = fit.getProperty("OutputStatus");
      TS_ASSERT_EQUALS(status, "Failed to converge after 10 iterations.");
    }
    {
      Fit fit;
      fit.initialize();
      fit.setProperty("Function", "name=UserFunction,Formula=a+b*x*0");
      fit.setProperty("InputWorkspace", ws);
      fit.setProperty("Minimizer", "Trust Region");
      fit.setProperty("MaxIterations", 10);
      fit.execute();
      std::string status = fit.getProperty("OutputStatus");
      TS_ASSERT_EQUALS(status, "success");
    }
    {
      Fit fit;
      fit.initialize();
      fit.setProperty("Function", "name=UserFunction,Formula=a+b*x*0");
      fit.setProperty("InputWorkspace", ws);
      fit.setProperty("Minimizer", "BFGS");
      fit.setProperty("MaxIterations", 10);
      fit.execute();
      std::string status = fit.getProperty("OutputStatus");
      TS_ASSERT_EQUALS(status, "success");
    }
    {
      Fit fit;
      fit.initialize();
      fit.setProperty("Function", "name=UserFunction,Formula=a+b*x*0");
      fit.setProperty("InputWorkspace", ws);
      fit.setProperty("Minimizer", "Conjugate gradient (Fletcher-Reeves imp.)");
      fit.setProperty("MaxIterations", 10);
      fit.execute();
      std::string status = fit.getProperty("OutputStatus");
      TS_ASSERT_EQUALS(status, "success");
    }
    {
      Fit fit;
      fit.initialize();
      fit.setProperty("Function", "name=UserFunction,Formula=a+b*x*0");
      fit.setProperty("InputWorkspace", ws);
      fit.setProperty("Minimizer", "Conjugate gradient (Polak-Ribiere imp.)");
      fit.setProperty("MaxIterations", 10);
      fit.execute();
      std::string status = fit.getProperty("OutputStatus");
      TS_ASSERT_EQUALS(status, "success");
    }
    {
      Fit fit;
      fit.initialize();
      fit.setProperty("Function", "name=UserFunction,Formula=a+b*x*0");
      fit.setProperty("InputWorkspace", ws);
      fit.setProperty("Minimizer", "SteepestDescent");
      fit.setProperty("MaxIterations", 10);
      fit.execute();
      std::string status = fit.getProperty("OutputStatus");
      TS_ASSERT_EQUALS(status, "Failed to converge after 10 iterations.");
    }
    {
      Fit fit;
      fit.initialize();
      fit.setProperty("Function", "name=UserFunction,Formula=a+b*x*0");
      fit.setProperty("InputWorkspace", ws);
      fit.setProperty("Minimizer", "Damped GaussNewton");
      fit.setProperty("MaxIterations", 10);
      fit.execute();
      std::string status = fit.getProperty("OutputStatus");
      TS_ASSERT_EQUALS(status, "Function doesn\'t depend on parameter b");
    }
    {
      Fit fit;
      fit.initialize();
      fit.setProperty("Function", "name=UserFunction,Formula=a+b*x*0");
      fit.setProperty("InputWorkspace", ws);
      fit.setProperty("Minimizer", "Damped GaussNewton,Damping=0.001");
      fit.setProperty("MaxIterations", 10);
      fit.execute();
      std::string status = fit.getProperty("OutputStatus");
      TS_ASSERT_EQUALS(status, "success");
    }
  }

  void test_tough_problem_matlab() {
    std::vector<double> x({0,   0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9, 1,
                           1.1, 1.2, 1.3, 1.4, 1.5, 1.6, 1.7, 1.8, 1.9, 2});
    std::vector<double> y({5.8955, 3.5639, 2.5173, 1.979,  1.899,  1.3938,
                           1.1359, 1.0096, 1.0343, 0.8435, 0.6856, 0.61,
                           0.5392, 0.3946, 0.3903, 0.5474, 0.3459, 0.137,
                           0.2211, 0.1704, 0.2636});
    auto ws = createData(x, y);
    auto funs = "name=UserFunction,Formula=c1*exp( -lam1*x)+c2*exp( "
                "-lam2*x),c1=5,lam1=1,c2=1,lam2=0";
    {
      Fit fit;
      fit.initialize();
      fit.setProperty("Function", funs);
      fit.setProperty("InputWorkspace", ws);
      fit.setProperty("Minimizer", "Levenberg-MarquardtMD");
      fit.execute();
      std::string status = fit.getProperty("OutputStatus");
      TS_ASSERT_EQUALS(status, "Failed to converge, maximum mu reached.");
    }
  }

  void test_tough_problem_bennett5() {
    std::vector<double> x(
        {7.447168,  8.102586, 8.452547, 8.711278, 8.916774, 9.087155,
         9.23259,   9.359535, 9.472166, 9.573384, 9.665293, 9.749461,
         9.827092,  9.899128, 9.966321, 10.02928, 10.08851, 10.14443,
         10.19738,  10.24767, 10.29556, 10.34125, 10.38495, 10.42682,
         10.467,    10.50564, 10.54283, 10.57869, 10.61331, 10.64678,
         10.67915,  10.71052, 10.74092, 10.77044, 10.7991,  10.82697,
         10.85408,  10.88047, 10.90619, 10.93126, 10.95572, 10.97959,
         11.00291,  11.0257,  11.04798, 11.06977, 11.0911,  11.11198,
         11.13244,  11.15248, 11.17213, 11.19141, 11.21031, 11.22887,
         11.24709,  11.26498, 11.28256, 11.29984, 11.31682, 11.33352,
         11.34994,  11.3661,  11.382,   11.39766, 11.41307, 11.42824,
         11.4432,   11.45793, 11.47244, 11.48675, 11.50086, 11.51477,
         11.52849,  11.54202, 11.55538, 11.56855, 11.58156, 11.59442,
         11.607121, 11.61964, 11.632,   11.64421, 11.65628, 11.6682,
         11.67998,  11.69162, 11.70313, 11.71451, 11.72576, 11.73688,
         11.74789,  11.75878, 11.76955, 11.7802,  11.79073, 11.80116,
         11.81148,  11.8217,  11.83181, 11.84182, 11.85173, 11.86155,
         11.87127,  11.88089, 11.89042, 11.89987, 11.90922, 11.91849,
         11.92768,  11.93678, 11.94579, 11.95473, 11.96359, 11.97237,
         11.98107,  11.9897,  11.99826, 12.00674, 12.01515, 12.02349,
         12.03176,  12.03997, 12.0481,  12.05617, 12.06418, 12.07212,
         12.08001,  12.08782, 12.09558, 12.10328, 12.11092, 12.1185,
         12.12603,  12.1335,  12.14091, 12.14827, 12.15557, 12.16283,
         12.17003,  12.17717, 12.18427, 12.19132, 12.19832, 12.20527,
         12.21217,  12.21903, 12.22584, 12.2326,  12.23932, 12.24599,
         12.25262,  12.2592,  12.26575, 12.27224});
    std::vector<double> y(
        {-34.834702, -34.3932,   -34.152901, -33.979099, -33.845901, -33.732899,
         -33.640301, -33.5592,   -33.486801, -33.4231,   -33.365101, -33.313,
         -33.260899, -33.2174,   -33.176899, -33.139198, -33.101601, -33.066799,
         -33.035,    -33.003101, -32.971298, -32.942299, -32.916302, -32.890202,
         -32.864101, -32.841,    -32.817799, -32.797501, -32.7743,   -32.757,
         -32.733799, -32.7164,   -32.6991,   -32.678799, -32.6614,   -32.644001,
         -32.626701, -32.612202, -32.597698, -32.583199, -32.568699, -32.554298,
         -32.539799, -32.525299, -32.510799, -32.499199, -32.487598, -32.473202,
         -32.461601, -32.435501, -32.435501, -32.4268,   -32.4123,   -32.400799,
         -32.392101, -32.380501, -32.366001, -32.3573,   -32.348598, -32.339901,
         -32.3284,   -32.319698, -32.311001, -32.2994,   -32.290699, -32.282001,
         -32.2733,   -32.264599, -32.256001, -32.247299, -32.238602, -32.2299,
         -32.224098, -32.215401, -32.2038,   -32.198002, -32.1894,   -32.183601,
         -32.1749,   -32.169102, -32.1633,   -32.154598, -32.145901, -32.140099,
         -32.131401, -32.125599, -32.119801, -32.111198, -32.1054,   -32.096699,
         -32.0909,   -32.088001, -32.0793,   -32.073502, -32.067699, -32.061901,
         -32.056099, -32.050301, -32.044498, -32.038799, -32.033001, -32.027199,
         -32.0243,   -32.018501, -32.012699, -32.004002, -32.001099, -31.9953,
         -31.9895,   -31.9837,   -31.9779,   -31.972099, -31.969299, -31.963501,
         -31.957701, -31.9519,   -31.9461,   -31.9403,   -31.937401, -31.931601,
         -31.9258,   -31.922899, -31.917101, -31.911301, -31.9084,   -31.902599,
         -31.8969,   -31.893999, -31.888201, -31.8853,   -31.882401, -31.8766,
         -31.873699, -31.867901, -31.862101, -31.8592,   -31.8563,   -31.8505,
         -31.8447,   -31.841801, -31.8389,   -31.833099, -31.8302,   -31.827299,
         -31.8216,   -31.818701, -31.812901, -31.809999, -31.8071,   -31.8013,
         -31.798401, -31.7955,   -31.7897,   -31.7868});
    auto ws = createData(x, y);
    auto funs =
        "name=UserFunction,Formula=b1*(b2+x)^( -1/b3),b1=-2000,b2=50,b3=0.8";
    {
      Fit fit;
      fit.initialize();
      fit.setProperty("Function", funs);
      fit.setProperty("InputWorkspace", ws);
      fit.setProperty("Minimizer", "Levenberg-MarquardtMD");
      fit.execute();
      std::string status = fit.getProperty("OutputStatus");
      TS_ASSERT_EQUALS(status, "Failed to converge after 500 iterations.");
    }
  }

  void test_tough_problem_rat43() {
    std::vector<double> x({1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15});
    std::vector<double> y({16.08, 33.83, 65.8, 97.2, 191.55, 326.2, 386.87,
                           520.53, 590.03, 651.92, 724.93, 699.56, 689.96,
                           637.56, 717.41});
    auto ws = createData(x, y);
    auto funs = "name=UserFunction,Formula=b1/((1+exp(b2-b3*x))^(1/"
                "b4)),b1=100,b2=10,b3=1,b4=1";
    {
      Fit fit;
      fit.initialize();
      fit.setProperty("Function", funs);
      fit.setProperty("InputWorkspace", ws);
      fit.setProperty("Minimizer", "Damped GaussNewton");
      fit.execute();
      std::string status = fit.getProperty("OutputStatus");
      TS_ASSERT_EQUALS(status, "Encountered an infinite number or NaN.");
    }
  }

private:
  MatrixWorkspace_sptr createData(const std::vector<double> &x,
                                  const std::vector<double> &y) {
    size_t nbins = x.size();
    auto ws =
        WorkspaceFactory::Instance().create("Workspace2D", 1, nbins, nbins);
    ws->mutableX(0) = x;
    ws->mutableY(0) = y;
    return ws;
  }
};

#endif /*CURVEFITTING_MINIMIZER_ERROR_MESSAGES_TEST_H_*/
