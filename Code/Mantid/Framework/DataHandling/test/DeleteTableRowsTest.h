#ifndef DELETETABLEROWSTEST_H_
#define DELETETABLEROWSTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidDataHandling/DeleteTableRows.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"

using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::DataHandling;

class DeleteTableRowsTest : public CxxTest::TestSuite
{
public:


  void testDeleteIsDone()
  {
    //int iii;
    //std::cin >> iii;
    std::string wsName = "DeleteTableRowsTest_table";
    ITableWorkspace_sptr tw = WorkspaceFactory::Instance().createTable("TableWorkspace");
    AnalysisDataService::Instance().add(wsName,tw);
    tw->addColumn("int","int");
    for(size_t i = 0; i < 10; ++i)
    {
      TableRow row = tw->appendRow();
      row << int(i);
    }
    IAlgorithm_sptr alg = AlgorithmManager::Instance().create("DeleteTableRows");
    alg->setPropertyValue("TableWorkspace",wsName);
    alg->setPropertyValue("Rows","1,3,5,7,9");
    alg->execute();

    TS_ASSERT_EQUALS(tw->rowCount(),5);
    TS_ASSERT_EQUALS(tw->cell<int>(0,0),0);
    TS_ASSERT_EQUALS(tw->cell<int>(1,0),2);
    TS_ASSERT_EQUALS(tw->cell<int>(2,0),4);
    TS_ASSERT_EQUALS(tw->cell<int>(3,0),6);
    TS_ASSERT_EQUALS(tw->cell<int>(4,0),8);
    AnalysisDataService::Instance().remove(wsName);
  }


};

#endif /*DELETETABLEROWSTEST_H_*/
