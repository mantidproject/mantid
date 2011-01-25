#ifndef REALFFT_TEST_H_
#define REALFFT_TEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/FFT.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"

using namespace Mantid;
using namespace Mantid::API;

class RealFFTTest : public CxxTest::TestSuite
{
public:
  static RealFFTTest *createSuite() { return new RealFFTTest(); }
  static void destroySuite(RealFFTTest *suite) { delete suite; }

  RealFFTTest():N(116),dX(0.3),XX(N*dX)
    {

        FrameworkManager::Instance();
        Mantid::DataObjects::Workspace2D_sptr ws = boost::dynamic_pointer_cast<Mantid::DataObjects::Workspace2D>
            (WorkspaceFactory::Instance().create("Workspace2D",1,N,N));

        Mantid::DataObjects::Workspace2D_sptr ws1 = boost::dynamic_pointer_cast<Mantid::DataObjects::Workspace2D>
            (WorkspaceFactory::Instance().create("Workspace2D",1,N+1,N));

        Mantid::MantidVec& X = ws->dataX(0);
        Mantid::MantidVec& Y = ws->dataY(0);
        Mantid::MantidVec& E = ws->dataE(0);

        int n2 = N / 2;
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
          if (i < N)
          {
            X[i] = dX*(k);
            Y[i] = exp(-X[i]*X[i] * 3.);
            E[i] = 1.;
          }
        }

        Mantid::MantidVec& X1 = ws1->dataX(0);

        std::copy(X.begin(),X.end(),X1.begin());
        X1.back() = X.back() + dX;
        ws1->dataY(0) = Y;
        ws1->dataE(0) = E;

