#ifndef MANTID_ALGORITHMS_CREATELOGPROPERTYTABLETEST_H_
#define MANTID_ALGORITHMS_CREATELOGPROPERTYTABLETEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/CreateLogPropertyTable.h"
#include "MantidDataHandling/Load.h"
#include "MantidAPI/Workspace.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/TableRow.h"

#include <string>
#include <vector>

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::Algorithms;
using namespace Mantid::DataHandling;

namespace
{
  std::vector<std::string> loadTestWorkspaces(std::vector<std::string> filenames)
  {
    std::vector<Workspace_sptr> wsList;

    Load loader;
    loader.initialize();

    for( auto filename = filenames.begin(); filename != filenames.end(); ++filename )
    {
      loader.setPropertyValue("Filename", *filename);
      loader.setPropertyValue("OutputWorkspace", *filename);
      TS_ASSERT_THROWS_NOTHING(loader.execute());
    }

    return filenames;
  }
}

class CreateLogPropertyTableTest : public CxxTest::TestSuite
{
public:
  void test_init()
  {
    CreateLogPropertyTable alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() );
    TS_ASSERT( alg.isInitialized() );
  }

  void test_exec()
  {
    const std::string filenamesArray[2] = {
      "TSC10076", 
      "OSI11886"};
    std::vector<std::string> filenames;
    filenames.assign(filenamesArray, filenamesArray+2);
    
    std::vector<std::string> wsNames = loadTestWorkspaces(filenames);

    CreateLogPropertyTable alg;
    alg.initialize();

    std::string propNamesArray[2] = {
      "run_number",
      "run_start"};
    std::vector<std::string> propNames;
    propNames.assign(propNamesArray, propNamesArray + 2);

    TS_ASSERT_THROWS_NOTHING( alg.setProperty("InputWorkspaces", wsNames) );
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("LogPropertyNames", propNames) );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("OutputWorkspace", "outputTest") );

    TS_ASSERT_THROWS_NOTHING( alg.execute() );

    TS_ASSERT( alg.isExecuted() );

    ITableWorkspace_sptr table = AnalysisDataService::Instance().retrieveWS<ITableWorkspace>("outputTest");
    
    TS_ASSERT( table );

    TableRow row1 = table->getRow(0);
    TableRow row2 = table->getRow(1);
    
    TS_ASSERT_EQUALS( row1.cell<std::string>(0), "10076" );
    TS_ASSERT_EQUALS( row1.cell<std::string>(1), "2008-12-10T10:35:23" );
    TS_ASSERT_EQUALS( row2.cell<std::string>(0), "11886" );
    TS_ASSERT_EQUALS( row2.cell<std::string>(1), "2000-03-12T08:54:42" );
  }
};


#endif /* MANTID_ALGORITHMS_CREATELOGPROPERTYTABLETEST_H_ */

