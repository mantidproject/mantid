#include "MantidVatesAPI/vtkPeakMarkerFactory.h"
#include "MantidVatesAPI/ProgressAction.h"
#include <boost/math/special_functions/fpclassify.hpp>
#include "MantidAPI/Workspace.h"
#include "MantidAPI/IPeaksWorkspace.h"
#include "MantidAPI/IPeak.h"
#include "MantidGeometry/Crystal/PeakShape.h"
#include "MantidDataObjects/PeakShapeSpherical.h"
#include "MantidDataObjects/PeakShapeEllipsoid.h"
#include "MantidKernel/V3D.h"
#include "MantidKernel/ReadLock.h"

#include <vtkAxes.h>
#include "vtkParametricEllipsoid.h"
#include "vtkParametricFunctionSource.h"
#include <vtkPolyDataAlgorithm.h>
#include <vtkAppendPolyData.h>
#include <vtkVertex.h>
#include <vtkGlyph3D.h>
#include <vtkSphereSource.h>
#include <vtkUnstructuredGrid.h>
#include <vtkFloatArray.h>
#include <vtkCellData.h>
#include <vtkPolyData.h>
#include <vtkTransform.h>
#include <vtkTransformPolyDataFilter.h>
#include <vtkPVGlyphFilter.h>
#include <vtkSmartPointer.h>

#include <cmath>

using Mantid::API::IPeaksWorkspace;
using Mantid::API::IPeak;
using Mantid::Kernel::V3D;

namespace Mantid
{
namespace VATES 
{

  vtkPeakMarkerFactory::vtkPeakMarkerFactory(const std::string& scalarName, ePeakDimensions dimensions) :
  m_scalarName(scalarName), m_dimensionToShow(dimensions), m_peakRadius(-1)
  {
  }

  /**
  Assigment operator
  @param other : vtkPeakMarkerFactory to assign to this instance from.
  @return ref to assigned current instance.
  */
  vtkPeakMarkerFactory& vtkPeakMarkerFactory::operator=(const vtkPeakMarkerFactory& other)
  {
    if(this != &other)
    {
      this->m_scalarName = other.m_scalarName;
      this->m_dimensionToShow = other.m_dimensionToShow;
      this->m_workspace = other.m_workspace;
    }
    return *this;
  }

  /**
  Copy Constructor
  @param other : instance to copy from.
  */
  vtkPeakMarkerFactory::vtkPeakMarkerFactory(const vtkPeakMarkerFactory& other)
  {
   this->m_scalarName = other.m_scalarName;
   this->m_dimensionToShow = other.m_dimensionToShow;
   this->m_workspace = other.m_workspace;
   this->m_peakRadius = other.m_peakRadius;
  }

  void vtkPeakMarkerFactory::initialize(Mantid::API::Workspace_sptr workspace)
  {
    m_workspace = boost::dynamic_pointer_cast<IPeaksWorkspace>(workspace);
    validateWsNotNull();

    try
    {
      m_peakRadius = atof(m_workspace->run().getProperty("PeakRadius")->value().c_str());
    }
    catch(Mantid::Kernel::Exception::NotFoundError&)
    {
    }
  }

  double vtkPeakMarkerFactory::getIntegrationRadius() const
  {
    return m_peakRadius;
  }

  bool vtkPeakMarkerFactory::isPeaksWorkspaceIntegrated() const
  {
    return (m_peakRadius > 0);
  }

  void vtkPeakMarkerFactory::validateWsNotNull() const
  {
    if(!m_workspace)
      throw std::runtime_error("IPeaksWorkspace is null");
  }

  void vtkPeakMarkerFactory::validate() const
  {
    validateWsNotNull();
  }


