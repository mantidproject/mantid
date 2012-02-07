#ifndef VTK_GENERIC_IMD_FACTORY_TEST
#define VTK_GENERIC_IMD_FACTORY_TEST 

#include <cxxtest/TestSuite.h>
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidVatesAPI/vtkMDQuadFactory.h"
#include "MantidVatesAPI/NoThresholdRange.h"
#include "MockObjects.h"
#include "MantidMDEvents/OneStepMDEW.h"
#include "MantidMDEvents/SliceMD.h"
#include "MantidTestHelpers/MDEventsTestHelper.h"
#include "vtkCellType.h"
#include "vtkUnstructuredGrid.h"

using namespace Mantid::VATES;
using namespace Mantid::API;
using namespace Mantid::MDEvents;
using namespace testing;

//=====================================================================================
// Functional tests
//=====================================================================================
class vtkMDQuadFactoryTest : public CxxTest::TestSuite
{

public:

  void testcreateMeshOnlyThrows()
  {
    vtkMDQuadFactory factory(ThresholdRange_scptr(new NoThresholdRange), "signal");
    TS_ASSERT_THROWS(factory.createMeshOnly(), std::runtime_error);
  }

  void testcreateScalarArray()
  {
    vtkMDQuadFactory factory(ThresholdRange_scptr(new NoThresholdRange), "signal");
    TS_ASSERT_THROWS(factory.createScalarArray(), std::runtime_error);
  }

  void testGetFactoryTypeName()
  {
    vtkMDQuadFactory factory(ThresholdRange_scptr(new NoThresholdRange), "signal");
    TS_ASSERT_EQUALS("vtkMDQuadFactory", factory.getFactoryTypeName());
  }

  void testInitializeDelegatesToSuccessor()
  {
    MockvtkDataSetFactory* mockSuccessor = new MockvtkDataSetFactory;
    EXPECT_CALL(*mockSuccessor, initialize(_)).Times(1);
    EXPECT_CALL(*mockSuccessor, getFactoryTypeName()).Times(1);

    vtkMDQuadFactory factory(ThresholdRange_scptr(new NoThresholdRange), "signal");
    factory.SetSuccessor(mockSuccessor);

    ITableWorkspace_sptr ws(new Mantid::DataObjects::TableWorkspace);
    TS_ASSERT_THROWS_NOTHING(factory.initialize(ws));

    TSM_ASSERT("Successor has not been used properly.", Mock::VerifyAndClearExpectations(mockSuccessor));
  }

  void testCreateDelegatesToSuccessor()
  {
    MockvtkDataSetFactory* mockSuccessor = new MockvtkDataSetFactory;
    EXPECT_CALL(*mockSuccessor, initialize(_)).Times(1);
    EXPECT_CALL(*mockSuccessor, create()).Times(1);
    EXPECT_CALL(*mockSuccessor, getFactoryTypeName()).Times(1);

    vtkMDQuadFactory factory(ThresholdRange_scptr(new NoThresholdRange), "signal");
    factory.SetSuccessor(mockSuccessor);

    ITableWorkspace_sptr ws(new Mantid::DataObjects::TableWorkspace);
    TS_ASSERT_THROWS_NOTHING(factory.initialize(ws));
    TS_ASSERT_THROWS_NOTHING(factory.create());

    TSM_ASSERT("Successor has not been used properly.", Mock::VerifyAndClearExpectations(mockSuccessor));
  }

  void testOnInitaliseCannotDelegateToSuccessor()
  {
    vtkMDQuadFactory factory(ThresholdRange_scptr(new NoThresholdRange), "signal");
    //factory.SetSuccessor(mockSuccessor); No Successor set.

    ITableWorkspace_sptr ws(new Mantid::DataObjects::TableWorkspace);
    TS_ASSERT_THROWS(factory.initialize(ws), std::runtime_error);
  }

  void testCreateWithoutInitaliseThrows()
  {
    vtkMDQuadFactory factory(ThresholdRange_scptr(new NoThresholdRange), "signal");
    //initialize not called!
    TS_ASSERT_THROWS(factory.create(), std::runtime_error);
  }

  void testCreation()
  {
    boost::shared_ptr<Mantid::MDEvents::MDEventWorkspace<Mantid::MDEvents::MDEvent<2>,2> >
            ws = MDEventsTestHelper::makeMDEWFull<2>(10, 10, 10, 10);

    //Rebin it to make it possible to compare cells to bins.
    SliceMD slice;
    slice.initialize();
    slice.setProperty("InputWorkspace", ws);
    slice.setPropertyValue("AlignedDimX", "Axis0, -10, 10, 10");
    slice.setPropertyValue("AlignedDimY", "Axis1, -10, 10, 10");
    slice.setPropertyValue("OutputWorkspace", "binned");
    slice.execute();

    Workspace_sptr binned = Mantid::API::AnalysisDataService::Instance().retrieve("binned");

    vtkMDQuadFactory factory(ThresholdRange_scptr(new NoThresholdRange), "signal");
    factory.initialize(binned);

    vtkDataSet* product = factory.create();

    TS_ASSERT(dynamic_cast<vtkUnstructuredGrid*>(product) != NULL);
    TS_ASSERT_EQUALS(100, product->GetNumberOfCells());
    TS_ASSERT_EQUALS(400, product->GetNumberOfPoints());
    TS_ASSERT_EQUALS(VTK_QUAD, product->GetCellType(0));

    product->Delete();
    AnalysisDataService::Instance().remove("binned");
  }


};

#endif
  