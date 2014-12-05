#ifndef MANTID_DATAHANDLING_SORTTABLEWORKSPACETEST_H_
#define MANTID_DATAHANDLING_SORTTABLEWORKSPACETEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidDataHandling/SortTableWorkspace.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/TableRow.h"
#include "MantidDataObjects/TableColumn.h"

using Mantid::DataHandling::SortTableWorkspace;
using namespace Mantid::API;

class SortTableWorkspaceTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static SortTableWorkspaceTest *createSuite() { return new SortTableWorkspaceTest(); }
  static void destroySuite( SortTableWorkspaceTest *suite ) { delete suite; }


  void test_Init()
  {
    SortTableWorkspace alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
  }
  
  void test_exec()
  {

    auto ws = WorkspaceFactory::Instance().createTable();
    ws->addColumn("int","x");
    ws->addColumn("str","y");
    ws->addColumn("double","z");

    TableRow row = ws->appendRow();
    row << 3 << "three (3)" << 0.0;
    row = ws->appendRow();
    row << 1 << "one (3)" << 1.0;
    row = ws->appendRow();
    row << 1 << "one (2)" << 2.0;
    row = ws->appendRow();
    row << 2 << "two (1)" << 3.0;
    row = ws->appendRow();
    row << 3 << "three (2)" << 4.0;
    row = ws->appendRow();
    row << 3 << "three (2)" << 5.0;
    row = ws->appendRow();
    row << 2 << "two (2)" << 6.0;
    row = ws->appendRow();
    row << 1 << "one (1)" << 7.0;
    row = ws->appendRow();
    row << 2 << "two (1)" << 8.0;
    row = ws->appendRow();
    row << 2 << "two (2)" << 9.0;


    // Name of the output workspace.
    std::string outWSName("SortTableWorkspaceTest_OutputWS");

    std::vector<std::string> columns(3);
    columns[0] = "x";
    columns[1] = "y";
    columns[2] = "z";
    std::vector<int> ascending(3);
    ascending[0] = 1;
    ascending[1] = 0;
    ascending[2] = 1;
  
    SortTableWorkspace alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("InputWorkspace", ws) );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("OutputWorkspace", outWSName) );
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("Columns", columns) );
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("Ascending", ascending) );
    TS_ASSERT_THROWS_NOTHING( alg.execute(); );
    TS_ASSERT( alg.isExecuted() );
    
    ITableWorkspace_sptr outws;
    TS_ASSERT_THROWS_NOTHING( outws = AnalysisDataService::Instance().retrieveWS<ITableWorkspace>(outWSName) );
    TS_ASSERT(outws);
    if (!outws) return;

    auto &data1 = static_cast<Mantid::DataObjects::TableColumn<int>&>(*outws->getColumn("x")).data();
    auto &data2 = static_cast<Mantid::DataObjects::TableColumn<std::string>&>(*outws->getColumn("y")).data();
    auto &data3 = static_cast<Mantid::DataObjects::TableColumn<double>&>(*outws->getColumn("z")).data();

    TS_ASSERT_EQUALS( data1[0], 1 );
    TS_ASSERT_EQUALS( data1[1], 1 );
    TS_ASSERT_EQUALS( data1[2], 1 );
    TS_ASSERT_EQUALS( data1[3], 2 );
    TS_ASSERT_EQUALS( data1[4], 2 );
    TS_ASSERT_EQUALS( data1[5], 2 );
    TS_ASSERT_EQUALS( data1[6], 2 );
    TS_ASSERT_EQUALS( data1[7], 3 );
    TS_ASSERT_EQUALS( data1[8], 3 );
    TS_ASSERT_EQUALS( data1[9], 3 );

    TS_ASSERT_EQUALS( data2[0], "one (3)" );
    TS_ASSERT_EQUALS( data2[1], "one (2)" );
    TS_ASSERT_EQUALS( data2[2], "one (1)" );
    TS_ASSERT_EQUALS( data2[3], "two (2)" );
    TS_ASSERT_EQUALS( data2[4], "two (2)" );
    TS_ASSERT_EQUALS( data2[5], "two (1)" );
    TS_ASSERT_EQUALS( data2[6], "two (1)" );
    TS_ASSERT_EQUALS( data2[7], "three (3)" );
    TS_ASSERT_EQUALS( data2[8], "three (2)" );
    TS_ASSERT_EQUALS( data2[9], "three (2)" );

    TS_ASSERT_EQUALS( data3[0], 1);
    TS_ASSERT_EQUALS( data3[1], 2);
    TS_ASSERT_EQUALS( data3[2], 7);
    TS_ASSERT_EQUALS( data3[3], 6);
    TS_ASSERT_EQUALS( data3[4], 9);
    TS_ASSERT_EQUALS( data3[5], 3);
    TS_ASSERT_EQUALS( data3[6], 8);
    TS_ASSERT_EQUALS( data3[7], 0);
    TS_ASSERT_EQUALS( data3[8], 4);
    TS_ASSERT_EQUALS( data3[9], 5);

    // Remove workspace from the data service.
    AnalysisDataService::Instance().clear();
  }
  
  void test_ascending()
  {

    auto ws = WorkspaceFactory::Instance().createTable();
    ws->addColumn("int","x");
    ws->addColumn("str","y");
    ws->addColumn("double","z");

    TableRow row = ws->appendRow();
    row << 3 << "three (3)" << 0.0;
    row = ws->appendRow();
    row << 1 << "one (3)" << 1.0;
    row = ws->appendRow();
    row << 1 << "one (2)" << 2.0;
    row = ws->appendRow();
    row << 2 << "two (1)" << 3.0;
    row = ws->appendRow();
    row << 3 << "three (2)" << 4.0;
    row = ws->appendRow();
    row << 3 << "three (2)" << 5.0;
    row = ws->appendRow();
    row << 2 << "two (2)" << 6.0;
    row = ws->appendRow();
    row << 1 << "one (1)" << 7.0;
    row = ws->appendRow();
    row << 2 << "two (1)" << 8.0;
    row = ws->appendRow();
    row << 2 << "two (2)" << 9.0;


    // Name of the output workspace.
    std::string outWSName("SortTableWorkspaceTest_OutputWS");

    std::vector<std::string> columns(3);
    columns[0] = "x";
    columns[1] = "y";
    columns[2] = "z";
    std::vector<int> ascending(1);
    ascending[0] = 1;
  
    SortTableWorkspace alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("InputWorkspace", ws) );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("OutputWorkspace", outWSName) );
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("Columns", columns) );
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("Ascending", ascending) );
    TS_ASSERT_THROWS_NOTHING( alg.execute(); );
    TS_ASSERT( alg.isExecuted() );
    
    ITableWorkspace_sptr outws;
    TS_ASSERT_THROWS_NOTHING( outws = AnalysisDataService::Instance().retrieveWS<ITableWorkspace>(outWSName) );
    TS_ASSERT(outws);
    if (!outws) return;

    auto &data1 = static_cast<Mantid::DataObjects::TableColumn<int>&>(*outws->getColumn("x")).data();
    auto &data2 = static_cast<Mantid::DataObjects::TableColumn<std::string>&>(*outws->getColumn("y")).data();
    auto &data3 = static_cast<Mantid::DataObjects::TableColumn<double>&>(*outws->getColumn("z")).data();

    TS_ASSERT_EQUALS( data1[0], 1 );
    TS_ASSERT_EQUALS( data1[1], 1 );
    TS_ASSERT_EQUALS( data1[2], 1 );
    TS_ASSERT_EQUALS( data1[3], 2 );
    TS_ASSERT_EQUALS( data1[4], 2 );
    TS_ASSERT_EQUALS( data1[5], 2 );
    TS_ASSERT_EQUALS( data1[6], 2 );
    TS_ASSERT_EQUALS( data1[7], 3 );
    TS_ASSERT_EQUALS( data1[8], 3 );
    TS_ASSERT_EQUALS( data1[9], 3 );

    TS_ASSERT_EQUALS( data2[0], "one (1)" );
    TS_ASSERT_EQUALS( data2[1], "one (2)" );
    TS_ASSERT_EQUALS( data2[2], "one (3)" );
    TS_ASSERT_EQUALS( data2[3], "two (1)" );
    TS_ASSERT_EQUALS( data2[4], "two (1)" );
    TS_ASSERT_EQUALS( data2[5], "two (2)" );
    TS_ASSERT_EQUALS( data2[6], "two (2)" );
    TS_ASSERT_EQUALS( data2[7], "three (2)" );
    TS_ASSERT_EQUALS( data2[8], "three (2)" );
    TS_ASSERT_EQUALS( data2[9], "three (3)" );

    TS_ASSERT_EQUALS( data3[0], 7);
    TS_ASSERT_EQUALS( data3[1], 2);
    TS_ASSERT_EQUALS( data3[2], 1);
    TS_ASSERT_EQUALS( data3[3], 3);
    TS_ASSERT_EQUALS( data3[4], 8);
    TS_ASSERT_EQUALS( data3[5], 6);
    TS_ASSERT_EQUALS( data3[6], 9);
    TS_ASSERT_EQUALS( data3[7], 4);
    TS_ASSERT_EQUALS( data3[8], 5);
    TS_ASSERT_EQUALS( data3[9], 0);

    // Remove workspace from the data service.
    AnalysisDataService::Instance().clear();
  }

  void test_default()
  {

    auto ws = WorkspaceFactory::Instance().createTable();
    ws->addColumn("int","x");
    ws->addColumn("str","y");
    ws->addColumn("double","z");

    TableRow row = ws->appendRow();
    row << 3 << "three (3)" << 0.0;
    row = ws->appendRow();
    row << 1 << "one (3)" << 1.0;
    row = ws->appendRow();
    row << 1 << "one (2)" << 2.0;
    row = ws->appendRow();
    row << 2 << "two (1)" << 3.0;
    row = ws->appendRow();
    row << 3 << "three (2)" << 4.0;
    row = ws->appendRow();
    row << 3 << "three (2)" << 5.0;
    row = ws->appendRow();
    row << 2 << "two (2)" << 6.0;
    row = ws->appendRow();
    row << 1 << "one (1)" << 7.0;
    row = ws->appendRow();
    row << 2 << "two (1)" << 8.0;
    row = ws->appendRow();
    row << 2 << "two (2)" << 9.0;


    // Name of the output workspace.
    std::string outWSName("SortTableWorkspaceTest_OutputWS");

    std::vector<std::string> columns(3);
    columns[0] = "x";
    columns[1] = "y";
    columns[2] = "z";
  
    SortTableWorkspace alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("InputWorkspace", ws) );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("OutputWorkspace", outWSName) );
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("Columns", columns) );
    TS_ASSERT_THROWS_NOTHING( alg.execute(); );
    TS_ASSERT( alg.isExecuted() );
    
    ITableWorkspace_sptr outws;
    TS_ASSERT_THROWS_NOTHING( outws = AnalysisDataService::Instance().retrieveWS<ITableWorkspace>(outWSName) );
    TS_ASSERT(outws);
    if (!outws) return;

    auto &data1 = static_cast<Mantid::DataObjects::TableColumn<int>&>(*outws->getColumn("x")).data();
    auto &data2 = static_cast<Mantid::DataObjects::TableColumn<std::string>&>(*outws->getColumn("y")).data();
    auto &data3 = static_cast<Mantid::DataObjects::TableColumn<double>&>(*outws->getColumn("z")).data();

    TS_ASSERT_EQUALS( data1[0], 1 );
    TS_ASSERT_EQUALS( data1[1], 1 );
    TS_ASSERT_EQUALS( data1[2], 1 );
    TS_ASSERT_EQUALS( data1[3], 2 );
    TS_ASSERT_EQUALS( data1[4], 2 );
    TS_ASSERT_EQUALS( data1[5], 2 );
    TS_ASSERT_EQUALS( data1[6], 2 );
    TS_ASSERT_EQUALS( data1[7], 3 );
    TS_ASSERT_EQUALS( data1[8], 3 );
    TS_ASSERT_EQUALS( data1[9], 3 );

    TS_ASSERT_EQUALS( data2[0], "one (1)" );
    TS_ASSERT_EQUALS( data2[1], "one (2)" );
    TS_ASSERT_EQUALS( data2[2], "one (3)" );
    TS_ASSERT_EQUALS( data2[3], "two (1)" );
    TS_ASSERT_EQUALS( data2[4], "two (1)" );
    TS_ASSERT_EQUALS( data2[5], "two (2)" );
    TS_ASSERT_EQUALS( data2[6], "two (2)" );
    TS_ASSERT_EQUALS( data2[7], "three (2)" );
    TS_ASSERT_EQUALS( data2[8], "three (2)" );
    TS_ASSERT_EQUALS( data2[9], "three (3)" );

    TS_ASSERT_EQUALS( data3[0], 7);
    TS_ASSERT_EQUALS( data3[1], 2);
    TS_ASSERT_EQUALS( data3[2], 1);
    TS_ASSERT_EQUALS( data3[3], 3);
    TS_ASSERT_EQUALS( data3[4], 8);
    TS_ASSERT_EQUALS( data3[5], 6);
    TS_ASSERT_EQUALS( data3[6], 9);
    TS_ASSERT_EQUALS( data3[7], 4);
    TS_ASSERT_EQUALS( data3[8], 5);
    TS_ASSERT_EQUALS( data3[9], 0);

    // Remove workspace from the data service.
    AnalysisDataService::Instance().clear();
  }

  void test_descending()
  {

    auto ws = WorkspaceFactory::Instance().createTable();
    ws->addColumn("int","x");
    ws->addColumn("str","y");
    ws->addColumn("double","z");

    TableRow row = ws->appendRow();
    row << 3 << "three (3)" << 0.0;
    row = ws->appendRow();
    row << 1 << "one (3)" << 1.0;
    row = ws->appendRow();
    row << 1 << "one (2)" << 2.0;
    row = ws->appendRow();
    row << 2 << "two (1)" << 3.0;
    row = ws->appendRow();
    row << 3 << "three (2)" << 4.0;
    row = ws->appendRow();
    row << 3 << "three (2)" << 5.0;
    row = ws->appendRow();
    row << 2 << "two (2)" << 6.0;
    row = ws->appendRow();
    row << 1 << "one (1)" << 7.0;
    row = ws->appendRow();
    row << 2 << "two (1)" << 8.0;
    row = ws->appendRow();
    row << 2 << "two (2)" << 9.0;


    // Name of the output workspace.
    std::string outWSName("SortTableWorkspaceTest_OutputWS");

    std::vector<std::string> columns(3);
    columns[0] = "x";
    columns[1] = "y";
    columns[2] = "z";
    std::vector<int> ascending(1);
    ascending[0] = 0;
  
    SortTableWorkspace alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("InputWorkspace", ws) );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("OutputWorkspace", outWSName) );
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("Columns", columns) );
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("Ascending", ascending) );
    TS_ASSERT_THROWS_NOTHING( alg.execute(); );
    TS_ASSERT( alg.isExecuted() );
    
    ITableWorkspace_sptr outws;
    TS_ASSERT_THROWS_NOTHING( outws = AnalysisDataService::Instance().retrieveWS<ITableWorkspace>(outWSName) );
    TS_ASSERT(outws);
    if (!outws) return;

    auto &data1 = static_cast<Mantid::DataObjects::TableColumn<int>&>(*outws->getColumn("x")).data();
    auto &data2 = static_cast<Mantid::DataObjects::TableColumn<std::string>&>(*outws->getColumn("y")).data();
    auto &data3 = static_cast<Mantid::DataObjects::TableColumn<double>&>(*outws->getColumn("z")).data();

    TS_ASSERT_EQUALS( data1[0], 3 );
    TS_ASSERT_EQUALS( data1[1], 3 );
    TS_ASSERT_EQUALS( data1[2], 3 );
    TS_ASSERT_EQUALS( data1[3], 2 );
    TS_ASSERT_EQUALS( data1[4], 2 );
    TS_ASSERT_EQUALS( data1[5], 2 );
    TS_ASSERT_EQUALS( data1[6], 2 );
    TS_ASSERT_EQUALS( data1[7], 1 );
    TS_ASSERT_EQUALS( data1[8], 1 );
    TS_ASSERT_EQUALS( data1[9], 1 );

    TS_ASSERT_EQUALS( data2[0], "three (3)" );
    TS_ASSERT_EQUALS( data2[1], "three (2)" );
    TS_ASSERT_EQUALS( data2[2], "three (2)" );
    TS_ASSERT_EQUALS( data2[3], "two (2)" );
    TS_ASSERT_EQUALS( data2[4], "two (2)" );
    TS_ASSERT_EQUALS( data2[5], "two (1)" );
    TS_ASSERT_EQUALS( data2[6], "two (1)" );
    TS_ASSERT_EQUALS( data2[7], "one (3)" );
    TS_ASSERT_EQUALS( data2[8], "one (2)" );
    TS_ASSERT_EQUALS( data2[9], "one (1)" );

    TS_ASSERT_EQUALS( data3[0], 0);
    TS_ASSERT_EQUALS( data3[1], 5);
    TS_ASSERT_EQUALS( data3[2], 4);
    TS_ASSERT_EQUALS( data3[3], 9);
    TS_ASSERT_EQUALS( data3[4], 6);
    TS_ASSERT_EQUALS( data3[5], 8);
    TS_ASSERT_EQUALS( data3[6], 3);
    TS_ASSERT_EQUALS( data3[7], 1);
    TS_ASSERT_EQUALS( data3[8], 2);
    TS_ASSERT_EQUALS( data3[9], 7);

    // Remove workspace from the data service.
    AnalysisDataService::Instance().clear();
  }

  void test_no_columns_given()
  {

    auto ws = WorkspaceFactory::Instance().createTable();
    ws->addColumn("int","x");
    ws->addColumn("str","y");
    ws->addColumn("double","z");

    TableRow row = ws->appendRow();
    row << 3 << "three (3)" << 0.0;
    row = ws->appendRow();
    row << 1 << "one (3)" << 1.0;
    row = ws->appendRow();
    row << 1 << "one (2)" << 2.0;


    // Name of the output workspace.
    std::string outWSName("SortTableWorkspaceTest_OutputWS");

    SortTableWorkspace alg;
    alg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("InputWorkspace", ws) );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("OutputWorkspace", outWSName) );
    TS_ASSERT_THROWS( alg.execute(), std::invalid_argument );
    TS_ASSERT( !alg.isExecuted() );
  }

  void test_wrong_ascending_size()
  {

    auto ws = WorkspaceFactory::Instance().createTable();
    ws->addColumn("int","x");
    ws->addColumn("str","y");
    ws->addColumn("double","z");

    TableRow row = ws->appendRow();
    row << 3 << "three (3)" << 0.0;
    row = ws->appendRow();
    row << 1 << "one (3)" << 1.0;
    row = ws->appendRow();
    row << 1 << "one (2)" << 2.0;


    // Name of the output workspace.
    std::string outWSName("SortTableWorkspaceTest_OutputWS");

    std::vector<std::string> columns(3);
    columns[0] = "x";
    columns[1] = "y";
    columns[2] = "z";
    std::vector<int> ascending(2);
    ascending[0] = 0;
    ascending[1] = 1;
  
    SortTableWorkspace alg;
    alg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("InputWorkspace", ws) );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("OutputWorkspace", outWSName) );
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("Columns", columns) );
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("Ascending", ascending) );
    TS_ASSERT_THROWS( alg.execute(), std::invalid_argument );
    TS_ASSERT( !alg.isExecuted() );
  }

  void test_wrong_column_names()
  {

    auto ws = WorkspaceFactory::Instance().createTable();
    ws->addColumn("int","x");
    ws->addColumn("str","y");
    ws->addColumn("double","z");

    TableRow row = ws->appendRow();
    row << 3 << "three (3)" << 0.0;
    row = ws->appendRow();
    row << 1 << "one (3)" << 1.0;
    row = ws->appendRow();
    row << 1 << "one (2)" << 2.0;


    // Name of the output workspace.
    std::string outWSName("SortTableWorkspaceTest_OutputWS");

    std::vector<std::string> columns(3);
    columns[0] = "x";
    columns[1] = "b";
    columns[2] = "z";
    std::vector<int> ascending(1);
    ascending[0] = 0;
  
    SortTableWorkspace alg;
    alg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("InputWorkspace", ws) );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("OutputWorkspace", outWSName) );
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("Columns", columns) );
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("Ascending", ascending) );
    TS_ASSERT_THROWS( alg.execute(), std::runtime_error );
    TS_ASSERT( !alg.isExecuted() );
  }

  void test_too_many_column_names()
  {

    auto ws = WorkspaceFactory::Instance().createTable();
    ws->addColumn("int","x");
    ws->addColumn("str","y");
    ws->addColumn("double","z");

    TableRow row = ws->appendRow();
    row << 3 << "three (3)" << 0.0;
    row = ws->appendRow();
    row << 1 << "one (3)" << 1.0;
    row = ws->appendRow();
    row << 1 << "one (2)" << 2.0;


    // Name of the output workspace.
    std::string outWSName("SortTableWorkspaceTest_OutputWS");

    std::vector<std::string> columns(4);
    columns[0] = "x";
    columns[1] = "y";
    columns[2] = "z";
    columns[3] = "a";
    std::vector<int> ascending(1);
    ascending[0] = 0;
  
    SortTableWorkspace alg;
    alg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("InputWorkspace", ws) );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("OutputWorkspace", outWSName) );
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("Columns", columns) );
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("Ascending", ascending) );
    TS_ASSERT_THROWS( alg.execute(), std::runtime_error );
    TS_ASSERT( !alg.isExecuted() );
  }

};


#endif /* MANTID_DATAHANDLING_SORTTABLEWORKSPACETEST_H_ */