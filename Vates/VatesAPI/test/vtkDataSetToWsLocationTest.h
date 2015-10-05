#ifndef VTK_DATASET_TO_WS_LOCATION_TEST 
#define VTK_DATASET_TO_WS_LOCATION_TEST

#include <cxxtest/TestSuite.h>
#include "MantidVatesAPI/vtkDataSetToWsLocation.h"
#include <vtkDataSet.h>
#include "MantidVatesAPI/vtkStructuredGrid_Silent.h"
#include "MockObjects.h"

using namespace Mantid::VATES;


//=====================================================================================
// Functional tests
//=====================================================================================
class vtkDataSetToWsLocationTest : public CxxTest::TestSuite
{

private:

  // Helper method. Create xml. Notice this is a subset of the full xml-schema, see Architectural design document.
  static std::string constructXML()
  {
    return std::string("<?xml version=\"1.0\" encoding=\"utf-8\"?>") +
      "<MDInstruction>" +
      "<MDWorkspaceLocation>WS_LOCATION</MDWorkspaceLocation>"
      "</MDInstruction>";
  }

public:

  void testThrowIfvtkDataSetNull()
  {
    vtkDataSet* nullArg = NULL;
    TS_ASSERT_THROWS(vtkDataSetToWsLocation temp(nullArg), std::runtime_error);
  }

  void testExecution()
  {
    vtkStructuredGrid* ds = vtkStructuredGrid::New();
    ds->SetFieldData(createFieldDataWithCharArray(constructXML()));

    vtkDataSetToWsLocation extractor(ds);
    TS_ASSERT_EQUALS("WS_LOCATION", extractor.execute());
    ds->Delete();
  }

  void testStaticUsage()
  {
    vtkStructuredGrid* ds = vtkStructuredGrid::New();
    ds->SetFieldData(createFieldDataWithCharArray(constructXML()));

    TS_ASSERT_EQUALS("WS_LOCATION", vtkDataSetToWsLocation::exec(ds));
  }

};

#endif
