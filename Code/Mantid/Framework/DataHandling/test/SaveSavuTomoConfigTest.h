#ifndef SAVETOMOCONFIGTEST_H_
#define SAVETOMOCONFIGTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidDataHandling/SaveTomoConfig.h"

#include <Poco/File.h>

using namespace Mantid::API;
using namespace Mantid::DataHandling;

class SaveTomoConfigTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static SaveTomoConfigTest *createSuite() { return new SaveTomoConfigTest(); }
  static void destroySuite(SaveTomoConfigTest *suite) { delete suite; }

  /// Tests casting, general algorithm properties: name, version, etc.
  void test_algorithm()
  {
    testSave =
      Mantid::API::AlgorithmManager::Instance().create("SaveTomoConfig" /*, 1*/);
    TS_ASSERT( testSave );
    TS_ASSERT_EQUALS( testSave->name(), "SaveTomoConfig" );
    TS_ASSERT_EQUALS( testSave->version(), 1 );
  }

  void test_init()
  {
    if (!testSave->isInitialized())
      testSave->initialize();

    TS_ASSERT_THROWS_NOTHING( testSave->initialize() );
    TS_ASSERT( testSave->isInitialized() );
  }

  /// this is the first test that uses the good workspace (wsName) so it creates it
  void test_wrongExec()
  {
    ITableWorkspace_sptr ws = makeTableWorkspace(wsName);

    IAlgorithm_sptr testFail =
      Mantid::API::AlgorithmManager::Instance().create("SaveTomoConfig" /*, 1*/);
    TS_ASSERT( testFail );
    TS_ASSERT_THROWS_NOTHING( testFail->initialize() );
    // exec without InputWorkspaces property set -> should throw
    TS_ASSERT_THROWS( testFail->execute(), std::runtime_error );
    // try to set empty InputWorkspaces
    TS_ASSERT_THROWS( testFail->setPropertyValue("InputWorkspaces",""), std::invalid_argument );
    TS_ASSERT( !testFail->isExecuted() );

    // exec with InputWorkspaces but empty Filename -> should throw
    IAlgorithm_sptr fail2 =
      Mantid::API::AlgorithmManager::Instance().create("SaveTomoConfig" /*, 1*/);
    TS_ASSERT_THROWS_NOTHING( fail2->initialize() );
    TS_ASSERT_THROWS_NOTHING( fail2->setPropertyValue("InputWorkspaces", wsName) );
    TS_ASSERT_THROWS( fail2->setPropertyValue("Filename", ""), std::invalid_argument );
    TS_ASSERT_THROWS( fail2->execute(), std::runtime_error );
    TS_ASSERT( !fail2->isExecuted() );

    // exec with InputWorkspaces but no Filename -> should throw
    IAlgorithm_sptr fail3 =
      Mantid::API::AlgorithmManager::Instance().create("SaveTomoConfig" /*, 1*/);
    TS_ASSERT_THROWS_NOTHING( fail3->initialize() );
    TS_ASSERT_THROWS_NOTHING( fail3->setPropertyValue("InputWorkspaces", wsName) );
    TS_ASSERT_THROWS( fail3->execute(), std::runtime_error );
  }

  void test_wrongTableFormat()
  {
    std::string badWSName = "bad_table";
    ITableWorkspace_sptr ws = makeTableWorkspace(badWSName);

    // using wrong table: should fail
    IAlgorithm_sptr fail =
      Mantid::API::AlgorithmManager::Instance().create("SaveTomoConfig" /*, 1*/);
    TS_ASSERT_THROWS_NOTHING( fail->initialize() );
    TS_ASSERT_THROWS_NOTHING( fail->setPropertyValue("InputWorkspaces", wsName) );
    TS_ASSERT_THROWS_NOTHING( fail->setPropertyValue("Filename", outFilename) );
    TS_ASSERT_THROWS_NOTHING( fail->execute() );
    TS_ASSERT( !fail->isExecuted() );

    AnalysisDataService::Instance().remove(badWSName);
  }

  /// this is the last test that uses the good workspace (wsName) so it removes it from teh ADS
  void test_saveOK()
  {
    TS_ASSERT( testSave->isInitialized() );
    TS_ASSERT_THROWS_NOTHING( testSave->setPropertyValue("InputWorkspaces", wsName) );
    TS_ASSERT_THROWS_NOTHING( testSave->setPropertyValue("Filename", outFilename) );
    TS_ASSERT_THROWS_NOTHING( testSave->execute() );
    TS_ASSERT( testSave->isExecuted() );

    // very basic test, to do more than this use the sibling LoadSavuTomo algorithm
    TS_ASSERT( Poco::File(outFilename).exists() );
    boost::shared_ptr<NeXus::File> file;
    // can open as NeXus and find one of the entries
    TS_ASSERT_THROWS_NOTHING( file = boost::make_shared<NeXus::File>(outFilename) );
    TS_ASSERT_THROWS_NOTHING( file->openPath("entry/process/0") );
    TS_ASSERT_THROWS_NOTHING( file->close() );

    cleanup();
  }

private:
  /// removes the output file and the input workspace
  void cleanup()
  {
    TS_ASSERT_THROWS_NOTHING( Poco::File(outFilename).remove() );
    TS_ASSERT_THROWS_NOTHING( AnalysisDataService::Instance().remove(wsName) );
  }

  /// helper: specific table format here for savu pipeline configurations
  ITableWorkspace_sptr makeTableWorkspace(const std::string &name)
  {
    ITableWorkspace_sptr ws = WorkspaceFactory::Instance().createTable();
    AnalysisDataService::Instance().addOrReplace(name, ws);
    ws->addColumn("str","ID");
    ws->addColumn("str","Name");
    ws->addColumn("str","Parameters");
    ws->addColumn("str","Cite");

    Mantid::API::TableRow row = ws->appendRow();
    row << "savu.id1" << "{\"param1\": val1}" << "name 1" << "cite 1";
    row = ws->appendRow();
    row << "savu.id2" << "{\"param2\": val2}" << "name 2" << "cite 2";

    return ws;
  }

  // helper to build a bad table: intentionally forgets to add some columns
  ITableWorkspace_sptr makeWrongTableWorkspace(const std::string &name)
  {
    ITableWorkspace_sptr ws = WorkspaceFactory::Instance().createTable();
    AnalysisDataService::Instance().addOrReplace(name, ws);
    ws->addColumn("str","ID");
    ws->addColumn("str","Name");

    Mantid::API::TableRow row = ws->appendRow();
    row << "savu.id1" << "{\"param1\": val1}";
    row = ws->appendRow();
    row << "savu.id2" << "{\"param2\": val2}";

    return ws;
  }

  IAlgorithm_sptr testSave;
  static const std::string outFilename;
  static const std::string wsName;
};

const std::string SaveTomoConfigTest::wsName = "simple_table";
const std::string SaveTomoConfigTest::outFilename = "savu_tomo_save_test.nxs";

#endif /* SAVETOMOCONFIGTEST_H */
