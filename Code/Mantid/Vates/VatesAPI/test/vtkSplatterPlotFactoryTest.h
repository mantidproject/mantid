#ifndef VTK_SPLATTERPLOT_FACTORY_TEST
#define VTK_SPLATTERPLOT_FACTORY_TEST

#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidDataObjects/MDEventFactory.h"
#include "MantidDataObjects/MDEventWorkspace.h"
#include "MantidDataObjects/MDHistoWorkspace.h"
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
using namespace Mantid::DataObjects;
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

  /*Demonstrative tests*/
  void test_3DHistoWorkspace()
  {
    FakeProgressAction progressUpdate;

    // Create workspace with 5x5x5 binning 
    size_t binning = 5;
    MDHistoWorkspace_sptr ws = MDEventsTestHelper::makeFakeMDHistoWorkspace(1.0, 3, binning);
    vtkSplatterPlotFactory factory(ThresholdRange_scptr(new UserDefinedThresholdRange(0, 1)), "signal");
    factory.initialize(ws);
    vtkDataSet* product = NULL;

    TS_ASSERT_THROWS_NOTHING(product = factory.create(progressUpdate));

    // Expecting 5x5x5 points; Signal equal for each box => 1/(10^3/5^3)
    const size_t expected_n_points = binning*binning*binning;
    const size_t expected_n_cells = binning*binning*binning;
    const size_t expected_n_signals = expected_n_cells;
    const double expected_signal_value = 1.0/((10.0*10.0*10.0)/(5.0*5.0*5.0));

    double* range = product->GetScalarRange();

    TSM_ASSERT_EQUALS("Should have one point per bin.", expected_n_points, product->GetNumberOfPoints());
    TSM_ASSERT_EQUALS("Should have one cells per bin", expected_n_cells, product->GetNumberOfCells());
    TSM_ASSERT_EQUALS("Should have signal flag", "signal", std::string(product->GetCellData()->GetArray(0)->GetName()));
    TSM_ASSERT_EQUALS("Should have one signal per bin", expected_n_signals, product->GetCellData()->GetArray(0)->GetSize());
    TSM_ASSERT_EQUALS("Should have a signal which is normalized to the 3D volume", expected_signal_value, range[0]);

    product->Delete();
  }

  void test_4DHistoWorkspace()
  {
    FakeProgressAction progressUpdate;

    // Create workspace with  5x5x5x5 binning, signal is 1 and extent for each dimension is 10
    size_t binning = 5;
    IMDHistoWorkspace_sptr ws = MDEventsTestHelper::makeFakeMDHistoWorkspace(1.0, 4, binning);
    vtkSplatterPlotFactory factory(ThresholdRange_scptr(new UserDefinedThresholdRange(0, 1)), "signal");
    factory.initialize(ws);
    vtkDataSet* product = NULL;

    TS_ASSERT_THROWS_NOTHING(product = factory.create(progressUpdate));

    // Expecting 5x5x5 points; Signal equal for each box => 1/(10^4/5^4)
    const size_t expected_n_points = binning*binning*binning;
    const size_t expected_n_cells = binning*binning*binning;
    const size_t expected_n_signals = expected_n_cells;
    const double expected_signal_value = 1.0/((10.0*10.0*10.0*10.0)/(5.0*5.0*5.0*5.0));

    double* range = product->GetScalarRange();

    TSM_ASSERT_EQUALS("Should have one point per bin.", expected_n_points, product->GetNumberOfPoints());
    TSM_ASSERT_EQUALS("Should have one cells per bin", expected_n_cells, product->GetNumberOfCells());
    TSM_ASSERT_EQUALS("Should have signal flag", "signal", std::string(product->GetCellData()->GetArray(0)->GetName()));
    TSM_ASSERT_EQUALS("Should have one signal per bin", expected_n_signals, product->GetCellData()->GetArray(0)->GetSize());
    TSM_ASSERT_EQUALS("Should have a signal which is normalized to the 4D volume", expected_signal_value, range[0]);

    product->Delete();
  }

  void test_3DWorkspace()
  {
    FakeProgressAction progressUpdate;

    Mantid::DataObjects::MDEventWorkspace3Lean::sptr ws = MDEventsTestHelper::makeMDEW<3>(10, 0.0, 10.0, 1);
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

    Mantid::DataObjects::MDEventWorkspace4Lean::sptr ws = MDEventsTestHelper::makeMDEW<4>(5, -10.0, 10.0, 1);
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

    product->Delete();
  }

};

#endif
