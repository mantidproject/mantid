// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef VTK_DATASET_TO_WS_LOCATION_TEST
#define VTK_DATASET_TO_WS_LOCATION_TEST

#include "MantidVatesAPI/vtkDataSetToWsLocation.h"
#include "MantidVatesAPI/vtkStructuredGrid_Silent.h"
#include "MockObjects.h"
#include <cxxtest/TestSuite.h>
#include <vtkDataSet.h>
#include <vtkNew.h>

using namespace Mantid::VATES;

//=====================================================================================
// Functional tests
//=====================================================================================
class vtkDataSetToWsLocationTest : public CxxTest::TestSuite {

private:
  // Helper method. Create xml. Notice this is a subset of the full xml-schema,
  // see Architectural design document.
  static std::string constructXML() {
    return std::string("<?xml version=\"1.0\" encoding=\"utf-8\"?>") +
           "<MDInstruction>" +
           "<MDWorkspaceLocation>WS_LOCATION</MDWorkspaceLocation>"
           "</MDInstruction>";
  }

public:
  void testThrowIfvtkDataSetNull() {
    vtkDataSet *nullArg = nullptr;
    TS_ASSERT_THROWS(vtkDataSetToWsLocation temp(nullArg),
                     const std::runtime_error &);
  }

  void testExecution() {
    vtkNew<vtkStructuredGrid> ds;
    ds->SetFieldData(createFieldDataWithCharArray(constructXML()));

    vtkDataSetToWsLocation extractor(ds.GetPointer());
    TS_ASSERT_EQUALS("WS_LOCATION", extractor.execute());
  }

  void testStaticUsage() {
    vtkNew<vtkStructuredGrid> ds;
    ds->SetFieldData(createFieldDataWithCharArray(constructXML()));

    TS_ASSERT_EQUALS("WS_LOCATION",
                     vtkDataSetToWsLocation::exec(ds.GetPointer()));
  }
};

#endif
