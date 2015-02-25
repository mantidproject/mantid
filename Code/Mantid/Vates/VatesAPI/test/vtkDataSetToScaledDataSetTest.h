#ifndef MANTID_VATESAPI_VTKDATASETTOSCALEDDATASETTEST_H_
#define MANTID_VATESAPI_VTKDATASETTOSCALEDDATASETTEST_H_

#include "MantidTestHelpers/MDEventsTestHelper.h"
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
#include <vtkUnsignedCharArray.h>
#include <vtkUnstructuredGrid.h>

using namespace Mantid::MDEvents;
using namespace Mantid::VATES;

class vtkDataSetToScaledDataSetTest : public CxxTest::TestSuite
{
private:
  vtkUnstructuredGrid *makeDataSet()
  {
    FakeProgressAction progressUpdate;
    MDEventWorkspace3Lean::sptr ws = MDEventsTestHelper::makeMDEW<3>(8,
                                                                     -10.0,
                                                                     10.0,
                                                                     1);
    vtkMDHexFactory factory(ThresholdRange_scptr(new NoThresholdRange),
                            "signal");
    factory.initialize(ws);
    return vtkUnstructuredGrid::SafeDownCast(factory.create(progressUpdate));
  }

  float *getRangeComp(vtkDataSet *ds, std::string fieldname)
  {
    vtkDataArray *arr = ds->GetFieldData()->GetArray(fieldname.c_str());
    vtkFloatArray *farr = vtkFloatArray::SafeDownCast(arr);
    static float vals[2];
    farr->GetTupleValue(0, vals);
    return vals;
  }

  unsigned char *getRangeActiveComp(vtkDataSet *ds, int index)
  {
    vtkDataArray *arr = ds->GetFieldData()->GetArray("LabelRangeActiveFlag");
    vtkUnsignedCharArray *uarr = vtkUnsignedCharArray::SafeDownCast(arr);
    static unsigned char vals[1];
    uarr->GetTupleValue(index, vals);
    return vals;
  }

  vtkUnstructuredGrid  *makeDataSetWithJsonMetadata()
  {
     vtkUnstructuredGrid* data = makeDataSet();
     
      MetadataJsonManager manager;
      std::string instrument = "OSIRIS";
      manager.setInstrument(instrument);
      std::string jsonString = manager.getSerializedJson();
      
      MetadataToFieldData convert;
      VatesConfigurations config;
      vtkFieldData* fieldData = data->GetFieldData();
      convert(fieldData, jsonString, config.getMetadataIdJson().c_str());

      data->SetFieldData(fieldData);

      return data;
  }

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static vtkDataSetToScaledDataSetTest *createSuite() { return new vtkDataSetToScaledDataSetTest(); }
  static void destroySuite( vtkDataSetToScaledDataSetTest *suite ) { delete suite; }

  void testThrowIfInputNull()
  {
    vtkUnstructuredGrid *in = NULL;
    vtkUnstructuredGrid *out = vtkUnstructuredGrid::New();
    TS_ASSERT_THROWS(vtkDataSetToScaledDataSet scaler(in, out),
                     std::runtime_error);
  }

  void testThrowIfOutputNull()
  {
    vtkUnstructuredGrid *in = vtkUnstructuredGrid::New();
    vtkUnstructuredGrid *out = NULL;
    TS_ASSERT_THROWS(vtkDataSetToScaledDataSet scaler(in, out),
                     std::runtime_error);
  }

  void testExecThrowIfNoInit()
  {
    vtkUnstructuredGrid *in = vtkUnstructuredGrid::New();
    vtkUnstructuredGrid *out = vtkUnstructuredGrid::New();
    vtkDataSetToScaledDataSet scaler(in, out);
    TS_ASSERT_THROWS(scaler.execute(), std::runtime_error);
  }

  void testExecution()
  {
    vtkUnstructuredGrid *in = makeDataSet();
    vtkUnstructuredGrid *out = vtkUnstructuredGrid::New();
    vtkDataSetToScaledDataSet scaler(in, out);
    scaler.initialize(0.1, 0.5, 0.2);
    TS_ASSERT_THROWS_NOTHING(scaler.execute());
    // Check bounds are scaled
    double *bb = out->GetBounds();
    TS_ASSERT_EQUALS(-1.0, bb[0]);
    TS_ASSERT_EQUALS(1.0, bb[1]);
    TS_ASSERT_EQUALS(-5.0, bb[2]);
    TS_ASSERT_EQUALS(5.0, bb[3]);
    TS_ASSERT_EQUALS(-2.0, bb[4]);
    TS_ASSERT_EQUALS(2.0, bb[5]);
    // Check that the range metadata is set
    float *rangeX = getRangeComp(out, "LabelRangeForX");
    TS_ASSERT_EQUALS(-10.0, rangeX[0]);
    TS_ASSERT_EQUALS(10.0, rangeX[1]);
    float *rangeY = getRangeComp(out, "LabelRangeForY");
    TS_ASSERT_EQUALS(-10.0, rangeY[0]);
    TS_ASSERT_EQUALS(10.0, rangeY[1]);
    float *rangeZ = getRangeComp(out, "LabelRangeForZ");
    TS_ASSERT_EQUALS(-10.0, rangeZ[0]);
    TS_ASSERT_EQUALS(10.0, rangeZ[1]);
    // Check that the scaling transform metadata is set
    float *transformX = getRangeComp(out, "LinearTransformForX");
    TS_ASSERT_EQUALS(1.0/0.1, transformX[0]);
    TS_ASSERT_EQUALS(0.0, transformX[1]);
    float *transformY = getRangeComp(out, "LinearTransformForY");
    TS_ASSERT_EQUALS(1.0/0.5, transformY[0]);
    TS_ASSERT_EQUALS(0.0, transformY[1]);
    float *transformZ = getRangeComp(out, "LinearTransformForZ");
    TS_ASSERT_EQUALS(1.0/0.2, transformZ[0]);
    TS_ASSERT_EQUALS(0.0, transformZ[1]);
    // Check the active label range flags are set
    unsigned char *activeX = getRangeActiveComp(out, 0);
    TS_ASSERT_EQUALS(1, activeX[0]);
    unsigned char *activeY = getRangeActiveComp(out, 1);
    TS_ASSERT_EQUALS(1, activeY[0]);
    unsigned char *activeZ = getRangeActiveComp(out, 2);
    TS_ASSERT_EQUALS(1, activeZ[0]);

    in->Delete();
    out->Delete();
  }

  void testJsonMetadataExtractionFromScaledDataSet()
  {
    // Arrange
    vtkUnstructuredGrid *in = makeDataSetWithJsonMetadata();
    vtkUnstructuredGrid *out = vtkUnstructuredGrid::New();

    // Act
    vtkDataSetToScaledDataSet scaler(in, out);
    scaler.initialize(0.1, 0.5, 0.2);
    TS_ASSERT_THROWS_NOTHING(scaler.execute());

    vtkFieldData* fieldData = out->GetFieldData();
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
};


#endif /* MANTID_VATESAPI_VTKDATASETTOSCALEDDATASETTEST_H_ */
