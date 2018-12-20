#ifndef ESTIMATEPEAKERRORSTEST_H_
#define ESTIMATEPEAKERRORSTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidCurveFitting/Algorithms/EstimatePeakErrors.h"

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FunctionDomain1D.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/FunctionValues.h"
#include "MantidAPI/IFunction.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceFactory.h"

#include "MantidTestHelpers/WorkspaceCreationHelper.h"

using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::CurveFitting;
using namespace Mantid::CurveFitting::Algorithms;
using namespace WorkspaceCreationHelper;

class EstimatePeakErrorsTest : public CxxTest::TestSuite {
public:
  void test_on_Gaussian() {
    auto fun = FunctionFactory::Instance().createInitialized(
        "name=Gaussian,PeakCentre=0,Height=1,Sigma=2");
    auto ws = createWorkspace(*fun);

    auto fit = AlgorithmManager::Instance().create("Fit");
    fit->setProperty("Function", fun);
    fit->setProperty("InputWorkspace", ws);
    fit->setProperty("CalcErrors", true);
    fit->execute();

    EstimatePeakErrors alg;
    alg.initialize();
    alg.setProperty("Function", fun);
    alg.setPropertyValue("OutputWorkspace", "Errors");
    alg.execute();

    auto res =
        AnalysisDataService::Instance().retrieveWS<ITableWorkspace>("Errors");
    TS_ASSERT_EQUALS(res->rowCount(), 4);

    TS_ASSERT_EQUALS(res->cell<std::string>(0, 0), "Centre");
    TS_ASSERT_EQUALS(res->cell<std::string>(1, 0), "Height");
    TS_ASSERT_EQUALS(res->cell<std::string>(2, 0), "FWHM");
    TS_ASSERT_EQUALS(res->cell<std::string>(3, 0), "Intensity");

    TS_ASSERT_DELTA(res->cell<double>(0, 1), -0.0068, 1e-4);
    TS_ASSERT_DELTA(res->cell<double>(1, 1), 1.0036, 1e-4);
    TS_ASSERT_DELTA(res->cell<double>(2, 1), 4.8046, 1e-4);
    TS_ASSERT_DELTA(res->cell<double>(3, 1), 5.1330, 1e-4);

    TS_ASSERT_DELTA(res->cell<double>(0, 2), 0.7467, 1e-4);
    TS_ASSERT_DELTA(res->cell<double>(1, 2), 0.3172, 1e-4);
    TS_ASSERT_DELTA(res->cell<double>(2, 2), 1.7598, 1e-4);
    TS_ASSERT_DELTA(res->cell<double>(3, 2), 1.6263, 1e-4);

    AnalysisDataService::Instance().clear();
  }

  void test_on_Gaussian_ties() {
    auto fun = FunctionFactory::Instance().createInitialized(
        "name=Gaussian,PeakCentre=0,Height=1,Sigma=2,ties=(Sigma=2)");
    auto ws = createWorkspace(*fun);

    auto fit = AlgorithmManager::Instance().create("Fit");
    fit->setProperty("Function", fun);
    fit->setProperty("InputWorkspace", ws);
    fit->setProperty("CalcErrors", true);
    fit->execute();

    EstimatePeakErrors alg;
    alg.initialize();
    alg.setProperty("Function", fun);
    alg.setPropertyValue("OutputWorkspace", "Errors");
    alg.execute();

    auto res =
        AnalysisDataService::Instance().retrieveWS<ITableWorkspace>("Errors");
    TS_ASSERT_EQUALS(res->rowCount(), 4);

    TS_ASSERT_EQUALS(res->cell<std::string>(0, 0), "Centre");
    TS_ASSERT_EQUALS(res->cell<std::string>(1, 0), "Height");
    TS_ASSERT_EQUALS(res->cell<std::string>(2, 0), "FWHM");
    TS_ASSERT_EQUALS(res->cell<std::string>(3, 0), "Intensity");

    TS_ASSERT_DELTA(res->cell<double>(0, 1), -0.0071, 1e-4);
    TS_ASSERT_DELTA(res->cell<double>(1, 1), 1.0136, 1e-4);
    TS_ASSERT_DELTA(res->cell<double>(2, 1), 4.7096, 1e-4);
    TS_ASSERT_DELTA(res->cell<double>(3, 1), 5.0816, 1e-4);

    TS_ASSERT_DELTA(res->cell<double>(0, 2), 0.7327, 1e-4);
    TS_ASSERT_DELTA(res->cell<double>(1, 2), 0.2625, 1e-4);
    TS_ASSERT_DELTA(res->cell<double>(2, 2), 0.0000, 1e-14);
    TS_ASSERT_DELTA(res->cell<double>(3, 2), 1.3164, 1e-4);

    AnalysisDataService::Instance().clear();
  }

