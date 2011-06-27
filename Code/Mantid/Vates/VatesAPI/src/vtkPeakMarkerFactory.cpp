#include "MantidVatesAPI/vtkPeakMarkerFactory.h"
#include <boost/math/special_functions/fpclassify.hpp>
#include "MantidAPI/Workspace.h"
#include "MantidAPI/IPeaksWorkspace.h"
#include "MantidAPI/IPeak.h"
#include "MantidKernel/V3D.h"

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
    if (!m_workspace) throw std::invalid_argument("Invalid IPeaksWorkspace passed to the initialize() method.");
  }

  void vtkPeakMarkerFactory::validateWsNotNull() const
  {
    if(NULL == m_workspace.get())
    {
      throw std::runtime_error("IPeaksWorkspace is null");
    }
  }

  void vtkPeakMarkerFactory::validate() const
  {
    validateWsNotNull();
  }

  vtkDataSet* vtkPeakMarkerFactory::create() const
  {
    validate();

    int numPeaks = m_workspace->getNumberPeaks();
    double width = 0.0;

    // Points generator
    vtkPoints *points = vtkPoints::New();
    points->Allocate(static_cast<int>(numPeaks*6));

    vtkFloatArray * signal = vtkFloatArray::New();
    signal->Allocate(numPeaks*6);
    signal->SetName(m_scalarName.c_str());
    signal->SetNumberOfComponents(1);

    // What we'll return
    vtkUnstructuredGrid *visualDataSet = vtkUnstructuredGrid::New();
    visualDataSet->Allocate(numPeaks*6);
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

      std::cout << "Making peak " << i << " at " << pos << std::endl;

      vtkHexahedron* hexahedron = vtkHexahedron::New();

      vtkIdType id_xyz =    points->InsertNextPoint(x,y,z);
      vtkIdType id_dxyz =   points->InsertNextPoint(dx,y,z);
      vtkIdType id_dxdyz =  points->InsertNextPoint(dx,dy,z);
      vtkIdType id_xdyz =   points->InsertNextPoint(x,dy,z);
      vtkIdType id_xydz =   points->InsertNextPoint(x,y,dz);
      vtkIdType id_dxydz =  points->InsertNextPoint(dx,y,dz);
      vtkIdType id_dxdydz = points->InsertNextPoint(dx,dy,dz);
      vtkIdType id_xdydz =  points->InsertNextPoint(x,dy,dz);

      //create the hexahedron
      vtkHexahedron *theHex = vtkHexahedron::New();
      theHex->GetPointIds()->SetId(0, id_xyz);
      theHex->GetPointIds()->SetId(1, id_dxyz);
      theHex->GetPointIds()->SetId(2, id_dxdyz);
      theHex->GetPointIds()->SetId(3, id_xdyz);
      theHex->GetPointIds()->SetId(4, id_xydz);
      theHex->GetPointIds()->SetId(5, id_dxydz);
      theHex->GetPointIds()->SetId(6, id_dxdydz);
      theHex->GetPointIds()->SetId(7, id_xdydz);

      signal->InsertNextValue(static_cast<float>(i*1.0));

      // Put it in what we'll show
      visualDataSet->InsertNextCell(VTK_HEXAHEDRON, hexahedron->GetPointIds());

      hexahedron->Delete();
    }

    points->Squeeze();
    //TODO: delete points with points->Delete()?
    visualDataSet->Squeeze();
    return visualDataSet;
  }


