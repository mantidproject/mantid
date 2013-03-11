#ifndef MANTID_VATES_VTKDATASETTONONORTHOGONALDATASETTEST_H_
#define MANTID_VATES_VTKDATASETTONONORTHOGONALDATASETTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidVatesAPI/vtkDataSetToNonOrthogonalDataSet.h"

#include <vtkRectilinearGrid.h>

using Mantid::VATES::vtkDataSetToNonOrthogonalDataSet;

class vtkDataSetToNonOrthogonalDataSetTest : public CxxTest::TestSuite
{
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

};


#endif /* MANTID_VATESAPI_VTKDATASETTONONORTHOGONALDATASETTEST_H_ */
