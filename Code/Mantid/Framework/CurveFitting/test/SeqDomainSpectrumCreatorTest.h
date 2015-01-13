#ifndef MANTID_CURVEFITTING_SEQDOMAINSPECTRUMCREATORTEST_H_
#define MANTID_CURVEFITTING_SEQDOMAINSPECTRUMCREATORTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidKernel/IPropertyManager.h"
#include "MantidCurveFitting/SeqDomainSpectrumCreator.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include "MantidCurveFitting/SeqDomain.h"
#include "MantidAPI/IFunction1DSpectrum.h"
#include "MantidAPI/ParamFunction.h"

#include "MantidCurveFitting/Fit.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/FunctionFactory.h"

#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/AlgorithmManager.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::CurveFitting;

class SeqDomainSpectrumCreatorTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static SeqDomainSpectrumCreatorTest *createSuite() { return new SeqDomainSpectrumCreatorTest(); }
  static void destroySuite( SeqDomainSpectrumCreatorTest *suite ) { delete suite; }

  SeqDomainSpectrumCreatorTest()
  {
      FrameworkManager::Instance();
  }

  void testConstructor()
  {
      TS_ASSERT_THROWS_NOTHING(SeqDomainSpectrumCreator creator(NULL, ""));

      TestableSeqDomainSpectrumCreator otherCreator(NULL, "Test");

      TS_ASSERT_EQUALS(otherCreator.m_workspacePropertyName, otherCreator.m_workspacePropertyNames.front());
      TS_ASSERT_EQUALS(otherCreator.m_workspacePropertyName, "Test");
  }

  void testSetMatrixWorkspace()
  {
      TestableSeqDomainSpectrumCreator creator(NULL, "");
      TS_ASSERT_THROWS_NOTHING(creator.setMatrixWorkspace(WorkspaceCreationHelper::Create2DWorkspace(5, 5)));

      TS_ASSERT_EQUALS(creator.m_matrixWorkspace->getNumberHistograms(), 5);

      TS_ASSERT_THROWS(creator.setMatrixWorkspace(MatrixWorkspace_sptr()), std::invalid_argument);
  }

  void testGetDomainSize()
  {
      TestableSeqDomainSpectrumCreator creator(NULL, "");
      creator.setMatrixWorkspace(WorkspaceCreationHelper::Create2DWorkspace123(4, 12));

      FunctionDomain_sptr domain;
      FunctionValues_sptr values;

      creator.createDomain(domain, values);

      boost::shared_ptr<SeqDomain> seqDomain = boost::dynamic_pointer_cast<SeqDomain>(domain);

      TS_ASSERT(seqDomain);
      TS_ASSERT_EQUALS(seqDomain->getNDomains(), 4);
      TS_ASSERT_EQUALS(seqDomain->size(), 4 * 12);
  }

  void testHistogramIsUsable()
  {
      TestableSeqDomainSpectrumCreator creator(NULL, "");

      TS_ASSERT_THROWS(creator.histogramIsUsable(0), std::invalid_argument);

      // Workspace with 2 histograms, one of which is masked (No. 0)
      std::set<int64_t> masked;
      masked.insert(0);
      creator.setMatrixWorkspace(WorkspaceCreationHelper::Create2DWorkspace123(2, 12, false, masked));

      TS_ASSERT(!creator.histogramIsUsable(0));
      TS_ASSERT(creator.histogramIsUsable(1));

      // No instrument
      creator.setMatrixWorkspace(WorkspaceCreationHelper::Create2DWorkspace123(2, 12));
      TS_ASSERT(creator.histogramIsUsable(0));
      TS_ASSERT(creator.histogramIsUsable(1));
  }

  void testCreateDomain()
  {
      TestableSeqDomainSpectrumCreator creator(NULL, "");
      creator.setMatrixWorkspace(WorkspaceCreationHelper::Create2DWorkspace123(4, 12));

      FunctionDomain_sptr domain;
      FunctionValues_sptr values;

      creator.createDomain(domain, values);

      boost::shared_ptr<SeqDomain> seqDomain = boost::dynamic_pointer_cast<SeqDomain>(domain);

      for(size_t i = 0; i < seqDomain->getNDomains(); ++i) {
          FunctionDomain_sptr localDomain;
          FunctionValues_sptr localValues;

          seqDomain->getDomainAndValues(i, localDomain, localValues);

          boost::shared_ptr<FunctionDomain1DSpectrum> localSpectrumDomain = boost::dynamic_pointer_cast<FunctionDomain1DSpectrum>(localDomain);
          TS_ASSERT(localSpectrumDomain);

          TS_ASSERT_EQUALS(localSpectrumDomain->getWorkspaceIndex(), i);
          TS_ASSERT_EQUALS(localSpectrumDomain->size(), 12);
      }
  }

  void testCreateDomainMaskedDetectors()
  {
      TestableSeqDomainSpectrumCreator creator(NULL, "");

      // Workspace with 4 histograms, one of which is masked (No. 2)
      std::set<int64_t> masked;
      masked.insert(2);
      creator.setMatrixWorkspace(WorkspaceCreationHelper::Create2DWorkspace123(4, 12, false, masked));

      FunctionDomain_sptr domain;
      FunctionValues_sptr values;

      creator.createDomain(domain, values);

      boost::shared_ptr<SeqDomain> seqDomain = boost::dynamic_pointer_cast<SeqDomain>(domain);

      // One less than the created workspace
      TS_ASSERT_EQUALS(seqDomain->getNDomains(), 3);
      for(size_t i = 0; i < seqDomain->getNDomains(); ++i) {
          FunctionDomain_sptr localDomain;
          FunctionValues_sptr localValues;

          seqDomain->getDomainAndValues(i, localDomain, localValues);

          boost::shared_ptr<FunctionDomain1DSpectrum> localSpectrumDomain = boost::dynamic_pointer_cast<FunctionDomain1DSpectrum>(localDomain);
          TS_ASSERT(localSpectrumDomain);

          TS_ASSERT_EQUALS(localSpectrumDomain->size(), 12);

          // Make sure we never find 2 (masking)
          TS_ASSERT_DIFFERS(localSpectrumDomain->getWorkspaceIndex(), 2);
      }
  }

  void testCreateOutputWorkspace()
  {
      double slope = 2.0;
      // all x values are 1.0

      MatrixWorkspace_sptr matrixWs = WorkspaceCreationHelper::Create2DWorkspace123(4, 12);

      TestableSeqDomainSpectrumCreator creator(NULL, "");
      creator.setMatrixWorkspace(matrixWs);

      FunctionDomain_sptr domain;
      FunctionValues_sptr values;

      creator.createDomain(domain, values);

      IFunction_sptr testFunction(new SeqDomainCreatorTestFunction);
      testFunction->initialize();
      testFunction->setParameter("Slope", slope);

      Workspace_sptr outputWs = creator.createOutputWorkspace("", testFunction, domain, values);

      MatrixWorkspace_sptr outputWsMatrix = boost::dynamic_pointer_cast<MatrixWorkspace>(outputWs);
      TS_ASSERT(outputWsMatrix);

      TS_ASSERT_EQUALS(outputWsMatrix->getNumberHistograms(), matrixWs->getNumberHistograms());

      // Spectrum 0: 0 + 2 * 1 -> All y-values should be 2
      // Spectrum 1: 1 + 2 * 1 -> All y-values should be 3...etc.
      for(size_t i = 0; i < outputWsMatrix->getNumberHistograms(); ++i) {
          const std::vector<double> &x = outputWsMatrix->readX(i);
          const std::vector<double> &y = outputWsMatrix->readY(i);

          for(size_t j = 0; j < x.size(); ++j) {
              TS_ASSERT_EQUALS(x[j], 1.0);
              TS_ASSERT_EQUALS(y[j], static_cast<double>(i) + slope * x[j]);
          }
      }
  }

  void testCreateOutputWorkspaceMasked()
  {
      double slope = 2.0;
      // all x values are 1.0
      // Mask one histogram (No. 2)
      std::set<int64_t> masked;
      masked.insert(2);
      MatrixWorkspace_sptr matrixWs = WorkspaceCreationHelper::Create2DWorkspace123(4, 12, false, masked);

      TestableSeqDomainSpectrumCreator creator(NULL, "");
      creator.setMatrixWorkspace(matrixWs);

      FunctionDomain_sptr domain;
      FunctionValues_sptr values;

      creator.createDomain(domain, values);

      IFunction_sptr testFunction(new SeqDomainCreatorTestFunction);
      testFunction->initialize();
      testFunction->setParameter("Slope", slope);

      Workspace_sptr outputWs = creator.createOutputWorkspace("", testFunction, domain, values);

      MatrixWorkspace_sptr outputWsMatrix = boost::dynamic_pointer_cast<MatrixWorkspace>(outputWs);
      TS_ASSERT(outputWsMatrix);

      // Still has to be the same number of histograms.
      TS_ASSERT_EQUALS(outputWsMatrix->getNumberHistograms(), matrixWs->getNumberHistograms());

      // Spectrum 0: 0 + 2 * 1 -> All y-values should be 2
      // Spectrum 1: 1 + 2 * 1 -> All y-values should be 3...etc.
      for(size_t i = 0; i < outputWsMatrix->getNumberHistograms(); ++i) {
          const std::vector<double> &x = outputWsMatrix->readX(i);
          const std::vector<double> &y = outputWsMatrix->readY(i);

          for(size_t j = 0; j < x.size(); ++j) {
              TS_ASSERT_EQUALS(x[j], 1.0);

              // If detector is not masked, there should be values, otherwise 0.
              if(!outputWsMatrix->getDetector(i)->isMasked()) {
                  TS_ASSERT_EQUALS(y[j], static_cast<double>(i) + slope * x[j]);
              } else {
                  TS_ASSERT_EQUALS(y[j], 0.0);
              }
          }
      }

  }

  void testFit()
  {
      double slope = 2.0;

      MatrixWorkspace_sptr matrixWs = WorkspaceCreationHelper::Create2DWorkspace123(400, 500);
      for(size_t i = 0; i < matrixWs->getNumberHistograms(); ++i) {
          std::vector<double> &x = matrixWs->dataX(i);
          std::vector<double> &y = matrixWs->dataY(i);
          std::vector<double> &e = matrixWs->dataE(i);

          for(size_t j = 0; j < x.size(); ++j) {
              x[j] = static_cast<double>(j);
              y[j] = static_cast<double>(i) + slope * x[j];
              e[j] = 0.0001 * y[j];
          }
      }

      WorkspaceCreationHelper::addNoise(matrixWs, 0.0, -0.1, 0.1);

      IFunction_sptr fun(new SeqDomainCreatorTestFunction);
      fun->initialize();
      fun->setParameter("Slope", 0.0);

      Mantid::API::IAlgorithm_sptr fit = Mantid::API::AlgorithmManager::Instance().create("Fit");
      fit->initialize();

      fit->setProperty("Function",fun);
      fit->setProperty("InputWorkspace",matrixWs);
      fit->setProperty("CreateOutput",true);
      fit->setProperty("Minimizer", "Levenberg-MarquardtMD");

      fit->execute();

      TS_ASSERT(fit->isExecuted());

      TS_ASSERT_DELTA(fun->getParameter(0), 2.0, 1e-6);
      TS_ASSERT_LESS_THAN(fun->getError(0), 1e-6);
  }

  void testFitComplex()
  {
      std::vector<double> slopes(40);
      for(size_t i = 0; i < slopes.size(); ++i) {
          slopes[i] = static_cast<double>(i);
      }

      MatrixWorkspace_sptr matrixWs = WorkspaceCreationHelper::Create2DWorkspace123(400, 50);
      for(size_t i = 0; i < matrixWs->getNumberHistograms(); ++i) {
          std::vector<double> &x = matrixWs->dataX(i);
          std::vector<double> &y = matrixWs->dataY(i);
          std::vector<double> &e = matrixWs->dataE(i);

          for(size_t j = 0; j < x.size(); ++j) {
              x[j] = static_cast<double>(j);
              y[j] = static_cast<double>(i) + slopes[i % slopes.size()] * x[j];
              e[j] = 0.001 * std::max(1.0, sqrt(y[j]));
          }
      }

      //WorkspaceCreationHelper::addNoise(matrixWs, 0.0, -0.1, 0.1);

      IFunction_sptr fun(new SeqDomainCreatorTestFunctionComplex);
      fun->initialize();
      for(size_t i = 0; i < slopes.size(); ++ i) {
        fun->setParameter(i, static_cast<double>(i) + 1.1);
      }

      Mantid::API::IAlgorithm_sptr fit = Mantid::API::AlgorithmManager::Instance().create("Fit");
      fit->initialize();

      fit->setProperty("Function",fun);
      fit->setProperty("InputWorkspace",matrixWs);
      fit->setProperty("CreateOutput",true);
      fit->setProperty("Minimizer", "Levenberg-MarquardtMD");

      fit->execute();

      TS_ASSERT(fit->isExecuted());

      for(size_t i = 0; i < slopes.size(); ++ i) {
          TS_ASSERT_DELTA(fun->getParameter(i), static_cast<double>(i), 1e-5);
          TS_ASSERT_LESS_THAN(fun->getError(i), 2e-4);
      }

  }


