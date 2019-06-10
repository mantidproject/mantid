// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef VTK_DATASET_TO_WS_IMPLICITFUNCTION_TEST
#define VTK_DATASET_TO_WS_IMPLICITFUNCTION_TEST

#include "MantidGeometry/MDGeometry/MDImplicitFunction.h"
#include "MantidVatesAPI/vtkDataSetToImplicitFunction.h"
#include "MantidVatesAPI/vtkStructuredGrid_Silent.h"
#include "MockObjects.h"
#include <cxxtest/TestSuite.h>
#include <vtkDataSet.h>
#include <vtkNew.h>

using namespace Mantid::VATES;

//=====================================================================================
// Functional tests
//=====================================================================================
class vtkDataSetToImplicitFunctionTest : public CxxTest::TestSuite {

public:
  void testThrowIfvtkDataSetNull() {
    vtkDataSet *nullArg = nullptr;
    TS_ASSERT_THROWS(vtkDataSetToImplicitFunction temp(nullArg),
                     const std::runtime_error &);
  }

  void testNoImplcitFunction() {
    vtkNew<vtkStructuredGrid> ds;
    ds->SetFieldData(createFieldDataWithCharArray("<MDInstruction/>"));

    vtkDataSetToImplicitFunction extractor(ds.GetPointer());
    std::unique_ptr<Mantid::Geometry::MDImplicitFunction> func;
    TS_ASSERT_THROWS_NOTHING(
        func = std::unique_ptr<Mantid::Geometry::MDImplicitFunction>(
            extractor.execute()));
    TS_ASSERT_EQUALS("NullImplicitFunction", func->getName());
  }

  void testStaticUsage() {
    vtkNew<vtkStructuredGrid> ds;
    ds->SetFieldData(createFieldDataWithCharArray("<MDInstruction/>"));

    std::unique_ptr<Mantid::Geometry::MDImplicitFunction> func;
    TS_ASSERT_THROWS_NOTHING(
        func = std::unique_ptr<Mantid::Geometry::MDImplicitFunction>(
            vtkDataSetToImplicitFunction::exec(ds.GetPointer())));
    TS_ASSERT_EQUALS("NullImplicitFunction", func->getName());
  }
};

#endif
