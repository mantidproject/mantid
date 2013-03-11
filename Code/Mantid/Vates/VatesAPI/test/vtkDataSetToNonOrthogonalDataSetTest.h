#ifndef MANTID_VATES_VTKDATASETTONONORTHOGONALDATASETTEST_H_
#define MANTID_VATES_VTKDATASETTONONORTHOGONALDATASETTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidVatesAPI/vtkDataSetToNonOrthogonalDataSet.h"

#include <vtkPoints.h>
#include <vtkRectilinearGrid.h>
#include <vtkUnstructuredGrid.h>

using namespace Mantid::VATES;

class vtkDataSetToNonOrthogonalDataSetTest : public CxxTest::TestSuite
{
private:
  vtkUnstructuredGrid *createSingleVoxelPoints()
  {
    vtkUnstructuredGrid* ds = vtkUnstructuredGrid::New();
    vtkPoints *points = vtkPoints::New();
    points->Allocate(8);
    points->InsertNextPoint(0,0,0);
    points->InsertNextPoint(1,0,0);
    points->InsertNextPoint(1,1,0);
    points->InsertNextPoint(0,1,0);
    points->InsertNextPoint(0,0,1);
    points->InsertNextPoint(1,0,1);
    points->InsertNextPoint(1,1,1);
    points->InsertNextPoint(0,1,1);

    ds->SetPoints(points);
    return ds;
  }

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static vtkDataSetToNonOrthogonalDataSetTest *createSuite() { return new vtkDataSetToNonOrthogonalDataSetTest(); }
  static void destroySuite( vtkDataSetToNonOrthogonalDataSetTest *suite ) { delete suite; }

  void testThrowIfVtkDatasetNull()
  {
    vtkDataSet *dataset = NULL;
    TS_ASSERT_THROWS(vtkDataSetToNonOrthogonalDataSet temp(dataset),
                     std::runtime_error);
  }

  void testThrowIfVtkDatasetWrongType()
  {
    vtkRectilinearGrid *grid = vtkRectilinearGrid::New();
    vtkDataSetToNonOrthogonalDataSet converter(grid);
    TS_ASSERT_THROWS(converter.execute(), std::runtime_error);
    grid->Delete();
  }

  void testSimpleDataset()
  {
    vtkUnstructuredGrid *ds = createSingleVoxelPoints();
    vtkDataSetToNonOrthogonalDataSet converter(ds);
    TS_ASSERT_THROWS_NOTHING(converter.execute());
    // Now, check some values
    /// Get the (1,1,1) point
    const double eps = 1.0e-5;
    double *point = ds->GetPoint(6);
    TS_ASSERT_DELTA(point[0], 0.47544628, eps);
    TS_ASSERT_DELTA(point[1], 1.0, eps);
    TS_ASSERT_DELTA(point[2], 0.27449904, eps);
    ds->Delete();
  }

  void testStaticUseForSimpleDataSet()
  {
    vtkUnstructuredGrid *ds = createSingleVoxelPoints();
    TS_ASSERT_THROWS_NOTHING(vtkDataSetToNonOrthogonalDataSet::exec(ds));
    ds->Delete();
  }

};


#endif /* MANTID_VATESAPI_VTKDATASETTONONORTHOGONALDATASETTEST_H_ */