    /**
  Create the vtkStructuredGrid from the provided workspace
  @param progressUpdating: Reporting object to pass progress information up the stack.
  @return vtkPolyData glyph.
  */
  vtkPolyData* vtkPeakMarkerFactory::create(ProgressAction& progressUpdating) const
  {
    validate();

    int numPeaks = m_workspace->getNumberPeaks();

   // Acquire a scoped read-only lock to the workspace (prevent segfault from algos modifying ws)
    Mantid::Kernel::ReadLock lock(*m_workspace);

    const int resolution = 6;

    double progressFactor = 1.0/double(numPeaks);

    vtkAppendPolyData* appendFilter = vtkAppendPolyData::New();
    // Go peak-by-peak
    for (int i=0; i < numPeaks; i++)
    {
      progressUpdating.eventRaised(double(i)*progressFactor);

      // Point
      vtkPoints *peakPoint = vtkPoints::New();
      peakPoint->Allocate(1);

      vtkFloatArray * peakSignal = vtkFloatArray::New();
      peakSignal->Allocate(1);
      peakSignal->SetName(m_scalarName.c_str());
      peakSignal->SetNumberOfComponents(1);

      // What we'll return
      vtkUnstructuredGrid *peakDataSet = vtkUnstructuredGrid::New();
      peakDataSet->Allocate(numPeaks);
      peakDataSet->SetPoints(peakPoint);
      peakDataSet->GetCellData()->SetScalars(peakSignal);

      IPeak & peak = m_workspace->getPeak(i);

      // Choose the dimensionality of the position to show
      V3D pos;
      switch (m_dimensionToShow)
      {
      case Peak_in_Q_lab:
        pos = peak.getQLabFrame();
        break;
      case Peak_in_Q_sample:
        pos = peak.getQSampleFrame();
        break;
      case Peak_in_HKL:
        pos = peak.getHKL();
        break;
      default:
        pos = peak.getQLabFrame();
      }

      double x = pos.X();
      double y = pos.Y();
      double z = pos.Z();

      // One point per peak
      vtkVertex * vertex = vtkVertex::New();
      vtkIdType id_xyz = peakPoint->InsertNextPoint(x,y,z);
      vertex->GetPointIds()->SetId(0, id_xyz);

      peakDataSet->InsertNextCell(VTK_VERTEX, vertex->GetPointIds());

      // The integrated intensity = the signal on that point.
      peakSignal->InsertNextValue(static_cast<float>( peak.getIntensity() ));
      peakPoint->Squeeze();
      peakDataSet->Squeeze();

      // Add a glyph and append to the appendFilter
      const Mantid::Geometry::PeakShape& shape = m_workspace->getPeakPtr(i)->getPeakShape();
      std::string name = shape.shapeName();
      // Pick the radius up from the factory if possible, otherwise use the user-provided value.
      vtkPolyDataAlgorithm* shapeMarker = NULL;
      if(shape.shapeName() == Mantid::DataObjects::PeakShapeSpherical::sphereShapeName())
      {
        const Mantid::DataObjects::PeakShapeSpherical& sphericalShape = dynamic_cast<const Mantid::DataObjects::PeakShapeSpherical&>(shape);
        double peakRadius = sphericalShape.radius();
        vtkSphereSource *sphere = vtkSphereSource::New();
        sphere->SetRadius(peakRadius);
        sphere->SetPhiResolution(resolution);
        sphere->SetThetaResolution(resolution);
        shapeMarker = sphere;
      }
      else if (shape.shapeName() == Mantid::DataObjects::PeakShapeEllipsoid::ellipsoidShapeName())
      {
        const Mantid::DataObjects::PeakShapeEllipsoid& ellipticalShape = dynamic_cast<const Mantid::DataObjects::PeakShapeEllipsoid&>(shape);

        std::vector<double> radii = ellipticalShape.abcRadii();
        std::vector<Mantid::Kernel::V3D> directions = ellipticalShape.directions();

        vtkParametricEllipsoid* ellipsoid = vtkParametricEllipsoid::New();
        ellipsoid->SetXRadius(radii[0]);
        ellipsoid->SetYRadius(radii[1]);
        ellipsoid->SetZRadius(radii[2]);

        vtkParametricFunctionSource* ellipsoidSource = vtkParametricFunctionSource::New();
        ellipsoidSource->SetParametricFunction(ellipsoid);
        ellipsoidSource->SetUResolution(resolution);
        ellipsoidSource->SetVResolution(resolution);
        ellipsoidSource->SetWResolution(resolution);
        ellipsoidSource->Update();

#if 0
        vtkTransform* transform = vtkTransform::New();
        const double rotationDegrees = 0;
        transform->RotateX(rotationDegrees);
        transform->RotateY(rotationDegrees);
        transform->RotateZ(rotationDegrees);
#else
        vtkSmartPointer<vtkTransform> transform = createEllipsoidTransform(directions);
#endif
        vtkTransformPolyDataFilter* transformFilter = vtkTransformPolyDataFilter::New();
        transformFilter->SetTransform(transform);
        transformFilter->SetInputConnection(ellipsoidSource->GetOutputPort());
        transformFilter->Update();
        shapeMarker = transformFilter;
      }
      else
      {
        vtkAxes* axis = vtkAxes::New();
        axis->SymmetricOn();
        axis->SetScaleFactor(0.3);

        vtkTransform* transform = vtkTransform::New();
        const double rotationDegrees = 45;
        transform->RotateX(rotationDegrees);
        transform->RotateY(rotationDegrees);
        transform->RotateZ(rotationDegrees);

        vtkTransformPolyDataFilter* transformFilter = vtkTransformPolyDataFilter::New();
        transformFilter->SetTransform(transform);
        transformFilter->SetInputConnection(axis->GetOutputPort());
        transformFilter->Update();
        shapeMarker = transformFilter;
      }

      vtkPVGlyphFilter *glyphFilter = vtkPVGlyphFilter::New();
      glyphFilter->SetInputData(peakDataSet);
      glyphFilter->SetSourceConnection(shapeMarker->GetOutputPort());
      glyphFilter->Update();
      vtkPolyData *glyphed = glyphFilter->GetOutput();

      appendFilter->AddInputData(glyphed);
    } // for each peak

    appendFilter->Update();
    vtkPolyData* polyData = appendFilter->GetOutput();
    return polyData;
  }

