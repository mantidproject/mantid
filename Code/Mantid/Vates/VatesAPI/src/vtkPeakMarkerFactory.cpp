#include "MantidVatesAPI/vtkPeakMarkerFactory.h"
#include <boost/math/special_functions/fpclassify.hpp>
#include "MantidAPI/Workspace.h"
#include "MantidAPI/IPeaksWorkspace.h"
#include "MantidAPI/IPeak.h"
#include "MantidKernel/V3D.h"
#include <vtkVertex.h>

using Mantid::API::IPeaksWorkspace;
using Mantid::API::IPeak;
using Mantid::Kernel::V3D;

namespace Mantid
{
namespace VATES 
{

  vtkPeakMarkerFactory::vtkPeakMarkerFactory(const std::string& scalarName, ePeakDimensions dimensions) :
  m_scalarName(scalarName), m_dimensionToShow(dimensions)
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
  }

  void vtkPeakMarkerFactory::initialize(Mantid::API::Workspace_sptr workspace)
  {
    m_workspace = boost::dynamic_pointer_cast<IPeaksWorkspace>(workspace);
    validateWsNotNull();
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


  /** Create a vtkDataSet of points, one per peak.
   *
   * @return vtkUnstructuredGrid with a bunch of points.
   */
  vtkDataSet* vtkPeakMarkerFactory::create() const
  {
    validate();

    int numPeaks = m_workspace->getNumberPeaks();

    // Points generator
    vtkPoints *points = vtkPoints::New();
    points->Allocate(static_cast<int>(numPeaks));

    vtkFloatArray * signal = vtkFloatArray::New();
    signal->Allocate(numPeaks);
    signal->SetName(m_scalarName.c_str());
    signal->SetNumberOfComponents(1);

    // What we'll return
    vtkUnstructuredGrid *visualDataSet = vtkUnstructuredGrid::New();
    visualDataSet->Allocate(numPeaks);
    visualDataSet->SetPoints(points);
    visualDataSet->GetCellData()->SetScalars(signal);

    // Go peak-by-peak
    for (int i=0; i < numPeaks; i++)
    {
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
      }

      double x = pos.X();
      double y = pos.Y();
      double z = pos.Z();

      // One point per peak
      vtkVertex * vertex = vtkVertex::New();
      vtkIdType id_xyz =    points->InsertNextPoint(x,y,z);
      vertex->GetPointIds()->SetId(0, id_xyz);

      visualDataSet->InsertNextCell(VTK_VERTEX, vertex->GetPointIds());

      // The integrated intensity = the signal on that point.
      signal->InsertNextValue(static_cast<float>( peak.getIntensity() ));

    } // for each peak

    points->Squeeze();
    visualDataSet->Squeeze();
    return visualDataSet;
  }



  vtkPeakMarkerFactory::~vtkPeakMarkerFactory()
  {
  }
}
}
