#ifndef FFT_TEST_H_
#define FFT_TEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/FFT.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"

using namespace Mantid;
using namespace Mantid::API;

class FFTTest : public CxxTest::TestSuite
{
public:
  FFTTest():dX(0.2),PI(3.1415926535897932384626433832795),h(sqrt(PI/3)),a(PI*PI/3)
  {
  }
  ~FFTTest()
  {
  }

  void testForward()
  {
    const int N = 100;
    const double XX = dX * N;
    double dx = 1/(XX);

    createWS(N,0,"even_points");

    IAlgorithm* fft = Mantid::API::FrameworkManager::Instance().createAlgorithm("FFT");
    fft->initialize();
    fft->setPropertyValue("InputWorkspace","FFT_WS_even_points");
    fft->setPropertyValue("OutputWorkspace","FFT_out");
    fft->setPropertyValue("Real","0");
    fft->execute();

    MatrixWorkspace_sptr fWS = boost::dynamic_pointer_cast<MatrixWorkspace>
      (AnalysisDataService::Instance().retrieve("FFT_out"));

    const MantidVec& X = fWS->readX(3);
    const MantidVec& Yr = fWS->readY(3);
    const MantidVec& Yi = fWS->readY(4);


    const MantidVec::const_iterator it = std::find(X.begin(),X.end(),0.);

    TS_ASSERT( it != X.end() );
    int i0 = static_cast<int>(it - X.begin());

    for(int i=0;i<N/4;i++)
    {
      int j = i0 + i;
      double x = X[j];
      //std::cerr<<j<<' '<<x<<' '<<Yr[j]<<' '<<h*exp(-a*x*x)<<'\n';
      TS_ASSERT_DELTA(x,dx*i,0.00001);
      TS_ASSERT_DELTA(Yr[i0+i] / (h*exp(-a*x*x)),1.,0.001);
      TS_ASSERT_DELTA(Yi[i0+i],0.0,0.00001);
      TS_ASSERT_DELTA(Yr[i0-i] / (h*exp(-a*x*x)),1.,0.001);
      TS_ASSERT_DELTA(Yi[i0-i],0.0,0.00001);
    }
    FrameworkManager::Instance().deleteWorkspace("FFT_WS_even_points");
    FrameworkManager::Instance().deleteWorkspace("FFT_out");
  }

  void testBackward()
  {
    const int N = 100;

    MatrixWorkspace_sptr WS = createWS(N,0,"even_points");

    IAlgorithm* fft = Mantid::API::FrameworkManager::Instance().createAlgorithm("FFT");
    fft->initialize();
    fft->setPropertyValue("InputWorkspace","FFT_WS_even_points");
    fft->setPropertyValue("OutputWorkspace","FFT_out");
    fft->setPropertyValue("Real","0");
    fft->execute();

    fft = Mantid::API::FrameworkManager::Instance().createAlgorithm("FFT");
    fft->initialize();
    fft->setPropertyValue("InputWorkspace","FFT_out");
    fft->setPropertyValue("OutputWorkspace","FFT_WS_backward");
    fft->setPropertyValue("Real","3");
    fft->setPropertyValue("Imaginary","4");
    fft->setPropertyValue("Transform","Backward");
    fft->execute();

    MatrixWorkspace_sptr fWS = boost::dynamic_pointer_cast<MatrixWorkspace>
      (AnalysisDataService::Instance().retrieve("FFT_WS_backward"));

    const MantidVec& Y0 = WS->readY(0);

    const MantidVec& X = fWS->readX(0);
    const MantidVec& Y = fWS->readY(0);

    for(int i=0;i<N;i++)
    {
      TS_ASSERT_DELTA(X[i],dX*(i - N/2),0.00001);
      TS_ASSERT_DELTA(Y[i],Y0[i],0.00001);
    }
    FrameworkManager::Instance().deleteWorkspace("FFT_WS_even_points");
    FrameworkManager::Instance().deleteWorkspace("FFT_out");
    FrameworkManager::Instance().deleteWorkspace("FFT_WS_backward");
  }

