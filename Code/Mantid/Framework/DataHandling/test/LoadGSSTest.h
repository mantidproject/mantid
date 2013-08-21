#ifndef LOADGSSTEST_H_
#define LOADGSSTEST_H_

#include "cxxtest/TestSuite.h"
#include "MantidDataHandling/LoadGSS.h"
#include "MantidAPI/AlgorithmManager.h"
//#include "MantidAPI/AnalysisDataService.h"
//#include <Poco/File.h>
//#include <fstream>

using namespace Mantid;
using Mantid::DataHandling::LoadGSS;

class LoadGSSTest : public CxxTest::TestSuite
{
public:

  void test_TheBasics()
  {
    Mantid::DataHandling::LoadGSS loader;
    TS_ASSERT_THROWS_NOTHING( loader.initialize() )
    TS_ASSERT_EQUALS( loader.name(), "LoadGSS" )
    TS_ASSERT_EQUALS( loader.category(), "Diffraction;DataHandling\\Text" )
    TS_ASSERT_EQUALS( loader.version(), 1 )
  }

  void test_load_gss_txt()
  {
    API::IAlgorithm_sptr loader = createAlgorithm();
	loader->setPropertyValue("Filename","gss.txt");
    TS_ASSERT( loader->execute() )
    // Check a few things in the workspace
	checkWorkspace( loader->getProperty("OutputWorkspace"), 8, 816);
  }

  void test_load_gss_ExtendedHeader_gsa()
  {
    API::IAlgorithm_sptr loader = createAlgorithm();
	loader->setPropertyValue("Filename","gss-ExtendedHeader.gsa");
    TS_ASSERT( loader->execute() )
    // Check a few things in the workspace
	checkWorkspace( loader->getProperty("OutputWorkspace"), 1, 6);
  }

  /** Test LoadGSS with setting spectrum ID as bank ID
    */
  void test_load_gss_use_spec()
  {
    // Set property and execute
    LoadGSS loader;
    loader.initialize();

    loader.setPropertyValue("Filename","gss1.txt");
    loader.setProperty("OutputWorkspace", "TestWS");
    loader.setProperty("UseBankIDasSpectrumNumber", true);

    TS_ASSERT( loader.execute() );

    // Check result
    API::MatrixWorkspace_sptr outws = boost::dynamic_pointer_cast<API::MatrixWorkspace>(
          API::AnalysisDataService::Instance().retrieve("TestWS"));
    TS_ASSERT(outws);
    if (!outws)
      return;

    TS_ASSERT_EQUALS(outws->getNumberHistograms(), 3);
    if (outws->getNumberHistograms() != 3)
      return;

    TS_ASSERT_EQUALS(outws->getSpectrum(0)->getSpectrumNo(), 1);
    TS_ASSERT_EQUALS(outws->getSpectrum(1)->getSpectrumNo(), 3);
    TS_ASSERT_EQUALS(outws->getSpectrum(2)->getSpectrumNo(), 5);

    API::AnalysisDataService::Instance().remove("TestWS");
  }

  void test_fails_gracefully_if_passed_wrong_filetype()
  {
    API::IAlgorithm_sptr loader = createAlgorithm();
	loader->setPropertyValue("Filename","argus0026287.nxs");
    // Throws different exception type on different platforms!
    TS_ASSERT_THROWS_ANYTHING( loader->execute() )

    API::IAlgorithm_sptr loader2 = createAlgorithm();
	loader2->setPropertyValue("Filename","AsciiExample.txt");
    TS_ASSERT_THROWS( loader2->execute(), std::out_of_range )

    API::IAlgorithm_sptr loader3 = createAlgorithm();
	loader3->setPropertyValue("Filename","CSP79590.raw");
    TS_ASSERT_THROWS( loader3->execute(), std::out_of_range )

    API::IAlgorithm_sptr loader4 = createAlgorithm();
	loader4->setPropertyValue("Filename","VULCAN_2916_neutron0_event.dat");
    TS_ASSERT_THROWS( loader4->execute(), std::out_of_range )
  }

private:
  API::IAlgorithm_sptr createAlgorithm()
  {
    API::IAlgorithm_sptr alg = API::AlgorithmManager::Instance().createUnmanaged("LoadGSS");
    alg->initialize();
    alg->setChild(true);
    alg->setPropertyValue("OutputWorkspace","fakeName");
    return alg;
  }

  void checkWorkspace(const API::MatrixWorkspace_const_sptr & ws, int nHist, int nBins)
  {
    TS_ASSERT_EQUALS( ws->id(), "Workspace2D" )
    TS_ASSERT_EQUALS( ws->getNumberHistograms(), nHist )
    TS_ASSERT_EQUALS( ws->size(), nBins )
    TS_ASSERT_EQUALS( ws->getAxis(0)->unit()->unitID(), "TOF" )
  }

};


#endif //LOADGSSTEST_H_
