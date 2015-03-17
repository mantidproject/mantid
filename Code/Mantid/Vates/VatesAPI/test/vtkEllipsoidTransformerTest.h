#ifndef VTK_VATES_API_ELLIPSOIDTRANSFORMER_TEST
#define VTK_VATES_API_ELLIPSOIDTRANSFORMER_TEST

#include <cxxtest/TestSuite.h>
#include "MantidVatesAPI/vtkEllipsoidTransformer.h"

#include "MantidVatesAPI/vtkEllipsoidTransformer.h"
#include "MantidKernel/V3D.h"

#include "MockObjects.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <vtkTransform.h>
#include <vtkSmartPointer.h>
#include <vtkLineSource.h>
#include <vtkTransformPolyDataFilter.h>
#include <vector>
#include <cmath>

using namespace Mantid::VATES;

class vtkEllipsoidTransformerTest : public CxxTest::TestSuite
{
public:
  void do_test(std::vector<Mantid::Kernel::V3D> directions,
               std::vector<std::vector<double>> majorAxis,
               std::vector<std::vector<double>> minorAxis, 
               std::vector<std::vector<double>> majorExpected,
               std::vector<std::vector<double>> minorExpected)
  {
    // Arrange 
    vtkSmartPointer<vtkLineSource> major =  vtkSmartPointer<vtkLineSource>::New();
    double majorPoint1[3] = {majorAxis[0][0], majorAxis[0][1], majorAxis[0][2]};
    double majorPoint2[3] = {majorAxis[1][0], majorAxis[1][1], majorAxis[1][2]};
    major->SetPoint1(majorPoint1);
    major->SetPoint2(majorPoint2);
    major->Update();

    vtkSmartPointer<vtkLineSource> minor =  vtkSmartPointer<vtkLineSource>::New();
    double minorPoint1[3] = {minorAxis[0][0], minorAxis[0][1], minorAxis[0][2]};
    double minorPoint2[3] = {minorAxis[1][0], minorAxis[1][1], minorAxis[1][2]};
    minor->SetPoint1(minorPoint1);
    minor->SetPoint2(minorPoint2);
    minor->Update();
    
    // Act
    vtkEllipsoidTransformer transformer;
    vtkSmartPointer<vtkTransform> transform = transformer.generateTransform(directions);

    vtkTransformPolyDataFilter* transformFilter = vtkTransformPolyDataFilter::New();

    transformFilter->SetTransform(transform);
    transformFilter->SetInputConnection(major->GetOutputPort());
    transformFilter->Update();
    vtkPolyData* out = transformFilter->GetOutput();
    
    double majorPoint1Rotated[3] = {0.0,0.0,0.0};
    double majorPoint2Rotated[3] = {0.0,0.0,0.0};
    out->GetPoint(0, majorPoint1Rotated);
    out->GetPoint(1, majorPoint2Rotated);

    transformFilter->SetInputConnection(minor->GetOutputPort());
    transformFilter->Update();
    out = transformFilter->GetOutput();
    double minorPoint1Rotated[3] = {0.0,0.0,0.0};
    double minorPoint2Rotated[3] = {0.0,0.0,0.0};
    out->GetPoint(0, minorPoint1Rotated);
    out->GetPoint(1, minorPoint2Rotated);
    
    // Assert
    const double delta = 1e-5;

    TSM_ASSERT_DELTA("Point 1 of the major axis should not change", majorPoint1Rotated[0], majorExpected[0][0], delta);
    TSM_ASSERT_DELTA("Point 1 of the major axis should not change", majorPoint1Rotated[1], majorExpected[0][1], delta);
    TSM_ASSERT_DELTA("Point 1 of the major axis should not change", majorPoint1Rotated[2], majorExpected[0][2], delta);
    
    TSM_ASSERT_DELTA("Point 2 of the major axis should have changed", majorPoint2Rotated[0], majorExpected[1][0], delta);
    TSM_ASSERT_DELTA("Point 2 of the major axis should have changed", majorPoint2Rotated[1], majorExpected[1][1], delta);
    TSM_ASSERT_DELTA("Point 2 of the major axis should have changed", majorPoint2Rotated[2], majorExpected[1][2], delta);

    TSM_ASSERT_DELTA("Point 1 of the minor axis should not change", minorPoint1Rotated[0], minorExpected[0][0], delta);
    TSM_ASSERT_DELTA("Point 1 of the minor axis should not change", minorPoint1Rotated[1], minorExpected[0][1], delta);
    TSM_ASSERT_DELTA("Point 1 of the minor axis should not change", minorPoint1Rotated[2], minorExpected[0][2], delta);

    TSM_ASSERT_DELTA("Point 2 of the minor axis should have changed", minorPoint2Rotated[0], minorExpected[1][0], delta);
    TSM_ASSERT_DELTA("Point 2 of the minor axis should have changed", minorPoint2Rotated[1], minorExpected[1][1], delta);
    TSM_ASSERT_DELTA("Point 2 of the minor axis should have changed", minorPoint2Rotated[2], minorExpected[1][2], delta);
  }

