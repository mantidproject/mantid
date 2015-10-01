#ifndef MANTID_VATESAPI_VTKDATASETTOSCALEDDATASETTEST_H_
#define MANTID_VATESAPI_VTKDATASETTOSCALEDDATASETTEST_H_

#include "MantidTestHelpers/MDEventsTestHelper.h"
#include "MantidAPI/IMDWorkspace.h"
#include "MantidVatesAPI/FieldDataToMetadata.h"
#include "MantidVatesAPI/MetadataJsonManager.h"
#include "MantidVatesAPI/MetadataToFieldData.h"
#include "MantidVatesAPI/VatesConfigurations.h"
#include "MantidVatesAPI/NoThresholdRange.h"
#include "MantidVatesAPI/vtkDataSetToScaledDataSet.h"
#include "MantidVatesAPI/vtkMDHexFactory.h"
#include "MockObjects.h"

#include <cxxtest/TestSuite.h>
#include <vtkCellData.h>
#include <vtkDataSet.h>
#include <vtkFieldData.h>
#include <vtkFloatArray.h>
#include <vtkMatrix4x4.h>
#include <vtkPVChangeOfBasisHelper.h>
#include <vtkSmartPointer.h>
#include <vtkUnsignedCharArray.h>
#include <vtkUnstructuredGrid.h>
#include <vtkPointSet.h>

using namespace Mantid::DataObjects;
using namespace Mantid::VATES;

class vtkDataSetToScaledDataSetTest : public CxxTest::TestSuite {
private:
  vtkUnstructuredGrid *makeDataSet() {
    FakeProgressAction progressUpdate;
    MDEventWorkspace3Lean::sptr ws =
        MDEventsTestHelper::makeMDEW<3>(8, -10.0, 10.0, 1);
    vtkMDHexFactory factory(ThresholdRange_scptr(new NoThresholdRange),
                            VolumeNormalization);
    factory.initialize(ws);
    return vtkUnstructuredGrid::SafeDownCast(factory.create(progressUpdate));
  }

  vtkUnstructuredGrid *makeDataSetWithNonOrthogonal() {
    auto grid = makeDataSet();
    auto u = vtkVector3d(4, 4, 0);
    auto v = vtkVector3d(-2, 2, 0);
    auto w = vtkVector3d(0, 0, 8);

    vtkSmartPointer<vtkMatrix4x4> cobMatrix = vtkSmartPointer<vtkMatrix4x4>::New();
    cobMatrix->Identity();
    std::copy(u.GetData(), u.GetData() + 3, cobMatrix->Element[0]);
    std::copy(v.GetData(), v.GetData() + 3, cobMatrix->Element[1]);
    std::copy(w.GetData(), w.GetData() + 3, cobMatrix->Element[2]);
    cobMatrix->Transpose();

    vtkPVChangeOfBasisHelper::AddChangeOfBasisMatrixToFieldData(grid, cobMatrix);
    return grid;
  }

  vtkUnstructuredGrid *makeDataSetWithJsonMetadata() {
    vtkUnstructuredGrid *data = makeDataSet();

    MetadataJsonManager manager;
    std::string instrument = "OSIRIS";
    manager.setInstrument(instrument);
    std::string jsonString = manager.getSerializedJson();

    MetadataToFieldData convert;
    VatesConfigurations config;
    vtkFieldData *fieldData = data->GetFieldData();
    convert(fieldData, jsonString, config.getMetadataIdJson().c_str());

    data->SetFieldData(fieldData);

    return data;
  }

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static vtkDataSetToScaledDataSetTest *createSuite() {
    return new vtkDataSetToScaledDataSetTest();
  }
  static void destroySuite(vtkDataSetToScaledDataSetTest *suite) {
    delete suite;
  }

  void testThrowIfInputNull() {
    vtkUnstructuredGrid *in = NULL;

    vtkDataSetToScaledDataSet scaler;

    TS_ASSERT_THROWS(scaler.execute(1, 1, 1, in),
                     std::runtime_error);
  }

