// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef TABULATEDFUNCTIONTEST_H_
#define TABULATEDFUNCTIONTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/AlgorithmFactory.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FileFinder.h"
#include "MantidAPI/FunctionDomain.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidCurveFitting/Functions/TabulatedFunction.h"
#include "MantidCurveFitting/Functions/UserFunction.h"
#include "MantidCurveFitting/Jacobian.h"

#include "MantidTestHelpers/WorkspaceCreationHelper.h"

#include <Poco/File.h>

#include <fstream>

using namespace Mantid::CurveFitting;
using namespace Mantid::CurveFitting::Functions;
using namespace Mantid::API;

namespace {
struct Fun {
  double operator()(double x, int i) { return exp(-x * x) + double(i); }
};
} // namespace

class TabulatedFunctionTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static TabulatedFunctionTest *createSuite() {
    return new TabulatedFunctionTest();
  }
  static void destroySuite(TabulatedFunctionTest *suite) { delete suite; }

  TabulatedFunctionTest()
      : m_asciiFileName("TabulatedFunctionTest_testAsciiFile.txt"),
        m_nexusFileName(Mantid::API::FileFinder::Instance().getFullPath(
            "argus0026287.nxs")) {
    FunctionDomain1DVector x(-5.0, 5.0, 100);
    FunctionValues y(x);
    UserFunction fun;
    fun.setAttributeValue("Formula", "exp(-x*x)");
    fun.function(x, y);

    std::ofstream fil(m_asciiFileName.c_str());
    for (size_t i = 0; i < x.size(); ++i) {
      fil << x[i] << ' ' << y[i] << '\n';
    }
  }

  ~TabulatedFunctionTest() override {
    Poco::File hAscii(m_asciiFileName);
    if (hAscii.exists()) {
      hAscii.remove();
    }
  }

  void test_loadAscii() {
    TS_ASSERT(true);
    TabulatedFunction fun;
    fun.setAttributeValue("FileName", m_asciiFileName);
    TS_ASSERT_EQUALS(fun.getParameter("Scaling"), 1.0);
    FunctionDomain1DVector x(-5.0, 5.0, 83);
    FunctionValues y(x);
    fun.function(x, y);
    for (size_t i = 0; i < x.size(); ++i) {
      const double xx = x[i];
      const double tol = fabs(xx) > 4.0 ? 0.2 : 0.06;
      TS_ASSERT_DELTA(fabs(y[i] - exp(-xx * xx)) / y[i], 0, tol);
    }
    TS_ASSERT_EQUALS(fun.getAttribute("FileName").asUnquotedString(),
                     m_asciiFileName);
    TS_ASSERT_EQUALS(fun.getAttribute("Workspace").asString(), "");
    TS_ASSERT_EQUALS(fun.getAttribute("WorkspaceIndex").asInt(), 0);
  }

  void test_loadNexus() {
    TS_ASSERT(true);
    TabulatedFunction fun;
    fun.setAttributeValue("FileName", m_nexusFileName);
    TS_ASSERT_EQUALS(fun.getParameter("Scaling"), 1.0);
    TS_ASSERT_EQUALS(fun.getAttribute("FileName").asUnquotedString(),
                     m_nexusFileName);
    TS_ASSERT_EQUALS(fun.getAttribute("Workspace").asString(), "");
    TS_ASSERT_EQUALS(fun.getAttribute("WorkspaceIndex").asInt(), 0);

    FunctionDomain1DVector x(1.0, 30.0, 100);
    FunctionValues y(x);
    fun.function(x, y);

    TS_ASSERT_DELTA(y[5], 304.8886, 1e-4);
    TS_ASSERT_DELTA(y[10], 136.7575, 1e-4);
    TS_ASSERT_DELTA(y[20], 32.4847, 1e-4);
    TS_ASSERT_DELTA(y[25], 16.8940, 1e-4);
    TS_ASSERT_DELTA(y[30], 9.2728, 1e-4);
  }

  void test_loadNexus_nondefault_index() {
    TS_ASSERT(true);
    TabulatedFunction fun;
    fun.setAttributeValue("FileName", m_nexusFileName);
    fun.setAttributeValue("WorkspaceIndex", 10);
    TS_ASSERT_EQUALS(fun.getParameter("Scaling"), 1.0);
    TS_ASSERT_EQUALS(fun.getAttribute("FileName").asUnquotedString(),
                     m_nexusFileName);
    TS_ASSERT_EQUALS(fun.getAttribute("Workspace").asString(), "");
    TS_ASSERT_EQUALS(fun.getAttribute("WorkspaceIndex").asInt(), 10);

    FunctionDomain1DVector x(1.0, 30.0, 100);
    FunctionValues y(x);
    fun.function(x, y);

    TS_ASSERT_DELTA(y[5], 367.2980, 1e-4);
    TS_ASSERT_DELTA(y[10], 179.5151, 1e-4);
    TS_ASSERT_DELTA(y[20], 50.4847, 1e-4);
    TS_ASSERT_DELTA(y[25], 21.2980, 1e-4);
    TS_ASSERT_DELTA(y[30], 17.4847, 1e-4);
  }

  void test_loadWorkspace() {
    auto ws = WorkspaceCreationHelper::create2DWorkspaceFromFunction(
        Fun(), 1, -5.0, 5.0, 0.1, false);
    AnalysisDataService::Instance().add("TABULATEDFUNCTIONTEST_WS", ws);
    TabulatedFunction fun;
    fun.setAttributeValue("Workspace", "TABULATEDFUNCTIONTEST_WS");
    TS_ASSERT_EQUALS(fun.getParameter("Scaling"), 1.0);
    FunctionDomain1DVector x(-5.0, 5.0, 83);
    FunctionValues y(x);
    fun.function(x, y);
    for (size_t i = 0; i < x.size(); ++i) {
      const double xx = x[i];
      const double tol = fabs(xx) > 4.0 ? 0.2 : 0.07;
      TS_ASSERT_DELTA(fabs(y[i] - exp(-xx * xx)) / y[i], 0, tol);
    }
    TS_ASSERT_EQUALS(fun.getAttribute("Workspace").asString(),
                     "TABULATEDFUNCTIONTEST_WS");
    TS_ASSERT_EQUALS(fun.getAttribute("FileName").asUnquotedString(), "");
    auto X = fun.getAttribute("X").asVector();
    TS_ASSERT_EQUALS(X.size(), 0);
    auto Y = fun.getAttribute("Y").asVector();
    TS_ASSERT_EQUALS(Y.size(), 0);
    AnalysisDataService::Instance().clear();
  }

  void test_loadWorkspace_nondefault_index() {
    auto ws = WorkspaceCreationHelper::create2DWorkspaceFromFunction(
        Fun(), 3, -5.0, 5.0, 0.1, false);
    AnalysisDataService::Instance().add("TABULATEDFUNCTIONTEST_WS", ws);
    TabulatedFunction fun;
    fun.setAttributeValue("Workspace", "TABULATEDFUNCTIONTEST_WS");
    fun.setAttributeValue("WorkspaceIndex", 2);
    TS_ASSERT_EQUALS(fun.getParameter("Scaling"), 1.0);
    TS_ASSERT_EQUALS(fun.getParameter("Shift"), 0.0);
    FunctionDomain1DVector x(-5.0, 5.0, 83);
    FunctionValues y(x);
    fun.function(x, y);
    for (size_t i = 0; i < x.size(); ++i) {
      const double xx = x[i];
      const double tol = fabs(xx) > 4.0 ? 0.2 : 0.07;
      TS_ASSERT_DELTA(fabs(y[i] - exp(-xx * xx) - 2.0) / y[i], 0, tol);
    }
    TS_ASSERT_EQUALS(fun.getAttribute("Workspace").asString(),
                     "TABULATEDFUNCTIONTEST_WS");
    TS_ASSERT_EQUALS(fun.getAttribute("WorkspaceIndex").asInt(), 2);
    TS_ASSERT_EQUALS(fun.getAttribute("FileName").asUnquotedString(), "");
    AnalysisDataService::Instance().clear();
  }

  void test_loadWorkspace_nondefault_wrong_index() {
    auto ws = WorkspaceCreationHelper::create2DWorkspaceFromFunction(
        Fun(), 3, -5.0, 5.0, 0.1, false);
    AnalysisDataService::Instance().add("TABULATEDFUNCTIONTEST_WS", ws);
    TabulatedFunction fun;
    fun.setAttributeValue("Workspace", "TABULATEDFUNCTIONTEST_WS");
    fun.setAttributeValue("WorkspaceIndex", 20);
    FunctionDomain1DVector x(-5.0, 5.0, 83);
    FunctionValues y(x);
    TS_ASSERT_THROWS(fun.function(x, y), const std::range_error &);
    AnalysisDataService::Instance().clear();
  }

  void test_loadWorkspaceWhichDoesNotExist() {
    TabulatedFunction fun;
    TS_ASSERT_THROWS(fun.setAttributeValue("Workspace", "SomeWorkspace"),
                     const Mantid::Kernel::Exception::NotFoundError &);
  }

  void test_Derivatives() {
    auto ws = WorkspaceCreationHelper::create2DWorkspaceFromFunction(
        Fun(), 1, -5.0, 5.0, 0.1, false);
    AnalysisDataService::Instance().add("TABULATEDFUNCTIONTEST_WS", ws);
    TabulatedFunction fun;
    fun.setAttributeValue("Workspace", "TABULATEDFUNCTIONTEST_WS");
    fun.setParameter("Scaling", 3.3);
    TS_ASSERT_EQUALS(fun.getParameter("Scaling"), 3.3);
    fun.setParameter("Shift", 0.0);
    TS_ASSERT_EQUALS(fun.getParameter("Shift"), 0.0);
    fun.setParameter("XScaling", 1.0);
    TS_ASSERT_EQUALS(fun.getParameter("XScaling"), 1.0);
    FunctionDomain1DVector x(-5.0, 5.0, 83);

    FunctionValues y(x);
    fun.function(x, y);

    Mantid::CurveFitting::Jacobian jac(x.size(), 3);
    fun.functionDeriv(x, jac);

    for (size_t i = 0; i < x.size(); ++i) {
      const double xx = x[i];
      const double tol = fabs(xx) > 4.0 ? 0.2 : 0.07;
      TS_ASSERT_DELTA(fabs(y[i] - 3.3 * exp(-xx * xx)) / y[i], 0, tol);
      TS_ASSERT_DELTA(fabs(jac.get(i, 0) - exp(-xx * xx)) / y[i], 0, tol);
      // std::cerr << xx << ' ' << y[i] << '\n';
    }
    AnalysisDataService::Instance().clear();
  }

  void test_Attributes() {
    TabulatedFunction fun;
    auto names = fun.getAttributeNames();
    TS_ASSERT_EQUALS(names.size(), 5);
    TS_ASSERT_EQUALS(names[0], "FileName");
    TS_ASSERT_EQUALS(names[1], "Workspace");
    TS_ASSERT_EQUALS(names[2], "WorkspaceIndex");
    TS_ASSERT_EQUALS(names[3], "X");
    TS_ASSERT_EQUALS(names[4], "Y");
    TS_ASSERT(fun.hasAttribute("FileName"));
    TS_ASSERT(fun.hasAttribute("Workspace"));
    TS_ASSERT(fun.hasAttribute("WorkspaceIndex"));
    TS_ASSERT(fun.hasAttribute("X"));
    TS_ASSERT(fun.hasAttribute("Y"));
  }

  void test_factory_create_from_file() {
    std::string inif = "name=TabulatedFunction,FileName=\"" + m_nexusFileName +
                       "\",WorkspaceIndex=17,Scaling=2,Shift=0.02,XScaling=0.2";
    auto funf =
        Mantid::API::FunctionFactory::Instance().createInitialized(inif);
    TS_ASSERT(funf);
    TS_ASSERT_EQUALS(funf->getAttribute("Workspace").asString(), "");
    TS_ASSERT_EQUALS(funf->getAttribute("WorkspaceIndex").asInt(), 17);
    TS_ASSERT_EQUALS(funf->getAttribute("FileName").asUnquotedString(),
                     m_nexusFileName);
    TS_ASSERT_EQUALS(funf->getParameter("Scaling"), 2.0);
    TS_ASSERT_EQUALS(funf->getParameter("Shift"), 0.02);
    TS_ASSERT_EQUALS(funf->getParameter("XScaling"), 0.2);
  }

  void test_factory_create_from_workspace() {
    auto ws = WorkspaceCreationHelper::create2DWorkspaceFromFunction(
        Fun(), 1, -5.0, 5.0, 0.1, false);
    AnalysisDataService::Instance().add("TABULATEDFUNCTIONTEST_WS", ws);
    std::string inif = "name=TabulatedFunction,Workspace=TABULATEDFUNCTIONTEST_"
                       "WS,WorkspaceIndex=71,Scaling=3.14,Shift=0.02,XScaling="
                       "0.2";
    auto funf =
        Mantid::API::FunctionFactory::Instance().createInitialized(inif);
    TS_ASSERT(funf);
    TS_ASSERT_EQUALS(funf->getAttribute("Workspace").asString(),
                     "TABULATEDFUNCTIONTEST_WS");
    TS_ASSERT_EQUALS(funf->getAttribute("WorkspaceIndex").asInt(), 71);
    TS_ASSERT_EQUALS(funf->getAttribute("FileName").asUnquotedString(), "");
    TS_ASSERT_EQUALS(funf->getParameter("Scaling"), 3.14);
    TS_ASSERT_EQUALS(funf->getParameter("Shift"), 0.02);
    TS_ASSERT_EQUALS(funf->getParameter("XScaling"), 0.2);
    AnalysisDataService::Instance().clear();
  }

  void test_set_X_Y_attributes() {
    TabulatedFunction fun;
    const size_t n = 10;
    std::vector<double> X(n);
    std::vector<double> Y(n);
    for (size_t i = 0; i < n; ++i) {
      X[i] = double(i);
      Y[i] = X[i] * X[i];
    }
    fun.setAttributeValue("X", X);
    fun.setAttributeValue("Y", Y);

    FunctionDomain1DVector x(0.0, 9.0, 33);
    FunctionValues y(x);
    fun.function(x, y);

    for (size_t i = 0; i < x.size(); ++i) {
      double xx = x[i];
      TS_ASSERT_DELTA(y[i], xx * xx, 0.5);
    }
  }

  void test_set_X_Y_attributes_different_sizes() {
    TabulatedFunction fun;
    const size_t n = 10;
    std::vector<double> X(n);
    std::vector<double> Y(n - 1);

    fun.setAttributeValue("X", X);
    fun.setAttributeValue("Y", Y);

    auto x = fun.getAttribute("X").asVector();
    auto y = fun.getAttribute("Y").asVector();

    TS_ASSERT_EQUALS(x.size(), y.size());
    TS_ASSERT_EQUALS(x.size(), Y.size());
  }

  void test_set_X_Y_attributes_string() {
    std::string inif = "name=TabulatedFunction,X=(1,2,3),Y=(4,5,6)";
    auto fun = FunctionFactory::Instance().createInitialized(inif);
    TS_ASSERT(fun);
    auto x = fun->getAttribute("X").asVector();
    TS_ASSERT_EQUALS(x.size(), 3);
    TS_ASSERT_EQUALS(x[0], 1);
    TS_ASSERT_EQUALS(x[1], 2);
    TS_ASSERT_EQUALS(x[2], 3);
    auto y = fun->getAttribute("Y").asVector();
    TS_ASSERT_EQUALS(y.size(), 3);
    TS_ASSERT_EQUALS(y[0], 4);
    TS_ASSERT_EQUALS(y[1], 5);
    TS_ASSERT_EQUALS(y[2], 6);
  }

  void test_set_X_Y_attributes_string_empty() {
    std::string inif = "name=TabulatedFunction,X=(),Y=()";
    auto fun = FunctionFactory::Instance().createInitialized(inif);
    TS_ASSERT(fun);
  }

private:
  const std::string m_asciiFileName;
  const std::string m_nexusFileName;
};

#endif /*TABULATEDFUNCTIONTEST_H_*/
