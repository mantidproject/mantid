#include "MantidVisitPresenters/GenerateStructuredGrid.h"
#include "vtkStructuredGrid.h"
#include "vtkPoints.h"

namespace Mantid
{
namespace VATES
{
using namespace Mantid::MDDataObjects;

GenerateStructuredGrid::GenerateStructuredGrid(MDWorkspace_sptr workspace) : m_workspace(workspace)
{
}

GenerateStructuredGrid::~GenerateStructuredGrid()
{
}

vtkDataSet* GenerateStructuredGrid::execute()
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

  const int imageSize = nBinsX * nBinsY * nBinsZ;
  vtkStructuredGrid* visualDataSet = vtkStructuredGrid::New();
  vtkPoints *points = vtkPoints::New();
  points->Allocate(imageSize);

  //The following represent actual calculated positions.
  double posX, posY, posZ;
  //Loop through dimensions
  boost::shared_ptr<const Mantid::MDDataObjects::MDImage> spImage = m_workspace->get_spMDImage();
  for (int i = 0; i < nBinsX; i++)
  {
    for (int j = 0; j < nBinsY; j++)
    {
      for (int k = 0; k < nBinsZ; k++)
      {
        posX = minX + i*incrementX; //Calculate increment in x;
        posY = minY + j*incrementY; //Calculate increment in y;
        posZ = minZ + k*incrementZ; //Calculate increment in z;
        points->InsertNextPoint(posX, posY, posZ);
      }
    }
  }

  //Attach points to dataset.
  visualDataSet->SetPoints(points);
  visualDataSet->SetDimensions(nBinsX, nBinsY, nBinsZ);
  points->Delete();
  return visualDataSet;
}

}
}

