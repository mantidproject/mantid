#ifndef MULTIDIMENSIONAL_DB_PRESENTER_TEST_H_
#define MULTIDIMENSIONAL_DB_PRESENTER_TEST_H_

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <cxxtest/TestSuite.h>
#include <vtkPointData.h>
#include "MantidVisitPresenters/MultiDimensionalDbPresenter.h"

using namespace Mantid::VATES;

class MultiDimensionalDbPresenterTest : public CxxTest::TestSuite
{

public:

//Simple schenario testing end-to-end working of this presenter.
void testConstruction()
{
  MultiDimensionalDbPresenter mdPresenter;
  mdPresenter.execute("fe_demo_bin.sqw");

  vtkDataArray* data = mdPresenter.getScalarData(1);
  vtkDataSet* visData = mdPresenter.getMesh();
  TSM_ASSERT_EQUALS("Incorrect number of scalar signal points.", 117649, data->GetSize());
  TSM_ASSERT_EQUALS("Incorrect number of visualisation vtkPoints generated", 125000, visData->GetNumberOfPoints());
  TSM_ASSERT_EQUALS("Incorrect number of timesteps returned", 50, mdPresenter.getNumberOfTimesteps());
}

void testGetScalarDataThrows()
{
  MultiDimensionalDbPresenter mdPresenter;

  //No execution call. Test that type cannot be used improperly.

  TSM_ASSERT_THROWS("Accessing scalar data without first calling execute should not be possible", mdPresenter.getScalarData(1), std::runtime_error);
}

void testGetMeshThrows()
{
  MultiDimensionalDbPresenter mdPresenter;

  //No execution call. Test that type cannot be used improperly.

  TSM_ASSERT_THROWS("Accessing mesh data without first calling execute should not be possible", mdPresenter.getMesh(), std::runtime_error);
}

void testGetNumberOfTimestepsThrows()
{
  MultiDimensionalDbPresenter mdPresenter;

  //No execution call. Test that type cannot be used improperly.

  TSM_ASSERT_THROWS("Accessing mesh data without first calling execute should not be possible", mdPresenter.getNumberOfTimesteps(), std::runtime_error);
}


};

#endif