  void testForwardHist()
  {
    const int N = 100;
    const double XX = dX * N;
    double dx = 1/(XX);

    createWS(N,1,"even_hist");

    IAlgorithm* fft = Mantid::API::FrameworkManager::Instance().createAlgorithm("FFT");
    fft->initialize();
    fft->setPropertyValue("InputWorkspace","FFT_WS_even_hist");
    fft->setPropertyValue("OutputWorkspace","FFT_out");
    fft->setPropertyValue("Real","0");
    fft->execute();

    MatrixWorkspace_sptr fWS = boost::dynamic_pointer_cast<MatrixWorkspace>
      (AnalysisDataService::Instance().retrieve("FFT_out"));

    const MantidVec& X = fWS->readX(3);
    const MantidVec& Yr = fWS->readY(3);
    const MantidVec& Yi = fWS->readY(4);


    const MantidVec::const_iterator it = std::find(X.begin(),X.end(),0.);

    TS_ASSERT( it != X.end() );
    int i0 = static_cast<int>(it - X.begin());

    for(int i=0;i<N/4;i++)
    {
      int j = i0 + i;
      double x = X[j];
      //std::cerr<<j<<' '<<x<<' '<<Yr[j]<<' '<<h*exp(-a*x*x)<<'\n';
      TS_ASSERT_DELTA(x,dx*i,0.00001);
      TS_ASSERT_DELTA(Yr[i0+i] / (h*exp(-a*x*x)),1.,0.001);
      TS_ASSERT_DELTA(Yi[i0+i],0.0,0.00001);
      TS_ASSERT_DELTA(Yr[i0-i] / (h*exp(-a*x*x)),1.,0.001);
      TS_ASSERT_DELTA(Yi[i0-i],0.0,0.00001);
    }
    FrameworkManager::Instance().deleteWorkspace("FFT_WS_even_hist");
    FrameworkManager::Instance().deleteWorkspace("FFT_out");
  }

  void testBackwardHist()
  {
    const int N = 100;

    MatrixWorkspace_sptr WS = createWS(N,1,"even_points");

    IAlgorithm* fft = Mantid::API::FrameworkManager::Instance().createAlgorithm("FFT");
    fft->initialize();
    fft->setPropertyValue("InputWorkspace","FFT_WS_even_points");
    fft->setPropertyValue("OutputWorkspace","FFT_out");
    fft->setPropertyValue("Real","0");
    fft->execute();

    fft = Mantid::API::FrameworkManager::Instance().createAlgorithm("FFT");
    fft->initialize();
    fft->setPropertyValue("InputWorkspace","FFT_out");
    fft->setPropertyValue("OutputWorkspace","FFT_WS_backward");
    fft->setPropertyValue("Real","3");
    fft->setPropertyValue("Imaginary","4");
    fft->setPropertyValue("Transform","Backward");
    fft->execute();

    MatrixWorkspace_sptr fWS = boost::dynamic_pointer_cast<MatrixWorkspace>
      (AnalysisDataService::Instance().retrieve("FFT_WS_backward"));

    const MantidVec& Y0 = WS->readY(0);

    const MantidVec& X = fWS->readX(0);
    const MantidVec& Y = fWS->readY(0);

    for(int i=0;i<N;i++)
    {
      TS_ASSERT_DELTA(X[i],dX*(i - N/2),0.00001);
      TS_ASSERT_DELTA(Y[i],Y0[i],0.00001);
    }
    FrameworkManager::Instance().deleteWorkspace("FFT_WS_even_points");
    FrameworkManager::Instance().deleteWorkspace("FFT_out");
    FrameworkManager::Instance().deleteWorkspace("FFT_WS_backward");
  }