  void testGenerateTransformTiltedByNinetyDegrees()
  {
    // Arrange
    Mantid::Kernel::V3D axis1(0.0,1.0,0.0);
    Mantid::Kernel::V3D axis2(-1.0,0.0,0.0);
    Mantid::Kernel::V3D axis3(0.0,0.0,1.0);

    std::vector<Mantid::Kernel::V3D> directions;
    directions.push_back(axis1);
    directions.push_back(axis2);
    directions.push_back(axis3);

    // Major Axis 
    std::vector<double> point1Major;
    point1Major.push_back(0.0);
    point1Major.push_back(0.0);
    point1Major.push_back(0.0);

    std::vector<double> point2Major;
    point2Major.push_back(1.0);
    point2Major.push_back(0.0);
    point2Major.push_back(0.0);

    std::vector<std::vector<double>> major;
    major.push_back(point1Major);
    major.push_back(point2Major);

    // Minor Axis
    std::vector<double> point1Minor;
    point1Minor.push_back(0.0);
    point1Minor.push_back(0.0);
    point1Minor.push_back(0.0);

    std::vector<double> point2Minor;
    point2Minor.push_back(0.0);
    point2Minor.push_back(1.0);
    point2Minor.push_back(0.0);

    std::vector<std::vector<double>> minor;
    minor.push_back(point1Minor);
    minor.push_back(point2Minor);
    
    std::vector<double> point1MajorExpected;
    point1MajorExpected.push_back(0.0);
    point1MajorExpected.push_back(0.0);
    point1MajorExpected.push_back(0.0);
    
    std::vector<double> point2MajorExpected;
    point2MajorExpected.push_back(0.0);
    point2MajorExpected.push_back(1.0);
    point2MajorExpected.push_back(0.0);

    std::vector<std::vector<double>> majorExpected;
    majorExpected.push_back(point1MajorExpected);
    majorExpected.push_back(point2MajorExpected);

    // Minor Axis
    std::vector<double> point1MinorExpected;
    point1MinorExpected.push_back(0.0);
    point1MinorExpected.push_back(0.0);
    point1MinorExpected.push_back(0.0);

    std::vector<double> point2MinorExpected;
    point2MinorExpected.push_back(-1.0);
    point2MinorExpected.push_back(0.0);
    point2MinorExpected.push_back(0.0);

    std::vector<std::vector<double>> minorExpected;
    minorExpected.push_back(point1MinorExpected);
    minorExpected.push_back(point2MinorExpected);

    // Act + Assert
    do_test(directions, major, minor, majorExpected, minorExpected);
  }

  void testGenerateTransformInRandomDirection()
  {
    // Arrange
    double xMajor = 1.3/sqrt(1.3*1.3 + 1.1*1.1 + 0.5*0.5);
    double yMajor = -1.1/sqrt(1.3*1.3 + 1.1*1.1 + 0.5*0.5);
    double zMajor = 0.5/sqrt(1.3*1.3 + 1.1*1.1 + 0.5*0.5);

    double xMinor = 1.1/1.3/sqrt(1*1 + (1.1/1.3)*(1.1/1.3));
    double yMinor = 1.0/sqrt(1*1 + (1.1/1.3)*(1.1/1.3));
    double zMinor = 0.0;

    Mantid::Kernel::V3D axis1(xMajor, yMajor, zMajor);
    Mantid::Kernel::V3D axis2(xMinor,yMinor, zMinor);
    // The third direction is not valid, but we don't need it for our calculations.
    Mantid::Kernel::V3D axis3(0.0,0.0,1.0); 

    std::vector<Mantid::Kernel::V3D> directions;
    directions.push_back(axis1);
    directions.push_back(axis2);
    directions.push_back(axis3);

    // Major Axis 
    std::vector<double> point1Major;
    point1Major.push_back(0.0);
    point1Major.push_back(0.0);
    point1Major.push_back(0.0);

    std::vector<double> point2Major;
    point2Major.push_back(1.0);
    point2Major.push_back(0.0);
    point2Major.push_back(0.0);

    std::vector<std::vector<double>> major;
    major.push_back(point1Major);
    major.push_back(point2Major);

    // Minor Axis
    std::vector<double> point1Minor;
    point1Minor.push_back(0.0);
    point1Minor.push_back(0.0);
    point1Minor.push_back(0.0);

    std::vector<double> point2Minor;
    point2Minor.push_back(0.0);
    point2Minor.push_back(1.0);
    point2Minor.push_back(0.0);

    std::vector<std::vector<double>> minor;
    minor.push_back(point1Minor);
    minor.push_back(point2Minor);
    
    std::vector<double> point1MajorExpected;
    point1MajorExpected.push_back(0.0);
    point1MajorExpected.push_back(0.0);
    point1MajorExpected.push_back(0.0);
    
    std::vector<double> point2MajorExpected;
    point2MajorExpected.push_back(xMajor);
    point2MajorExpected.push_back(yMajor);
    point2MajorExpected.push_back(zMajor);

    std::vector<std::vector<double>> majorExpected;
    majorExpected.push_back(point1MajorExpected);
    majorExpected.push_back(point2MajorExpected);

    // Minor Axis
    std::vector<double> point1MinorExpected;
    point1MinorExpected.push_back(0.0);
    point1MinorExpected.push_back(0.0);
    point1MinorExpected.push_back(0.0);

    std::vector<double> point2MinorExpected;
    point2MinorExpected.push_back(xMinor);
    point2MinorExpected.push_back(yMinor);
    point2MinorExpected.push_back(zMinor);

    std::vector<std::vector<double>> minorExpected;
    minorExpected.push_back(point1MinorExpected);
    minorExpected.push_back(point2MinorExpected);

    // Act + Assert
    do_test(directions, major, minor, majorExpected, minorExpected);
  }
};


#endif