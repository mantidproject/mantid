#ifndef EXTRACTFFTSPECTRUM_H_
#define EXTRACTFFTSPECTRUM_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/ExtractFFTSpectrum.h"
#include "WorkspaceCreationHelper.hh"
#include "MantidNexus/LoadNeXus.h"
#include "MantidAlgorithms/Rebin.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::Algorithms;
using namespace Mantid::NeXus;
using namespace Mantid::DataObjects;

class ExtractFFTSpectrumTest : public CxxTest::TestSuite
{

public:
  ExtractFFTSpectrumTest() {}
  ~ExtractFFTSpectrumTest() {}

  void testMetaInfo()
  {
    alg = new ExtractFFTSpectrum();
    TS_ASSERT_EQUALS(alg->name(), "ExtractFFTSpectrum");
    TS_ASSERT_EQUALS(alg->version(), 1);
    TS_ASSERT_EQUALS(alg->category(), "General");
    delete alg;
  }

  void testInit()
  {
    alg = new ExtractFFTSpectrum();
    TS_ASSERT_THROWS_NOTHING(alg->initialize());
    TS_ASSERT(alg->isInitialized());
    delete alg;
  }

  void testExec()
  {
    IAlgorithm* loader;
    loader = new LoadNexus;
    loader->initialize();
    loader->setPropertyValue("Filename", "IRS26176_ipg.nxs");
    loader->setPropertyValue("OutputWorkspace", "alg_irs_r");
    loader->setPropertyValue("SpectrumMin", "2");
    loader->setPropertyValue("SpectrumMax", "3");
    TS_ASSERT_THROWS_NOTHING(loader->execute());
    TS_ASSERT(loader->isExecuted());
    delete loader;
    
    IAlgorithm* rebin;
    rebin = new Rebin;
    rebin->initialize();
    rebin->setPropertyValue("InputWorkspace", "alg_irs_r");
    rebin->setPropertyValue("OutputWorkspace", "alg_irs_r");
    rebin->setPropertyValue("Params", "-0.5,0.005,0.5");
    TS_ASSERT_THROWS_NOTHING(rebin->execute());
    TS_ASSERT(rebin->isExecuted());
    delete rebin;

    MatrixWorkspace_sptr inputWS = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve("alg_irs_r"));

    alg = new ExtractFFTSpectrum();

    if ( !alg->isInitialized() )
    {
      alg->initialize();
    }

    TS_ASSERT_THROWS(alg->execute(), std::runtime_error);
    TS_ASSERT(!alg->isExecuted());

    TS_ASSERT_THROWS_NOTHING(alg->setPropertyValue("InputWorkspace", "alg_irs_r"));
    TS_ASSERT_THROWS_NOTHING(alg->setPropertyValue("OutputWorkspace", "alg_irs_t"));
    TS_ASSERT_THROWS_NOTHING(alg->execute());
    TS_ASSERT(alg->isExecuted());

    // Get output workspace
    MatrixWorkspace_const_sptr outputWS;
    TS_ASSERT_THROWS_NOTHING(outputWS = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve("alg_irs_t")));

    // Dimensions
    TS_ASSERT_EQUALS(inputWS->getNumberHistograms(), outputWS->getNumberHistograms());
    TS_ASSERT_EQUALS(inputWS->blocksize(), outputWS->blocksize());

    // Units ( Axis 1 should be the same, Axis 0 should be "Time/ns"
    TS_ASSERT_EQUALS(inputWS->getAxis(1)->unit(),outputWS->getAxis(1)->unit());
    TS_ASSERT_EQUALS(outputWS->getAxis(0)->unit()->caption(), "Time");
    TS_ASSERT_EQUALS(outputWS->getAxis(0)->unit()->label(), "ns");

    delete alg;
  }
private:
  ExtractFFTSpectrum* alg;

};
#endif // EXTRACTFFTSPECTRUM_H
