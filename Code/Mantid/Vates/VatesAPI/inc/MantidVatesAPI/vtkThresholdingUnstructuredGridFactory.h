#ifndef MANTID_VATES_VTKUNSTRUCTUREDGRIDFACTORY_H_
#define MANTID_VATES_VTKUNSTRUCTUREDGRIDFACTORY_H_

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

#include "MantidVatesAPI/vtkDataSetFactory.h"
#include "MantidAPI/IMDWorkspace.h"
#include <vtkUnstructuredGrid.h>
#include <vtkFloatArray.h>
#include <vtkCellData.h>
#include <vtkHexahedron.h>

namespace Mantid
{
namespace VATES
{

/// Helper struct allows recognition of points that we should not bother to draw.
struct UnstructuredPoint
{
  bool isSparse;
  vtkIdType pointId;
};

template<typename TimeMapper>
class DLLExport vtkThresholdingUnstructuredGridFactory: public vtkDataSetFactory
{
public:

  /// Constructor
  vtkThresholdingUnstructuredGridFactory(Mantid::API::IMDWorkspace_sptr workspace, const std::string& scalarname,
      const double timestep, const TimeMapper& timeMapper, double minThreshold = -10000, double maxThreshold = 10000);

  /// Destructor
  ~vtkThresholdingUnstructuredGridFactory();

  /// Factory method
  vtkUnstructuredGrid* create() const;

  typedef std::vector<std::vector<std::vector<UnstructuredPoint> > > PointMap;

  typedef std::vector<std::vector<UnstructuredPoint> > Plane;

  typedef std::vector<UnstructuredPoint> Column;

private:

  /// Image from which to draw.
  Mantid::API::IMDWorkspace_sptr m_workspace;

  /// timestep obtained from framework.
  const double m_timestep;

  /// Create a hexahedron.
  inline vtkHexahedron* createHexahedron(PointMap& pointMap, const int& i, const int& j, const int& k) const;

  /// Name of the scalar to provide on mesh.
  const std::string m_scalarName;

  /// Time mapper.
  TimeMapper m_timeMapper;

  /// Threshold for signal value. below which, we do not provide unstructured topologies for.
  const double m_minThreshold;

  /// Threshold for signal value, above which, we do not provide unstructured topologies for.
  const double m_maxThreshold;



};

template<typename TimeMapper>
vtkThresholdingUnstructuredGridFactory<TimeMapper>::vtkThresholdingUnstructuredGridFactory(Mantid::API::IMDWorkspace_sptr workspace, const std::string& scalarName, const double timestep, const TimeMapper& timeMapper, double minThreshold, double maxThreshold) :
  m_workspace(workspace), m_timestep(timestep), m_scalarName(scalarName), m_timeMapper(timeMapper), m_minThreshold(minThreshold), m_maxThreshold(maxThreshold)
{
}


template<typename TimeMapper>
vtkUnstructuredGrid* vtkThresholdingUnstructuredGridFactory<TimeMapper>::create() const
{
  const int nBinsX = m_workspace->getXDimension()->getNBins();
  const int nBinsY = m_workspace->getYDimension()->getNBins();
  const int nBinsZ = m_workspace->getZDimension()->getNBins();

  const double maxX = m_workspace-> getXDimension()->getMaximum();
  const double minX = m_workspace-> getXDimension()->getMinimum();
  const double maxY = m_workspace-> getYDimension()->getMaximum();
  const double minY = m_workspace-> getYDimension()->getMinimum();
  const double maxZ = m_workspace-> getZDimension()->getMaximum();
  const double minZ = m_workspace-> getZDimension()->getMinimum();

  double incrementX = (maxX - minX) / (nBinsX-1);
  double incrementY = (maxY - minY) / (nBinsY-1);
  double incrementZ = (maxZ - minZ) / (nBinsZ-1);

  const int imageSize = (nBinsX ) * (nBinsY ) * (nBinsZ );
  vtkPoints *points = vtkPoints::New();
  points->Allocate(imageSize);

  vtkFloatArray * signal = vtkFloatArray::New();
  signal->Allocate(imageSize);
  signal->SetName(m_scalarName.c_str());
  signal->SetNumberOfComponents(1);

  //The following represent actual calculated positions.
  double posX, posY, posZ;

  UnstructuredPoint unstructPoint;
  double signalScalar;
  const int nPointsX = nBinsX;
  const int nPointsY = nBinsY;
  const int nPointsZ = nBinsZ;
  PointMap pointMap(nPointsX);

  //Loop through dimensions
  for (int i = 0; i < nPointsX; i++)
  {
    posX = minX + (i * incrementX); //Calculate increment in x;
    Plane plane(nPointsY);
    for (int j = 0; j < nPointsY; j++)
    {

      posY = minY + (j * incrementY); //Calculate increment in y;
      Column col(nPointsZ);
      for (int k = 0; k < nPointsZ; k++)
      {

        posZ = minZ + (k * incrementZ); //Calculate increment in z;
        signalScalar = m_workspace->getSignalAt(i, j, k, m_timeMapper(m_timestep));

        if ((signalScalar <= m_minThreshold) || (signalScalar >= m_maxThreshold))
        {
          //Flagged so that topological and scalar data is not applied.
          unstructPoint.isSparse = true;
        }
        else
        {
          if ((i < (nBinsX -1)) && (j < (nBinsY - 1)) && (k < (nBinsZ -1)))
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

  for (int i = 0; i < nBinsX - 1; i++)
  {
    for (int j = 0; j < nBinsY -1; j++)
    {
      for (int k = 0; k < nBinsZ -1; k++)
      {
        //Only create topologies for those cells which are not sparse.
        if (!pointMap[i][j][k].isSparse)
        {
          // create a hexahedron topology
          vtkHexahedron* hexahedron = createHexahedron(pointMap, i, j, k);
          visualDataSet->InsertNextCell(VTK_HEXAHEDRON, hexahedron->GetPointIds());
          hexahedron->Delete();
        }
      }
    }
  }

  points->Delete();
  signal->Delete();
  visualDataSet->Squeeze();
  return visualDataSet;
}

template<typename TimeMapper>
inline vtkHexahedron* vtkThresholdingUnstructuredGridFactory<TimeMapper>::createHexahedron(PointMap& pointMap, const int& i, const int& j, const int& k) const
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
  return theHex;
}


template<typename TimeMapper>
vtkThresholdingUnstructuredGridFactory<TimeMapper>::~vtkThresholdingUnstructuredGridFactory()
{
}


}
}

#endif
