
#ifndef GENERATESTRUCTUREDGRID_H_
#define GENERATESTRUCTUREDGRID_H_

/** Creates a vtkStructuredGrid from a MDImage.

 @author Owen Arnold, Tessella plc
 @date 11/01/2010

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
#include <vtkStructuredGrid.h>
#include <vtkFloatArray.h>
#include <vtkPoints.h>
#include <vtkCellData.h>
#include "MantidVatesAPI/vtkDataSetFactory.h"
#include "MDDataObjects/MDWorkspace.h"

namespace Mantid
{
namespace VATES
{

template<typename TimeMapper>
class DLLExport vtkStructuredGridFactory : public vtkDataSetFactory
{
public:

  /// Constructor
  vtkStructuredGridFactory(Mantid::API::IMDWorkspace_sptr workspace, const std::string& scalarName, const double timeValue, const TimeMapper& timeMapper);

  /// Assignment operator
  vtkStructuredGridFactory& operator=(const vtkStructuredGridFactory<TimeMapper>& other);

  /// Copy constructor.
  vtkStructuredGridFactory(const vtkStructuredGridFactory<TimeMapper>& other);

  /// Destructor
  ~vtkStructuredGridFactory();

  /// Constructional method. Have to explicitly ask for a mesh only version.
  static vtkStructuredGridFactory constructAsMeshOnly(Mantid::API::IMDWorkspace_sptr image, const TimeMapper& timeMapper);

  /// Factory method
  vtkStructuredGrid* create() const;

  /// Generates the geometry of the mesh only.
  vtkStructuredGrid* createMeshOnly() const;

  /// Generates a scalar array for signal.
  vtkFloatArray* createScalarArray() const;

private:

  /// Private constructor for use by constructional static member
  vtkStructuredGridFactory(Mantid::API::IMDWorkspace_sptr workspace, const TimeMapper& timeMapper);

  Mantid::API::IMDWorkspace_sptr m_workspace;
  std::string m_scalarName;
  double m_timeValue;
  bool m_meshOnly;
  TimeMapper m_timeMapper;
};

template<typename TimeMapper>
vtkStructuredGridFactory<TimeMapper>::vtkStructuredGridFactory(Mantid::API::IMDWorkspace_sptr workspace, const std::string& scalarName, const double timeValue, const TimeMapper& timeMapper) :
    m_workspace(workspace), m_scalarName(scalarName), m_timeValue(timeValue), m_meshOnly(false), m_timeMapper(timeMapper)
{
}

template<typename TimeMapper>
vtkStructuredGridFactory<TimeMapper>::vtkStructuredGridFactory(const vtkStructuredGridFactory<TimeMapper>& other):
    m_workspace(other.m_workspace), m_scalarName(other.m_scalarName), m_timeValue(other.m_timeValue), m_meshOnly(other.m_meshOnly), m_timeMapper(other.m_timeMapper)
{
}

template<typename TimeMapper>
vtkStructuredGridFactory<TimeMapper> & vtkStructuredGridFactory<TimeMapper>::operator=(const vtkStructuredGridFactory<TimeMapper>& other)
{
    if (this != &other)
    {
        m_meshOnly = other.m_meshOnly;
        m_scalarName = other.m_scalarName;
        m_timeValue = other.m_timeValue;
        m_workspace = other.m_workspace;
    }
    return *this;
}


template<typename TimeMapper>
vtkStructuredGridFactory<TimeMapper>::vtkStructuredGridFactory(Mantid::API::IMDWorkspace_sptr workspace, const TimeMapper& timeMapper) : m_workspace(workspace), m_scalarName(""), m_timeValue(0),  m_meshOnly(true), m_timeMapper(timeMapper)
{
}

template<typename TimeMapper>
vtkStructuredGridFactory<TimeMapper> vtkStructuredGridFactory<TimeMapper>::constructAsMeshOnly(Mantid::API::IMDWorkspace_sptr workspace, const TimeMapper& timeMapper)
{
   return vtkStructuredGridFactory<TimeMapper>(workspace, timeMapper);
}

template<typename TimeMapper>
vtkStructuredGridFactory<TimeMapper>::~vtkStructuredGridFactory()
{
}

template<typename TimeMapper>
vtkStructuredGrid* vtkStructuredGridFactory<TimeMapper>::create() const
{
  vtkStructuredGrid* visualDataSet = this->createMeshOnly();
  vtkFloatArray* scalarData = this->createScalarArray();
  visualDataSet->GetCellData()->AddArray(scalarData);
  scalarData->Delete();
  return visualDataSet;
}

template<typename TimeMapper>
vtkStructuredGrid* vtkStructuredGridFactory<TimeMapper>::createMeshOnly() const
{
  const int nBinsX = m_workspace->getXDimension()->getNBins();
  const int nBinsY = m_workspace->getYDimension()->getNBins();
  const int nBinsZ = m_workspace->getZDimension()->getNBins();

  const double maxX = m_workspace->getXDimension()->getMaximum();
  const double minX = m_workspace->getXDimension()->getMinimum();
  const double maxY = m_workspace->getYDimension()->getMaximum();
  const double minY = m_workspace->getYDimension()->getMinimum();
  const double maxZ = m_workspace->getZDimension()->getMaximum();
  const double minZ = m_workspace->getZDimension()->getMinimum();

  double incrementX = (maxX - minX) / nBinsX;
  double incrementY = (maxY - minY) / nBinsY;
  double incrementZ = (maxZ - minZ) / nBinsZ;

  const int imageSize = (nBinsX + 1) * (nBinsY + 1) * (nBinsZ + 1);
  vtkStructuredGrid* visualDataSet = vtkStructuredGrid::New();
  vtkPoints *points = vtkPoints::New();
  points->Allocate(imageSize);

  //The following represent actual calculated positions.
  double posX, posY, posZ;
  //Loop through dimensions
  double currentXIncrement, currentYIncrement;

  int nPointsX = nBinsX+1;
  int nPointsY = nBinsY+1;
  int nPointsZ = nBinsZ+1;
  for (int i = 0; i < nPointsX; i++)
  {
    currentXIncrement = i*incrementX;
    for (int j = 0; j < nPointsY; j++)
    {
      currentYIncrement = j*incrementY;
      for (int k = 0; k < nPointsZ; k++)
      {
        posX = minX + currentXIncrement; //Calculate increment in x;
        posY = minY + currentYIncrement; //Calculate increment in y;
        posZ = minZ + k*incrementZ; //Calculate increment in z;
        points->InsertNextPoint(posX, posY, posZ);
      }
    }
  }

  //Attach points to dataset.
  visualDataSet->SetPoints(points);
  visualDataSet->SetDimensions(nPointsZ, nPointsY, nPointsX);
  points->Delete();
  return visualDataSet;
}

template<typename TimeMapper>
vtkFloatArray* vtkStructuredGridFactory<TimeMapper>::createScalarArray() const
{
  if(true == m_meshOnly)
  {
    throw std::runtime_error("This vtkStructuredGridFactory factory has not been constructed with all the information required to create scalar data.");
  }

  using namespace MDDataObjects;

  //Add scalar data to the mesh.
  vtkFloatArray* scalars = vtkFloatArray::New();

  const int sizeX = m_workspace->getXDimension()->getNBins();
  const int sizeY = m_workspace->getYDimension()->getNBins();
  const int sizeZ = m_workspace->getZDimension()->getNBins();
  scalars->Allocate(sizeX * sizeY * sizeZ);
  scalars->SetName(m_scalarName.c_str());
  MD_image_point point;
  for (int i = 0; i < sizeX; i++)
  {
    for (int j = 0; j < sizeY; j++)
    {
      for (int k = 0; k < sizeZ; k++)
      {
        // Create an image from the point data.
        double signal =  m_workspace->getSignalAt(i, j, k, m_timeMapper(m_timeValue));
        // Insert scalar data.
        scalars->InsertNextValue(signal);
      }
    }
  }
  scalars->Squeeze();
  return scalars;
}

}
}



#endif
