#ifndef VTK_DATASET_TO_WS_IMPLICITFUNCTION_TEST
#define VTK_DATASET_TO_WS_IMPLICITFUNCTION_TEST 

#include <cxxtest/TestSuite.h>
#include "MantidVatesAPI/vtkDataSetToImplicitFunction.h"
#include "MockObjects.h"
#include <vtkDataSet.h>
#include <vtkStructuredGrid.h>

using namespace Mantid::VATES;


//=====================================================================================
// Functional tests
//=====================================================================================
class vtkDataSetToImplicitFunctionTest : public CxxTest::TestSuite
{

private:

  // Helper method. Create xml. Notice this is a subset of the full xml-schema, see Architectural design document.
  static std::string constructXML()
  {
    return std::string("<MDInstruction>") +
      "<Function>" + 
      "<Type>PlaneImplicitFunction</Type>" +
      "<ParameterList>" +
      "<Parameter>" +
      "<Type>NormalParameter</Type>" +
      "<Value>1, -1, 1</Value>" +
      "</Parameter>" +
      "<Parameter>" +
      "<Type>OriginParameter</Type>" +
      "<Value>0, 1, 0</Value>" +
      "</Parameter>" +
      "<Parameter>" +
      "<Type>WidthParameter</Type>" +
      "<Value>1</Value>" +
      "</Parameter>" +
      "</ParameterList>" +
      "</Function>" +
      "</MDInstruction>";
  }

public:

  void testThrowIfvtkDataSetNull()
  {
    vtkDataSet* nullArg = NULL;
    TS_ASSERT_THROWS(vtkDataSetToImplicitFunction temp(nullArg), std::runtime_error);
  }

  void testExecution()
  {
    vtkStructuredGrid* ds = vtkStructuredGrid::New();
    ds->SetFieldData(createFieldDataWithCharArray(constructXML()));

    vtkDataSetToImplicitFunction extractor(ds);
    Mantid::API::ImplicitFunction* func = NULL;
    TS_ASSERT_THROWS_NOTHING(func = extractor.execute());
    TS_ASSERT_EQUALS("PlaneImplicitFunction", func->getName());
    ds->Delete();
    delete func;
  }

  void testNoImplcitFunction()
  {
    vtkStructuredGrid* ds = vtkStructuredGrid::New();
    ds->SetFieldData(createFieldDataWithCharArray("<MDInstruction/>"));

    vtkDataSetToImplicitFunction extractor(ds);
    Mantid::API::ImplicitFunction* func = NULL;
    TS_ASSERT_THROWS_NOTHING(func = extractor.execute());
    TS_ASSERT_EQUALS("NullImplicitFunction", func->getName());
    ds->Delete();
    delete func;
  }

  void testStaticUsage()
  {
    vtkStructuredGrid* ds = vtkStructuredGrid::New();
    ds->SetFieldData(createFieldDataWithCharArray(constructXML()));

    Mantid::API::ImplicitFunction* func = NULL;
    TS_ASSERT_THROWS_NOTHING(func = vtkDataSetToImplicitFunction::exec(ds));
    TS_ASSERT_EQUALS("PlaneImplicitFunction", func->getName());
    ds->Delete();
    delete func;
  }

};

#endif