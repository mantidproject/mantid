#ifndef MANTID_VATESAPI_VTKDATASETTOPEAKSFILTEREDDATASETTEST_H_
#define MANTID_VATESAPI_VTKDATASETTOPEAKSFILTEREDDATASETTEST_H_

#include "MantidTestHelpers/MDEventsTestHelper.h"
#include "MantidVatesAPI/FieldDataToMetadata.h"
#include "MantidVatesAPI/MetadataJsonManager.h"
#include "MantidVatesAPI/MetadataToFieldData.h"
#include "MantidVatesAPI/VatesConfigurations.h"
#include "MantidVatesAPI/NoThresholdRange.h"
#include "MantidVatesAPI/vtkDataSetToPeaksFilteredDataSet.h"
#include "MantidVatesAPI/vtkSplatterPlotFactory.h"
#include "MantidVatesAPI/UserDefinedThresholdRange.h"
#include "MantidAPI/IPeaksWorkspace.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidGeometry/Crystal/PeakShape.h"
#include "MantidDataObjects/PeakShapeSpherical.h"
#include "MantidDataObjects/PeakShapeEllipsoid.h"
#include "MantidKernel/V3D.h"
#include "MockObjects.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cxxtest/TestSuite.h>
#include <vtkCellData.h>
#include <vtkDataSet.h>
#include <vtkFieldData.h>
#include <vtkFloatArray.h>
#include <vtkUnsignedCharArray.h>
#include <vtkUnstructuredGrid.h>

using namespace Mantid::MDEvents;
using namespace Mantid::VATES;
using namespace ::testing;

class MockPeak : public Mantid::DataObjects::Peak
{
public:
  MOCK_CONST_METHOD0(getHKL, Mantid::Kernel::V3D (void));
  MOCK_CONST_METHOD0(getQLabFrame, Mantid::Kernel::V3D (void));
  MOCK_CONST_METHOD0(getQSampleFrame, Mantid::Kernel::V3D (void));
};

class MockPeaksWorkspace : public Mantid::DataObjects::PeaksWorkspace
{
public:
  MOCK_CONST_METHOD0(getSpecialCoordinateSystem, Mantid::Kernel::SpecialCoordinateSystem());
  MOCK_CONST_METHOD0(getNumberPeaks, int());
  MOCK_METHOD1(getPeak, Mantid::DataObjects::Peak & (int peakNum));
  MOCK_CONST_METHOD2(createPeak, Mantid::API::IPeak* (Mantid::Kernel::V3D QLabFrame, double detectorDistance));
};


class vtkDataSetToPeaksFilteredDataSetTest : public CxxTest::TestSuite
{
private:
  vtkUnstructuredGrid* makeSplatterSourceGrid()
  {
    FakeProgressAction progressUpdate;
    Mantid::MDEvents::MDEventWorkspace3Lean::sptr ws = MDEventsTestHelper::makeMDEW<3>(10, -10.0, 10.0, 1);
    vtkSplatterPlotFactory factory(ThresholdRange_scptr(new UserDefinedThresholdRange(0, 1)), "signal");
    factory.initialize(ws);
    vtkDataSet* product = NULL;
    TS_ASSERT_THROWS_NOTHING(product = factory.create(progressUpdate));
    vtkUnstructuredGrid* splatData =  vtkUnstructuredGrid::SafeDownCast(product);
    return splatData;
  }

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static vtkDataSetToPeaksFilteredDataSetTest *createSuite() { return new vtkDataSetToPeaksFilteredDataSetTest(); }
  static void destroySuite( vtkDataSetToPeaksFilteredDataSetTest *suite ) { delete suite; }

  void do_test_peak_inSphere(vtkPoints* points,int numberOfPoints, double radius, Mantid::Kernel::V3D position, int& inside, int& outside, bool testingOutput)
  {
    for (int i = 0; i < numberOfPoints; i++)
    {
      double point[3];
      points->GetPoint(static_cast<vtkIdType>(i), point);

      double diffSquared = 0;

      for (int k = 0; k <3; k++)
      {
        diffSquared += (position[k]-point[k])*(position[k]-point[k]);
      }

      bool isInSphere = ((radius*radius - diffSquared) >= 0) ? true : false;
      isInSphere ? (inside++) : (outside++);

      // We expect only for the output, that all points are within the sphere
      if (testingOutput)
      {
        TSM_ASSERT("Should be insinde the sphere.", isInSphere);
      }
    }
  }

  void do_test_peaks(vtkUnstructuredGrid* in, vtkUnstructuredGrid* out, Mantid::Kernel::V3D position, double radius)
  {
    vtkPoints* inPoints = in->GetPoints();
    vtkPoints* outPoints = out->GetPoints();

    int numberOfInPoints = inPoints->GetNumberOfPoints();
    int numberOfOutPoints = outPoints->GetNumberOfPoints();

    int insideSphereInput = 0;
    int outsideSphereInput = 0;
    do_test_peak_inSphere(inPoints, numberOfInPoints, radius, position, insideSphereInput , outsideSphereInput, false);

    int insideSphereOutput = 0;
    int outsideSphereOutput = 0;
    do_test_peak_inSphere(outPoints, numberOfOutPoints, radius, position, insideSphereOutput , outsideSphereOutput, true);

    TSM_ASSERT("The number of elements inside the sphere should be the same for input and output.", insideSphereInput == insideSphereOutput);
  }

