// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidAPI/IFunction1DSpectrum.h"
#include "MantidAPI/ParamFunction.h"
#include "MantidAPI/WorkspaceOpOverloads.h"
#include "MantidCurveFitting/SeqDomain.h"
#include "MantidCurveFitting/SeqDomainSpectrumCreator.h"
#include "MantidFrameworkTestHelpers/WorkspaceCreationHelper.h"
#include "MantidKernel/IPropertyManager.h"

#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/TableRow.h"
#include "MantidCurveFitting/Algorithms/Fit.h"

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/SpectrumInfo.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::CurveFitting;
using namespace Mantid::CurveFitting::Algorithms;

class SeqDomainSpectrumCreatorTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static SeqDomainSpectrumCreatorTest *createSuite() { return new SeqDomainSpectrumCreatorTest(); }
  static void destroySuite(SeqDomainSpectrumCreatorTest *suite) { delete suite; }

  SeqDomainSpectrumCreatorTest() { FrameworkManager::Instance(); }

  void testConstructor() {
    TS_ASSERT_THROWS_NOTHING(SeqDomainSpectrumCreator creator(nullptr, ""));

    TestableSeqDomainSpectrumCreator otherCreator(nullptr, "Test");

    TS_ASSERT_EQUALS(otherCreator.m_workspacePropertyName, otherCreator.m_workspacePropertyNames.front());
    TS_ASSERT_EQUALS(otherCreator.m_workspacePropertyName, "Test");
  }

  void testSetMatrixWorkspace() {
    TestableSeqDomainSpectrumCreator creator(nullptr, "");
    TS_ASSERT_THROWS_NOTHING(creator.setMatrixWorkspace(WorkspaceCreationHelper::create2DWorkspace(5, 5)));

    TS_ASSERT_EQUALS(creator.m_matrixWorkspace->getNumberHistograms(), 5);

    TS_ASSERT_THROWS(creator.setMatrixWorkspace(MatrixWorkspace_sptr()), const std::invalid_argument &);
  }

  void testGetDomainSize() {
    TestableSeqDomainSpectrumCreator creator(nullptr, "");
    creator.setMatrixWorkspace(WorkspaceCreationHelper::create2DWorkspace123(4, 12));

    FunctionDomain_sptr domain;
    FunctionValues_sptr values;

    creator.createDomain(domain, values);

    std::shared_ptr<SeqDomain> seqDomain = std::dynamic_pointer_cast<SeqDomain>(domain);

    TS_ASSERT(seqDomain);
    TS_ASSERT_EQUALS(seqDomain->getNDomains(), 4);
    TS_ASSERT_EQUALS(seqDomain->size(), 4 * 12);
  }

  void testHistogramIsUsable() {
    TestableSeqDomainSpectrumCreator creator(nullptr, "");

    TS_ASSERT_THROWS(creator.histogramIsUsable(0), const std::invalid_argument &);

    // Workspace with 2 histograms, one of which is masked (No. 0)
    std::set<int64_t> masked;
    masked.insert(0);
    creator.setMatrixWorkspace(WorkspaceCreationHelper::create2DWorkspace123(2, 12, false, masked));

    TS_ASSERT(!creator.histogramIsUsable(0));
    TS_ASSERT(creator.histogramIsUsable(1));

    // No instrument
    creator.setMatrixWorkspace(WorkspaceCreationHelper::create2DWorkspace123(2, 12));
    TS_ASSERT(creator.histogramIsUsable(0));
    TS_ASSERT(creator.histogramIsUsable(1));
  }

  void testCreateDomain() {
    TestableSeqDomainSpectrumCreator creator(nullptr, "");
    creator.setMatrixWorkspace(WorkspaceCreationHelper::create2DWorkspace123(4, 12, true));

    FunctionDomain_sptr domain;
    FunctionValues_sptr values;

    creator.createDomain(domain, values);

    std::shared_ptr<SeqDomain> seqDomain = std::dynamic_pointer_cast<SeqDomain>(domain);

    for (size_t i = 0; i < seqDomain->getNDomains(); ++i) {
      FunctionDomain_sptr localDomain;
      FunctionValues_sptr localValues;

      seqDomain->getDomainAndValues(i, localDomain, localValues);

      std::shared_ptr<FunctionDomain1DSpectrum> localSpectrumDomain =
          std::dynamic_pointer_cast<FunctionDomain1DSpectrum>(localDomain);
      TS_ASSERT(localSpectrumDomain);

      TS_ASSERT_EQUALS(localSpectrumDomain->getWorkspaceIndex(), i);
      TS_ASSERT_EQUALS(localSpectrumDomain->size(), 12);
    }
  }

  void testCreateDomainMaskedDetectors() {
    TestableSeqDomainSpectrumCreator creator(nullptr, "");

    // Workspace with 4 histograms, one of which is masked (No. 2)
    std::set<int64_t> masked;
    masked.insert(2);
    creator.setMatrixWorkspace(WorkspaceCreationHelper::create2DWorkspace123(4, 12, true, masked));

    FunctionDomain_sptr domain;
    FunctionValues_sptr values;

    creator.createDomain(domain, values);

    std::shared_ptr<SeqDomain> seqDomain = std::dynamic_pointer_cast<SeqDomain>(domain);

    // One less than the created workspace
    TS_ASSERT_EQUALS(seqDomain->getNDomains(), 3);
    for (size_t i = 0; i < seqDomain->getNDomains(); ++i) {
      FunctionDomain_sptr localDomain;
      FunctionValues_sptr localValues;

      seqDomain->getDomainAndValues(i, localDomain, localValues);

      std::shared_ptr<FunctionDomain1DSpectrum> localSpectrumDomain =
          std::dynamic_pointer_cast<FunctionDomain1DSpectrum>(localDomain);
      TS_ASSERT(localSpectrumDomain);

      TS_ASSERT_EQUALS(localSpectrumDomain->size(), 12);

      // Make sure we never find 2 (masking)
      TS_ASSERT_DIFFERS(localSpectrumDomain->getWorkspaceIndex(), 2);
    }
  }

  void testCreateOutputWorkspace() {
    double slope = 2.0;
    // all x values are 1.0

    // TODO is the workspace created with the wrong values here
    MatrixWorkspace_sptr matrixWs = WorkspaceCreationHelper::create2DWorkspace123(4, 12);

    TestableSeqDomainSpectrumCreator creator(nullptr, "");
    creator.setMatrixWorkspace(matrixWs);

    FunctionDomain_sptr domain;
    FunctionValues_sptr values;

    creator.createDomain(domain, values);

    IFunction_sptr testFunction(new SeqDomainCreatorTestFunction);
    testFunction->initialize();
    testFunction->setParameter("Slope", slope);

    // TODO or are the output values from createOutputWorkspace wrong
    Workspace_sptr outputWs = creator.createOutputWorkspace("", testFunction, domain, values);

    MatrixWorkspace_sptr outputWsMatrix = std::dynamic_pointer_cast<MatrixWorkspace>(outputWs);
    TS_ASSERT(outputWsMatrix);

    TS_ASSERT_EQUALS(outputWsMatrix->getNumberHistograms(), matrixWs->getNumberHistograms());

    // Spectrum 0: 0 + 2 * 1 -> All y-values should be 2
    // Spectrum 1: 1 + 2 * 1 -> All y-values should be 3...etc.
    for (size_t i = 0; i < outputWsMatrix->getNumberHistograms(); ++i) {
      const auto &x = outputWsMatrix->x(i);
      const auto &y = outputWsMatrix->y(i);

      TS_ASSERT_EQUALS(x, matrixWs->x(i));
      for (size_t j = 0; j < x.size(); ++j) {
        TS_ASSERT_EQUALS(y[j], static_cast<double>(i) + slope * x[j]);
      }
    }
  }

  void testCreateOutputWorkspaceMasked() {
    double slope = 2.0;
    // all x values are 1.0
    // Mask one histogram (No. 2)
    std::set<int64_t> masked;
    masked.insert(2);
    MatrixWorkspace_sptr matrixWs = WorkspaceCreationHelper::create2DWorkspace123(4, 12, false, masked);

    TestableSeqDomainSpectrumCreator creator(nullptr, "");
    creator.setMatrixWorkspace(matrixWs);

    FunctionDomain_sptr domain;
    FunctionValues_sptr values;

    creator.createDomain(domain, values);

    IFunction_sptr testFunction(new SeqDomainCreatorTestFunction);
    testFunction->initialize();
    testFunction->setParameter("Slope", slope);

    Workspace_sptr outputWs = creator.createOutputWorkspace("", testFunction, domain, values);

    MatrixWorkspace_sptr outputWsMatrix = std::dynamic_pointer_cast<MatrixWorkspace>(outputWs);
    TS_ASSERT(outputWsMatrix);

    // Still has to be the same number of histograms.
    TS_ASSERT_EQUALS(outputWsMatrix->getNumberHistograms(), matrixWs->getNumberHistograms());

    // Spectrum 0: 0 + 2 * 1 -> All y-values should be 2
    // Spectrum 1: 1 + 2 * 1 -> All y-values should be 3...etc.
    for (size_t i = 0; i < outputWsMatrix->getNumberHistograms(); ++i) {
      const auto &x = outputWsMatrix->x(i);
      const auto &y = outputWsMatrix->y(i);

      TS_ASSERT_EQUALS(x, matrixWs->x(i));
      const auto &spectrumInfo = outputWsMatrix->spectrumInfo();
      for (size_t j = 0; j < x.size(); ++j) {
        // If detector is not masked, there should be values, otherwise 0.
        if (!spectrumInfo.isMasked(i)) {
          TS_ASSERT_EQUALS(y[j], static_cast<double>(i) + slope * x[j]);
        } else {
          TS_ASSERT_EQUALS(y[j], 0.0);
        }
      }
    }
  }

  void testCreateOutputWorkspaceWithDistributionAsInput() {
    // Arrange
    MatrixWorkspace_sptr matrixWs = WorkspaceCreationHelper::create2DWorkspace123(4, 12, true);
    Mantid::API::WorkspaceHelpers::makeDistribution(matrixWs);

    TestableSeqDomainSpectrumCreator creator(nullptr, "");
    creator.setMatrixWorkspace(matrixWs);

    FunctionDomain_sptr domain;
    FunctionValues_sptr values;
    creator.createDomain(domain, values);

    IFunction_sptr testFunction(new SeqDomainCreatorTestFunction);
    testFunction->initialize();
    testFunction->setParameter("Slope", 2.0);

    Workspace_sptr outputWs = creator.createOutputWorkspace("", testFunction, domain, values);

    MatrixWorkspace_sptr outputWsMatrix = std::dynamic_pointer_cast<MatrixWorkspace>(outputWs);
    TS_ASSERT(outputWsMatrix);
    TSM_ASSERT("Output should be a distribution", outputWsMatrix->isDistribution());
    Mantid::API::AnalysisDataService::Instance().clear();
  }

  void testFit() {
    double slope = 2.0;

    MatrixWorkspace_sptr matrixWs = WorkspaceCreationHelper::create2DWorkspace123(400, 500);
    for (size_t i = 0; i < matrixWs->getNumberHistograms(); ++i) {
      auto &x = matrixWs->mutableX(i);
      auto &y = matrixWs->mutableY(i);
      auto &e = matrixWs->mutableE(i);

      for (size_t j = 0; j < x.size(); ++j) {
        x[j] = static_cast<double>(j);
        y[j] = static_cast<double>(i) + slope * x[j];
        e[j] = 0.0001 * y[j];
      }
    }

    WorkspaceCreationHelper::addNoise(matrixWs, 0.0, -0.1, 0.1);

    IFunction_sptr fun(new SeqDomainCreatorTestFunction);
    fun->initialize();
    fun->setParameter("Slope", 0.0);

    auto fit = Mantid::API::AlgorithmManager::Instance().create("Fit");
    fit->initialize();

    fit->setProperty("Function", fun);
    fit->setProperty("InputWorkspace", matrixWs);
    fit->setProperty("CreateOutput", true);
    fit->setProperty("Minimizer", "Levenberg-MarquardtMD");
    fit->setProperty("IgnoreInvalidData", true);
    fit->execute();

    TS_ASSERT(fit->isExecuted());

    TS_ASSERT_DELTA(fun->getParameter(0), 2.0, 1e-6);
    TS_ASSERT_LESS_THAN(fun->getError(0), 1e-6);
  }

  void testFitComplex() {
    std::vector<double> slopes(40);
    for (size_t i = 0; i < slopes.size(); ++i) {
      slopes[i] = static_cast<double>(i);
    }

    MatrixWorkspace_sptr matrixWs = WorkspaceCreationHelper::create2DWorkspace123(400, 50);
    for (size_t i = 0; i < matrixWs->getNumberHistograms(); ++i) {
      auto &x = matrixWs->mutableX(i);
      auto &y = matrixWs->mutableY(i);
      auto &e = matrixWs->mutableE(i);

      for (size_t j = 0; j < x.size(); ++j) {
        x[j] = static_cast<double>(j);
        y[j] = static_cast<double>(i) + slopes[i % slopes.size()] * x[j];
        e[j] = 0.001 * std::max(1.0, sqrt(y[j]));
      }
    }

    // WorkspaceCreationHelper::addNoise(matrixWs, 0.0, -0.1, 0.1);

    IFunction_sptr fun(new SeqDomainCreatorTestFunctionComplex);
    fun->initialize();
    for (size_t i = 0; i < slopes.size(); ++i) {
      fun->setParameter(i, static_cast<double>(i) + 1.1);
    }

    auto fit = Mantid::API::AlgorithmManager::Instance().create("Fit");
    fit->initialize();

    fit->setProperty("Function", fun);
    fit->setProperty("InputWorkspace", matrixWs);
    fit->setProperty("CreateOutput", true);
    fit->setProperty("Minimizer", "Levenberg-MarquardtMD");
    fit->setProperty("IgnoreInvalidData", true);
    fit->execute();

    TS_ASSERT(fit->isExecuted());

    for (size_t i = 0; i < slopes.size(); ++i) {
      TS_ASSERT_DELTA(fun->getParameter(i), static_cast<double>(i), 1e-5);
      TS_ASSERT_LESS_THAN(fun->getError(i), 2e-4);
    }
  }