  void test_on_Gaussian_unfitted() {
    auto fun = FunctionFactory::Instance().createInitialized(
        "name=Gaussian,PeakCentre=0,Height=1,Sigma=2");

    EstimatePeakErrors alg;
    alg.initialize();
    alg.setProperty("Function", fun);
    alg.setPropertyValue("OutputWorkspace", "Errors");
    alg.execute();

    auto res =
        AnalysisDataService::Instance().retrieveWS<ITableWorkspace>("Errors");
    TS_ASSERT_EQUALS(res->rowCount(), 0);

    AnalysisDataService::Instance().clear();
  }

  void test_on_Lorentzians() {
    std::string funStr = "name=Lorentzian,Amplitude=10,PeakCentre=-4,FWHM=2;"
                         "name=Lorentzian,Amplitude=10,PeakCentre=3,FWHM=3;"
                         "name=FlatBackground,A0=3";
    auto fun = FunctionFactory::Instance().createInitialized(funStr);
    auto ws = createWorkspace(*fun);

    auto fit = AlgorithmManager::Instance().create("Fit");
    fit->setProperty("Function", fun);
    fit->setProperty("InputWorkspace", ws);
    fit->setProperty("CalcErrors", true);
    fit->execute();

    EstimatePeakErrors alg;
    alg.initialize();
    alg.setProperty("Function", fun);
    alg.setPropertyValue("OutputWorkspace", "Errors");
    alg.execute();

    auto res =
        AnalysisDataService::Instance().retrieveWS<ITableWorkspace>("Errors");
    TS_ASSERT_EQUALS(res->rowCount(), 8);

    TS_ASSERT_EQUALS(res->cell<std::string>(0, 0), "f0.Centre");
    TS_ASSERT_EQUALS(res->cell<std::string>(1, 0), "f0.Height");
    TS_ASSERT_EQUALS(res->cell<std::string>(2, 0), "f0.FWHM");
    TS_ASSERT_EQUALS(res->cell<std::string>(3, 0), "f0.Intensity");

    TS_ASSERT_DELTA(res->cell<double>(0, 1), -3.9865, 1e-4);
    TS_ASSERT_DELTA(res->cell<double>(1, 1), 3.1883, 1e-4);
    TS_ASSERT_DELTA(res->cell<double>(2, 1), 2.0007, 1e-4);
    TS_ASSERT_DELTA(res->cell<double>(3, 1), 10.0200, 1e-4);

    TS_ASSERT_DELTA(res->cell<double>(0, 2), 0.1764, 1e-4);
    TS_ASSERT_DELTA(res->cell<double>(1, 2), 0.5684, 1e-4);
    TS_ASSERT_DELTA(res->cell<double>(2, 2), 0.6063, 1e-4);
    TS_ASSERT_DELTA(res->cell<double>(3, 2), 2.6687, 1e-4);

    TS_ASSERT_EQUALS(res->cell<std::string>(4, 0), "f1.Centre");
    TS_ASSERT_EQUALS(res->cell<std::string>(5, 0), "f1.Height");
    TS_ASSERT_EQUALS(res->cell<std::string>(6, 0), "f1.FWHM");
    TS_ASSERT_EQUALS(res->cell<std::string>(7, 0), "f1.Intensity");

    TS_ASSERT_DELTA(res->cell<double>(4, 1), 3.0064, 1e-4);
    TS_ASSERT_DELTA(res->cell<double>(5, 1), 2.1327, 1e-4);
    TS_ASSERT_DELTA(res->cell<double>(6, 1), 2.9908, 1e-4);
    TS_ASSERT_DELTA(res->cell<double>(7, 1), 10.0188, 1e-4);

    TS_ASSERT_DELTA(res->cell<double>(4, 2), 0.3232, 1e-4);
    TS_ASSERT_DELTA(res->cell<double>(5, 2), 0.4771, 1e-4);
    TS_ASSERT_DELTA(res->cell<double>(6, 2), 1.2008, 1e-4);
    TS_ASSERT_DELTA(res->cell<double>(7, 2), 3.8074, 1e-4);

    AnalysisDataService::Instance().clear();
  }