private:
  class TestableSeqDomainSpectrumCreator : public SeqDomainSpectrumCreator {
      friend class SeqDomainSpectrumCreatorTest;

  public:
      TestableSeqDomainSpectrumCreator(IPropertyManager* manager,
                                        const std::string& workspacePropertyName)
          : SeqDomainSpectrumCreator(manager, workspacePropertyName)
      {

      }

      ~TestableSeqDomainSpectrumCreator() { }
  };

  class SeqDomainCreatorTestFunction : public virtual IFunction1DSpectrum, public virtual ParamFunction
  {
  public:
      ~SeqDomainCreatorTestFunction() { }

      std::string name() const { return "SeqDomainCreatorTestFunction"; }

      void function1DSpectrum(const FunctionDomain1DSpectrum &domain, FunctionValues &values) const
      {

          double wsIndex = static_cast<double>(domain.getWorkspaceIndex());
          double slope = getParameter("Slope");

          for(size_t j = 0; j < domain.size(); ++j) {
              values.addToCalculated(j, wsIndex + slope * domain[j]);
          }
      }

  protected:
      void init()
      {
          declareParameter("Slope", 1.0);
      }
  };

  class SeqDomainCreatorTestFunctionComplex : public virtual IFunction1DSpectrum, public virtual ParamFunction
  {
  public:
      ~SeqDomainCreatorTestFunctionComplex() { }

      std::string name() const { return "SeqDomainCreatorTestFunctionComplex"; }

      void function1DSpectrum(const FunctionDomain1DSpectrum &domain, FunctionValues &values) const
      {
          double wsIndex = static_cast<double>(domain.getWorkspaceIndex());
          double slope = getParameter(domain.getWorkspaceIndex() % 40);

          for(size_t j = 0; j < domain.size(); ++j) {
              values.addToCalculated(j, wsIndex + slope * domain[j]);
          }
      }

      void functionDeriv1DSpectrum(const FunctionDomain1DSpectrum &domain, Jacobian &jacobian)
      {
          for(size_t j = 0; j < domain.size(); ++j) {
              jacobian.set(j, domain.getWorkspaceIndex() % 40, domain[j]);
          }
      }

  protected:
      void init()
      {
          for(size_t i = 0; i < 40; ++ i) {
            declareParameter("Slope" + boost::lexical_cast<std::string>(i), 4.0);
          }
      }
  };

};

#endif /* MANTID_CURVEFITTING_SEQDOMAINSPECTRUMCREATORTEST_H_ */
