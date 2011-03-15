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
#include "MDDataObjects/MDWorkspace.h"
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

template<typename Image, typename TimeMapper>
class DLLExport vtkThresholdingUnstructuredGridFactory: public vtkDataSetFactory
{
public:

  /// Constructor
  vtkThresholdingUnstructuredGridFactory(boost::shared_ptr<Image> image, const std::string& scalarname,
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
  boost::shared_ptr<Image> m_image;

  /// timestep obtained from framework.
  const double m_timestep;

  /// Create a hexahedron.
  inline vtkHexahedron* createHexahedron(PointMap& pointMap, const int& i, const int& j, const int& k) const;

  /// Name of the scalar to provide on mesh.
  const std::string m_scalarName;

  /// Time mapper.
  TimeMapper m_timeMapper;

  /// Threshold for signal value, above which, we do not provide unstructured topologies for.
  const double m_maxThreshold;

  /// Threshold for signal value. below which, we do not provide unstructured topologies for.
  const double m_minThreshold;



};

template<typename Image, typename TimeMapper>
vtkThresholdingUnstructuredGridFactory<Image, TimeMapper>::vtkThresholdingUnstructuredGridFactory(boost::shared_ptr<
    Image> image, const std::string& scalarName, const double timestep, const TimeMapper& timeMapper, double minThreshold, double maxThreshold) :
  m_image(image), m_timestep(timestep), m_scalarName(scalarName), m_timeMapper(timeMapper), m_minThreshold(minThreshold), m_maxThreshold(maxThreshold)
{
}


template<typename Image, typename TimeMapper>
vtkUnstructuredGrid* vtkThresholdingUnstructuredGridFactory<Image, TimeMapper>::create() const
{
  using namespace Mantid::MDDataObjects;
  typename Image::GeometryType const * const pGeometry = m_image->getGeometry();

  const int nBinsX = pGeometry->getXDimension()->getNBins();
  const int nBinsY = pGeometry->getYDimension()->getNBins();
  const int nBinsZ = pGeometry->getZDimension()->getNBins();

  const double maxX = pGeometry-> getXDimension()->getMaximum();
  const double minX = pGeometry-> getXDimension()->getMinimum();
  const double maxY = pGeometry-> getYDimension()->getMaximum();
  const double minY = pGeometry-> getYDimension()->getMinimum();
  const double maxZ = pGeometry-> getZDimension()->getMaximum();
  const double minZ = pGeometry-> getZDimension()->getMinimum();

  double incrementX = (maxX - minX) / nBinsX;
  double incrementY = (maxY - minY) / nBinsY;
  double incrementZ = (maxZ - minZ) / nBinsZ;

  const int imageSize = (nBinsX + 1) * (nBinsY + 1) * (nBinsZ + 1);
  vtkPoints *points = vtkPoints::New();
  points->Allocate(imageSize);

  vtkFloatArray * signal = vtkFloatArray::New();
  signal->Allocate(imageSize);
  signal->SetName(m_scalarName.c_str());
  signal->SetNumberOfComponents(1);

  //The following represent actual calculated positions.
  double posX, posY, posZ;

  UnstructuredPoint unstructPoint;
  double currentXIncrement, currentYIncrement;
  double signalScalar;
  const int nPointsX = nBinsX + 1;
  const int nPointsY = nBinsY + 1;
  const int nPointsZ = nBinsZ + 1;
  PointMap pointMap(nPointsX);

  //Loop through dimensions
  for (int i = 0; i < nPointsX; i++)
  {
    currentXIncrement = i * incrementX;
    Plane plane(nPointsY);
    for (int j = 0; j < nPointsY; j++)
    {
      currentYIncrement = j * incrementY;
      Column col(nPointsX);
      for (int k = 0; k < nPointsZ; k++)
      {
        posX = minX + currentXIncrement; //Calculate increment in x;
        posY = minY + currentYIncrement; //Calculate increment in y;
        posZ = minZ + k * incrementZ; //Calculate increment in z;
        signalScalar = m_image->getPoint(i, j, k, m_timeMapper(m_timestep)).s;

        if ((signalScalar <= m_minThreshold) || (signalScalar >= m_maxThreshold))
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

template<typename Image, typename TimeMapper>
inline vtkHexahedron* vtkThresholdingUnstructuredGridFactory<Image, TimeMapper>::createHexahedron(PointMap& pointMap, const int& i, const int& j, const int& k) const
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


template <typename Image, typename TimeMapper>
vtkThresholdingUnstructuredGridFactory<Image, TimeMapper>::~vtkThresholdingUnstructuredGridFactory()
{
}


}
}

#endif
