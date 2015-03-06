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
#include "MantidDataObjects/NoShape.h"
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

  void do_test_peak_inSphere(vtkPoints* points,int numberOfPoints, double radius, Mantid::Kernel::V3D position, int& inside, int& outside, bool testingOutput, double radiusFactor)
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

      bool isInSphere = ((radius*radius*radiusFactor*radiusFactor - diffSquared) >= 0) ? true : false;
      isInSphere ? (inside++) : (outside++);

      // We expect only for the output, that all points are within the sphere
      if (testingOutput)
      {
        TSM_ASSERT("Should be insinde the sphere.", isInSphere);
      }
    }
  }

  void do_test_peaks(vtkUnstructuredGrid* in, vtkUnstructuredGrid* out, Mantid::Kernel::V3D position, double radius, double radiusFactor)
  {
    vtkPoints* inPoints = in->GetPoints();
    vtkPoints* outPoints = out->GetPoints();
    
    int numberOfInPoints = inPoints->GetNumberOfPoints();
    int numberOfOutPoints = outPoints->GetNumberOfPoints();

    int numCellsIn = in->GetNumberOfCells();
    int numCellsOut = out->GetNumberOfCells();

    int insideSphereInput = 0;
    int outsideSphereInput = 0;
    do_test_peak_inSphere(inPoints, numberOfInPoints, radius, position, insideSphereInput , outsideSphereInput, false, radiusFactor);

    int insideSphereOutput = 0;
    int outsideSphereOutput = 0;
    do_test_peak_inSphere(outPoints, numberOfOutPoints, radius, position, insideSphereOutput , outsideSphereOutput, true, radiusFactor);

    TSM_ASSERT("The number of elements inside the sphere should be the same for input and output.", insideSphereInput == insideSphereOutput);
  }

#if 0
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

    EXPECT_CALL(pw, getNumberPeaks()).Times(2).WillRepeatedly(Return(1));
    EXPECT_CALL(pw, getPeak(_)).WillOnce(ReturnRef(peak));
    EXPECT_CALL(pw, getSpecialCoordinateSystem()).WillOnce(Return(coordinateSystem));

    std::vector<Mantid::API::IPeaksWorkspace_sptr> peaksContainer;
    peaksContainer.push_back(pw_ptr);

    peaksFilter.initialize(peaksContainer, 0.5, 0);
    FakeProgressAction updateProgress;
    TSM_ASSERT_THROWS_NOTHING("Should execute regularly.", peaksFilter.execute(updateProgress));
  }
