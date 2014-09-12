#ifndef LOADTEST_H_
#define LOADTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidDataHandling/Load.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/AlgorithmManager.h"

#include <boost/algorithm/string/predicate.hpp> //for ends_with

using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::DataHandling;

class LoadTest : public CxxTest::TestSuite
{
public:
  void tearDown()
  {
    AnalysisDataService::Instance().clear();
  }

  void testViaProxy()
  {
    IAlgorithm_sptr proxy = AlgorithmManager::Instance().create("Load");
    TS_ASSERT_EQUALS(proxy->existsProperty("Filename"), true);
    TS_ASSERT_EQUALS(proxy->existsProperty("OutputWorkspace"), true);
    
    TS_ASSERT_THROWS_NOTHING(proxy->setPropertyValue("Filename","IRS38633.raw"));
    TS_ASSERT_EQUALS(proxy->existsProperty("Cache"), true);
    TS_ASSERT_EQUALS(proxy->existsProperty("LoadLogFiles"), true);

    proxy->setPropertyValue("Filename","IRS38633.raw");
    TS_ASSERT_EQUALS(proxy->existsProperty("Cache"), true);
    TS_ASSERT_EQUALS(proxy->existsProperty("LoadLogFiles"), true);

    TS_ASSERT_THROWS_NOTHING(proxy->setPropertyValue("Filename","LOQ49886.nxs"));   
    TS_ASSERT_EQUALS(proxy->existsProperty("Cache"), false);
    TS_ASSERT_EQUALS(proxy->existsProperty("LoadLogFiles"), false);
  }

  void testPropertyValuesViaProxy()
  {
    IAlgorithm_sptr proxy = AlgorithmManager::Instance().create("Load");
    TS_ASSERT_EQUALS(proxy->existsProperty("Filename"), true);
    TS_ASSERT_EQUALS(proxy->existsProperty("OutputWorkspace"), true);
    
    TS_ASSERT_THROWS_NOTHING(proxy->setPropertyValue("Filename","IRS38633.raw"));
    TS_ASSERT_EQUALS(proxy->existsProperty("Cache"), true);
    TS_ASSERT_EQUALS(proxy->existsProperty("LoadLogFiles"), true);
    TS_ASSERT_THROWS_NOTHING(proxy->setPropertyValue("SpectrumMin","10"));
    TS_ASSERT_THROWS_NOTHING(proxy->setPropertyValue("SpectrumMax","100"));

    // Test that the properties have the correct values
    TS_ASSERT_EQUALS(proxy->getPropertyValue("SpectrumMin"),"10");
    TS_ASSERT_EQUALS(proxy->getPropertyValue("SpectrumMax"),"100");
  }

  void testSwitchingLoaderViaProxy()
  {    
    IAlgorithm_sptr proxy = AlgorithmManager::Instance().create("Load");
    TS_ASSERT_EQUALS(proxy->existsProperty("Filename"), true);
    TS_ASSERT_EQUALS(proxy->existsProperty("OutputWorkspace"), true);
    TS_ASSERT_THROWS_NOTHING(proxy->setPropertyValue("Filename","IRS38633.raw"));
    TS_ASSERT_EQUALS(proxy->existsProperty("Cache"), true);
    TS_ASSERT_EQUALS(proxy->existsProperty("LoadLogFiles"), true);

    TS_ASSERT_THROWS_NOTHING(proxy->setPropertyValue("SpectrumMin","10"));
    TS_ASSERT_THROWS_NOTHING(proxy->setPropertyValue("SpectrumMax","100"));

    // Test that the properties have the correct values
    TS_ASSERT_EQUALS(proxy->getPropertyValue("SpectrumMin"),"10");
    TS_ASSERT_EQUALS(proxy->getPropertyValue("SpectrumMax"),"100");

    // Change loader
    proxy->setPropertyValue("Filename","LOQ49886.nxs");
    TS_ASSERT_EQUALS(proxy->existsProperty("EntryNumber"), true);
    TS_ASSERT_EQUALS(proxy->existsProperty("Cache"), false);

    TS_ASSERT_THROWS_NOTHING(proxy->setPropertyValue("SpectrumMin","11"));
    TS_ASSERT_THROWS_NOTHING(proxy->setPropertyValue("SpectrumMax","101"));

    TS_ASSERT_EQUALS(proxy->getPropertyValue("SpectrumMin"),"11");
    TS_ASSERT_EQUALS(proxy->getPropertyValue("SpectrumMax"),"101");

  }

