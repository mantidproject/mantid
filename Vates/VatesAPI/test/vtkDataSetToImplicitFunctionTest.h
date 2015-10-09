#ifndef VTK_DATASET_TO_WS_IMPLICITFUNCTION_TEST
#define VTK_DATASET_TO_WS_IMPLICITFUNCTION_TEST 

#include <cxxtest/TestSuite.h>
#include "MantidVatesAPI/vtkDataSetToImplicitFunction.h"
#include "MantidGeometry/MDGeometry/MDImplicitFunction.h"
#include "MockObjects.h"
#include <vtkDataSet.h>
#include "MantidVatesAPI/vtkStructuredGrid_Silent.h"

using namespace Mantid::VATES;


//=====================================================================================
// Functional tests
//=====================================================================================
class vtkDataSetToImplicitFunctionTest : public CxxTest::TestSuite
{

public:

  void testThrowIfvtkDataSetNull()
  {
    vtkDataSet* nullArg = NULL;
    TS_ASSERT_THROWS(vtkDataSetToImplicitFunction temp(nullArg), std::runtime_error);
  }

  //void testExecution()
  //{
  //  vtkStructuredGrid* ds = vtkStructuredGrid::New();
  //  ds->SetFieldData(createFieldDataWithCharArray(constructXML()));

  //  vtkDataSetToImplicitFunction extractor(ds);
  //  Mantid::Geometry::MDImplicitFunction* func = NULL;
  //  TS_ASSERT_THROWS_NOTHING(func = extractor.execute());
  //  TS_ASSERT_EQUALS("PlaneImplicitFunction", func->getName());
  //  ds->Delete();
  //  delete func;
  //}

  void testNoImplcitFunction()
  {
    vtkStructuredGrid* ds = vtkStructuredGrid::New();
    ds->SetFieldData(createFieldDataWithCharArray("<MDInstruction/>"));

    vtkDataSetToImplicitFunction extractor(ds);
    Mantid::Geometry::MDImplicitFunction* func = NULL;
    TS_ASSERT_THROWS_NOTHING(func = extractor.execute());
    TS_ASSERT_EQUALS("NullImplicitFunction", func->getName());
    ds->Delete();
    delete func;
  }

  void testStaticUsage()
  {
    vtkStructuredGrid* ds = vtkStructuredGrid::New();
    ds->SetFieldData(createFieldDataWithCharArray("<MDInstruction/>"));

    Mantid::Geometry::MDImplicitFunction* func = NULL;
    TS_ASSERT_THROWS_NOTHING(func = vtkDataSetToImplicitFunction::exec(ds));
    TS_ASSERT_EQUALS("NullImplicitFunction", func->getName());
    ds->Delete();
    delete func;
  }

};

#endif
