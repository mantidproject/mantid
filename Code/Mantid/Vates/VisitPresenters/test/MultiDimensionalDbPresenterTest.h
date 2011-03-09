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

private:

  static std::string getTestFileName(){return "fe_demo_30.sqw";}

public:

//Simple schenario testing end-to-end working of this presenter.
void testConstruction()
{
  MultiDimensionalDbPresenter mdPresenter;
  mdPresenter.execute(getTestFileName());

  vtkDataArray* data = mdPresenter.getScalarDataFromTimeBin(1, "signal");
  vtkDataSet* visData = mdPresenter.getMesh();
  TSM_ASSERT_EQUALS("Incorrect number of scalar signal points.", 125000, data->GetSize());
  TSM_ASSERT_EQUALS("Incorrect number of visualisation vtkPoints generated", 132651, visData->GetNumberOfPoints());
  TSM_ASSERT_EQUALS("Incorrect number of timesteps returned", 30, mdPresenter.getNumberOfTimesteps());
  data->Delete();
  visData->Delete();
}

void testGetCycles()
{
  MultiDimensionalDbPresenter mdPresenter;
  mdPresenter.execute(getTestFileName());
  std::vector<int> vecCycles = mdPresenter.getCycles();
  TSM_ASSERT_EQUALS("Wrong number of cycles in cycles collection.", vecCycles.size(), mdPresenter.getNumberOfTimesteps());
}

void testGetTimesteps()
{
  MultiDimensionalDbPresenter mdPresenter;
  mdPresenter.execute(getTestFileName());
  std::vector<double> vecTimes = mdPresenter.getTimesteps();
  TSM_ASSERT_EQUALS("Wrong number of times in times collection.", vecTimes.size(), mdPresenter.getNumberOfTimesteps());
}

void testGetScalarDataThrows()
{
  MultiDimensionalDbPresenter mdPresenter;

  //No execution call. Test that type cannot be used improperly.

  TSM_ASSERT_THROWS("Accessing scalar data without first calling execute should not be possible", mdPresenter.getScalarDataFromTimeBin(1, "signal"), std::runtime_error);
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

  TSM_ASSERT_THROWS("Accessing timestep number data without first calling execute should not be possible", mdPresenter.getNumberOfTimesteps(), std::runtime_error);
}

void testGetCylesThrows()
{
  MultiDimensionalDbPresenter mdPresenter;

  //No execution call. Test that type cannot be used improperly.

  TSM_ASSERT_THROWS("Accessing cycles data without first calling execute should not be possible", mdPresenter.getCycles(), std::runtime_error);

}

void testGetTimestepsThrows()
{

  MultiDimensionalDbPresenter mdPresenter;

  //No execution call. Test that type cannot be used improperly.

  TSM_ASSERT_THROWS("Accessing timestep data without first calling execute should not be possible", mdPresenter.getTimesteps(), std::runtime_error);

}


};

#endif