  void testFindLoader()
  {
    Load loader;
    loader.initialize();
    static const size_t NUMPROPS = 5;
    const char * loadraw_props[NUMPROPS] = {"SpectrumMin", "SpectrumMax", "SpectrumList", "Cache", "LoadLogFiles"};
    // Basic load has no additional loader properties
    for( size_t i = 0; i < NUMPROPS ; ++i )
    {
      TS_ASSERT_EQUALS(loader.existsProperty(loadraw_props[i]), false);
    }
    //After setting the file property, the algorithm should have aquired the appropriate properties
    TS_ASSERT_THROWS_NOTHING(loader.setPropertyValue("Filename","IRS38633.raw"));
    // Now
    for( size_t i = 0; i < NUMPROPS; ++i )
    {
      TS_ASSERT_EQUALS(loader.existsProperty(loadraw_props[i]), true);
    }

    // Did it find the right loader
    TS_ASSERT_EQUALS(loader.getPropertyValue("LoaderName"), "LoadRaw");
  }

  void test_Comma_Separated_List_Finds_Correct_Number_Of_Files()
  {
    Load loader;
    loader.initialize();
    loader.setPropertyValue("Filename", "MUSR15189,15190,15191.nxs");

    std::vector<std::vector<std::string> > foundFiles = loader.getProperty("Filename");

    // Outer vector holds separate lists of files to be summed together
    // In this case no summing required
    TS_ASSERT_EQUALS(3, foundFiles.size());
    // Inner vector holds files to be summed
    TS_ASSERT_EQUALS(1, foundFiles[0].size());
    TS_ASSERT_EQUALS(1, foundFiles[1].size());
    TS_ASSERT_EQUALS(1, foundFiles[2].size());
  }

  void test_Plus_Operator_Finds_Correct_Number_Of_Files()
  {
    Load loader;
    loader.initialize();
    loader.setPropertyValue("Filename", "IRS38633+38633.nxs");

    std::vector<std::vector<std::string> > foundFiles = loader.getProperty("Filename");

    // Outer vector holds separate lists of files to be summed together
    TS_ASSERT_EQUALS(1, foundFiles.size());
    // Inner vector holds files to be summed
    TS_ASSERT_EQUALS(2, foundFiles[0].size());
  }

  void test_Range_Operator_Finds_Correct_Number_Of_Files()
  {
    Load loader;
    loader.initialize();
    loader.setPropertyValue("Filename", "MUSR15189:15192.nxs");

    std::vector<std::vector<std::string> > foundFiles = loader.getProperty("Filename");

    // Outer vector holds separate lists of files to be summed together
    // In this case no summing required
    TS_ASSERT_EQUALS(4, foundFiles.size());
    // Inner vector holds files to be summed
    TS_ASSERT_EQUALS(1, foundFiles[0].size());
    TS_ASSERT_EQUALS(1, foundFiles[1].size());
    TS_ASSERT_EQUALS(1, foundFiles[2].size());
    TS_ASSERT_EQUALS(1, foundFiles[3].size());
  }

  void test_Stepped_Range_Operator_Finds_Correct_Number_Of_Files()
  {
    Load loader;
    loader.initialize();
    loader.setPropertyValue("Filename", "MUSR15189:15192:2.nxs");

    std::vector<std::vector<std::string> > foundFiles = loader.getProperty("Filename");

    // Outer vector holds separate lists of files to be summed together
    // In this case no summing required
    TS_ASSERT_EQUALS(2, foundFiles.size());
    // Inner vector holds files to be summed
    TS_ASSERT_EQUALS(1, foundFiles[0].size());
    TS_ASSERT_EQUALS(1, foundFiles[1].size());

    // Check it has found the correct two
    const std::string first = foundFiles[0][0];
    TSM_ASSERT(std::string("Incorrect first file has been found: ") + first, boost::algorithm::ends_with(first, "MUSR00015189.nxs"));
    const std::string second = foundFiles[1][0];
    TSM_ASSERT(std::string("Incorrect second file has been found") + second, boost::algorithm::ends_with(second, "MUSR00015191.nxs"));

    // A more through test of the loading and value checking is done in the LoadTest.py system test
  }