  void test_on_Lorentzians_ties() {
    std::string funStr =
        "name=Lorentzian,Amplitude=10,FWHM=2,ties=(PeakCentre=-4);"
        "name=Lorentzian,Amplitude=10,PeakCentre=3,FWHM=3;"
        "name=FlatBackground,A0=3;ties=(f1.Amplitude=f0.Amplitude)";
    auto fun = FunctionFactory::Instance().createInitialized(funStr);
    auto ws = createWorkspace(*fun);

    auto fit = AlgorithmManager::Instance().create("Fit");
    fit->setProperty("Function", fun);
    fit->setProperty("InputWorkspace", ws);
    fit->setProperty("CalcErrors", true);
    fit->execute();

    EstimatePeakErrors alg;
    alg.initialize();
    alg.setProperty("Function", fun);
    alg.setPropertyValue("OutputWorkspace", "Errors");
    alg.execute();

    auto res =
        AnalysisDataService::Instance().retrieveWS<ITableWorkspace>("Errors");
    TS_ASSERT_EQUALS(res->rowCount(), 8);

    TS_ASSERT_EQUALS(res->cell<std::string>(0, 0), "f0.Centre");
    TS_ASSERT_EQUALS(res->cell<std::string>(1, 0), "f0.Height");
    TS_ASSERT_EQUALS(res->cell<std::string>(2, 0), "f0.FWHM");
    TS_ASSERT_EQUALS(res->cell<std::string>(3, 0), "f0.Intensity");

    TS_ASSERT_DELTA(res->cell<double>(0, 1), -4.0000, 1e-4);
    TS_ASSERT_DELTA(res->cell<double>(1, 1), 3.1878, 1e-4);
    TS_ASSERT_DELTA(res->cell<double>(2, 1), 2.0006, 1e-4);
    TS_ASSERT_DELTA(res->cell<double>(3, 1), 10.0181, 1e-4);

    TS_ASSERT_DELTA(res->cell<double>(0, 2), 0.0000, 1e-4);
    TS_ASSERT_DELTA(res->cell<double>(1, 2), 0.5605, 1e-4);
    TS_ASSERT_DELTA(res->cell<double>(2, 2), 0.5872, 1e-4);
    TS_ASSERT_DELTA(res->cell<double>(3, 2), 2.4510, 1e-4);

    TS_ASSERT_EQUALS(res->cell<std::string>(4, 0), "f1.Centre");
    TS_ASSERT_EQUALS(res->cell<std::string>(5, 0), "f1.Height");
    TS_ASSERT_EQUALS(res->cell<std::string>(6, 0), "f1.FWHM");
    TS_ASSERT_EQUALS(res->cell<std::string>(7, 0), "f1.Intensity");

    TS_ASSERT_DELTA(res->cell<double>(4, 1), 3.0056, 1e-4);
    TS_ASSERT_DELTA(res->cell<double>(5, 1), 2.1320, 1e-4);
    TS_ASSERT_DELTA(res->cell<double>(6, 1), 2.9915, 1e-4);
    TS_ASSERT_DELTA(res->cell<double>(7, 1), 10.0181, 1e-4);

    TS_ASSERT_DELTA(res->cell<double>(4, 2), 0.3229, 1e-4);
    TS_ASSERT_DELTA(res->cell<double>(5, 2), 0.4677, 1e-4);
    TS_ASSERT_DELTA(res->cell<double>(6, 2), 0.6563, 1e-4);
    TS_ASSERT_DELTA(res->cell<double>(7, 2), 0.0000, 1e-4);

    AnalysisDataService::Instance().clear();
  }