  vtkPeakMarkerFactory::~vtkPeakMarkerFactory()
  {
  }

  /**
   * Generates a transform based on the directions of the ellipsoid
   * @param directions The directions of the ellipsoid.
   * @returns A transform for the ellipsoid.
   */
  vtkSmartPointer<vtkTransform> vtkPeakMarkerFactory::createEllipsoidTransform(std::vector<Mantid::Kernel::V3D> directions) const
  {
    // The original ellipsoid is oriented along the y axis
    Mantid::Kernel::V3D principalAxisOriginal(0.0, 1.0, 0.0);
    Mantid::Kernel::V3D principalAxisTransformed(directions[0]);
    Mantid::Kernel::V3D minorAxisOriginal(1.0, 0.0, 0.0);
    Mantid::Kernel::V3D minorAxisTransformed(directions[1]);

    // Compute the axis of rotation. This is the normal between the original and the transformed direction
    Mantid::Kernel::V3D rotationAxis1 = principalAxisOriginal.cross_prod(principalAxisTransformed);
    // Compute the angle of rotation, i.e. the angle between the original and the transformed axis.
    double angle1 = acos(principalAxisOriginal.scalar_prod(principalAxisTransformed));

    // After the prinicpal axis is rotated into its right position we need to rotate the (rotated) minor axis
    // into its right position. The rotation axis is given by the new prinicipal rotation axis
    Mantid::Kernel::V3D minorAxisOriginalRotated(rotateVector(minorAxisOriginal, rotationAxis1, angle1));

    Mantid::Kernel::V3D rotationAxis2(principalAxisTransformed);
    double angle2 = acos(minorAxisOriginalRotated.scalar_prod(minorAxisTransformed));

    vtkSmartPointer<vtkTransform> transform = vtkSmartPointer<vtkTransform>::New();

    double angle1Degree = angle1*180/3.1415;
    double angle2Degree = angle2*180/3.1415;
    transform->RotateWXYZ(angle1Degree, rotationAxis1[0], rotationAxis1[1], rotationAxis1[2]);
    transform->RotateWXYZ(angle2Degree, rotationAxis2[0], rotationAxis2[1], rotationAxis2[2]);

    return transform;
  }

  /**
   * Rotate the a given vector around a specified axis by a specified angle. See http://en.wikipedia.org/wiki/Rodrigues%27_rotation_formula
   * @param original The original vector
   * @param rotationAxis The axis around which to rotate.
   * @param angle The rotation angle.
   * @returns The rotated vector.
   */
  Mantid::Kernel::V3D vtkPeakMarkerFactory::rotateVector(Mantid::Kernel::V3D original, Mantid::Kernel::V3D rotationAxis, double angle) const
  {
    Mantid::Kernel::V3D cross(rotationAxis.cross_prod(original));
    double scalar = rotationAxis.scalar_prod(original);
    double cos = std::cos(angle);
    double sin = std::sin(angle);

    Mantid::Kernel::V3D rotated = original*cos + cross*sin + rotationAxis*(scalar)*(1-cos);

    return rotated;
  }
}
}
