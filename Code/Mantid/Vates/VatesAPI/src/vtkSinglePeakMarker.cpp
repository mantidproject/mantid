#include "MantidVatesAPI/vtkSinglePeakMarker.h"
#include <vtkPolyData.h>
#include "vtkFloatArray.h"
#include "vtkPoints.h"
#include "vtkUnstructuredGrid.h"
#include "vtkVertex.h"
#include "vtkPVGlyphFilter.h"
#include "vtkCellData.h"
#include "vtkSphereSource.h"
#include "vtkTransform.h"
#include "vtkTransformPolyDataFilter.h"

namespace Mantid
{
namespace VATES 
{
  vtkSinglePeakMarker::vtkSinglePeakMarker(){
  }

  vtkSinglePeakMarker::~vtkSinglePeakMarker(){
  }

  vtkPolyData* vtkSinglePeakMarker::createSinglePeakMarker(double x, double y, double z, double radius){

    // Point
    vtkPoints *peakPoint = vtkPoints::New();
    peakPoint->Allocate(1);

    vtkFloatArray * peakSignal = vtkFloatArray::New();
    peakSignal->Allocate(1);
    peakSignal->SetName("signal");
    peakSignal->SetNumberOfComponents(1);

    // What we'll return
    vtkUnstructuredGrid *peakDataSet = vtkUnstructuredGrid::New();
    peakDataSet->Allocate(1);
    peakDataSet->SetPoints(peakPoint);
    peakDataSet->GetCellData()->SetScalars(peakSignal);

    // One point per peak
    vtkVertex * vertex = vtkVertex::New();
    vtkIdType id_xyz = peakPoint->InsertNextPoint(x,y,z);
    vertex->GetPointIds()->SetId(0, id_xyz);

    peakDataSet->InsertNextCell(VTK_VERTEX, vertex->GetPointIds());

    // The integrated intensity = the signal on that point.
    peakSignal->InsertNextValue(static_cast<float>(1.0));
    peakPoint->Squeeze();
    peakDataSet->Squeeze();

    //Get the position info and create the glyph which is to be displayed.
    vtkSphereSource* sphere = vtkSphereSource::New();
    const int resolution = 16;
    sphere->SetRadius(radius);
    sphere->SetPhiResolution(resolution);
    sphere->SetThetaResolution(resolution);

    vtkTransform* transform = vtkTransform::New();
    transform->Translate(0, 0, 0);

    vtkTransformPolyDataFilter* transformFilter = vtkTransformPolyDataFilter::New();
    transformFilter->SetTransform(transform);
    transformFilter->SetInputConnection(sphere->GetOutputPort());
    transformFilter->Update();

    vtkPVGlyphFilter *glyphFilter = vtkPVGlyphFilter::New();
    glyphFilter->SetInputData(peakDataSet);
    glyphFilter->SetSourceConnection(transformFilter->GetOutputPort());
    glyphFilter->Update();
    vtkPolyData *glyphed = glyphFilter->GetOutput();

    return glyphed;
  }
}
}