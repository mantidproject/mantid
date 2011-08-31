#ifndef MANTID_WORKSPACE_MEMENTO_COLLECTION_TEST_H_
#define MANTID_WORKSPACE_MEMENTO_COLLECTION_TEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidDataObjects/WorkspaceMementoCollection.h"
#include "MantidDataObjects/TableWorkspace.h"

using namespace Mantid::DataObjects;
using namespace Mantid::API;

class WorkspaceMementoCollectionTest : public CxxTest::TestSuite
{
public:

  void testSerialize()
  {
    WorkspaceMementoCollection collection;
    ITableWorkspace* productA = collection.serialize();
    ITableWorkspace* productB = collection.serialize();

    TSM_ASSERT_EQUALS("Characterisation test. Current table schema has 1 column", 1, productA->columnCount());
    TSM_ASSERT_EQUALS("No workspaces registered, so should have no rows.", 0, productA->rowCount());
    TSM_ASSERT("Check are different locations on heap", productA != productB);
    delete productA;
    delete productB;
  }

void testRegisterWorkspace()
{
  TableWorkspace ws;
  ws.setName("WSName");

  WorkspaceMementoCollection collection;
  collection.registerWorkspace(ws); //Could be any other kind of workspace.
  ITableWorkspace* product = collection.serialize();
  
  TSM_ASSERT_EQUALS("Registered workspace property not serialized.", "WSName", product->cell<std::string>(0, 0));
  delete product;
}

void testGetMemento()
{
  TableWorkspace ws;
  ws.setName("WSName");

  WorkspaceMementoCollection collection;
  collection.registerWorkspace(ws); //Could be any other kind of workspace registered.
  LockingMemento smrtPtr = collection.at(0);
  boost::shared_ptr<WorkspaceMementoItem<0, std::string> > pExact = boost::dynamic_pointer_cast<WorkspaceMementoItem<0, std::string> >(smrtPtr->getItem(0));

  TS_ASSERT(pExact != NULL);
  TS_ASSERT_EQUALS("WSName", pExact->getValue());

}


};

#endif