  void test_no_peaks() {
    std::string funStr = "name=FlatBackground,A0=3";
    auto fun = FunctionFactory::Instance().createInitialized(funStr);
    auto ws = createWorkspace(*fun);

    auto fit = AlgorithmManager::Instance().create("Fit");
    fit->setProperty("Function", fun);
    fit->setProperty("InputWorkspace", ws);
    fit->setProperty("CalcErrors", true);
    fit->execute();

    EstimatePeakErrors alg;
    alg.initialize();
    alg.setProperty("Function", fun);
    alg.setPropertyValue("OutputWorkspace", "Errors");
    alg.execute();

    auto res =
        AnalysisDataService::Instance().retrieveWS<ITableWorkspace>("Errors");
    TS_ASSERT_EQUALS(res->rowCount(), 0);

    AnalysisDataService::Instance().clear();
  }

private:
  MatrixWorkspace_sptr createWorkspace(const IFunction &fun) const {
    const double noiseLevel = 0.1;
    const std::vector<double> noise(
        {0.429616,  0.390155,  -0.183624,   -0.369293,   -0.316081,
         -0.460241, -0.29544,  0.326436,    0.067725,    0.0320779,
         0.0955447, 0.45631,   0.464515,    -0.0380737,  0.153177,
         0.423738,  0.248907,  -0.126079,   0.15357,     -0.345027,
         0.247715,  0.392344,  0.461307,    -0.47321,    -0.491612,
         -0.208498, -0.393556, -0.101256,   -0.201296,   0.307289,
         0.156411,  0.127094,  0.309813,    0.407925,    0.372176,
         0.0563973, 0.464648,  0.339919,    0.223685,    -0.449512,
         0.142475,  0.306235,  0.217454,    0.430816,    -0.032401,
         -0.13597,  -0.174415, 0.190948,    -0.0603554,  -0.370685,
         0.229689,  0.332686,  0.494015,    -0.181646,   0.176874,
         0.237202,  0.290823,  0.0967696,   -0.329086,   -0.136293,
         -0.473151, 0.294971,  0.30037,     0.198481,    0.403723,
         0.321879,  -0.475324, 0.399466,    -0.00825268, 0.331899,
         0.0262552, 0.409958,  0.096366,    0.474914,    -0.448042,
         0.15612,   0.39509,   0.31199,     0.228266,    -0.397256,
         0.31835,   -0.262472, 0.000222752, -0.120655,   0.310189,
         0.199236,  -0.404031, -0.0150548,  -0.28105,    -0.0847778,
         -0.241281, 0.138662,  -0.0318942,  0.25929,     -0.0406268,
         0.0575229, 0.20951,   0.193455,    -0.321947,   0.413259});
    size_t n = 100;
    auto ws = WorkspaceFactory::Instance().create("Workspace2D", 1, n, n);
    FunctionDomain1DVector x(-10, 10, n);
    FunctionValues y(x);
    std::vector<double> e(n, 1.0);

    fun.function(x, y);
    ws->setPoints(0, x.toVector());
    ws->dataY(0) = y.toVector();
    ws->dataE(0) = e;
    assert(n == noise.size());
    for (size_t i = 0; i < n; ++i) {
      ws->dataY(0)[i] += noiseLevel * noise[i];
      ws->dataE(0)[i] += noiseLevel;
    }
    return ws;
  }
};

#endif /*CHEBYSHEVTEST_H_*/