        AnalysisDataService::Instance().add("RealFFT_WS",ws);
        AnalysisDataService::Instance().add("RealFFT_WS_hist",ws1);

    }
    ~RealFFTTest()
    {
        FrameworkManager::Instance().deleteWorkspace("RealFFT_WS");
        FrameworkManager::Instance().deleteWorkspace("RealFFT_WS_hist");
        FrameworkManager::Instance().deleteWorkspace("RealFFT_WS_forward");
        FrameworkManager::Instance().deleteWorkspace("RealFFT_WS_backward");
    }

    void dotestForward(std::string IgnoreXBins)
    {
      IAlgorithm* fft = Mantid::API::FrameworkManager::Instance().createAlgorithm("RealFFT");
      fft->initialize();
      fft->setPropertyValue("InputWorkspace","RealFFT_WS");
      fft->setPropertyValue("OutputWorkspace","RealFFT_WS_forward");
      fft->setPropertyValue("WorkspaceIndex","0");
      fft->execute();

      MatrixWorkspace_sptr fWS = boost::dynamic_pointer_cast<MatrixWorkspace>
          (AnalysisDataService::Instance().retrieve("RealFFT_WS_forward"));

      const MantidVec& X = fWS->readX(0);
      const MantidVec& Yr = fWS->readY(0);
      const MantidVec& Yi = fWS->readY(1);

      const double PI = 3.1415926535897932384626433832795;
      double h = sqrt(PI/3);
      double a = PI*PI/3;
      double dx = 1/(XX);

      for(int i=0;i<N/4;i++)
      {
        double x = X[i];
        double tmp = sqrt(Yr[i]*Yr[i]+Yi[i]*Yi[i]);
        //std::cerr<<x<<' '<<tmp<<' '<<h*exp(-a*x*x)<<'\n';
        TS_ASSERT_DELTA(x,dx*i,0.00001);
        TS_ASSERT_DELTA(tmp / (h*exp(-a*x*x)),1.,0.001);
        TS_ASSERT_DELTA(Yi[i],0.0,0.00001);
      }
    }


    void testForward()
    {
      dotestForward("0");
    }

    void testForward_IgnoringX()
    {
      dotestForward("1");
    }


    void testBackward()
    {
        IAlgorithm* fft = Mantid::API::FrameworkManager::Instance().createAlgorithm("RealFFT");
        fft->initialize();
        fft->setPropertyValue("InputWorkspace","RealFFT_WS_forward");
        fft->setPropertyValue("OutputWorkspace","RealFFT_WS_backward");
        fft->setPropertyValue("Transform","Backward");
        fft->execute();

        MatrixWorkspace_sptr WS = boost::dynamic_pointer_cast<MatrixWorkspace>
            (AnalysisDataService::Instance().retrieve("RealFFT_WS"));

        MatrixWorkspace_sptr fWS = boost::dynamic_pointer_cast<MatrixWorkspace>
            (AnalysisDataService::Instance().retrieve("RealFFT_WS_backward"));

        const MantidVec& X0 = WS->readX(0);
        const MantidVec& Y0 = WS->readY(0);

        const MantidVec& X = fWS->readX(0);
        const MantidVec& Y = fWS->readY(0);

        for(int i=0;i<N;i++)
        {
          TS_ASSERT_DELTA(X[i],dX*i,0.00001);
          TS_ASSERT_DELTA(Y[i],Y0[i],0.00001);
        }
    }

    void testForwardHistogram()
    {
        IAlgorithm* fft = Mantid::API::FrameworkManager::Instance().createAlgorithm("RealFFT");
        fft->initialize();
        fft->setPropertyValue("InputWorkspace","RealFFT_WS_hist");
        fft->setPropertyValue("OutputWorkspace","RealFFT_WS_forward_hist");
        fft->setPropertyValue("WorkspaceIndex","0");
        fft->execute();

        MatrixWorkspace_sptr fWS = boost::dynamic_pointer_cast<MatrixWorkspace>
            (AnalysisDataService::Instance().retrieve("RealFFT_WS_forward_hist"));

        const MantidVec& X = fWS->readX(0);
        const MantidVec& Yr = fWS->readY(0);
        const MantidVec& Yi = fWS->readY(1);

        const double PI = 3.1415926535897932384626433832795;
        double h = sqrt(PI/3);
        double a = PI*PI/3;
        double dx = 1/(XX);

        for(int i=0;i<N/4;i++)
        {
          double x = X[i];
          double tmp = sqrt(Yr[i]*Yr[i]+Yi[i]*Yi[i]);
          //std::cerr<<x<<' '<<tmp<<' '<<h*exp(-a*x*x)<<'\n';
          TS_ASSERT_DELTA(x,dx*i,0.00001);
          TS_ASSERT_DELTA(tmp / (h*exp(-a*x*x)),1.,0.001);
          TS_ASSERT_DELTA(Yi[i],0.0,0.00001);
        }
    }

    void testBackwardHistogram()
    {
        IAlgorithm* fft = Mantid::API::FrameworkManager::Instance().createAlgorithm("RealFFT");
        fft->initialize();
        fft->setPropertyValue("InputWorkspace","RealFFT_WS_forward_hist");
        fft->setPropertyValue("OutputWorkspace","RealFFT_WS_backward_hist");
        fft->setPropertyValue("Transform","Backward");
        fft->execute();

        MatrixWorkspace_sptr WS = boost::dynamic_pointer_cast<MatrixWorkspace>
            (AnalysisDataService::Instance().retrieve("RealFFT_WS"));

        MatrixWorkspace_sptr fWS = boost::dynamic_pointer_cast<MatrixWorkspace>
            (AnalysisDataService::Instance().retrieve("RealFFT_WS_backward_hist"));

        const MantidVec& X0 = WS->readX(0);
        const MantidVec& Y0 = WS->readY(0);

        const MantidVec& X = fWS->readX(0);
        const MantidVec& Y = fWS->readY(0);

        for(int i=0;i<N;i++)
        {
          TS_ASSERT_DELTA(X[i],dX*i,0.00001);
          TS_ASSERT_DELTA(Y[i],Y0[i],0.00001);
        }
    }

private:
  const int N;
  const double dX,XX;
};

#endif /*REALFFT_TEST_H_*/
