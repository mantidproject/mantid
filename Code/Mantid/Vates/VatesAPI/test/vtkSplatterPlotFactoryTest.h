#ifndef VTK_MDEW_HEXAHEDRON_FACTORY_TEST
#define VTK_MDEW_HEXAHEDRON_FACTORY_TEST

#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidMDEvents/MDEventFactory.h"
#include "MantidMDEvents/MDEventWorkspace.h"
#include "MantidMDEvents/MDHistoWorkspace.h"
#include "MantidTestHelpers/MDEventsTestHelper.h"
#include "MantidVatesAPI/UserDefinedThresholdRange.h"
#include "MantidVatesAPI/vtkSplatterPlotFactory.h"
#include "MockObjects.h"
#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <vtkCellData.h>
#include <vtkDataArray.h>


using namespace Mantid;
using namespace Mantid::VATES;
using namespace Mantid::API;
using namespace Mantid::MDEvents;

//=====================================================================================
// Functional tests
//=====================================================================================
class vtkSplatterPlotFactoryTest : public CxxTest::TestSuite
{

public:

  /* Destructive tests. Test works correctly when misused.*/

  void testCreateWithoutInitalizeThrows()
  {
    vtkSplatterPlotFactory factory(ThresholdRange_scptr(new UserDefinedThresholdRange(0, 1)), "signal");
    TSM_ASSERT_THROWS("Have NOT initalized object. Should throw.", factory.create(), std::runtime_error);
  }

  void testInitalizeWithNullWorkspaceThrows()
  {
    vtkSplatterPlotFactory factory(ThresholdRange_scptr(new UserDefinedThresholdRange(0, 1)), "signal");

    IMDEventWorkspace* ws = NULL;
    TSM_ASSERT_THROWS("This is a NULL workspace. Should throw.", factory.initialize( Workspace_sptr(ws) ), std::invalid_argument);
  }

  void testInitalizeWithWrongWorkspaceTypeThrows()
  {
    IMDWorkspace* ws = new MockIMDWorkspace;
    ws->setName("OTHER_WS_TYPE");

    vtkSplatterPlotFactory factory(ThresholdRange_scptr(new UserDefinedThresholdRange(0, 1)), "signal");
    TSM_ASSERT_THROWS("This is an invalid workspace. Should throw.", factory.initialize( Workspace_sptr(ws) ), std::invalid_argument);
  }

  /*Demonstrative tests*/

  void test_3DWorkspace()
  {
    Mantid::MDEvents::MDEventWorkspace3Lean::sptr ws = MDEventsTestHelper::makeMDEW<3>(10, 0.0, 10.0, 1);
    vtkSplatterPlotFactory factory(ThresholdRange_scptr(new UserDefinedThresholdRange(0, 1)), "signal");
    factory.initialize(ws);
    vtkDataSet* product = NULL;

    TS_ASSERT_THROWS_NOTHING(product = factory.create());

    const size_t expected_n_points = 1000;
    const size_t expected_n_cells = 999;
    const size_t expected_n_signals = expected_n_cells;

    TSM_ASSERT_EQUALS("Wrong number of points", expected_n_points, product->GetNumberOfPoints());
    TSM_ASSERT_EQUALS("Wrong number of cells", expected_n_cells, product->GetNumberOfCells());
    TSM_ASSERT_EQUALS("No signal Array", "signal", std::string(product->GetCellData()->GetArray(0)->GetName()));
    TSM_ASSERT_EQUALS("Wrong sized signal Array", expected_n_signals, product->GetCellData()->GetArray(0)->GetSize());

    /*Check dataset bounds*/
    //double* bounds = product->GetBounds();
    //TS_ASSERT_DELTA(0, bounds[0], 0.1);
    //TS_ASSERT_DELTA(9.5, bounds[1], 0.1);
    //TS_ASSERT_DELTA(0, bounds[2], 0.1);
    //TS_ASSERT_DELTA(9.5, bounds[3], 0.1);
    //TS_ASSERT_DELTA(0, bounds[4], 0.1);
    //TS_ASSERT_DELTA(9.5, bounds[5], 0.1);

    product->Delete();
  }

//  void test_4DWorkspace()
//  {
//    Mantid::MDEvents::MDEventWorkspace4Lean::sptr ws = MDEventsTestHelper::makeMDEW<4>(5, -10.0, 10.0, 1);
//    vtkSplatterPlotFactory factory(ThresholdRange_scptr(new UserDefinedThresholdRange(0, 1)), "signal");
//    factory.initialize(ws);
//    vtkDataSet* product = NULL;
//
//    TS_ASSERT_THROWS_NOTHING(product = factory.create());
//
//    const size_t expected_n_points = 8*125;
//    const size_t expected_n_cells = 125;
//    const size_t expected_n_signals = expected_n_cells;
//
//    TSM_ASSERT_EQUALS("Wrong number of points", expected_n_points, product->GetNumberOfPoints());
//    TSM_ASSERT_EQUALS("Wrong number of cells", expected_n_cells, product->GetNumberOfCells());
//    TSM_ASSERT_EQUALS("Wrong number of points to cells. Hexahedron has 8 vertexes.", expected_n_cells * 8,  product->GetNumberOfPoints());
//    TSM_ASSERT_EQUALS("No signal Array", "signal", std::string(product->GetCellData()->GetArray(0)->GetName()));
//    TSM_ASSERT_EQUALS("Wrong sized signal Array", expected_n_signals, product->GetCellData()->GetArray(0)->GetSize());
//
//    /*Check dataset bounds*/
//    double* bounds = product->GetBounds();
//    TS_ASSERT_EQUALS(-9.5, bounds[0]);
//    TS_ASSERT_EQUALS(9.5, bounds[1]);
//    TS_ASSERT_EQUALS(-9.5, bounds[2]);
//    TS_ASSERT_EQUALS(9.5, bounds[3]);
//    TS_ASSERT_EQUALS(-9.5, bounds[4]);
//    TS_ASSERT_EQUALS(9.5, bounds[5]);
//
//    product->Delete();
//  }


};

#endif
