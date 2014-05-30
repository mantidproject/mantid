#ifndef MANTID_CURVEFITTING_SEQDOMAINSPECTRUMCREATORTEST_H_
#define MANTID_CURVEFITTING_SEQDOMAINSPECTRUMCREATORTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidKernel/IPropertyManager.h"
#include "MantidCurveFitting/SeqDomainSpectrumCreator.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include "MantidCurveFitting/SeqDomain.h"
#include "MantidAPI/ParamFunction.h"

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

  class SeqDomainCreatorTestFunction : public ParamFunction
  {
  public:
      SeqDomainCreatorTestFunction() : ParamFunction() { }
      ~SeqDomainCreatorTestFunction() { }

      std::string name() const { return "SeqDomainCreatorTestFunction"; }

      void function(const FunctionDomain &domain, FunctionValues &values) const
      {
          const FunctionDomain1DSpectrum &spectrumDomain = dynamic_cast<const FunctionDomain1DSpectrum &>(domain);

          double wsIndex = static_cast<double>(spectrumDomain.getWorkspaceIndex());
          double slope = getParameter("Slope");

          for(size_t j = 0; j < spectrumDomain.size(); ++j) {
              values.addToCalculated(j, wsIndex + slope * spectrumDomain[j]);
          }
      }

  protected:
      void init()
      {
          declareParameter("Slope", 1.0);
      }
  };

};


#endif /* MANTID_CURVEFITTING_SEQDOMAINSPECTRUMCREATORTEST_H_ */
