#include "MantidVatesAPI/vtkPeakMarkerFactory.h"
#include "MantidVatesAPI/ProgressAction.h"
#include "MantidAPI/Workspace.h"
#include "MantidAPI/IPeaksWorkspace.h"
#include "MantidAPI/Run.h"
#include "MantidGeometry/Crystal/PeakShape.h"
#include "MantidDataObjects/PeakShapeSpherical.h"
#include "MantidKernel/V3D.h"
#include "MantidKernel/ReadLock.h"

#include <vtkAppendPolyData.h>
#include <vtkAxes.h>
#include <vtkFloatArray.h>
#include <vtkNew.h>
#include <vtkPVGlyphFilter.h>
#include <vtkPointData.h>
#include <vtkPolyData.h>
#include <vtkRegularPolygonSource.h>
#include <vtkSmartPointer.h>
#include <vtkTensorGlyph.h>
#include <vtkTransform.h>
#include <vtkTransformPolyDataFilter.h>

#include <cmath>

using Mantid::API::IPeaksWorkspace;
using Mantid::Geometry::IPeak;
using Mantid::Kernel::V3D;

namespace Mantid {
namespace VATES {

vtkPeakMarkerFactory::vtkPeakMarkerFactory(const std::string &scalarName,
                                           ePeakDimensions dimensions)
    : m_scalarName(scalarName), m_dimensionToShow(dimensions),
      m_peakRadius(-1) {}

void vtkPeakMarkerFactory::initialize(Mantid::API::Workspace_sptr workspace) {
  m_workspace = boost::dynamic_pointer_cast<IPeaksWorkspace>(workspace);
  validateWsNotNull();

  try {
    m_peakRadius =
        std::stod(m_workspace->run().getProperty("PeakRadius")->value());

  } catch (Mantid::Kernel::Exception::NotFoundError &) {
  }
}

double vtkPeakMarkerFactory::getIntegrationRadius() const {
  return m_peakRadius;
}

bool vtkPeakMarkerFactory::isPeaksWorkspaceIntegrated() const {
  return (m_peakRadius > 0);
}

void vtkPeakMarkerFactory::validateWsNotNull() const {
  if (!m_workspace)
    throw std::runtime_error("IPeaksWorkspace is null");
}

void vtkPeakMarkerFactory::validate() const { validateWsNotNull(); }

/**
 Get ellipsoid axes
 @param ellipticalShape: ellipsoidal parameters.
 @param peak: peak under consideration.
 */
std::vector<Mantid::Kernel::V3D> vtkPeakMarkerFactory::getAxes(
    const Mantid::DataObjects::PeakShapeEllipsoid &ellipticalShape,
    const IPeak &peak) const {
  std::vector<Mantid::Kernel::V3D> axes;
  switch (m_dimensionToShow) {
  case vtkPeakMarkerFactory::Peak_in_Q_lab:
    axes = ellipticalShape.directions();
    break;
  case vtkPeakMarkerFactory::Peak_in_Q_sample: {
    Mantid::Kernel::Matrix<double> goniometerMatrix =
        peak.getGoniometerMatrix();
    if (goniometerMatrix.Invert() != 0.0) {
      axes = ellipticalShape.getDirectionInSpecificFrame(goniometerMatrix);
    } else {
      axes = ellipticalShape.directions();
    }
  } break;
  case vtkPeakMarkerFactory::Peak_in_HKL:
    axes = ellipticalShape.directions();
    break;
  default:
    axes = ellipticalShape.directions();
  }
  return axes;
}

/**
 Get glyph position in the appropiate coordinates.
 @param peak: peak under consideration.
 */
V3D vtkPeakMarkerFactory::getPosition(const IPeak &peak) const {

  V3D pos;
  switch (m_dimensionToShow) {
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
  return pos;
}

/**
 Get the tranform tensor for vtkTensorGlyph
 @param ellipticalShape: ellipsoidal parameters.
 @param peak: peak under consideration.
 */
std::array<float, 9> vtkPeakMarkerFactory::getTransformTensor(
    const Mantid::DataObjects::PeakShapeEllipsoid &ellipticalShape,
    const IPeak &peak) const {

  std::vector<double> radii = ellipticalShape.abcRadii();
  std::vector<Mantid::Kernel::V3D> axes = getAxes(ellipticalShape, peak);

  // The rotation+scaling matrix is the
  // principal axes of the ellipsoid scaled by the radii.
  std::array<float, 9> tensor;
  for (unsigned j = 0; j < 3; ++j) {
    for (unsigned k = 0; k < 3; ++k) {
      tensor[3 * j + k] = static_cast<float>(radii[j] * axes[j][k]);
    }
  }
  return tensor;
}

namespace {
/**
 Set the normal direction.
 @param source: source on which the normal direction is set.
 @param direction: direction (x,y,z) to set.
 */
void setNormal(vtkRegularPolygonSource *source, unsigned direction) {
  assert(direction < 3);
  switch (direction) {
  case (0):
    source->SetNormal(1.0, 0.0, 0.0);
    break;
  case (1):
    source->SetNormal(0.0, 1.0, 0.0);
    break;
  case (2):
    source->SetNormal(0.0, 0.0, 1.0);
    break;
  }
}
}

/**
Create the vtkStructuredGrid from the provided workspace
@param progressUpdating: Reporting object to pass progress information up the
stack.
@return vtkPolyData glyph.
*/
vtkSmartPointer<vtkPolyData>
vtkPeakMarkerFactory::create(ProgressAction &progressUpdating) const {
  validate();

  int numPeaks = m_workspace->getNumberPeaks();

  // Acquire a scoped read-only lock to the workspace (prevent segfault from
  // algos modifying ws)
  Mantid::Kernel::ReadLock lock(*m_workspace);

  const int resolution = 100;
  double progressFactor = 1.0 / static_cast<double>(numPeaks);

  vtkAppendPolyData *appendFilter = vtkAppendPolyData::New();
  // Go peak-by-peak
  for (int i = 0; i < numPeaks; i++) {
    progressUpdating.eventRaised(double(i) * progressFactor);

    // Point
    vtkNew<vtkPoints> peakPoint;
    peakPoint->Allocate(1);

    // What we'll return
    vtkNew<vtkPolyData> peakDataSet;
    peakDataSet->Allocate(1);
    peakDataSet->SetPoints(peakPoint.GetPointer());

    const IPeak &peak = m_workspace->getPeak(i);

    // Choose the dimensionality of the position to show
    V3D pos = getPosition(peak);
    peakPoint->InsertNextPoint(pos.X(), pos.Y(), pos.Z());

    peakPoint->Squeeze();
    peakDataSet->Squeeze();

    // Add a glyph and append to the appendFilter
    const Mantid::Geometry::PeakShape &shape =
        m_workspace->getPeak(i).getPeakShape();

    // Pick the radius up from the factory if possible, otherwise use the
    // user-provided value.
    if (shape.shapeName() ==
        Mantid::DataObjects::PeakShapeSpherical::sphereShapeName()) {
      double peakRadius =
          shape.radius(Mantid::Geometry::PeakShape::Radius).get();

      vtkNew<vtkRegularPolygonSource> polygonSource;
      polygonSource->GeneratePolygonOff();
      polygonSource->SetNumberOfSides(resolution);
      polygonSource->SetRadius(peakRadius);
      polygonSource->SetCenter(0., 0., 0.);

      for (unsigned axis = 0; axis < 3; ++axis) {
        vtkNew<vtkPVGlyphFilter> glyphFilter;
        setNormal(polygonSource.GetPointer(), axis);
        glyphFilter->SetInputData(peakDataSet.GetPointer());
        glyphFilter->SetSourceConnection(polygonSource->GetOutputPort());
        glyphFilter->Update();
        appendFilter->AddInputData(glyphFilter->GetOutput());
        appendFilter->Update();
      }
    } else if (shape.shapeName() ==
               Mantid::DataObjects::PeakShapeEllipsoid::ellipsoidShapeName()) {
      vtkNew<vtkFloatArray> transformSignal;
      transformSignal->SetNumberOfComponents(9);
      transformSignal->SetNumberOfTuples(1);
      auto tensor = getTransformTensor(
          dynamic_cast<const Mantid::DataObjects::PeakShapeEllipsoid &>(shape),
          peak);
      transformSignal->SetTypedTuple(0, tensor.data());
      peakDataSet->GetPointData()->SetTensors(transformSignal.GetPointer());

      vtkNew<vtkRegularPolygonSource> polygonSource;
      polygonSource->GeneratePolygonOff();
      polygonSource->SetNumberOfSides(resolution);
      polygonSource->SetRadius(1.);
      polygonSource->SetCenter(0., 0., 0.);

      for (unsigned axis = 0; axis < 3; ++axis) {
        vtkNew<vtkTensorGlyph> glyphFilter;
        setNormal(polygonSource.GetPointer(), axis);
        glyphFilter->SetInputData(peakDataSet.GetPointer());
        glyphFilter->SetSourceConnection(polygonSource->GetOutputPort());
        glyphFilter->ExtractEigenvaluesOff();
        glyphFilter->Update();
        appendFilter->AddInputData(glyphFilter->GetOutput());
        appendFilter->Update();
      }
    } else {
      vtkNew<vtkAxes> axis;
      axis->SymmetricOn();
      axis->SetScaleFactor(0.2);

      vtkNew<vtkPVGlyphFilter> glyphFilter;
      glyphFilter->SetInputData(peakDataSet.GetPointer());
      glyphFilter->SetSourceConnection(axis->GetOutputPort());
      glyphFilter->Update();
      appendFilter->AddInputData(glyphFilter->GetOutput());
      appendFilter->Update();
    }

  } // for each peak

  return vtkSmartPointer<vtkPolyData>::Take(appendFilter->GetOutput());
}
}
}
