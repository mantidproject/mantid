#ifndef MANTID_VATES_VTKDATASETTONONORTHOGONALDATASETTEST_H_
#define MANTID_VATES_VTKDATASETTONONORTHOGONALDATASETTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidVatesAPI/vtkDataSetToNonOrthogonalDataSet.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/ExperimentInfo.h"
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidKernel/Matrix.h"
#include "MantidMDEvents/CoordTransformAffine.h"
#include "MantidTestHelpers/MDEventsTestHelper.h"

#include <vtkDataArray.h>
#include <vtkFieldData.h>
#include <vtkFloatArray.h>
#include <vtkPoints.h>
#include <vtkRectilinearGrid.h>
#include <vtkUnstructuredGrid.h>

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::MDEvents;
using namespace Mantid::MDEvents::MDEventsTestHelper;
using namespace Mantid::VATES;

class vtkDataSetToNonOrthogonalDataSetTest : public CxxTest::TestSuite
{
private:

  std::string createMantidWorkspace()
  {
    // Creating an MDEventWorkspace as the content is not germain to the
    // information necessary for the non-orthogonal axes
    std::string wsName = "simpleWS";
    IMDEventWorkspace_sptr ws = makeAnyMDEW<MDEvent<4>, 4>(1, 0.0, 1.0, 1, wsName);

    // Set the UB matrix
    ExperimentInfo_sptr expInfo = ExperimentInfo_sptr(new ExperimentInfo());
    ws->addExperimentInfo(expInfo);

    IAlgorithm_sptr alg = AlgorithmManager::Instance().create("SetUB");
    alg->initialize();
    alg->setRethrows(true);
    alg->setProperty("Workspace", wsName);
    alg->setProperty("a", 3.643);
    alg->setProperty("b", 3.643);
    alg->setProperty("c", 5.781);
    alg->setProperty("alpha", 90.0);
    alg->setProperty("beta", 90.0);
    alg->setProperty("gamma", 120.0);
    std::vector<double> uVec {1, 1, 0};
    std::vector<double> vVec {0, 0, 1};
    alg->setProperty("u", uVec);
    alg->setProperty("v", vVec);
    alg->execute();

    // Create the coordinate transformation information
    std::vector<Mantid::coord_t> affMatVals {1, 0, 0, 0, 0,
                                             0, 0, 1, 0, 0,
                                             0, 0, 0, 1, 0,
                                             0, 1, 0, 0, 0,
                                             0, 0, 0, 0, 1};
    CoordTransformAffine affMat(4, 4);
    affMat.setMatrix(Matrix<Mantid::coord_t>(affMatVals));

    // Create the transform (W) matrix
    DblMatrix wMat(3, 3, true);
    ws->setWTransf(wMat);

    return wsName;
  }

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

  float *getRangeComp(vtkDataSet *ds, std::string fieldname, int size)
  {
    vtkDataArray *arr = ds->GetFieldData()->GetArray(fieldname.c_str());
    vtkFloatArray *farr = vtkFloatArray::SafeDownCast(arr);
    float *vals = new float[size];
    farr->GetTupleValue(0, vals);
    return vals;
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
    std::string wsName = createMantidWorkspace();
    vtkUnstructuredGrid *ds = createSingleVoxelPoints();
    vtkDataSetToNonOrthogonalDataSet converter(ds);
    TS_ASSERT_THROWS_NOTHING(converter.execute());
    // Now, check some values
    /// Get the (1,1,1) point
    const double eps = 1.0e-5;
    double *point = ds->GetPoint(6);
    TS_ASSERT_DELTA(point[0], 1.5, eps);
    TS_ASSERT_DELTA(point[1], 1.0, eps);
    TS_ASSERT_DELTA(point[2], 0.8660254, eps);
    // See if the basis vectors are available
    float *xBasis = getRangeComp(ds, "AxisBaseForX", 3);
    TS_ASSERT_DELTA(xBasis[0], 1.0, eps);
    TS_ASSERT_DELTA(xBasis[1], 0.0, eps);
    TS_ASSERT_DELTA(xBasis[2], 0.0, eps);
    delete [] xBasis;
    float *yBasis = getRangeComp(ds, "AxisBaseForY", 3);
    TS_ASSERT_DELTA(yBasis[0], 0.0, eps);
    TS_ASSERT_DELTA(yBasis[1], 1.0, eps);
    TS_ASSERT_DELTA(yBasis[2], 0.0, eps);
    delete [] yBasis;
    float *zBasis = getRangeComp(ds, "AxisBaseForZ", 3);
    TS_ASSERT_DELTA(zBasis[0], 0.5, eps);
    TS_ASSERT_DELTA(zBasis[1], 0.0, eps);
    TS_ASSERT_DELTA(zBasis[2], 0.8660254, eps);
    delete [] zBasis;
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