  void testOddForward()
  {
    const int N = 101;
    const double XX = dX * N;
    double dx = 1/(XX);

    createWS(N,0,"odd_points");

    IAlgorithm* fft = Mantid::API::FrameworkManager::Instance().createAlgorithm("FFT");
    fft->initialize();
    fft->setPropertyValue("InputWorkspace","FFT_WS_odd_points");
    fft->setPropertyValue("OutputWorkspace","FFT_out");
    fft->setPropertyValue("Real","0");
    fft->execute();

    MatrixWorkspace_sptr fWS = boost::dynamic_pointer_cast<MatrixWorkspace>
      (AnalysisDataService::Instance().retrieve("FFT_out"));

    const MantidVec& X = fWS->readX(3);
    const MantidVec& Yr = fWS->readY(3);
    const MantidVec& Yi = fWS->readY(4);


    const MantidVec::const_iterator it = std::find(X.begin(),X.end(),0.);

    TS_ASSERT( it != X.end() );
    int i0 = static_cast<int>(it - X.begin());

    for(int i=0;i<N/4;i++)
    {
      int j = i0 + i;
      double x = X[j];
      //std::cerr<<j<<' '<<x<<' '<<Yr[j]<<' '<<h*exp(-a*x*x)<<'\n';
      TS_ASSERT_DELTA(x,dx*i,0.00001);
      TS_ASSERT_DELTA(Yr[i0+i] / (h*exp(-a*x*x)),1.,0.001);
      TS_ASSERT_DELTA(Yi[i0+i],0.0,0.00001);
      TS_ASSERT_DELTA(Yr[i0-i] / (h*exp(-a*x*x)),1.,0.001);
      TS_ASSERT_DELTA(Yi[i0-i],0.0,0.00001);
    }
    FrameworkManager::Instance().deleteWorkspace("FFT_WS_odd_points");
    FrameworkManager::Instance().deleteWorkspace("FFT_out");
  }

  void testOddBackward()
  {
    const int N = 101;

    MatrixWorkspace_sptr WS = createWS(N,0,"odd_points");

    IAlgorithm* fft = Mantid::API::FrameworkManager::Instance().createAlgorithm("FFT");
    fft->initialize();
    fft->setPropertyValue("InputWorkspace","FFT_WS_odd_points");
    fft->setPropertyValue("OutputWorkspace","FFT_out");
    fft->setPropertyValue("Real","0");
    fft->execute();

    fft = Mantid::API::FrameworkManager::Instance().createAlgorithm("FFT");
    fft->initialize();
    fft->setPropertyValue("InputWorkspace","FFT_out");
    fft->setPropertyValue("OutputWorkspace","FFT_WS_backward");
    fft->setPropertyValue("Real","3");
    fft->setPropertyValue("Imaginary","4");
    fft->setPropertyValue("Transform","Backward");
    fft->execute();

    MatrixWorkspace_sptr fWS = boost::dynamic_pointer_cast<MatrixWorkspace>
      (AnalysisDataService::Instance().retrieve("FFT_WS_backward"));

    const MantidVec& Y0 = WS->readY(0);

    const MantidVec& X = fWS->readX(0);
    const MantidVec& Y = fWS->readY(0);

    for(int i=0;i<N;i++)
    {
      //std::cerr<<i<<' '<<X[i]<<' '<<X0[i]<<' '<<dX*(i - N/2)<<'\n';
      TS_ASSERT_DELTA(X[i],dX*(i - N/2),0.00001);
      TS_ASSERT_DELTA(Y[i],Y0[i],0.00001);
    }
    FrameworkManager::Instance().deleteWorkspace("FFT_WS_odd_points");
    FrameworkManager::Instance().deleteWorkspace("FFT_out");
    FrameworkManager::Instance().deleteWorkspace("FFT_WS_backward");
  }

