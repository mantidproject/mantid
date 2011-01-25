#include "MantidVisitPresenters/GenerateStructuredGrid.h"
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

vtkStructuredGrid* GenerateStructuredGrid::create() const
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
  boost::shared_ptr<const Mantid::MDDataObjects::MDImage> spImage = m_workspace->get_spMDImage();
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
  visualDataSet->SetDimensions(nPointsX, nPointsY, nPointsZ);
  points->Delete();
  return visualDataSet;
}

}
}

