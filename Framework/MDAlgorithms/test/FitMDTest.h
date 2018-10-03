// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef CURVEFITTING_FITMDTEST_H_
#define CURVEFITTING_FITMDTEST_H_

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/IAlgorithm.h"
#include "MantidAPI/IMDHistoWorkspace.h"
#include "MantidAPI/IMDIterator.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidMDAlgorithms/UserFunctionMD.h"
#include "MantidTestHelpers/FakeObjects.h"

#include <cxxtest/TestSuite.h>

#include <sstream>

using namespace Mantid;
using namespace Mantid::API;

class IMDWorkspaceTester;

class IMDWorkspaceTesterIterator : public IMDIterator {
  const IMDWorkspaceTester *m_ws;
  size_t ix, iy;

public:
  IMDWorkspaceTesterIterator(const IMDWorkspaceTester *ws)
      : m_ws(ws), ix(0), iy(0) {}
  size_t getDataSize() const override;
  bool valid() const override { return true; }
  void jumpTo(size_t index) override;
  bool next() override;
  bool next(size_t) override { return false; }
  signal_t getNormalizedSignal() const override;
  signal_t getNormalizedError() const override;
  signal_t getSignal() const override { return 0; }
  signal_t getError() const override { return 0; }
  std::unique_ptr<coord_t[]> getVertexesArray(size_t &) const override {
    return nullptr;
  }
  std::unique_ptr<coord_t[]> getVertexesArray(size_t &, const size_t,
                                              const bool *) const override {
    return nullptr;
  }
  Mantid::Kernel::VMD getCenter() const override;
  size_t getNumEvents() const override { return 0; }
  uint16_t getInnerRunIndex(size_t) const override { return 0; }
  int32_t getInnerDetectorID(size_t) const override { return 0; }
  coord_t getInnerPosition(size_t, size_t) const override { return 0; }
  signal_t getInnerSignal(size_t) const override { return 0; }
  signal_t getInnerError(size_t) const override { return 0; }
  bool getIsMasked() const override { return false; }
  std::vector<size_t> findNeighbourIndexes() const override {
    throw std::runtime_error(
        "findNeighbourIndexes not implemented on IMDWorkspaceTesterIterator");
  }
  std::vector<size_t> findNeighbourIndexesFaceTouching() const override {
    throw std::runtime_error(
        "findNeighbourIndexes not implemented on IMDWorkspaceTesterIterator");
  }
  size_t getLinearIndex() const override {
    throw std::runtime_error(
        "getLinearIndex not implemented on IMDWorkspaceTesterIterator");
  }
  bool isWithinBounds(size_t) const override {
    throw std::runtime_error(
        "isWithinBounds not implemented on IMDWorkspaceTestIterator");
  }
};

class IMDWorkspaceTester : public WorkspaceTester {
public:
  std::vector<std::unique_ptr<IMDIterator>>
  createIterators(size_t,
                  Mantid::Geometry::MDImplicitFunction *) const override {

    std::vector<std::unique_ptr<IMDIterator>> ret;
    auto ptr = std::unique_ptr<IMDIterator>{
        Kernel::make_unique<IMDWorkspaceTesterIterator>(this)};
    ret.push_back(std::move(ptr));
    return ret;
  }
};

size_t IMDWorkspaceTesterIterator::getDataSize() const {
  return m_ws->getNumberHistograms() * m_ws->blocksize();
}

bool IMDWorkspaceTesterIterator::next() {
  if (ix == m_ws->blocksize() - 1) {
    ix = 0;
    ++iy;
    if (iy == m_ws->getNumberHistograms()) {
      --iy;
      return false;
    }
  } else {
    ++ix;
  }
  return true;
}

signal_t IMDWorkspaceTesterIterator::getNormalizedSignal() const {
  return signal_t(m_ws->readY(iy)[ix]);
}

signal_t IMDWorkspaceTesterIterator::getNormalizedError() const {
  return signal_t(m_ws->readE(iy)[ix]);
}

void IMDWorkspaceTesterIterator::jumpTo(size_t index) {
  iy = index % m_ws->blocksize();
  ix = index - iy * m_ws->blocksize();
}

