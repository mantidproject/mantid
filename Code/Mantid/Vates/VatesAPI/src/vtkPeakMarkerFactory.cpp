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

  vtkPeakMarkerFactory::vtkPeakMarkerFactory(const std::string& scalarName, double minThreshold, double maxThreshold) :
  m_scalarName(scalarName), m_minThreshold(minThreshold), m_maxThreshold(maxThreshold)
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
      this->m_minThreshold = other.m_minThreshold;
      this->m_maxThreshold = other.m_maxThreshold;
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
   this->m_minThreshold = other.m_minThreshold;
   this->m_maxThreshold = other.m_maxThreshold;
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
    double width = 0.0;

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

      // TODO: Which Q/hkl???
      V3D pos = peak.getQLabFrame();
      double x = pos.X();
      double y = pos.Y();
      double z = pos.Z();
      double dx = x+width;
      double dy = y+width;
      double dz = z+width;

      std::cout << "Making peak " << i << " at " << pos << " with signal " << peak.getIntensity() << std::endl;

      // One point per peak
      vtkVertex * vertex = vtkVertex::New();
      vtkIdType id_xyz =    points->InsertNextPoint(x,y,z);
      vertex->GetPointIds()->SetId(0, id_xyz);

      visualDataSet->InsertNextCell(VTK_VERTEX, vertex->GetPointIds());

      // The integrated intensity = the signal on that point.
      signal->InsertNextValue(static_cast<float>( peak.getIntensity() ));

//      vtkHexahedron* hexahedron = vtkHexahedron::New();
//
//      vtkIdType id_xyz =    points->InsertNextPoint(x,y,z);
//      vtkIdType id_dxyz =   points->InsertNextPoint(dx,y,z);
//      vtkIdType id_dxdyz =  points->InsertNextPoint(dx,dy,z);
//      vtkIdType id_xdyz =   points->InsertNextPoint(x,dy,z);
//      vtkIdType id_xydz =   points->InsertNextPoint(x,y,dz);
//      vtkIdType id_dxydz =  points->InsertNextPoint(dx,y,dz);
//      vtkIdType id_dxdydz = points->InsertNextPoint(dx,dy,dz);
//      vtkIdType id_xdydz =  points->InsertNextPoint(x,dy,dz);
//
//      //create the hexahedron
//      vtkHexahedron *theHex = vtkHexahedron::New();
//      theHex->GetPointIds()->SetId(0, id_xyz);
//      theHex->GetPointIds()->SetId(1, id_dxyz);
//      theHex->GetPointIds()->SetId(2, id_dxdyz);
//      theHex->GetPointIds()->SetId(3, id_xdyz);
//      theHex->GetPointIds()->SetId(4, id_xydz);
//      theHex->GetPointIds()->SetId(5, id_dxydz);
//      theHex->GetPointIds()->SetId(6, id_dxdydz);
//      theHex->GetPointIds()->SetId(7, id_xdydz);
//
//      visualDataSet->InsertNextCell(VTK_HEXAHEDRON, hexahedron->GetPointIds());
//      hexahedron->Delete();

    } // for each peak


    points->Squeeze();
    //TODO: delete points with points->Delete()?
    visualDataSet->Squeeze();
    return visualDataSet;
  }



  vtkPeakMarkerFactory::~vtkPeakMarkerFactory()
  {
  }

  vtkDataSet* vtkPeakMarkerFactory::createMeshOnly() const
  {
    throw std::runtime_error("::createMeshOnly() does not apply for this type of factory.");
  }

  vtkFloatArray* vtkPeakMarkerFactory::createScalarArray() const
  {
    throw std::runtime_error("::createScalarArray() does not apply for this type of factory.");
  }

}
}