  void testExecution() {

    vtkDataSetToScaledDataSet scaler;
    vtkUnstructuredGrid *in = makeDataSet();
    vtkPointSet* out = scaler.execute(0.1, 0.5, 0.2,in);

    // Check bounds are scaled
    double *bb = out->GetBounds();
    TS_ASSERT_EQUALS(-1.0, bb[0]);
    TS_ASSERT_EQUALS(1.0, bb[1]);
    TS_ASSERT_EQUALS(-5.0, bb[2]);
    TS_ASSERT_EQUALS(5.0, bb[3]);
    TS_ASSERT_EQUALS(-2.0, bb[4]);
    TS_ASSERT_EQUALS(2.0, bb[5]);

    // Check that the Change-Of-Basis-Matrix is correct
    vtkSmartPointer<vtkMatrix4x4> cobMatrix =
        vtkPVChangeOfBasisHelper::GetChangeOfBasisMatrix(out);
    TS_ASSERT_EQUALS(0.1, cobMatrix->Element[0][0])
    TS_ASSERT_EQUALS(0.0, cobMatrix->Element[0][1])
    TS_ASSERT_EQUALS(0.0, cobMatrix->Element[0][2])
    TS_ASSERT_EQUALS(0.0, cobMatrix->Element[0][3])

    TS_ASSERT_EQUALS(0.0, cobMatrix->Element[1][0])
    TS_ASSERT_EQUALS(0.5, cobMatrix->Element[1][1])
    TS_ASSERT_EQUALS(0.0, cobMatrix->Element[1][2])
    TS_ASSERT_EQUALS(0.0, cobMatrix->Element[1][3])

    TS_ASSERT_EQUALS(0.0, cobMatrix->Element[2][0])
    TS_ASSERT_EQUALS(0.0, cobMatrix->Element[2][1])
    TS_ASSERT_EQUALS(0.2, cobMatrix->Element[2][2])
    TS_ASSERT_EQUALS(0.0, cobMatrix->Element[2][3])

    TS_ASSERT_EQUALS(0.0, cobMatrix->Element[3][0])
    TS_ASSERT_EQUALS(0.0, cobMatrix->Element[3][1])
    TS_ASSERT_EQUALS(0.0, cobMatrix->Element[3][2])
    TS_ASSERT_EQUALS(1.0, cobMatrix->Element[3][3])

    // Check the bounding box element for axes
    double bounds[6] = {0, 0, 0, 0, 0, 0};
    vtkPVChangeOfBasisHelper::GetBoundingBoxInBasis(out, bounds);

    TS_ASSERT_EQUALS(-10.0, bounds[0]);
    TS_ASSERT_EQUALS(10.0, bounds[1]);
    TS_ASSERT_EQUALS(-10.0, bounds[2]);
    TS_ASSERT_EQUALS(10.0, bounds[3]);
    TS_ASSERT_EQUALS(-10.0, bounds[4]);
    TS_ASSERT_EQUALS(10.0, bounds[5]);

    in->Delete();
    out->Delete();
  }

  void testJsonMetadataExtractionFromScaledDataSet() {
    // Arrange
    vtkUnstructuredGrid *in = makeDataSetWithJsonMetadata();

    // Act
    vtkDataSetToScaledDataSet scaler;
    vtkPointSet* out = scaler.execute(0.1, 0.5, 0.2, in);

    vtkFieldData *fieldData = out->GetFieldData();
    MetadataJsonManager manager;
    VatesConfigurations config;
    FieldDataToMetadata convert;

    std::string jsonString = convert(fieldData, config.getMetadataIdJson());
    manager.readInSerializedJson(jsonString);

    // Assert
    TS_ASSERT("OSIRIS" == manager.getInstrument());

    in->Delete();
    out->Delete();
  }

  void testExecutionWithNonOrthogonalDataSet() {

    vtkDataSetToScaledDataSet scaler;
    vtkUnstructuredGrid *in = makeDataSetWithNonOrthogonal();
    vtkPointSet* out = scaler.execute(0.25, 0.5, 0.125, in);

    // Check bounds are scaled
    double *bb = out->GetBounds();
    TS_ASSERT_EQUALS(-10.0/4, bb[0]);
    TS_ASSERT_EQUALS(10.0/4, bb[1]);
    TS_ASSERT_EQUALS(-10.0/2, bb[2]);
    TS_ASSERT_EQUALS(10.0/2, bb[3]);
    TS_ASSERT_EQUALS(-10.0/8, bb[4]);
    TS_ASSERT_EQUALS(10.0/8, bb[5]);

    auto cobMatrix = vtkPVChangeOfBasisHelper::GetChangeOfBasisMatrix(out);

    TS_ASSERT_EQUALS(1.0, cobMatrix->Element[0][0])
    TS_ASSERT_EQUALS(-1.0, cobMatrix->Element[0][1])
    TS_ASSERT_EQUALS(0.0, cobMatrix->Element[0][2])
    TS_ASSERT_EQUALS(0.0, cobMatrix->Element[0][3])

    TS_ASSERT_EQUALS(1.0, cobMatrix->Element[1][0])
    TS_ASSERT_EQUALS(1.0, cobMatrix->Element[1][1])
    TS_ASSERT_EQUALS(0.0, cobMatrix->Element[1][2])
    TS_ASSERT_EQUALS(0.0, cobMatrix->Element[1][3])

    TS_ASSERT_EQUALS(0.0, cobMatrix->Element[2][0])
    TS_ASSERT_EQUALS(0.0, cobMatrix->Element[2][1])
    TS_ASSERT_EQUALS(1.0, cobMatrix->Element[2][2])
    TS_ASSERT_EQUALS(0.0, cobMatrix->Element[2][3])

    TS_ASSERT_EQUALS(0.0, cobMatrix->Element[3][0])
    TS_ASSERT_EQUALS(0.0, cobMatrix->Element[3][1])
    TS_ASSERT_EQUALS(0.0, cobMatrix->Element[3][2])
    TS_ASSERT_EQUALS(1.0, cobMatrix->Element[3][3])

    // Check the bounding box element for axes
    double bounds[6] = {0, 0, 0, 0, 0, 0};
    vtkPVChangeOfBasisHelper::GetBoundingBoxInBasis(out, bounds);

    TS_ASSERT_EQUALS(-10.0, bounds[0]);
    TS_ASSERT_EQUALS(10.0, bounds[1]);
    TS_ASSERT_EQUALS(-10.0, bounds[2]);
    TS_ASSERT_EQUALS(10.0, bounds[3]);
    TS_ASSERT_EQUALS(-10.0, bounds[4]);
    TS_ASSERT_EQUALS(10.0, bounds[5]);

    in->Delete();
    out->Delete();
  }

};

#endif /* MANTID_VATESAPI_VTKDATASETTOSCALEDDATASETTEST_H_ */