Mantid::Kernel::VMD IMDWorkspaceTesterIterator::getCenter() const {
  double y = double(iy);
  double x;
  if (m_ws->isHistogramData()) {
    x = (m_ws->readX(iy)[ix] + m_ws->readX(iy)[ix + 1]) / 2;
  } else {
    x = m_ws->readX(iy)[ix];
  }
  return Mantid::Kernel::VMD(x, y);
}

class FitMDTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static FitMDTest *createSuite() { return new FitMDTest(); }
  static void destroySuite(FitMDTest *suite) { delete suite; }

  FitMDTest() {
    // need to have DataObjects loaded
    FrameworkManager::Instance();
  }

  void test_exec_point_data() {
    MatrixWorkspace_sptr ws2(new IMDWorkspaceTester);
    ws2->initialize(10, 10, 10);

    for (size_t is = 0; is < ws2->getNumberHistograms(); ++is) {
      Mantid::MantidVec &x = ws2->dataX(is);
      Mantid::MantidVec &y = ws2->dataY(is);
      // Mantid::MantidVec& e = ws2->dataE(is);
      for (size_t i = 0; i < y.size(); ++i) {
        x[i] = 0.1 * static_cast<double>(i);
        const double is_d = static_cast<double>(is);
        y[i] = 10.0 + is_d + (0.5 + 0.1 * is_d) * x[i];
      }
    }

    API::IFunction_sptr fun(new Mantid::MDAlgorithms::UserFunctionMD);
    fun->setAttributeValue("Formula", "h + y + (s + 0.1*y) * x");
    fun->setParameter("h", 1.0);
    fun->setParameter("s", 1.0);

    API::IAlgorithm_sptr fit = API::AlgorithmManager::Instance().create("Fit");
    fit->initialize();

    fit->setProperty("Function", fun);
    fit->setProperty("InputWorkspace", ws2);
    fit->setProperty("CreateOutput", true);
    fit->setPropertyValue("Minimizer", "Levenberg-MarquardtMD");

    fit->execute();

    TS_ASSERT(fit->isExecuted());

    TS_ASSERT_DELTA(fun->getParameter("h"), 10.0, 1e-3);
    TS_ASSERT_DELTA(fun->getParameter("s"), 0.5, 1e-4);

    double chi2 = fit->getProperty("OutputChi2overDoF");
    TS_ASSERT_DELTA(chi2, 0.0, 1e-8);
    // TS_ASSERT_DIFFERS(chi2, 0.0);
    TS_ASSERT_EQUALS(fit->getPropertyValue("OutputStatus"), "success");

    // ITableWorkspace_sptr covar =
    // boost::dynamic_pointer_cast<ITableWorkspace>(
    //  API::AnalysisDataService::Instance().retrieve("Output_NormalisedCovarianceMatrix"));

    // TS_ASSERT(covar);
    // TS_ASSERT_EQUALS(covar->columnCount(), 3);
    // TS_ASSERT_EQUALS(covar->rowCount(), 2);
    // TS_ASSERT_EQUALS(covar->String(0,0), "Height");
    // TS_ASSERT_EQUALS(covar->String(1,0), "Lifetime");
    // TS_ASSERT_EQUALS(covar->getColumn(0)->type(), "str");
    // TS_ASSERT_EQUALS(covar->getColumn(0)->name(), "Name");
    // TS_ASSERT_EQUALS(covar->getColumn(1)->type(), "double");
    // TS_ASSERT_EQUALS(covar->getColumn(1)->name(), "Height");
    // TS_ASSERT_EQUALS(covar->getColumn(2)->type(), "double");
    // TS_ASSERT_EQUALS(covar->getColumn(2)->name(), "Lifetime");
    // TS_ASSERT_EQUALS(covar->Double(0,1), 100.0);
    // TS_ASSERT_EQUALS(covar->Double(1,2), 100.0);
    // TS_ASSERT(fabs(covar->Double(0,2)) < 100.0);
    // TS_ASSERT(fabs(covar->Double(0,2)) > 0.0);
    // TS_ASSERT_EQUALS(covar->Double(0,2), covar->Double(1,1));

    TS_ASSERT_DIFFERS(fun->getError(0), 0.0);
    TS_ASSERT_DIFFERS(fun->getError(1), 0.0);

    ITableWorkspace_sptr params = boost::dynamic_pointer_cast<ITableWorkspace>(
        API::AnalysisDataService::Instance().retrieve("Output_Parameters"));

    TS_ASSERT(params);
    TS_ASSERT_EQUALS(params->columnCount(), 3);
    TS_ASSERT_EQUALS(params->rowCount(), 3);
    TS_ASSERT_EQUALS(params->String(0, 0), "h");
    TS_ASSERT_EQUALS(params->String(1, 0), "s");
    TS_ASSERT_EQUALS(params->String(2, 0), "Cost function value");
    TS_ASSERT_EQUALS(params->Double(0, 1), fun->getParameter(0));
    TS_ASSERT_EQUALS(params->Double(1, 1), fun->getParameter(1));
    TS_ASSERT_EQUALS(params->Double(2, 1), chi2);
    TS_ASSERT_EQUALS(params->Double(0, 2), fun->getError(0));
    TS_ASSERT_EQUALS(params->Double(1, 2), fun->getError(1));
    TS_ASSERT_EQUALS(params->Double(2, 2), 0.0);

    API::AnalysisDataService::Instance().clear();
  }

  void test_output_histo_workspace() {
    auto inputWS = createHistoWorkspace(
        3, 4, "name=UserFunctionMD,Formula=10 + y + (2 + 0.1*y) * x");

    auto fit = API::AlgorithmManager::Instance().create("Fit");
    fit->initialize();

    fit->setProperty(
        "Function",
        "name=UserFunctionMD,Formula=h + y + (s + 0.1*y) * x, h = 0, s = 0");
    fit->setProperty("InputWorkspace", inputWS);
    fit->setPropertyValue("Output", "out");
    fit->execute();

    IMDHistoWorkspace_sptr outputWS =
        AnalysisDataService::Instance().retrieveWS<IMDHistoWorkspace>(
            "out_Workspace");
    TS_ASSERT(outputWS);
    if (!outputWS)
      return;

    TS_ASSERT_EQUALS(inputWS->getNPoints(), outputWS->getNPoints());

    uint64_t n = outputWS->getNPoints();
    coord_t invVolume = inputWS->getInverseVolume();
    for (uint64_t i = 0; i < n; ++i) {
      TS_ASSERT_DELTA(outputWS->signalAt(i) / inputWS->signalAt(i) / invVolume,
                      1.0, 0.1);
    }

    AnalysisDataService::Instance().clear();
  }

  // ---------------------------------------------------------- //
  IMDHistoWorkspace_sptr createHistoWorkspace(size_t nx, size_t ny,
                                              const std::string &function) {

    std::vector<double> values(nx * ny, 1.0);
    std::vector<int> dims(2);
    dims[0] = static_cast<int>(nx);
    dims[1] = static_cast<int>(ny);

    auto alg = AlgorithmManager::Instance().create("CreateMDHistoWorkspace");
    alg->initialize();
    alg->setProperty("SignalInput", values);
    alg->setProperty("ErrorInput", values);
    alg->setProperty("Dimensionality", 2);
    alg->setProperty("NumberOfBins", dims);
    alg->setPropertyValue("Extents", "-1,1,-1,1");
    alg->setPropertyValue("Names", "A,B");
    alg->setPropertyValue("Units", "U,U");
    alg->setPropertyValue("OutputWorkspace", "out");
    alg->execute();
    TS_ASSERT(alg->isExecuted());

    IMDHistoWorkspace_sptr ws =
        AnalysisDataService::Instance().retrieveWS<IMDHistoWorkspace>("out");

    alg = AlgorithmManager::Instance().create("EvaluateMDFunction");
    alg->initialize();
    alg->setProperty("InputWorkspace", ws);
    alg->setPropertyValue("Function", function);
    alg->setPropertyValue("OutputWorkspace", "out");
    alg->execute();

    ws = AnalysisDataService::Instance().retrieveWS<IMDHistoWorkspace>("out");

    AnalysisDataService::Instance().remove("out");
    return ws;
  }
};

#endif /*CURVEFITTING_FITMFTEST_H_*/
