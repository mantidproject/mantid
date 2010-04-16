#ifndef FITMULTISPECTRALTESTTEST_H_
#define FITMULTISPECTRALTESTTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidCurveFitting/FitMultispectral.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidNexus/LoadNexus.h"
#include "MantidDataHandling/LoadInstrument.h"

using namespace Mantid::API;
using namespace Mantid::CurveFitting;

class FitMultispectralTest : public CxxTest::TestSuite
{
public:

  FitMultispectralTest()
  {
    FrameworkManager::Instance();
  }

  void testSimple()
  {
    WS_type ws_in = loadNexus("../../../../Test/Data/MultispectralTestData.nxs","FitMultispectralTest_input");

    IAlgorithm* fit = FrameworkManager::Instance().createAlgorithm("FitMultispectral");
    fit->setPropertyValue("InputWorkspace","FitMultispectralTest_input");
    fit->setPropertyValue("Function","name=Lorentzian,Height=1,PeakCentre=0,HWHM=0.01");
    fit->setPropertyValue("Output","FitMultispectralTest_output");
    fit->execute();
    TS_ASSERT(fit->isExecuted());
    WS_type ws_out = getWS("FitMultispectralTest_output_Workspace");
    TS_ASSERT(ws_out);
    for(int spec=0;spec<ws_in->getNumberHistograms();spec++)
    {
      const Mantid::MantidVec& Yin = ws_in->readY(spec);
      const Mantid::MantidVec& Yout = ws_out->readY(spec);
      for(int i=0;i<Yin.size();i++)
      {
        TS_ASSERT_DELTA(Yin[i],Yout[i],1e-6);
      }
    }
    removeWS("FitMultispectralTest_output_Workspace");
    removeWS("FitMultispectralTest_input");
  }

  typedef Mantid::DataObjects::Workspace2D_sptr WS_type;

  WS_type loadNexus(const std::string& fName,const std::string& wsName)
  {
    IAlgorithm* alg = FrameworkManager::Instance().createAlgorithm("LoadNexus");
    alg->initialize();
    alg->setPropertyValue("FileName",fName);
    alg->setPropertyValue("OutputWorkspace",wsName);
    alg->execute();
    return getWS(wsName);
  }

  WS_type getWS(const std::string& name)
  {
    return boost::dynamic_pointer_cast<Mantid::DataObjects::Workspace2D>(AnalysisDataService::Instance().retrieve(name));
  }

  void removeWS(const std::string& name)
  {
    AnalysisDataService::Instance().remove(name);
  }

};

#endif /*FITMULTISPECTRALTESTTEST_H_*/