private:
  class TestableSeqDomainSpectrumCreator : public SeqDomainSpectrumCreator {
    friend class SeqDomainSpectrumCreatorTest;

  public:
    TestableSeqDomainSpectrumCreator(IPropertyManager *manager, const std::string &workspacePropertyName)
        : SeqDomainSpectrumCreator(manager, workspacePropertyName) {}

    ~TestableSeqDomainSpectrumCreator() override = default;
  };

  class SeqDomainCreatorTestFunction : public IFunction1DSpectrum, public ParamFunction {
  public:
    ~SeqDomainCreatorTestFunction() override = default;

    std::string name() const override { return "SeqDomainCreatorTestFunction"; }

    void function1DSpectrum(const FunctionDomain1DSpectrum &domain, FunctionValues &values) const override {

      double wsIndex = static_cast<double>(domain.getWorkspaceIndex());
      double slope = getParameter("Slope");

      for (size_t j = 0; j < domain.size(); ++j) {
        values.addToCalculated(j, wsIndex + slope * domain[j]);
      }
    }

  protected:
    void init() override { declareParameter("Slope", 1.0); }
  };

  class SeqDomainCreatorTestFunctionComplex : public IFunction1DSpectrum, public ParamFunction {
  public:
    ~SeqDomainCreatorTestFunctionComplex() override = default;

    std::string name() const override { return "SeqDomainCreatorTestFunctionComplex"; }

    void function1DSpectrum(const FunctionDomain1DSpectrum &domain, FunctionValues &values) const override {
      double wsIndex = static_cast<double>(domain.getWorkspaceIndex());
      double slope = getParameter(domain.getWorkspaceIndex() % 40);

      for (size_t j = 0; j < domain.size(); ++j) {
        values.addToCalculated(j, wsIndex + slope * domain[j]);
      }
    }

    void functionDeriv1DSpectrum(const FunctionDomain1DSpectrum &domain, Jacobian &jacobian) override {
      for (size_t j = 0; j < domain.size(); ++j) {
        jacobian.set(j, domain.getWorkspaceIndex() % 40, domain[j]);
      }
    }

  protected:
    void init() override {
      for (size_t i = 0; i < 40; ++i) {
        declareParameter("Slope" + boost::lexical_cast<std::string>(i), 4.0);
      }
    }
  };
};
