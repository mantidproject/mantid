#include "MantidVatesAPI/vtkPeakMarkerFactory.h"
#include "MantidVatesAPI/ProgressAction.h"
#include "MantidVatesAPI/vtkEllipsoidTransformer.h"
#include <boost/math/special_functions/fpclassify.hpp>
#include "MantidAPI/Workspace.h"
#include "MantidAPI/IPeaksWorkspace.h"
#include "MantidGeometry/Crystal/IPeak.h"
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
#include "MantidVatesAPI/vtkGlyph3D_Silent.h"
#include <vtkSphereSource.h>
#include <vtkUnstructuredGrid.h>
#include <vtkFloatArray.h>
#include <vtkCellData.h>
#include <vtkPolyData.h>
#include <vtkTransform.h>
#include <vtkTransformPolyDataFilter.h>
#include <vtkPVGlyphFilter.h>
#include <vtkSmartPointer.h>

#include <vtkLineSource.h>
#include <cmath>

using Mantid::API::IPeaksWorkspace;
using Mantid::Geometry::IPeak;
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

    vtkEllipsoidTransformer ellipsoidTransformer;

    const int resolution = 8;
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
      peakDataSet->Allocate(1);
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
        std::vector<Mantid::Kernel::V3D> directions;
         
        switch (m_dimensionToShow)
        {
          case Peak_in_Q_lab:
            directions = ellipticalShape.directions();
            break;
          case Peak_in_Q_sample:
          {
            Mantid::Kernel::Matrix<double> goniometerMatrix = peak.getGoniometerMatrix();
            if (goniometerMatrix.Invert())
            {
              directions = ellipticalShape.getDirectionInSpecificFrame(goniometerMatrix);
            }
            else
            {
              directions = ellipticalShape.directions();
            }
          }
          break;
        case Peak_in_HKL:
          directions = ellipticalShape.directions();
          break;
        default:
          directions = ellipticalShape.directions();
        }

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

        vtkSmartPointer<vtkTransform> transform = ellipsoidTransformer.generateTransform(directions);

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
}
}
