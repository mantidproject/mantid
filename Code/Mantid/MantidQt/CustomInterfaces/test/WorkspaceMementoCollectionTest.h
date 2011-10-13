#ifndef CUSTOM_INTERFACES_MEMENTO_COLLECTION_TEST_H_
#define CUSTOM_INTERFACES_MEMENTO_COLLECTION_TEST_H_

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "MantidQtCustomInterfaces/WorkspaceMementoCollection.h"
#include "MantidQtCustomInterfaces/WorkspaceMemento.h"
#include "MantidQtCustomInterfaces/WorkspaceMementoItem.h"
#include "MantidQtCustomInterfaces/Updateable.h"

#include "MantidDataObjects/TableWorkspace.h"
#include "MantidAPI/MatrixWorkspace.h"

#include "MantidTestHelpers/WorkspaceCreationHelper.h"

using namespace Mantid::DataObjects;
using namespace MantidQt::CustomInterfaces;
using namespace Mantid::API;
using namespace testing;

class WorkspaceMementoCollectionTest : public CxxTest::TestSuite
{

private:

  // Helper type. 
  class MockExternalDrivenModel : public Updateable
  {
  public:
    MOCK_METHOD0(update, void());
  };

public:

  //=====================================================================================
  // Functional tests
  //=====================================================================================
  void testSerialize()
  {
    WorkspaceMementoCollection collection;
    ITableWorkspace* productA = collection.serialize();
    ITableWorkspace* productB = collection.serialize();

    TSM_ASSERT_EQUALS("No workspaces registered, so should have no rows.", 0, productA->rowCount());
    TSM_ASSERT("Check are different locations on heap", productA != productB);
    delete productA;
    delete productB;
  }

  void testRegisterWorkspace()
  {
    Workspace2D_sptr ws = WorkspaceCreationHelper::Create2DWorkspace(1, 1);
    ws->setName("WSName");

    MockExternalDrivenModel model;
    EXPECT_CALL(model, update()).Times(AtLeast(1)); //Test that registration of a workspace causes model update!

    WorkspaceMementoCollection collection;
    collection.registerWorkspace(ws, &model); //Could be any other kind of workspace.
    ITableWorkspace* product = collection.serialize();

    TSM_ASSERT("Model not used as expected.", Mock::VerifyAndClearExpectations(&model));
    TSM_ASSERT_EQUALS("Registered workspace property not serialized.", "WSName", product->cell<std::string>(0, 0));
    delete product;
  }

  void testGetMemento()
  {
    Workspace2D_sptr ws = WorkspaceCreationHelper::Create2DWorkspace(1, 1);
    ws->setName("WSName");

    MockExternalDrivenModel model;
    EXPECT_CALL(model, update()).Times(AtLeast(1)); //Test that registration of a workspace causes model update!

    WorkspaceMementoCollection collection;
    collection.registerWorkspace(ws, &model); //Could be any other kind of workspace registered.
    LoanedMemento smrtPtr = collection.at(0);
    boost::shared_ptr<WorkspaceMementoItem<0, std::string> > pExact = boost::dynamic_pointer_cast<WorkspaceMementoItem<0, std::string> >(smrtPtr->getItem(0));

    TS_ASSERT(pExact != NULL);
    TS_ASSERT_EQUALS("WSName", pExact->getValue());

  }


};

#endif