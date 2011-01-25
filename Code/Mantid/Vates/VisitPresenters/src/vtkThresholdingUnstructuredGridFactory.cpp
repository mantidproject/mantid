#include "MantidVisitPresenters/vtkThresholdingUnstructuredGridFactory.h"
#include "MDDataObjects/MDWorkspace.h"
#include <vtkUnstructuredGrid.h>
#include <vtkDoubleArray.h>
#include <vtkCellData.h>
#include <vtkHexahedron.h>

namespace Mantid
{
namespace VATES
{

/** Concrete implementation of vtkDataSetFactory. Creates a vtkUnStructuredGrid. Uses Thresholding technique
 * to create sparse representation of data.

 @author Owen Arnold, Tessella plc
 @date 24/01/2010

 Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

 This file is part of Mantid.

 Mantid is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 3 of the License, or
 (at your option) any later version.

 Mantid is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.

 File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>
 Code Documentation is available at: <http://doxygen.mantidproject.org>
 */


using namespace Mantid::MDDataObjects;

/// Helper struct allows recognition of points that we should not bother to draw.
struct UnstructuredPoint
{
  bool isSparse;
  vtkIdType pointId;
};

vtkThresholdingUnstructuredGridFactory::vtkThresholdingUnstructuredGridFactory(
    MDWorkspace_sptr workspace, const std::string& scalarName, const int timestep, double threshold) :
  m_workspace(workspace), m_timestep(timestep), m_scalarName(scalarName), m_threshold(threshold)
{
}

vtkUnstructuredGrid* vtkThresholdingUnstructuredGridFactory::create() const
{
    using namespace Mantid::MDDataObjects;

    const int nBinsX = m_workspace->getXDimension()->getNBins();
    const int nBinsY = m_workspace->getYDimension()->getNBins();
    const int nBinsZ = m_workspace->getZDimension()->getNBins();

    const double maxX = m_workspace-> getXDimension()->getMaximum();
    const double minX = m_workspace-> getXDimension()->getMinimum();
    const double maxY = m_workspace-> getYDimension()->getMaximum();
    const double minY = m_workspace-> getYDimension()->getMinimum();
    const double maxZ = m_workspace-> getZDimension()->getMaximum();
    const double minZ = m_workspace-> getZDimension()->getMinimum();

    double incrementX = (maxX - minX) / nBinsX;
    double incrementY = (maxY - minY) / nBinsY;
    double incrementZ = (maxZ - minZ) / nBinsZ;

    const int imageSize = (nBinsX + 1) * (nBinsY + 1) * (nBinsZ + 1);
    vtkPoints *points = vtkPoints::New();
    points->Allocate(imageSize);

    vtkDoubleArray * signal = vtkDoubleArray::New();
    signal->Allocate(imageSize);
    signal->SetName(m_scalarName.c_str());
    signal->SetNumberOfComponents(1);

    //The following represent actual calculated positions.
    double posX, posY, posZ;
    //Loop through dimensions
    boost::shared_ptr<const Mantid::MDDataObjects::MDImage> spImage = m_workspace->get_spMDImage();
    UnstructuredPoint unstructPoint;
    double currentXIncrement, currentYIncrement;
    double signalScalar;
    const int nPointsX = nBinsX+1;
    const int nPointsY = nBinsY+1;
    const int nPointsZ = nBinsZ+1;
    std::vector<std::vector<std::vector<UnstructuredPoint> > > pointMap(nPointsX);

    for (int i = 0; i < nPointsX; i++)
    {
      currentXIncrement = i * incrementX;
      std::vector<std::vector<UnstructuredPoint> > plane(nPointsY);
      for (int j = 0; j < nPointsY; j++)
      {
        currentYIncrement = j * incrementY;
        std::vector<UnstructuredPoint> col(nPointsX);
        for (int k = 0; k < nPointsZ; k++)
        {
          posX = minX + currentXIncrement; //Calculate increment in x;
          posY = minY + currentYIncrement; //Calculate increment in y;
          posZ = minZ + k * incrementZ; //Calculate increment in z;
          signalScalar = m_workspace->get_spMDImage()->getPoint(i, j, k, m_timestep).s;

          if (signalScalar <= m_threshold)
          {
            //Flagged so that topological and scalar data is not applied.
            unstructPoint.isSparse = true;
          }
          else
          {
            if ((i < nBinsX) && (j < nBinsY) && (k < nBinsZ))
            {
              signal->InsertNextValue(signalScalar);
            }
            unstructPoint.isSparse = false;
          }
          unstructPoint.pointId = points->InsertNextPoint(posX, posY, posZ);
          col[k] = unstructPoint;
        }
        plane[j] = col;
      }
      pointMap[i] = plane;
    }

    points->Squeeze();
    signal->Squeeze();

    vtkUnstructuredGrid *visualDataSet = vtkUnstructuredGrid::New();
    visualDataSet->Allocate(imageSize);
    visualDataSet->SetPoints(points);
    visualDataSet->GetCellData()->SetScalars(signal);

    for (int i = 0; i < nBinsX; i++)
    {
      for (int j = 0; j < nBinsY; j++)
      {
        for (int k = 0; k < nBinsZ; k++)
        {
          //Identify points for hexahedron

          if(!pointMap[i][j][k].isSparse)
          {
          vtkIdType id_xyz = pointMap[i][j][k].pointId;
          vtkIdType id_dxyz = pointMap[i + 1][j][k].pointId;
          vtkIdType id_dxdyz = pointMap[i + 1][j + 1][k].pointId;
          vtkIdType id_xdyz = pointMap[i][j + 1][k].pointId;

          vtkIdType id_xydz = pointMap[i][j][k + 1].pointId;
          vtkIdType id_dxydz = pointMap[i + 1][j][k + 1].pointId;
          vtkIdType id_dxdydz = pointMap[i + 1][j + 1][k + 1].pointId;
          vtkIdType id_xdydz = pointMap[i][j + 1][k + 1].pointId;

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

          visualDataSet->InsertNextCell(VTK_HEXAHEDRON, theHex->GetPointIds());
          }
        }
      }
    }

    points->Delete();
    signal->Delete();
    visualDataSet->Squeeze();
    return visualDataSet;
}

vtkThresholdingUnstructuredGridFactory::~vtkThresholdingUnstructuredGridFactory()
{
}

}
}
