#ifndef LOADSAVUTOMOCONFIGTEST_H_
#define LOADSAVUTOMOCONFIGTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidDataHandling/LoadSavuTomoConfig.h"

using namespace Mantid::API;
using Mantid::DataHandling::LoadSavuTomoConfig;

class LoadSavuTomoConfigTest : public CxxTest::TestSuite
{
public: 
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static LoadSavuTomoConfigTest *createSuite() { return new LoadSavuTomoConfigTest(); }
  static void destroySuite(LoadSavuTomoConfigTest *suite) { delete suite; }

  /// Tests casting, general algorithm properties: name, version, etc.
  void test_algorithm()
  {
    testAlg =
      Mantid::API::AlgorithmManager::Instance().create("LoadSavuTomoConfig" /*, 1*/);
    TS_ASSERT( testAlg );
    TS_ASSERT_EQUALS( testAlg->name(), "LoadSavuTomoConfig" );
    TS_ASSERT_EQUALS( testAlg->version(), 1 );
  }

  void test_init()
  {
    if (!testAlg->isInitialized())
      testAlg->initialize();

    TS_ASSERT_THROWS_NOTHING( testAlg->initialize() );
    TS_ASSERT( testAlg->isInitialized() );
  }

  void test_wrongExec()
  {
    IAlgorithm_sptr testFail =
      Mantid::API::AlgorithmManager::Instance().create("LoadSavuTomoConfig" /*, 1*/);
    TS_ASSERT( testFail );
    TS_ASSERT_THROWS_NOTHING( testFail->initialize() );
    // exec without Filename property set -> should throw
    TS_ASSERT_THROWS( testFail->execute(), std::runtime_error );
    // try to set empty filename
    TS_ASSERT_THROWS( testFail->setPropertyValue("Filename",""), std::invalid_argument );
    TS_ASSERT( !testFail->isExecuted() );

    // exec with Filename but empty OutputWorkspace -> should throw
    IAlgorithm_sptr fail2 =
      Mantid::API::AlgorithmManager::Instance().create("LoadSavuTomoConfig" /*, 1*/);
    TS_ASSERT_THROWS_NOTHING( fail2->initialize() );
    TS_ASSERT_THROWS_NOTHING( fail2->setPropertyValue("Filename", testFilename) );
    TS_ASSERT_THROWS( fail2->setPropertyValue("OutputWorkspace", ""), std::invalid_argument );
    TS_ASSERT_THROWS( fail2->execute(), std::runtime_error );
    TS_ASSERT( !fail2->isExecuted() );

    // exec with Filename but no OutputWorkspace -> should not finish
    IAlgorithm_sptr fail3 =
      Mantid::API::AlgorithmManager::Instance().create("LoadSavuTomoConfig" /*, 1*/);
    TS_ASSERT_THROWS_NOTHING( fail3->initialize() );
    TS_ASSERT_THROWS_NOTHING( fail3->setPropertyValue("Filename", testFilename) );
    TS_ASSERT_THROWS_NOTHING( fail3->execute() );
    TS_ASSERT( !fail2->isExecuted() );
  }

  // one file with errors/unrecognized content
  void test_wrongContentsFile()
  {
    // TODO: wait until we have a final spec of the format, then try to fool
    // the loader here
  }

  // one example file that should load fine
  void test_loadOK()
  {
    // Uses examples from the savu repository:
    // https://github.com/DiamondLightSource/Savu/tree/master/test_data

    // TODO: At the moment load just one file to test basic
    // functionality. Probably more files should be added here as we
    // have more certainty about the format
    std::string outWSName = "LoadSavuTomoConfig_test_ws";
    // Load examples from https://github.com/DiamondLightSource/Savu/tree/master/test_data
    TS_ASSERT_THROWS_NOTHING( testAlg->setPropertyValue("Filename", testFilename) );
    TS_ASSERT_THROWS_NOTHING( testAlg->setPropertyValue("OutputWorkspace", outWSName) );

    if (!testAlg->isInitialized())
      testAlg->initialize();
    TS_ASSERT( testAlg->isInitialized() );

    TS_ASSERT_THROWS_NOTHING( testAlg->execute() );
    TS_ASSERT( testAlg->isExecuted() );

    AnalysisDataServiceImpl &ads = AnalysisDataService::Instance();

    TS_ASSERT( ads.doesExist(outWSName) );
    ITableWorkspace_sptr ws;
    TS_ASSERT_THROWS_NOTHING(ws = ads.retrieveWS<ITableWorkspace>(outWSName) );

    // general format: 3 columns (data, id, name)
    TS_ASSERT_EQUALS( ws->columnCount(), 4) ;
    TS_ASSERT_EQUALS( ws->rowCount(), 3 );

    checkColumns(ws);

    // this example has 3 plugins: savu.plugins.timeseries_fields_corrections, savu.median_filter,
    // savu.plugins.simple_recon
    // ID
    TS_ASSERT_EQUALS( ws->cell<std::string>(0, 0), "savu.plugins.timeseries_field_corrections" );
    TS_ASSERT_EQUALS( ws->cell<std::string>(1, 0), "savu.plugins.median_filter" );
    TS_ASSERT_EQUALS( ws->cell<std::string>(2, 0), "savu.plugins.simple_recon" );

    // data entry in NeXus file (Params column)
    TS_ASSERT_EQUALS( ws->cell<std::string>(0, 1), "{}" );
    TS_ASSERT_EQUALS( ws->cell<std::string>(1, 1), "{\"kernel_size\": [1, 3, 3]}" );
    TS_ASSERT_EQUALS( ws->cell<std::string>(2, 1), "{\"center_of_rotation\": 86}" );

    // name entry in NeXus file
    TS_ASSERT_EQUALS( ws->cell<std::string>(0, 2), "Timeseries Field Corrections" );
    TS_ASSERT_EQUALS( ws->cell<std::string>(1, 2), "Median Filter" );
    TS_ASSERT_EQUALS( ws->cell<std::string>(2, 2), "Simple Reconstruction" );

    // cite information, not presently available in example files
    for (size_t i=0; i<nRows; i++) {
      TS_ASSERT_EQUALS( ws->cell<std::string>(i, 3), "Not available" );
    }
  }

private:

  void checkColumns(ITableWorkspace_sptr& table)
  {
    // each row of the workspace should have: ID, Params, Name, Cite
    std::vector<std::string> names = table->getColumnNames();
    TS_ASSERT_EQUALS( names.size(), 4 );
    TS_ASSERT_EQUALS( names[0], "ID" );
    TS_ASSERT_EQUALS( names[1], "Params" );
    TS_ASSERT_EQUALS( names[2], "Name" );
    TS_ASSERT_EQUALS( names[3], "Cite" );
  }

  IAlgorithm_sptr testAlg;
  static const size_t nRows;
  static const size_t nCols;
  static const std::string testFilename;

};

const size_t LoadSavuTomoConfigTest::nRows = 3;
const size_t LoadSavuTomoConfigTest::nCols = 4;
const std::string LoadSavuTomoConfigTest::testFilename = "savu_test_data_process03.nxs";

#endif /* LOADSAVUTOMOCONFIGTEST_H__*/