  void test_Added_Range_Operator_Finds_Correct_Number_Of_Files()
  {
    Load loader;
    loader.initialize();
    loader.setPropertyValue("Filename", "MUSR15189-15192.nxs");

    std::vector<std::vector<std::string> > foundFiles = loader.getProperty("Filename");

    // Outer vector holds separate lists of files to be summed together
    // In this case no summing required
    TS_ASSERT_EQUALS(1, foundFiles.size());
    // Inner vector holds files to be summed
    TS_ASSERT_EQUALS(4, foundFiles[0].size());

    // Check it has found the correct two
    const std::string first = foundFiles[0][0];
    TSM_ASSERT(std::string("Incorrect first file has been found: ") + first, boost::algorithm::ends_with(first, "MUSR00015189.nxs"));
    const std::string last = foundFiles[0][3];
    TSM_ASSERT(std::string("Incorrect last file has been found") + last, boost::algorithm::ends_with(last, "MUSR00015192.nxs"));

    // A more through test of the loading and value checking is done in the LoadTest.py system test
  }

  void test_Comma_Separated_List_Of_Different_Intruments_Finds_Correct_Files()
  {
    Load loader;
    loader.initialize();
    loader.setPropertyValue("Filename", "LOQ48127.raw, CSP79590.raw");

    std::vector<std::vector<std::string> > foundFiles = loader.getProperty("Filename");

    // Outer vector holds separate lists of files to be summed together
    // In this case no summing required
    TS_ASSERT_EQUALS(2, foundFiles.size());
    // Inner vector holds files to be summed
    TS_ASSERT_EQUALS(1, foundFiles[0].size());
    TS_ASSERT_EQUALS(1, foundFiles[1].size());

    // Check it has found the correct two
    const std::string first = foundFiles[0][0];
    TSM_ASSERT(std::string("Incorrect first file has been found: ") + first, boost::algorithm::ends_with(first, "LOQ48127.raw"));
    const std::string second = foundFiles[1][0];
    TSM_ASSERT(std::string("Incorrect second file has been found") + second, boost::algorithm::ends_with(second, "CSP79590.raw"));
  }

  void test_EventPreNeXus_WithNoExecute()
  {
    Load loader;
    loader.initialize();
    TS_ASSERT_THROWS_NOTHING(loader.setPropertyValue("Filename", "CNCS_7860_neutron_event.dat"));
    TS_ASSERT_EQUALS(loader.existsProperty("EventFilename"), false);
    TS_ASSERT_EQUALS(loader.getPropertyValue("LoaderName"), "LoadEventPreNexus");
  }

  void test_SNSEventNeXus_WithNoExecute()
  {
    Load loader;
    loader.initialize();
    TS_ASSERT_THROWS_NOTHING(loader.setPropertyValue("Filename", "CNCS_7860_event.nxs"));
    TS_ASSERT_EQUALS(loader.existsProperty("EventFilename"), false);
    TS_ASSERT_EQUALS(loader.getPropertyValue("LoaderName"), "LoadEventNexus");
  }

  void testArgusFileWithIncorrectZeroPadding_NoExecute()
  {
    Load loader;
    loader.initialize();
    loader.setPropertyValue("Filename", "argus0026287.nxs");
    TS_ASSERT_EQUALS(loader.getPropertyValue("LoaderName"), "LoadMuonNexus");
  }

};


//-------------------------------------------------------------------------------------------------
// Performance test
//
// This simple checks how long it takes to run the search for a Loader, which is done when
// the file property is set
//-------------------------------------------------------------------------------------------------

class LoadTestPerformance : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static LoadTestPerformance *createSuite() { return new LoadTestPerformance(); }
  static void destroySuite( LoadTestPerformance *suite ) { delete suite; }

  void test_find_loader_performance()
  {
    const size_t ntimes(5);

    for(size_t i = 0; i < ntimes; ++i)
    {
      Mantid::DataHandling::Load loader;
      loader.initialize();
      loader.setPropertyValue("Filename", "CNCS_7860_event.nxs");
    }
  }
};


#endif /*LOADTEST_H_*/
