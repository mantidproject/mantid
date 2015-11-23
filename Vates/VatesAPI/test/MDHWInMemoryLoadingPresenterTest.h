#ifndef MDHW_IN_MEMORY_LOADING_PRESENTER_TEST_H 
#define MDHW_IN_MEMORY_LOADING_PRESENTER_TEST_H

#include <cxxtest/TestSuite.h>
#include "MockObjects.h"

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/IMDHistoWorkspace.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidTestHelpers/MDEventsTestHelper.h"
#include "MantidVatesAPI/FilteringUpdateProgressAction.h"
#include "MantidVatesAPI/MDHWInMemoryLoadingPresenter.h"
#include "MantidKernel/make_unique.h"
#include <vtkUnstructuredGrid.h>
#include <vtkSmartPointer.h>

using namespace Mantid::VATES;
using namespace Mantid::API;
using namespace testing;
using Mantid::DataObjects::MDEventsTestHelper::makeFakeMDHistoWorkspace;

class MDHWInMemoryLoadingPresenterTest: public CxxTest::TestSuite
{

private:

  // Helper type. Mocks a Workspace Provider.
  class MockWorkspaceProvider : public Mantid::VATES::WorkspaceProvider
  {
  public:
    MOCK_CONST_METHOD1(canProvideWorkspace, bool(std::string));
    MOCK_CONST_METHOD1(fetchWorkspace, Mantid::API::Workspace_sptr(std::string));
    MOCK_CONST_METHOD1(disposeWorkspace, void(std::string));
  };

  // Helper method. Generates and returns a valid IMDHistoWorkspace
  Mantid::API::Workspace_sptr getGoodWorkspace()
  {
    Mantid::DataObjects::MDHistoWorkspace_sptr ws = makeFakeMDHistoWorkspace(1.0, 4, 5, 1.0, 0.1,"MD_HISTO_WS");
    return ws;
  }

  // Helper method. Generates a non-IMDHistoWorkspace.
  static Mantid::API::Workspace_sptr getBadWorkspace()
  {
    //Return a table workspace.
    return WorkspaceFactory::Instance().createTable();
  }

public:

  void testConstructWithNullViewThrows()
  {
    TSM_ASSERT_THROWS(
        "Should throw with null view.",
        MDHWInMemoryLoadingPresenter(nullptr, new MockWorkspaceProvider, "_"),
        std::invalid_argument);
  }

  void testConstructWithNullRepositoryThrows()
  {
    TSM_ASSERT_THROWS(
        "Should throw with null repository.",
        MDHWInMemoryLoadingPresenter(
            Mantid::Kernel::make_unique<MockMDLoadingView>(), nullptr, "_"),
        std::invalid_argument);
  }

  void testConstructWithEmptyWsNameThrows()
  {
    std::string emptyName = "";
    TSM_ASSERT_THROWS("Should throw with empty Workspace name.",
                      MDHWInMemoryLoadingPresenter(
                          Mantid::Kernel::make_unique<MockMDLoadingView>(),
                          new MockWorkspaceProvider, emptyName),
                      std::invalid_argument);
  }

  void testConstruction()
  {
    TS_ASSERT_THROWS_NOTHING(MDHWInMemoryLoadingPresenter(
        Mantid::Kernel::make_unique<MockMDLoadingView>(),
        new MockWorkspaceProvider, "_"));
  }

  void testCanLoadWithInvalidName()
  {
    MockWorkspaceProvider* repository = new MockWorkspaceProvider;
    EXPECT_CALL(*repository, canProvideWorkspace(_)).WillOnce(Return(false)); //No matter what the argument, always returns false.

    //Give a dummy name corresponding to the workspace.
    MDHWInMemoryLoadingPresenter presenter(
        Mantid::Kernel::make_unique<MockMDLoadingView>(), repository, "_");

    TSM_ASSERT("Should indicate that the workspace cannot be read-out since the name is not in the Repository.", !presenter.canReadFile());
  }

  void testCanLoadWithWrongWsType()
  {
    MockWorkspaceProvider* repository = new MockWorkspaceProvider;
    Mantid::API::Workspace_sptr badWs = getBadWorkspace(); // Not an IMDHistoWorkspace.
    EXPECT_CALL(*repository, canProvideWorkspace(_)).WillOnce(Return(true)); //No matter what the argument, always returns true.
    EXPECT_CALL(*repository, fetchWorkspace(_)).WillOnce(Return(badWs)); 

    //Give a dummy name corresponding to the workspace.
    MDHWInMemoryLoadingPresenter presenter(
        Mantid::Kernel::make_unique<MockMDLoadingView>(), repository, "_");

    TSM_ASSERT("Should indicate that the workspace cannot be read-out since it is not of the right type.", !presenter.canReadFile());
  }

  void testCanLoadSucceeds()
  {
    MockWorkspaceProvider* repository = new MockWorkspaceProvider;
    Mantid::API::Workspace_sptr goodWs = getGoodWorkspace();
    EXPECT_CALL(*repository, canProvideWorkspace(_)).WillOnce(Return(true)); //No matter what the argument, always returns true.
    EXPECT_CALL(*repository, fetchWorkspace(_)).WillOnce(Return(goodWs)); 

    //Give a dummy name corresponding to the workspace.
    MDHWInMemoryLoadingPresenter presenter(
        Mantid::Kernel::make_unique<MockMDLoadingView>(), repository, "_");

    TSM_ASSERT("Should have worked! Workspace is of correct type and repository says ws is present.!", presenter.canReadFile());
  }

