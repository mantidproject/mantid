#ifndef FFT_DERIVATIVE_TEST_H_
#define FFT_DERIVATIVE_TEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/FFTDerivative.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"

using namespace Mantid;
using namespace Mantid::API;

class FFTDerivativeTest : public CxxTest::TestSuite
{
public:
  FFTDerivativeTest()
  {
  }
  ~FFTDerivativeTest()
  {
  }

  void testGaussianDerivative()
  {
    const int N = 100;

    createWS(N,0,"exp");

    IAlgorithm* fft = Mantid::API::FrameworkManager::Instance().createAlgorithm("FFTDerivative");
    fft->initialize();
    fft->setPropertyValue("InputWorkspace","FFTDerivative_WS_exp");
    fft->setPropertyValue("OutputWorkspace","FFTDerivative_out");
    fft->execute();

    MatrixWorkspace_sptr fWS = boost::dynamic_pointer_cast<MatrixWorkspace>
      (AnalysisDataService::Instance().retrieve("FFTDerivative_out"));

    const MantidVec& X = fWS->readX(0);
    const MantidVec& Y = fWS->readY(0);

    TS_ASSERT_EQUALS(Y.size(),101);

    for(size_t i=0; i < Y.size(); ++i)
    {
      double xx = X[i] - 5.1;
      TS_ASSERT_DELTA( Y[i],(-2 * xx * exp(-(xx*xx)*2)*2),0.000001);
    }

    FrameworkManager::Instance().deleteWorkspace("FFTDerivative_WS_exp");
    FrameworkManager::Instance().deleteWorkspace("FFTDerivative_out");
  }


private:

  MatrixWorkspace_sptr createWS(int n,int dn,const std::string& name)
  {
    FrameworkManager::Instance();
    Mantid::DataObjects::Workspace2D_sptr ws = boost::dynamic_pointer_cast<Mantid::DataObjects::Workspace2D>
      (WorkspaceFactory::Instance().create("Workspace2D",1,n+dn,n));

    const double dX = 10.0 / (n-1);
    const double x0 = 0.;
    const double c = 5.1;
    Mantid::MantidVec& X = ws->dataX(0);
    Mantid::MantidVec& Y = ws->dataY(0);
    Mantid::MantidVec& E = ws->dataE(0);

    for(int i=0;i<n;i++)
    {
      double x = x0 + dX*(i);
      X[i] = x;
      Y[i] = exp(-(x-c)*(x-c)*2);
      E[i] = 1.;
    }

    if (dn > 0)
      X[n] = X[n-1] + dX;

    AnalysisDataService::Instance().add("FFTDerivative_WS_"+name,ws);

    return ws;
  }

};

#endif /*FFT_DERIVATIVE_TEST_H_*/
