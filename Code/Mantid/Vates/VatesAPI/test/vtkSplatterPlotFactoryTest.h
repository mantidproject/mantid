#ifndef VTK_SPLATTERPLOT_FACTORY_TEST
#define VTK_SPLATTERPLOT_FACTORY_TEST

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
using namespace testing;

//=====================================================================================
// Functional tests
//=====================================================================================
class vtkSplatterPlotFactoryTest : public CxxTest::TestSuite
{

public:

  /* Destructive tests. Test works correctly when misused.*/

  void testCreateWithoutInitializeThrows()
  {
    FakeProgressAction progressUpdate;
    vtkSplatterPlotFactory factory(ThresholdRange_scptr(new UserDefinedThresholdRange(0, 1)), "signal");
    TSM_ASSERT_THROWS("Have NOT initalized object. Should throw.", factory.create(progressUpdate), std::runtime_error);
  }

  void testInitializeWithNullWorkspaceThrows()
  {
    vtkSplatterPlotFactory factory(ThresholdRange_scptr(new UserDefinedThresholdRange(0, 1)), "signal");
    IMDEventWorkspace* ws = NULL;
    TSM_ASSERT_THROWS("This is a NULL workspace. Should throw.", factory.initialize( Workspace_sptr(ws) ), std::invalid_argument);
  }

  void testInitializeWithWrongWorkspaceTypeThrows()
  {
    IMDWorkspace* ws = new MockIMDWorkspace;
    vtkSplatterPlotFactory factory(ThresholdRange_scptr(new UserDefinedThresholdRange(0, 1)), "signal");
    TSM_ASSERT_THROWS("This is an invalid workspace. Should throw.", factory.initialize( Workspace_sptr(ws) ), std::invalid_argument);
  }

  /*Demonstrative tests*/

  void test_3DWorkspace()
  {
    FakeProgressAction progressUpdate;

    Mantid::MDEvents::MDEventWorkspace3Lean::sptr ws = MDEventsTestHelper::makeMDEW<3>(10, 0.0, 10.0, 1);
    vtkSplatterPlotFactory factory(ThresholdRange_scptr(new UserDefinedThresholdRange(0, 1)), "signal");
    factory.initialize(ws);
    vtkDataSet* product = NULL;

    TS_ASSERT_THROWS_NOTHING(product = factory.create(progressUpdate));

    /* original sizes for splatter plot test, before 5/28/2013
    const size_t expected_n_points = 1000;
    const size_t expected_n_cells = 999;
    */

    // New sizes for splatter plot test, after changing the way the points 
    // are selected, 5/28/2013
    const size_t expected_n_points = 50;
    const size_t expected_n_cells = 50;

    const size_t expected_n_signals = expected_n_cells;

    TSM_ASSERT_EQUALS("Wrong number of points", expected_n_points, product->GetNumberOfPoints());
    TSM_ASSERT_EQUALS("Wrong number of cells", expected_n_cells, product->GetNumberOfCells());
    TSM_ASSERT_EQUALS("No signal Array", "signal", std::string(product->GetCellData()->GetArray(0)->GetName()));
    TSM_ASSERT_EQUALS("Wrong sized signal Array", expected_n_signals, product->GetCellData()->GetArray(0)->GetSize());

    product->Delete();
  }

  void test_4DWorkspace()
  {
    FakeProgressAction progressUpdate;

    Mantid::MDEvents::MDEventWorkspace4Lean::sptr ws = MDEventsTestHelper::makeMDEW<4>(5, -10.0, 10.0, 1);
    vtkSplatterPlotFactory factory(ThresholdRange_scptr(new UserDefinedThresholdRange(0, 1)), "signal");
    factory.initialize(ws);
    vtkDataSet* product = NULL;

    TS_ASSERT_THROWS_NOTHING(product = factory.create(progressUpdate));

    // 6 is 5% of 125
    const size_t expected_n_points = 6;
    const size_t expected_n_cells = 6;
    const size_t expected_n_signals = expected_n_cells;

    TSM_ASSERT_EQUALS("Wrong number of points", expected_n_points, product->GetNumberOfPoints());
    TSM_ASSERT_EQUALS("Wrong number of cells", expected_n_cells, product->GetNumberOfCells());
    TSM_ASSERT_EQUALS("No signal Array", "signal", std::string(product->GetCellData()->GetArray(0)->GetName()));
    TSM_ASSERT_EQUALS("Wrong sized signal Array", expected_n_signals, product->GetCellData()->GetArray(0)->GetSize());

    /*
    //Check dataset bounds
    double* bounds = product->GetBounds();
    TS_ASSERT_EQUALS(-8.0, bounds[0]);
    TS_ASSERT_EQUALS(8.0, bounds[1]);
    TS_ASSERT_EQUALS(-8.0, bounds[2]);
    TS_ASSERT_EQUALS(4.0, bounds[3]);
    TS_ASSERT_EQUALS(4.0, bounds[4]);
    TS_ASSERT_EQUALS(4.0, bounds[5]);
    */
    product->Delete();
  }

};

#endif