  void testExtractMetadata()
  {
    MockWorkspaceProvider* repository = new MockWorkspaceProvider;
    Mantid::API::Workspace_sptr ws = getGoodWorkspace();
    EXPECT_CALL(*repository, fetchWorkspace(_)).Times(1).WillRepeatedly(Return(ws));

    MDHWInMemoryLoadingPresenter presenter(
        Mantid::Kernel::make_unique<MockMDLoadingView>(), repository, "_");

    //Test that it doesn't work when not setup.
    TSM_ASSERT_THROWS("::executeLoadMetadata is critical to setup, should throw if not run first.", presenter.getGeometryXML(), std::runtime_error);
    
    //Test that it does work when setup.
    presenter.executeLoadMetadata();

    std::string ins = presenter.getInstrument();

    TSM_ASSERT("Should export geometry xml metadata on request.", !presenter.getGeometryXML().empty())
    TSM_ASSERT("Should export min value metadata on request.", presenter.getMinValue() <= presenter.getMaxValue())
    TSM_ASSERT("Should export instrument metadata on request", presenter.getInstrument().empty())
  }

  void testExecution()
  {

    //Setup view
    MockMDLoadingView view;
    EXPECT_CALL(view, getRecursionDepth()).Times(0);
    EXPECT_CALL(view, getLoadInMemory())
        .Times(0); // Not a question that needs asking for this presenter type.
    EXPECT_CALL(view, updateAlgorithmProgress(_, _)).Times(AnyNumber());

    //Setup rendering factory
    MockvtkDataSetFactory factory;
    EXPECT_CALL(factory, initialize(_)).Times(1);
    EXPECT_CALL(factory, create(_)).WillOnce(Return(vtkUnstructuredGrid::New()));

    MockWorkspaceProvider* repository = new MockWorkspaceProvider;
    Mantid::API::Workspace_sptr ws = getGoodWorkspace();
    EXPECT_CALL(*repository, fetchWorkspace(_)).Times(2).WillRepeatedly(Return(ws));

    //Setup progress updates objects
    MockProgressAction mockLoadingProgressAction;
    MockProgressAction mockDrawingProgressAction;

    //Create the presenter and run it!
    MDHWInMemoryLoadingPresenter presenter(
        std::unique_ptr<MDLoadingView>(&view), repository, "_");
    presenter.executeLoadMetadata();
    auto product = vtkSmartPointer<vtkDataSet>::Take(presenter.execute(
        &factory, mockLoadingProgressAction, mockDrawingProgressAction));

    TSM_ASSERT("Should have generated a vtkDataSet", NULL != product);
    TSM_ASSERT_EQUALS("Wrong type of output generated", "vtkUnstructuredGrid", std::string(product->GetClassName()));
    TSM_ASSERT("No field data!", NULL != product->GetFieldData());
    TSM_ASSERT_EQUALS("Two arrays expected on field data, one for XML and one for JSON!", 2, product->GetFieldData()->GetNumberOfArrays());
    TS_ASSERT_THROWS_NOTHING(presenter.hasTDimensionAvailable());
    TS_ASSERT_THROWS_NOTHING(presenter.getGeometryXML());
    TS_ASSERT(!presenter.getWorkspaceTypeName().empty());
    TSM_ASSERT("Special coordinate metadata failed.", -1 < presenter.getSpecialCoordinates());
    TS_ASSERT(Mock::VerifyAndClearExpectations(&view));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&factory));
  }

  void testCallHasTDimThrows()
  {
    MDHWInMemoryLoadingPresenter presenter(
        Mantid::Kernel::make_unique<MockMDLoadingView>(),
        new MockWorkspaceProvider, "_");
    TSM_ASSERT_THROWS("Should throw. Execute not yet run.",
                      presenter.hasTDimensionAvailable(), std::runtime_error);
  }

  void testCallGetTDimensionValuesThrows()
  {
    MDHWInMemoryLoadingPresenter presenter(
        Mantid::Kernel::make_unique<MockMDLoadingView>(),
        new MockWorkspaceProvider, "_");
    TSM_ASSERT_THROWS("Should throw. Execute not yet run.",
                      presenter.getTimeStepValues(), std::runtime_error);
  }

  void testCallGetGeometryThrows()
  {
    MDHWInMemoryLoadingPresenter presenter(
        Mantid::Kernel::make_unique<MockMDLoadingView>(),
        new MockWorkspaceProvider, "_");
    TSM_ASSERT_THROWS("Should throw. Execute not yet run.",
                      presenter.getGeometryXML(), std::runtime_error);
  }

  void testGetWorkspaceTypeName()
  {
    MDHWInMemoryLoadingPresenter presenter(
        Mantid::Kernel::make_unique<MockMDLoadingView>(),
        new MockWorkspaceProvider, "_");
    TSM_ASSERT_EQUALS("Characterisation Test Failed", "",
                      presenter.getWorkspaceTypeName());
  }

  void testGetSpecialCoordinates()
  {
    MDHWInMemoryLoadingPresenter presenter(
        Mantid::Kernel::make_unique<MockMDLoadingView>(),
        new MockWorkspaceProvider, "_");
    TSM_ASSERT_EQUALS("Characterisation Test Failed", -1,
                      presenter.getSpecialCoordinates());
  }

};

#endif
