#ifndef MANTID_CURVEFITTING_FABADAMINIMIZERTEST_H_
#define MANTID_CURVEFITTING_FABADAMINIMIZERTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidCurveFitting/FABADAMinimizer.h"

#include "MantidCurveFitting/Fit.h"
#include "MantidAPI/AlgorithmManager.h"

#include "MantidCurveFitting/ExpDecay.h"
#include "MantidKernel/PropertyManager.h"
#include "MantidTestHelpers/FakeObjects.h"
#include "MantidKernel/Exception.h"


using Mantid::CurveFitting::FABADAMinimizer;
using namespace Mantid::API;
using namespace Mantid;
using namespace Mantid::CurveFitting;

class FABADAMinimizerTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static FABADAMinimizerTest *createSuite() { return new FABADAMinimizerTest(); }
  static void destroySuite( FABADAMinimizerTest *suite ) { delete suite; }


  void test_expDecay()
  {
    const bool histogram(false);
    auto ws2 = createTestWorkspace(histogram);

    API::IFunction_sptr fun(new ExpDecay);
    fun->setParameter("Height",8.);
    fun->setParameter("Lifetime",1.0);

    Fit fit;
    fit.initialize();

    fit.setRethrows(true);
    fit.setProperty("Function",fun);
    fit.setProperty("InputWorkspace",ws2);
    fit.setProperty("WorkspaceIndex",0);
    fit.setProperty("CreateOutput",true);
    fit.setProperty("MaxIterations",100000);
    fit.setProperty("Minimizer", "FABADA,ChainLength=5000,ConvergenceCriteria = 0.1, OutputWorkspaceConverged=conv");

    TS_ASSERT_THROWS_NOTHING( fit.execute() );

    TS_ASSERT(fit.isExecuted());

    TS_ASSERT_DELTA( fun->getParameter("Height"), 10.0, 1e-1);
    TS_ASSERT_DELTA( fun->getParameter("Lifetime"), 0.5, 1e-2);

    TS_ASSERT_EQUALS(fit.getPropertyValue("OutputStatus"), "success");

    size_t n = fun -> nParams();

    TS_ASSERT( AnalysisDataService::Instance().doesExist("pdf") );
    MatrixWorkspace_sptr wsPDF = boost::dynamic_pointer_cast<MatrixWorkspace>(
      API::AnalysisDataService::Instance().retrieve("pdf"));
    TS_ASSERT(wsPDF);
    TS_ASSERT_EQUALS(wsPDF->getNumberHistograms(),n);

    const Mantid::MantidVec& X = wsPDF->dataX(0);
    const Mantid::MantidVec& Y = wsPDF->dataY(0);
    TS_ASSERT_EQUALS(X.size(), 51);
    TS_ASSERT_EQUALS(Y.size(), 50);

    TS_ASSERT( AnalysisDataService::Instance().doesExist("chi2") );
    ITableWorkspace_sptr chi2table = boost::dynamic_pointer_cast<ITableWorkspace>(
      API::AnalysisDataService::Instance().retrieve("chi2"));

    TS_ASSERT(chi2table);
    TS_ASSERT_EQUALS(chi2table->columnCount(), 4);
    TS_ASSERT_EQUALS(chi2table->rowCount(), 1);
    TS_ASSERT_EQUALS(chi2table->getColumn(0)->type(), "double");
    TS_ASSERT_EQUALS(chi2table->getColumn(0)->name(), "Chi2min");
    TS_ASSERT_EQUALS(chi2table->getColumn(1)->type(), "double");
    TS_ASSERT_EQUALS(chi2table->getColumn(1)->name(), "Chi2MP");
    TS_ASSERT_EQUALS(chi2table->getColumn(2)->type(), "double");
    TS_ASSERT_EQUALS(chi2table->getColumn(2)->name(), "Chi2min_red");
    TS_ASSERT_EQUALS(chi2table->getColumn(3)->type(), "double");
    TS_ASSERT_EQUALS(chi2table->getColumn(3)->name(), "Chi2MP_red");
    TS_ASSERT(chi2table->Double(0,0) <= chi2table->Double(0,1));
    TS_ASSERT(chi2table->Double(0,2) <= chi2table->Double(0,3));
    TS_ASSERT_DELTA(chi2table->Double(0,0), chi2table->Double(0,1), 1);
    TS_ASSERT_DELTA(chi2table->Double(0,0), 0.0, 1.0);

    TS_ASSERT( AnalysisDataService::Instance().doesExist("conv") );
    MatrixWorkspace_sptr wsConv = boost::dynamic_pointer_cast<MatrixWorkspace>(
      API::AnalysisDataService::Instance().retrieve("conv"));
    TS_ASSERT(wsConv);
    TS_ASSERT_EQUALS(wsConv->getNumberHistograms(),n+1);

    const Mantid::MantidVec& Xconv = wsConv->dataX(0);
    TS_ASSERT_EQUALS(Xconv.size(), 5000);
    TS_ASSERT_EQUALS(Xconv[2437], 2437);

    TS_ASSERT( AnalysisDataService::Instance().doesExist("chain") );
    MatrixWorkspace_sptr wsChain = boost::dynamic_pointer_cast<MatrixWorkspace>(
      API::AnalysisDataService::Instance().retrieve("chain"));
    TS_ASSERT(wsChain);
    TS_ASSERT_EQUALS(wsChain->getNumberHistograms(),n+1);

    const Mantid::MantidVec& Xchain = wsChain->dataX(0);
    TS_ASSERT_EQUALS(Xchain.size(), 6881);
    TS_ASSERT_EQUALS(Xchain[5000], 5000);

    TS_ASSERT(Xconv.size() < Xchain.size());

    TS_ASSERT( AnalysisDataService::Instance().doesExist("pdfE") );
    ITableWorkspace_sptr Etable = boost::dynamic_pointer_cast<ITableWorkspace>(
      API::AnalysisDataService::Instance().retrieve("pdfE"));

    TS_ASSERT(Etable);
    TS_ASSERT_EQUALS(Etable->columnCount(), 4);
    TS_ASSERT_EQUALS(Etable->rowCount(), n);
    TS_ASSERT_EQUALS(Etable->getColumn(0)->type(), "str");
    TS_ASSERT_EQUALS(Etable->getColumn(0)->name(), "Name");
    TS_ASSERT_EQUALS(Etable->getColumn(1)->type(), "double");
    TS_ASSERT_EQUALS(Etable->getColumn(1)->name(), "Value");
    TS_ASSERT_EQUALS(Etable->getColumn(2)->type(), "double");
    TS_ASSERT_EQUALS(Etable->getColumn(2)->name(), "Left's error");
    TS_ASSERT_EQUALS(Etable->getColumn(3)->type(), "double");
    TS_ASSERT_EQUALS(Etable->getColumn(3)->name(), "Rigth's error");
    TS_ASSERT(Etable->Double(0,1) == fun->getParameter("Height"));
    TS_ASSERT(Etable->Double(1,1) == fun->getParameter("Lifetime"));

  }

  void test_low_MaxIterations()
  {
    const bool histogram(false);
    auto ws2 = createTestWorkspace(histogram);

    API::IFunction_sptr fun(new ExpDecay);
    fun->setParameter("Height",1.);
    fun->setParameter("Lifetime",1.0);

    Fit fit;
    fit.initialize();

    fit.setRethrows(true);
    fit.setProperty("Function",fun);
    fit.setProperty("InputWorkspace",ws2);
    fit.setProperty("WorkspaceIndex",0);
    fit.setProperty("CreateOutput",true);
    fit.setProperty("MaxIterations",10);
    fit.setProperty("Minimizer", "FABADA,ChainLength=5000,ConvergenceCriteria = 0.01, OutputWorkspaceConverged=conv");

    TS_ASSERT_THROWS( fit.execute(), std::runtime_error );

    TS_ASSERT( !fit.isExecuted() );

  }
private:

  API::MatrixWorkspace_sptr createTestWorkspace(const bool histogram)
  {
    MatrixWorkspace_sptr ws2(new WorkspaceTester);
    ws2->initialize(2,20,20);

    for(size_t is = 0; is < ws2->getNumberHistograms(); ++is)
    {
      Mantid::MantidVec& x = ws2->dataX(is);
      Mantid::MantidVec& y = ws2->dataY(is);
      for(size_t i = 0; i < ws2->blocksize(); ++i)
      {
        x[i] = 0.1 * double(i);
        y[i] =  (10.0 + double(is)) * exp( -(x[i])/ (0.5*(1 + double(is))) );
      }
      if(histogram) x.back() = x[x.size()-2] + 0.1;
    }
    return ws2;
  }
};


#endif /* MANTID_CURVEFITTING_FABADAMINIMIZERTEST_H_ */