  void testOddForwardHist()
  {
    const int N = 101;
    const double XX = dX * N;
    double dx = 1/(XX);

    createWS(N,1,"odd_hist");

    IAlgorithm* fft = Mantid::API::FrameworkManager::Instance().createAlgorithm("FFT");
    fft->initialize();
    fft->setPropertyValue("InputWorkspace","FFT_WS_odd_hist");
    fft->setPropertyValue("OutputWorkspace","FFT_out");
    fft->setPropertyValue("Real","0");
    fft->execute();

    MatrixWorkspace_sptr fWS = boost::dynamic_pointer_cast<MatrixWorkspace>
      (AnalysisDataService::Instance().retrieve("FFT_out"));

    const MantidVec& X = fWS->readX(3);
    const MantidVec& Yr = fWS->readY(3);
    const MantidVec& Yi = fWS->readY(4);


    const MantidVec::const_iterator it = std::find(X.begin(),X.end(),0.);

    TS_ASSERT( it != X.end() );
    int i0 = static_cast<int>(it - X.begin());

    for(int i=0;i<N/4;i++)
    {
      int j = i0 + i;
      double x = X[j];
      //std::cerr<<j<<' '<<x<<' '<<Yr[j]<<' '<<h*exp(-a*x*x)<<'\n';
      TS_ASSERT_DELTA(x, dx*i,0.00001);
      TS_ASSERT_DELTA(Yr[i0+i] / (h*exp(-a*x*x)),1.,0.001);
      TS_ASSERT_DELTA(Yi[i0+i],0.0,0.00001);
      TS_ASSERT_DELTA(Yr[i0-i] / (h*exp(-a*x*x)),1.,0.001);
      TS_ASSERT_DELTA(Yi[i0-i],0.0,0.00001);
    }
    FrameworkManager::Instance().deleteWorkspace("FFT_WS_odd_hist");
    FrameworkManager::Instance().deleteWorkspace("FFT_out");
  }

  void testOddBackwardHist()
  {
    const int N = 101;

    MatrixWorkspace_sptr WS = createWS(N,1,"odd_hist");

    IAlgorithm* fft = Mantid::API::FrameworkManager::Instance().createAlgorithm("FFT");
    fft->initialize();
    fft->setPropertyValue("InputWorkspace","FFT_WS_odd_hist");
    fft->setPropertyValue("OutputWorkspace","FFT_out");
    fft->setPropertyValue("Real","0");
    fft->execute();

    fft = Mantid::API::FrameworkManager::Instance().createAlgorithm("FFT");
    fft->initialize();
    fft->setPropertyValue("InputWorkspace","FFT_out");
    fft->setPropertyValue("OutputWorkspace","FFT_WS_backward");
    fft->setPropertyValue("Real","3");
    fft->setPropertyValue("Imaginary","4");
    fft->setPropertyValue("Transform","Backward");
    fft->execute();

    MatrixWorkspace_sptr fWS = boost::dynamic_pointer_cast<MatrixWorkspace>
      (AnalysisDataService::Instance().retrieve("FFT_WS_backward"));

    const MantidVec& Y0 = WS->readY(0);

    const MantidVec& X = fWS->readX(0);
    const MantidVec& Y = fWS->readY(0);

    for(int i=0;i<N;i++)
    {
      TS_ASSERT_DELTA(X[i],dX*(i - N/2),0.00001);
      TS_ASSERT_DELTA(Y[i],Y0[i],0.00001);
    }
    FrameworkManager::Instance().deleteWorkspace("FFT_WS_odd_hist");
    FrameworkManager::Instance().deleteWorkspace("FFT_out");
    FrameworkManager::Instance().deleteWorkspace("FFT_WS_backward");
  }

private:

  MatrixWorkspace_sptr createWS(int n,int dn,const std::string& name)
  {
    FrameworkManager::Instance();
    Mantid::DataObjects::Workspace2D_sptr ws = boost::dynamic_pointer_cast<Mantid::DataObjects::Workspace2D>
      (WorkspaceFactory::Instance().create("Workspace2D",1,n+dn,n));

    Mantid::MantidVec& X = ws->dataX(0);
    Mantid::MantidVec& Y = ws->dataY(0);
    Mantid::MantidVec& E = ws->dataE(0);

    int n2 = n / 2;
    for(int k=0;k<=n2;k++)
    {
      int i = n2 - k;
      if (i >= 0)
      {
        X[i] = -dX*(k);
        Y[i] = exp(-X[i]*X[i] * 3.);
        E[i] = 1.;
      }
      i = n2 + k;
      if (i < n)
      {
        X[i] = dX*(k);
        Y[i] = exp(-X[i]*X[i] * 3.);
        E[i] = 1.;
      }
    }

    if (dn > 0)
      X[n] = X[n-1] + dX;
    AnalysisDataService::Instance().add("FFT_WS_"+name,ws);

    return ws;
  }

  const double dX;
    const double PI;
    const double h;
    const double a;
};

#endif /*FFT_TEST_H_*/
