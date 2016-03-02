#ifndef VTK_MD_0D_FACTORY_TEST
#define VTK_MD_0D_FACTORY_TEST
#include <cxxtest/TestSuite.h>

#include "MantidVatesAPI/vtkMD0DFactory.h"
#include "MockObjects.h"

using namespace Mantid::VATES;

class vtkMD0DFactoryTest : public CxxTest::TestSuite
{
public:

  void testCreatesA0DDataSet()
  {
    // Arrange
    FakeProgressAction progressUpdater;
    vtkMD0DFactory factory;

    vtkSmartPointer<vtkDataSet> dataSet;

    // Assert
    TSM_ASSERT_THROWS_NOTHING(
        "0D factory should create data set without exceptions",
        dataSet = factory.create(progressUpdater));
    TSM_ASSERT("Should have exactly one point", dataSet->GetNumberOfPoints() == 1);
    TSM_ASSERT("Should have exactly one cell", dataSet->GetNumberOfCells() == 1);
  }
};


#endif