  void do_test_execute(vtkDataSetToPeaksFilteredDataSet peaksFilter, MockPeak& peak, Mantid::Kernel::V3D coordinate, Mantid::Kernel::SpecialCoordinateSystem coordinateSystem)
  {
    // Set up the peak
    switch(coordinateSystem)
    {
      case(Mantid::Kernel::SpecialCoordinateSystem::QLab):
        EXPECT_CALL(peak, getQLabFrame()).WillOnce(Return(coordinate));
        EXPECT_CALL(peak, getHKL()).Times(0);
        EXPECT_CALL(peak, getQSampleFrame()).Times(0);
        break;
      case(Mantid::Kernel::SpecialCoordinateSystem::HKL):
        EXPECT_CALL(peak, getQLabFrame()).Times(0);
        EXPECT_CALL(peak, getHKL()).WillOnce(Return(coordinate));
        EXPECT_CALL(peak, getQSampleFrame()).Times(0);
        break;
      case(Mantid::Kernel::SpecialCoordinateSystem::QSample):
        EXPECT_CALL(peak, getQLabFrame()).Times(0);
        EXPECT_CALL(peak, getHKL()).Times(0);
        EXPECT_CALL(peak, getQSampleFrame()).WillOnce(Return(coordinate));
        break;
      default:
        break;
    }

    // Set up the peaks workspace
    boost::shared_ptr<MockPeaksWorkspace> pw_ptr(new MockPeaksWorkspace());
    MockPeaksWorkspace & pw = *pw_ptr;

    EXPECT_CALL(pw, getNumberPeaks()).WillOnce(Return(1));
    EXPECT_CALL(pw, getPeak(_)).WillOnce(ReturnRef(peak));
    EXPECT_CALL(pw, getSpecialCoordinateSystem()).WillOnce(Return(coordinateSystem));

    peaksFilter.initialize(pw_ptr, 0.5, 0);
    TSM_ASSERT_THROWS_NOTHING("Should execute regularly.", peaksFilter.execute());
  }

  void testThrowIfInputNull()
  {
    vtkUnstructuredGrid *in = NULL;
    vtkUnstructuredGrid *out = vtkUnstructuredGrid::New();
    TS_ASSERT_THROWS(vtkDataSetToPeaksFilteredDataSet peaksFilter(in, out), std::runtime_error);
  }

  void testThrowIfOutputNull()
  {
    vtkUnstructuredGrid *in = vtkUnstructuredGrid::New();
    vtkUnstructuredGrid *out = NULL;
    TS_ASSERT_THROWS(vtkDataSetToPeaksFilteredDataSet peaksFilter(in, out), std::runtime_error);
  }

  void testExecThrowIfNoInit()
  {
    vtkUnstructuredGrid *in = vtkUnstructuredGrid::New();
    vtkUnstructuredGrid *out = vtkUnstructuredGrid::New();
    vtkDataSetToPeaksFilteredDataSet peaksFilter(in, out);
    TS_ASSERT_THROWS(peaksFilter.execute(), std::runtime_error);
  }

  void testExecutionWithSingleSphericalPeakInQSample()
  {
    // Arrange
    vtkUnstructuredGrid *in = makeSplatterSourceGrid();
    vtkUnstructuredGrid *out = vtkUnstructuredGrid::New();
    vtkDataSetToPeaksFilteredDataSet peaksFilter(in, out);

    Mantid::Kernel::V3D coordinate(0,0,0);
    double peakRadius = 10;
    Mantid::Kernel::SpecialCoordinateSystem coordinateSystem = Mantid::Kernel::SpecialCoordinateSystem::QSample;
    Mantid::Geometry::PeakShape_sptr shape(new Mantid::DataObjects::PeakShapeSpherical(peakRadius, coordinateSystem, "test", 1));
    MockPeak peak;
    peak.setPeakShape(shape);

     // Act
    do_test_execute(peaksFilter, peak, coordinate, coordinateSystem);

    // Assert
    do_test_peaks(in, out, coordinate, peakRadius);

    in->Delete();
    out->Delete();
  }

  void testExecutionWithSingleEllipsoidPeakInQSample()
  {
    // Arrange
    vtkUnstructuredGrid *in = makeSplatterSourceGrid();
    vtkUnstructuredGrid *out = vtkUnstructuredGrid::New();
    vtkDataSetToPeaksFilteredDataSet peaksFilter(in, out);

    Mantid::Kernel::V3D coordinate(0,0,0);
    double peakRadiusMax = 10;
    std::vector<double> radii;
    radii.push_back(peakRadiusMax);
    radii.push_back(9);
    radii.push_back(6);

    std::vector<Mantid::Kernel::V3D> directions;
    directions.push_back(Mantid::Kernel::V3D(0.0,1.0,0.0));
    directions.push_back(Mantid::Kernel::V3D(1.0,0.0,0.0));
    directions.push_back(Mantid::Kernel::V3D(0.0,0.0,1.0));

    Mantid::Kernel::SpecialCoordinateSystem coordinateSystem = Mantid::Kernel::SpecialCoordinateSystem::QSample;
    Mantid::Geometry::PeakShape_sptr shape(new Mantid::DataObjects::PeakShapeEllipsoid(directions, radii, radii, radii , coordinateSystem, "test", 1));
    MockPeak peak;
    peak.setPeakShape(shape);

     // Act
    do_test_execute(peaksFilter, peak, coordinate, coordinateSystem);

    // Assert
    do_test_peaks(in, out, coordinate, peakRadiusMax);

    in->Delete();
    out->Delete();
  }

  void testJsonMetadataExtractionFromScaledDataSet()
  {

  }
};


#endif