//
//    const size_t nonIntegratedSize = m_workspace->getNonIntegratedDimensions().size();
//    if(nonIntegratedSize != vtkDataSetFactory::ThreeDimensional)
//    {
//      return m_successor->create();
//    }
//
//    else
//    {
//      const int nBinsX = static_cast<int>( m_workspace->getXDimension()->getNBins() );
//      const int nBinsY = static_cast<int>( m_workspace->getYDimension()->getNBins() );
//      const int nBinsZ = static_cast<int>( m_workspace->getZDimension()->getNBins() );
//
//      const double maxX = m_workspace-> getXDimension()->getMaximum();
//      const double minX = m_workspace-> getXDimension()->getMinimum();
//      const double maxY = m_workspace-> getYDimension()->getMaximum();
//      const double minY = m_workspace-> getYDimension()->getMinimum();
//      const double maxZ = m_workspace-> getZDimension()->getMaximum();
//      const double minZ = m_workspace-> getZDimension()->getMinimum();
//
//      double incrementX = (maxX - minX) / (nBinsX-1);
//      double incrementY = (maxY - minY) / (nBinsY-1);
//      double incrementZ = (maxZ - minZ) / (nBinsZ-1);
//
//      const int imageSize = (nBinsX ) * (nBinsY ) * (nBinsZ );
//      vtkPoints *points = vtkPoints::New();
//      points->Allocate(static_cast<int>(imageSize));
//
//      vtkFloatArray * signal = vtkFloatArray::New();
//      signal->Allocate(imageSize);
//      signal->SetName(m_scalarName.c_str());
//      signal->SetNumberOfComponents(1);
//
//      //The following represent actual calculated positions.
//      double posX, posY, posZ;
//
//      UnstructuredPoint unstructPoint;
//      double signalScalar;
//      const int nPointsX = nBinsX;
//      const int nPointsY = nBinsY;
//      const int nPointsZ = nBinsZ;
//      PointMap pointMap(nPointsX);
//
//      double minSig=1e32;
//      double maxSig=-1e32;
//
//      //Loop through dimensions
//      for (int i = 0; i < nPointsX; i++)
//      {
//        posX = minX + (i * incrementX); //Calculate increment in x;
//        Plane plane(nPointsY);
//        for (int j = 0; j < nPointsY; j++)
//        {
//
//          posY = minY + (j * incrementY); //Calculate increment in y;
//          Column col(nPointsZ);
//          for (int k = 0; k < nPointsZ; k++)
//          {
//
//            posZ = minZ + (k * incrementZ); //Calculate increment in z;
//            signalScalar = m_workspace->getSignalNormalizedAt(i, j, k);
//
//            // Track max/min
//            if (signalScalar > maxSig) maxSig = signalScalar;
//            if (signalScalar < minSig) minSig = signalScalar;
//
//            if (boost::math::isnan( signalScalar ) || (signalScalar < m_minThreshold) || (signalScalar > m_maxThreshold))
//            {
//              //Flagged so that topological and scalar data is not applied.
//              unstructPoint.isSparse = true;
//            }
//            else
//            {
//              if ((i < (nBinsX -1)) && (j < (nBinsY - 1)) && (k < (nBinsZ -1)))
//              {
//                signal->InsertNextValue(static_cast<float>(signalScalar));
//              }
//              unstructPoint.isSparse = false;
//            }
//            unstructPoint.pointId = points->InsertNextPoint(posX, posY, posZ);
//            col[k] = unstructPoint;
//          }
//          plane[j] = col;
//        }
//        pointMap[i] = plane;
//      }
//
//      std::cout << "Min signal was " << minSig << ". Max was " << maxSig << std::endl;
//
//      points->Squeeze();
//      signal->Squeeze();
//
//      vtkUnstructuredGrid *visualDataSet = vtkUnstructuredGrid::New();
//      visualDataSet->Allocate(imageSize);
//      visualDataSet->SetPoints(points);
//      visualDataSet->GetCellData()->SetScalars(signal);
//
//      for (int i = 0; i < nBinsX - 1; i++)
//      {
//        for (int j = 0; j < nBinsY -1; j++)
//        {
//          for (int k = 0; k < nBinsZ -1; k++)
//          {
//            //Only create topologies for those cells which are not sparse.
//            if (!pointMap[i][j][k].isSparse)
//            {
//              // create a hexahedron topology
//              vtkHexahedron* hexahedron = createHexahedron(pointMap, i, j, k);
//              visualDataSet->InsertNextCell(VTK_HEXAHEDRON, hexahedron->GetPointIds());
//              hexahedron->Delete();
//            }
//          }
//        }
//      }
//
//      points->Delete();
//      signal->Delete();
//      visualDataSet->Squeeze();
//      return visualDataSet;
//    }
//  }

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
