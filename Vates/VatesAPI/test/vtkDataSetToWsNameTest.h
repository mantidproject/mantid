#ifndef VTK_DATASET_TO_WS_NAME_TEST
#define VTK_DATASET_TO_WS_NAME_TEST 

#include <cxxtest/TestSuite.h>
#include "MantidVatesAPI/vtkDataSetToWsName.h"
#include "MockObjects.h"
#include "MantidVatesAPI/vtkStructuredGrid_Silent.h"

using namespace Mantid::VATES;


//=====================================================================================
// Functional tests
//=====================================================================================
class vtkDataSetToWsNameTest : public CxxTest::TestSuite
{

private:

  // Helper method. Create xml. Notice this is a subset of the full xml-schema, see Architectural design document.
  static std::string constructXML()
  {
    return std::string("<?xml version=\"1.0\" encoding=\"utf-8\"?>") +
      "<MDInstruction>" +
      "<MDWorkspaceName>WS_NAME</MDWorkspaceName>"
      "</MDInstruction>";
  }


public:

  void testThrowIfvtkDataSetNull()
  {
    vtkDataSet* nullArg = NULL;
    TS_ASSERT_THROWS(vtkDataSetToWsName temp(nullArg), std::runtime_error);
  }

  void testExecution()
  {
    vtkStructuredGrid* ds = vtkStructuredGrid::New();
    ds->SetFieldData(createFieldDataWithCharArray(constructXML()));

    vtkDataSetToWsName extractor(ds);
    TS_ASSERT_EQUALS("WS_NAME", extractor.execute());
    ds->Delete();
  }

  void testStaticUsage()
  {
    vtkStructuredGrid* ds = vtkStructuredGrid::New();
    ds->SetFieldData(createFieldDataWithCharArray(constructXML()));

    TS_ASSERT_EQUALS("WS_NAME", vtkDataSetToWsName::exec(ds));
  }

};

#endif
