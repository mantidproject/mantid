#ifndef LOADGSSTEST_H_
#define LOADGSSTEST_H_

#include "cxxtest/TestSuite.h"
#include "MantidDataHandling/LoadGSS.h"
#include "MantidAPI/AlgorithmManager.h"
//#include "MantidAPI/AnalysisDataService.h"
//#include <Poco/File.h>
//#include <fstream>

using namespace Mantid;

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

  void test_fails_gracefully_if_passed_wrong_filetype()
  {
    API::IAlgorithm_sptr loader = createAlgorithm();
	loader->setPropertyValue("Filename","argus0026287.nxs");
    TS_ASSERT_THROWS( loader->execute(), Kernel::Exception::FileError )

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
