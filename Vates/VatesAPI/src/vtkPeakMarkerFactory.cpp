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

#include "MantidVatesAPI/vtkGlyph3D_Silent.h"
#include "vtkParametricEllipsoid.h"
#include "vtkParametricFunctionSource.h"
#include <vtkAppendPolyData.h>
#include <vtkAxes.h>
#include <vtkFloatArray.h>
#include <vtkNew.h>
#include <vtkPVGlyphFilter.h>
#include <vtkPointData.h>
#include <vtkPolyData.h>
#include <vtkPolyDataAlgorithm.h>
#include <vtkRegularPolygonSource.h>
#include <vtkSmartPointer.h>
#include <vtkSphereSource.h>
#include <vtkTensorGlyph.h>
#include <vtkTransform.h>
#include <vtkTransformPolyDataFilter.h>
#include <vtkUnstructuredGrid.h>
#include <vtkVertex.h>

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

  std::vector<Mantid::Kernel::V3D>
  getDirections(const Mantid::DataObjects::PeakShapeEllipsoid &ellipticalShape,
                const IPeak &peak,
                vtkPeakMarkerFactory::ePeakDimensions dimensionToShow) {
    std::vector<Mantid::Kernel::V3D> directions;
    switch (dimensionToShow) {
    case vtkPeakMarkerFactory::Peak_in_Q_lab:
      directions = ellipticalShape.directions();
      break;
    case vtkPeakMarkerFactory::Peak_in_Q_sample: {
      Mantid::Kernel::Matrix<double> goniometerMatrix =
          peak.getGoniometerMatrix();
      if (goniometerMatrix.Invert()) {
        directions =
            ellipticalShape.getDirectionInSpecificFrame(goniometerMatrix);
      } else {
        directions = ellipticalShape.directions();
      }
    } break;
    case vtkPeakMarkerFactory::Peak_in_HKL:
      directions = ellipticalShape.directions();
      break;
    default:
      directions = ellipticalShape.directions();
    }
    return directions;
  }

  /**
Create the vtkStructuredGrid from the provided workspace
@param progressUpdating: Reporting object to pass progress information up the
stack.
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
      vtkNew<vtkPoints> peakPoint;
      peakPoint->Allocate(1);

      vtkNew<vtkFloatArray> peakSignal;
      peakSignal->Allocate(1);
      peakSignal->SetName(m_scalarName.c_str());
      peakSignal->SetNumberOfComponents(1);

      vtkFloatArray *transformSignal = vtkFloatArray::New();
      transformSignal->Allocate(1);
      transformSignal->SetNumberOfComponents(9);

      // What we'll return
      vtkNew<vtkPolyData> peakDataSet;
      peakDataSet->Allocate(1);
      peakDataSet->SetPoints(peakPoint.GetPointer());
      peakDataSet->GetPointData()->SetScalars(peakSignal.GetPointer());

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

      peakPoint->InsertNextPoint(pos.X(), pos.Y(), pos.Z());

      // The integrated intensity = the signal on that point.
      peakSignal->InsertNextValue(static_cast<float>( peak.getIntensity() ));
      peakPoint->Squeeze();
      peakDataSet->Squeeze();

      // Add a glyph and append to the appendFilter
      const Mantid::Geometry::PeakShape& shape = m_workspace->getPeakPtr(i)->getPeakShape();

      // Pick the radius up from the factory if possible, otherwise use the user-provided value.
      if(shape.shapeName() == Mantid::DataObjects::PeakShapeSpherical::sphereShapeName())
      {
        const Mantid::DataObjects::PeakShapeSpherical& sphericalShape = dynamic_cast<const Mantid::DataObjects::PeakShapeSpherical&>(shape);
        double peakRadius = sphericalShape.radius();

        vtkNew<vtkRegularPolygonSource> polygonSource;
        polygonSource->GeneratePolygonOff();
        polygonSource->SetNumberOfSides(resolution * 10);
        polygonSource->SetRadius(peakRadius);
        polygonSource->SetCenter(0, 0, 0);

        polygonSource->SetNormal(1, 0, 0);
        vtkNew<vtkPVGlyphFilter> glyphFilter1;
        glyphFilter1->SetInputData(peakDataSet.GetPointer());
        glyphFilter1->SetSourceConnection(polygonSource->GetOutputPort());
        glyphFilter1->Update();
        appendFilter->AddInputData(glyphFilter1->GetOutput());
        appendFilter->Update();

        polygonSource->SetNormal(0, 1, 0);
        vtkNew<vtkPVGlyphFilter> glyphFilter2;
        glyphFilter2->SetInputData(peakDataSet.GetPointer());
        glyphFilter2->SetSourceConnection(polygonSource->GetOutputPort());
        glyphFilter2->Update();
        appendFilter->AddInputData(glyphFilter2->GetOutput());
        appendFilter->Update();

        polygonSource->SetNormal(0, 0, 1);
        vtkNew<vtkPVGlyphFilter> glyphFilter3;
        glyphFilter3->SetInputData(peakDataSet.GetPointer());
        glyphFilter3->SetSourceConnection(polygonSource->GetOutputPort());
        glyphFilter3->Update();
        appendFilter->AddInputData(glyphFilter3->GetOutput());
        appendFilter->Update();
      }
      else if (shape.shapeName() == Mantid::DataObjects::PeakShapeEllipsoid::ellipsoidShapeName())
      {
        const Mantid::DataObjects::PeakShapeEllipsoid& ellipticalShape = dynamic_cast<const Mantid::DataObjects::PeakShapeEllipsoid&>(shape);
        std::vector<double> radii = ellipticalShape.abcRadii();
        std::vector<Mantid::Kernel::V3D> directions =
            getDirections(ellipticalShape, peak, m_dimensionToShow);
        std::array<float, 9> tensor = {
            {static_cast<float>(radii[0] * directions[0][0]),
             static_cast<float>(radii[0] * directions[0][1]),
             static_cast<float>(radii[0] * directions[0][2]),
             static_cast<float>(radii[1] * directions[1][0]),
             static_cast<float>(radii[1] * directions[1][1]),
             static_cast<float>(radii[1] * directions[1][2]),
             static_cast<float>(radii[2] * directions[2][0]),
             static_cast<float>(radii[2] * directions[2][1]),
             static_cast<float>(radii[2] * directions[2][2])}};

        transformSignal->InsertNextTupleValue(tensor.data());
        peakDataSet->GetPointData()->SetTensors(transformSignal);

        vtkNew<vtkRegularPolygonSource> polygonSource;
        polygonSource->GeneratePolygonOff();
        polygonSource->SetNumberOfSides(resolution * 10.f);
        polygonSource->SetRadius(1.f);
        polygonSource->SetCenter(0.f, 0.f, 0.f);

        polygonSource->SetNormal(1.f, 0.f, 0.f);
        vtkNew<vtkTensorGlyph> glyphFilter1;
        glyphFilter1->SetInputData(peakDataSet.GetPointer());
        glyphFilter1->SetSourceConnection(0, polygonSource->GetOutputPort());
        glyphFilter1->ExtractEigenvaluesOff();
        glyphFilter1->Update();
        appendFilter->AddInputData(glyphFilter1->GetOutput());
        appendFilter->Update();

        polygonSource->SetNormal(0.f, 1.f, 0.f);
        vtkNew<vtkTensorGlyph> glyphFilter2;
        glyphFilter2->SetInputData(peakDataSet.GetPointer());
        glyphFilter2->SetSourceConnection(0, polygonSource->GetOutputPort());
        glyphFilter2->ExtractEigenvaluesOff();
        glyphFilter2->Update();
        appendFilter->AddInputData(glyphFilter2->GetOutput());
        appendFilter->Update();

        polygonSource->SetNormal(0.f, 0.f, 1.f);
        vtkNew<vtkTensorGlyph> glyphFilter3;
        glyphFilter3->SetInputData(peakDataSet.GetPointer());
        glyphFilter3->SetSourceConnection(0, polygonSource->GetOutputPort());
        glyphFilter3->ExtractEigenvaluesOff();
        glyphFilter3->Update();
        appendFilter->AddInputData(glyphFilter3->GetOutput());
        appendFilter->Update();

      }
      else
      {
        vtkNew<vtkAxes> axis;
        axis->SymmetricOn();
        axis->SetScaleFactor(0.3);

        vtkNew<vtkTransform> transform;
        const double rotationDegrees = 45;
        transform->RotateX(rotationDegrees);
        transform->RotateY(rotationDegrees);
        transform->RotateZ(rotationDegrees);

        vtkNew<vtkTransformPolyDataFilter> transformFilter;
        transformFilter->SetTransform(transform.GetPointer());
        transformFilter->SetInputConnection(axis->GetOutputPort());
        transformFilter->Update();

        vtkNew<vtkPVGlyphFilter> glyphFilter;
        glyphFilter->SetInputData(peakDataSet.GetPointer());
        glyphFilter->SetSourceConnection(transformFilter->GetOutputPort());
        glyphFilter->Update();
        appendFilter->AddInputData(glyphFilter->GetOutput());
        appendFilter->Update();
      }

    } // for each peak

    vtkPolyData* polyData = appendFilter->GetOutput();

    return polyData;
  }

  vtkPeakMarkerFactory::~vtkPeakMarkerFactory()
  {
  }
}
}
