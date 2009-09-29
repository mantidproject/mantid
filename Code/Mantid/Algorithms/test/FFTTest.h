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
    FFTTest()
    {

        FrameworkManager::Instance();
        Mantid::DataObjects::Workspace2D_sptr ws = boost::dynamic_pointer_cast<Mantid::DataObjects::Workspace2D>
            (WorkspaceFactory::Instance().create("Workspace2D",1,100,100));

        Mantid::MantidVec& X = ws->dataX(0);
        Mantid::MantidVec& Y = ws->dataY(0);
        Mantid::MantidVec& E = ws->dataE(0);
        for(int j=0;j<100;j++)
        {
            X[j] = 0.1*(j+1);
            Y[j] = exp(-(X[j]-6)*(X[j]-6)/2);
            E[j] = 1.;
        }
        AnalysisDataService::Instance().add("FFT_WS",ws);

    }
    ~FFTTest()
    {
        FrameworkManager::Instance().deleteWorkspace("FFT_WS");
        FrameworkManager::Instance().deleteWorkspace("FFT_WS_forward");
        FrameworkManager::Instance().deleteWorkspace("FFT_WS_backward");
    }

    void testForward()
    {
        IAlgorithm* fft = Mantid::API::FrameworkManager::Instance().createAlgorithm("FFT");
        fft->initialize();
        fft->setPropertyValue("InputWorkspace","FFT_WS");
        fft->setPropertyValue("OutputWorkspace","FFT_WS_forward");
        fft->setPropertyValue("Real","0");
        fft->execute();

        MatrixWorkspace_sptr fWS = boost::dynamic_pointer_cast<MatrixWorkspace>
            (AnalysisDataService::Instance().retrieve("FFT_WS_forward"));

        const MantidVec& X0 = fWS->readX(3);
        const MantidVec& X1 = fWS->readX(4);
        const MantidVec& X2 = fWS->readX(5);

        const MantidVec& Y0 = fWS->readY(3);
        const MantidVec& Y1 = fWS->readY(4);
        const MantidVec& Y2 = fWS->readY(5);

        TS_ASSERT_DELTA(X0[0],-5.0505,0.0001);
        TS_ASSERT_DELTA(X1[0],-5.0505,0.0001);
        TS_ASSERT_DELTA(X2[0],-5.0505,0.0001);

        TS_ASSERT_DELTA(X0[50],0.,0.0001);
        TS_ASSERT_DELTA(X1[50],0.,0.0001);
        TS_ASSERT_DELTA(X2[50],0.,0.0001);

        TS_ASSERT_DELTA(X0[99],4.94949,0.0001);
        TS_ASSERT_DELTA(X1[99],4.94949,0.0001);
        TS_ASSERT_DELTA(X2[99],4.94949,0.0001);

        TS_ASSERT_DELTA(Y0[44],-0.02033,0.00001);
        TS_ASSERT_DELTA(Y1[44],-0.00537,0.00001);
        TS_ASSERT_DELTA(Y2[44],0.02103,0.00001);

        TS_ASSERT_DELTA(Y0[50],25.0656,0.0001);
        TS_ASSERT_DELTA(Y1[50],0.,0.0001);
        TS_ASSERT_DELTA(Y2[50],25.0656,0.0001);

        TS_ASSERT_DELTA(Y0[56],-0.02033,0.00001);
        TS_ASSERT_DELTA(Y1[56],0.00537,0.00001);
        TS_ASSERT_DELTA(Y2[56],0.02103,0.00001);
    }

    void testBackward()
    {
        IAlgorithm* fft = Mantid::API::FrameworkManager::Instance().createAlgorithm("FFT");
        fft->initialize();
        fft->setPropertyValue("InputWorkspace","FFT_WS_forward");
        fft->setPropertyValue("OutputWorkspace","FFT_WS_backward");
        fft->setPropertyValue("Real","3");
        fft->setPropertyValue("Imaginary","4");
        fft->setPropertyValue("Transform","Backward");
        fft->execute();

        MatrixWorkspace_sptr fWS = boost::dynamic_pointer_cast<MatrixWorkspace>
            (AnalysisDataService::Instance().retrieve("FFT_WS_backward"));

        const MantidVec& X0 = fWS->readX(0);
        const MantidVec& X1 = fWS->readX(1);
        const MantidVec& X2 = fWS->readX(2);

        const MantidVec& Y0 = fWS->readY(0);
        const MantidVec& Y1 = fWS->readY(1);
        const MantidVec& Y2 = fWS->readY(2);

        TS_ASSERT_DELTA(X0[0],-5.0,0.0001);
        TS_ASSERT_DELTA(X1[0],-5.0,0.0001);
        TS_ASSERT_DELTA(X2[0],-5.0,0.0001);

        TS_ASSERT_DELTA(X0[50],0.,0.0001);
        TS_ASSERT_DELTA(X1[50],0.,0.0001);
        TS_ASSERT_DELTA(X2[50],0.,0.0001);

        TS_ASSERT_DELTA(X0[99],4.9,0.0001);
        TS_ASSERT_DELTA(X1[99],4.9,0.0001);
        TS_ASSERT_DELTA(X2[99],4.9,0.0001);

        TS_ASSERT_DELTA(Y0[52],0.78271,0.00001);
        TS_ASSERT_DELTA(Y1[52],0.,0.00001);
        TS_ASSERT_DELTA(Y2[52],0.78271,0.00001);

        TS_ASSERT_DELTA(Y0[59],1.,0.0001);
        TS_ASSERT_DELTA(Y1[59],0.,0.0001);
        TS_ASSERT_DELTA(Y2[59],1.,0.0001);

        TS_ASSERT_DELTA(Y0[66],0.78271,0.00001);
        TS_ASSERT_DELTA(Y1[66],0.,0.00001);
        TS_ASSERT_DELTA(Y2[66],0.78271,0.00001);
    }
};


#endif /*FFT_TEST_H_*/
