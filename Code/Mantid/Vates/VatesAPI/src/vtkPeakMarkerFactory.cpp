#include "MantidVatesAPI/vtkPeakMarkerFactory.h"
#include "MantidVatesAPI/ProgressAction.h"
#include <boost/math/special_functions/fpclassify.hpp>
#include "MantidAPI/Workspace.h"
#include "MantidAPI/IPeaksWorkspace.h"
#include "MantidAPI/IPeak.h"
#include "MantidKernel/V3D.h"
#include <vtkVertex.h>
#include <vtkGlyph3D.h>
#include <vtkSphereSource.h>
#include <vtkUnstructuredGrid.h>
#include <vtkFloatArray.h>
#include <vtkCellData.h>
#include "MantidKernel/ReadLock.h"

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
  vtkDataSet* vtkPeakMarkerFactory::create(ProgressAction& progressUpdating) const
  {
    validate();

    int numPeaks = m_workspace->getNumberPeaks();

   // Acquire a scoped read-only lock to the workspace (prevent segfault from algos modifying ws)
    Mantid::Kernel::ReadLock lock(*m_workspace);

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

    double progressFactor = 1.0/double(numPeaks);

    // Go peak-by-peak
    for (int i=0; i < numPeaks; i++)
    {
      progressUpdating.eventRaised(double(i)*progressFactor);

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
