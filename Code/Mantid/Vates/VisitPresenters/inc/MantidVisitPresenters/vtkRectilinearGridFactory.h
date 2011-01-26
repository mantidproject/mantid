#ifndef MANTID_VATES_VTKRECTILINEARGRIDFACTORY_H_
#define MANTID_VATES_VTKRECTILINEARGRIDFACTORY_H_

#include "MDDataObjects/MDWorkspace.h"
#include "MantidVisitPresenters/vtkDataSetFactory.h"
#include <vtkRectilinearGrid.h>
#include <vtkCellData.h>
#include <vtkDoubleArray.h>
#include <vtkFloatArray.h>

namespace Mantid
{
namespace VATES
{

template<typename Image>
class vtkRectilinearGridFactory : public vtkDataSetFactory
{
public:

  vtkRectilinearGridFactory(boost::shared_ptr<Image> image, const std::string& scalarName,
      const int timestep);

  ~vtkRectilinearGridFactory();

  /// Covarient Factory Method to generate the required mesh type.
  virtual vtkRectilinearGrid* create() const;

private:

  boost::shared_ptr<Image> m_image;
  std::string m_scalarName;
  int m_timestep;

};

template<typename Image>
vtkRectilinearGridFactory<Image>::~vtkRectilinearGridFactory()
{
}

template<typename Image>
vtkRectilinearGridFactory<Image>::vtkRectilinearGridFactory(boost::shared_ptr<Image> image,
    const std::string& scalarName, const int timestep) :
  m_image(image), m_scalarName(scalarName), m_timestep(timestep)
{
}

template<typename Image>
vtkRectilinearGrid* vtkRectilinearGridFactory<Image>::create() const
{

  using namespace Mantid::MDDataObjects;
  using namespace Mantid::Geometry;

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

  const int nPointsX = nBinsX + 1;
  const int nPointsY = nBinsY + 1;
  const int nPointsZ = nBinsZ + 1;

  vtkDoubleArray* xCoords = vtkDoubleArray::New();
  xCoords->Allocate(nPointsX);
  vtkDoubleArray* yCoords = vtkDoubleArray::New();
  yCoords->SetNumberOfTuples(nPointsY);
  yCoords->Allocate(nPointsY);
  vtkDoubleArray* zCoords = vtkDoubleArray::New();
  zCoords->SetNumberOfTuples(nPointsZ);
  zCoords->Allocate(nPointsZ);

  for (int i = 0; i < nPointsX; i++)
  {
    xCoords->InsertNextValue(minX + (incrementX * i));
  }
  for (int j = 0; j < nPointsY; j++)
  {
    yCoords->InsertNextValue(minY + (incrementY * j));
  }
  for (int k = 0; k < nPointsZ; k++)
  {
    zCoords->InsertNextValue(minZ + (incrementZ * k));
  }
  vtkRectilinearGrid* visualDataSet = vtkRectilinearGrid::New();
  visualDataSet->SetDimensions(nPointsX, nPointsY, nPointsZ);
  visualDataSet->SetXCoordinates(xCoords);
  visualDataSet->SetYCoordinates(yCoords);
  visualDataSet->SetZCoordinates(zCoords);
  visualDataSet->SetExtent(0, nBinsX, 0, nBinsY, 0, nBinsZ);

  vtkFloatArray* scalars = vtkFloatArray::New();

  scalars->Allocate(nBinsX * nBinsY * nBinsZ);
  scalars->SetName(m_scalarName.c_str());

  MD_image_point point;
  for (int i = 0; i < nBinsX; i++)
  {
    for (int j = 0; j < nBinsY; j++)
    {
      for (int k = 0; k < nBinsZ; k++)
      {
        // Create an image from the point data.

        point = m_image->getPoint(i, j, k, m_timestep);
        // Insert scalar data.
        scalars->InsertNextValue(point.s);
      }
    }
  }
  scalars->Squeeze();
  //Attach points to dataset.
  visualDataSet->GetCellData()->AddArray(scalars);
  return visualDataSet;

}
}
}


#endif
