#ifndef VTK_MD_0D_FACTORY_TEST
#define VTK_MD_0D_FACTORY_TEST
#include <cxxtest/TestSuite.h>

#include "MantidVatesAPI/vtkMD0DFactory.h"
#include "MantidTestHelpers/MDEventsTestHelper.h"
#include "MantidVatesAPI/UserDefinedThresholdRange.h"
#include "MantidVatesAPI/NoThresholdRange.h"

#include "MockObjects.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "MantidVatesAPI/vtkStructuredGrid_Silent.h"

using namespace Mantid;
using namespace Mantid::VATES;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace testing;


class vtkMD0DFactoryTest : public CxxTest::TestSuite
{
public:

  void testCreatesA0DDataSet()
  {
    // Arrange
    FakeProgressAction progressUpdater;
    vtkMD0DFactory factory(ThresholdRange_scptr(new UserDefinedThresholdRange(0, 1)), "signal");

    vtkDataSet* dataSet = NULL;

    // Assert
    TSM_ASSERT_THROWS_NOTHING("0D factory should create data set without exceptions", dataSet  = factory.create(progressUpdater));
    TSM_ASSERT("Should have exactly one point", dataSet->GetNumberOfPoints() == 1);
    TSM_ASSERT("Should have exactly one cell", dataSet->GetNumberOfCells() == 1);
  }
};


#endif