#endif
  void do_test_execute(vtkDataSetToPeaksFilteredDataSet peaksFilter, std::vector<std::pair<MockPeak&, Mantid::Kernel::V3D>> peakWsData, Mantid::Kernel::SpecialCoordinateSystem coordinateSystem) {
    std::vector<Mantid::API::IPeaksWorkspace_sptr> peaksContainer;
    for (std::vector<std::pair<MockPeak&, Mantid::Kernel::V3D>>::iterator it = peakWsData.begin(); it != peakWsData.end(); ++it) {
      // Set up the peak
      switch(coordinateSystem)
      {
        case(Mantid::Kernel::SpecialCoordinateSystem::QLab):
          EXPECT_CALL(it->first, getQLabFrame()).WillOnce(Return(it->second));
          EXPECT_CALL(it->first, getHKL()).Times(0);
          EXPECT_CALL(it->first, getQSampleFrame()).Times(0);
          break;
        case(Mantid::Kernel::SpecialCoordinateSystem::HKL):
          EXPECT_CALL(it->first, getQLabFrame()).Times(0);
          EXPECT_CALL(it->first, getHKL()).WillOnce(Return(it->second));
          EXPECT_CALL(it->first, getQSampleFrame()).Times(0);
          break;
        case(Mantid::Kernel::SpecialCoordinateSystem::QSample):
          EXPECT_CALL(it->first, getQLabFrame()).Times(0);
          EXPECT_CALL(it->first, getHKL()).Times(0);
          EXPECT_CALL(it->first, getQSampleFrame()).WillOnce(Return(it->second));
          break;
        default:
          break;
      }

      // Set up the peaks workspace
      boost::shared_ptr<MockPeaksWorkspace> pw_ptr(new MockPeaksWorkspace());
      MockPeaksWorkspace & pw = *pw_ptr;

      EXPECT_CALL(pw, getNumberPeaks()).Times(2).WillRepeatedly(Return(1));
      EXPECT_CALL(pw, getPeak(_)).WillOnce(ReturnRef(it->first));
      EXPECT_CALL(pw, getSpecialCoordinateSystem()).WillOnce(Return(coordinateSystem));
      peaksContainer.push_back(pw_ptr);
    }

    peaksFilter.initialize(peaksContainer, 0.5, 0);
    FakeProgressAction updateProgress;
    TSM_ASSERT_THROWS_NOTHING("Should execute regularly.", peaksFilter.execute(updateProgress));
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
    FakeProgressAction updateProgress;
    TS_ASSERT_THROWS(peaksFilter.execute(updateProgress), std::runtime_error);
  }

  void testExecutionWithSingleSphericalPeakInQSample()
  {
    // Arrange
    vtkUnstructuredGrid *in = makeSplatterSourceGrid();
    vtkUnstructuredGrid *out = vtkUnstructuredGrid::New();
    vtkDataSetToPeaksFilteredDataSet peaksFilter(in, out);

    Mantid::Kernel::V3D coordinate(0,0,0);
    // Note that the peak radius is not a 1-1 measure for which peaks will be culled and which not. 
    // The actual radius is multiplied by the radius factor.
    double peakRadius = 5;
    Mantid::Kernel::SpecialCoordinateSystem coordinateSystem = Mantid::Kernel::SpecialCoordinateSystem::QSample;
    Mantid::Geometry::PeakShape_sptr shape(new Mantid::DataObjects::PeakShapeSpherical(peakRadius, coordinateSystem, "test", 1));
    MockPeak peak;
    peak.setPeakShape(shape);

    std::vector<std::pair<MockPeak&, Mantid::Kernel::V3D>> fakeSinglePeakPeakWorkspaces;
    fakeSinglePeakPeakWorkspaces.push_back(std::pair<MockPeak&, Mantid::Kernel::V3D>(peak, coordinate));

     // Act
    do_test_execute(peaksFilter, fakeSinglePeakPeakWorkspaces, coordinateSystem);

    // Assert
    do_test_peaks(in, out, coordinate, peakRadius, peaksFilter.getRadiusFactor());

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
    double peakRadiusMax = 7;
    std::vector<double> radii;
    radii.push_back(peakRadiusMax);
    radii.push_back(6);
    radii.push_back(5);

    std::vector<Mantid::Kernel::V3D> directions;
    directions.push_back(Mantid::Kernel::V3D(0.0,1.0,0.0));
    directions.push_back(Mantid::Kernel::V3D(1.0,0.0,0.0));
    directions.push_back(Mantid::Kernel::V3D(0.0,0.0,1.0));

    Mantid::Kernel::SpecialCoordinateSystem coordinateSystem = Mantid::Kernel::SpecialCoordinateSystem::QSample;
    Mantid::Geometry::PeakShape_sptr shape(new Mantid::DataObjects::PeakShapeEllipsoid(directions, radii, radii, radii , coordinateSystem, "test", 1));
    MockPeak peak;
    peak.setPeakShape(shape);

    std::vector<std::pair<MockPeak&, Mantid::Kernel::V3D>> fakeSinglePeakPeakWorkspaces;
    fakeSinglePeakPeakWorkspaces.push_back(std::pair<MockPeak&, Mantid::Kernel::V3D>(peak, coordinate));

     // Act
    do_test_execute(peaksFilter, fakeSinglePeakPeakWorkspaces, coordinateSystem);

    // Assert
    do_test_peaks(in, out, coordinate, peakRadiusMax, peaksFilter.getRadiusFactor());

    in->Delete();
    out->Delete();
  }

  void testExecutionWithSingleNoShapePeakInQSample()
  {
    // Arrange
    vtkUnstructuredGrid *in = makeSplatterSourceGrid();
    vtkUnstructuredGrid *out = vtkUnstructuredGrid::New();
    vtkDataSetToPeaksFilteredDataSet peaksFilter(in, out);

    Mantid::Kernel::V3D coordinate(0,0,0);

    Mantid::Kernel::SpecialCoordinateSystem coordinateSystem = Mantid::Kernel::SpecialCoordinateSystem::QSample;
    double radius = peaksFilter.getRadiusNoShape();
    Mantid::Geometry::PeakShape_sptr shape(new Mantid::DataObjects::NoShape());
    MockPeak peak;
    peak.setPeakShape(shape);

    std::vector<std::pair<MockPeak&, Mantid::Kernel::V3D>> fakeSinglePeakPeakWorkspaces;
    fakeSinglePeakPeakWorkspaces.push_back(std::pair<MockPeak&, Mantid::Kernel::V3D>(peak, coordinate));

    // Act
    do_test_execute(peaksFilter, fakeSinglePeakPeakWorkspaces, coordinateSystem);

    // Assert
    do_test_peaks(in, out, coordinate, radius, peaksFilter.getRadiusFactor());

    in->Delete();
    out->Delete();
  }

  void testExecutionWithTwoWorkspacesWithSingleNoShapeInQSample() {
     // Arrange
    vtkUnstructuredGrid *in = makeSplatterSourceGrid();
    vtkUnstructuredGrid *out = vtkUnstructuredGrid::New();
    vtkDataSetToPeaksFilteredDataSet peaksFilter(in, out);

    // Peak 1

    // Peak 2


  }
};
#endif