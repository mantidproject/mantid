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

  /// Constructional method. Have to explicitly ask for a mesh only version.
  static vtkRectilinearGridFactory constructAsMeshOnly(boost::shared_ptr<Image> image);

  /// Covarient Factory Method to generate the required mesh type.
  virtual vtkRectilinearGrid* create() const;

  /// Generates the geometry of the mesh only.
  vtkRectilinearGrid* createMeshOnly() const;

  /// Generates a scalar array for signal.
  vtkFloatArray* createScalarArray() const;

private:

  vtkRectilinearGridFactory(boost::shared_ptr<Image> image);

  boost::shared_ptr<Image> m_image;
  std::string m_scalarName;
  int m_timestep;
  bool m_meshOnly;

};

template<typename Image>
vtkRectilinearGridFactory<Image>::~vtkRectilinearGridFactory()
{
}

template<typename Image>
vtkRectilinearGridFactory<Image>::vtkRectilinearGridFactory(boost::shared_ptr<Image> image,
    const std::string& scalarName, const int timestep) :
  m_image(image), m_scalarName(scalarName), m_timestep(timestep), m_meshOnly(false)
{
}

template<typename Image>
vtkRectilinearGridFactory<Image>::vtkRectilinearGridFactory(boost::shared_ptr<Image> image) : m_image(image), m_scalarName(""), m_timestep(0),  m_meshOnly(true)
{
}

template<typename Image>
vtkRectilinearGridFactory<Image> vtkRectilinearGridFactory<Image>::constructAsMeshOnly(boost::shared_ptr<Image> image)
{
   return vtkRectilinearGridFactory<Image>(image);
}

template<typename Image>
vtkRectilinearGrid* vtkRectilinearGridFactory<Image>::create() const
{
  vtkRectilinearGrid* visualDataSet = this->createMeshOnly();
  vtkFloatArray* scalarData = this->createScalarArray();
  visualDataSet->GetCellData()->AddArray(scalarData);
  scalarData->Delete();
  return visualDataSet;
}

template<typename Image>
vtkRectilinearGrid* vtkRectilinearGridFactory<Image>::createMeshOnly() const
{
  using namespace MDDataObjects;
  //Get geometry nested type information.
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

  vtkRectilinearGrid* visualDataSet = vtkRectilinearGrid::New();
  visualDataSet->SetDimensions(nPointsX, nPointsY, nPointsZ);
  vtkDoubleArray* xCoords = vtkDoubleArray::New();
  vtkDoubleArray* yCoords = vtkDoubleArray::New();
  vtkDoubleArray* zCoords = vtkDoubleArray::New();

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

  visualDataSet->SetXCoordinates(xCoords);
  visualDataSet->SetYCoordinates(yCoords);
  visualDataSet->SetZCoordinates(zCoords);

  return visualDataSet;
}

template<typename Image>
vtkFloatArray* vtkRectilinearGridFactory<Image>::createScalarArray() const
{
  if(true == m_meshOnly)
  {
    throw std::runtime_error("This vtkRectilinearGridFactory factory has not been constructed with all the information required to create scalar data.");
  }

  using namespace MDDataObjects;
    //Get geometry nested type information.
  typename Image::GeometryType const * const pGeometry = m_image->getGeometry();
  vtkFloatArray* scalars = vtkFloatArray::New();

  const int nBinsX = pGeometry->getXDimension()->getNBins();
  const int nBinsY = pGeometry->getYDimension()->getNBins();
  const int nBinsZ = pGeometry->getZDimension()->getNBins();

  scalars->Allocate(nBinsX * nBinsY * nBinsZ);
  scalars->SetName(m_scalarName.c_str());

  MD_image_point point;
  for (int i = 0; i < nBinsX; i++)
  {
    for (int j = 0; j < nBinsY; j++)
    {
      for (int k = 0; k < nBinsZ; k++)
      {
        point = m_image->getPoint(i, j, k, m_timestep);
        // Insert scalar data.
        scalars->InsertNextValue(point.s);
      }
    }
  }
  scalars->Squeeze();
  return scalars;
}


}
}


#endif
