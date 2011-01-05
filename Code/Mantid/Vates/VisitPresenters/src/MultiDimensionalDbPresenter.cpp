#include <MantidVisitPresenters/MultiDimensionalDbPresenter.h>
#include <MantidGeometry/MDGeometry/MDGeometry.h>
#include <MDDataObjects/IMD_FileFormat.h>
#include <MDDataObjects/MD_FileFormatFactory.h>
#include <MDDataObjects/MDWorkspace.h>
#include <MantidMDAlgorithms/CenterpieceRebinning.h>
#include "MantidMDAlgorithms/Load_MDWorkspace.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/WorkspaceFactory.h"
#include <vtkDoubleArray.h>
#include <vtkFieldData.h>
#include <boost/shared_ptr.hpp>
#include <vtkCharArray.h>

namespace Mantid
{
namespace VATES
{
MultiDimensionalDbPresenter::MultiDimensionalDbPresenter() : m_isExecuted(false)
{

}

void MultiDimensionalDbPresenter::execute(const std::string& fileName)
{
  using namespace Mantid::MDAlgorithms;
  using namespace Mantid::MDDataObjects;
  using namespace Mantid::API;

  Load_MDWorkspace wsLoaderAlg;
  wsLoaderAlg.initialize();
  std::string wsId = "InputMDWs";
  wsLoaderAlg.setPropertyValue("inFilename", fileName);
  wsLoaderAlg.setPropertyValue("MDWorkspace",wsId);
  wsLoaderAlg.execute();
  Workspace_sptr result=AnalysisDataService::Instance().retrieve(wsId);
  MDWorkspace_sptr inputWS = boost::dynamic_pointer_cast<MDWorkspace>(result);

  CenterpieceRebinning rebinAlg;
  AnalysisDataService::Instance().add("inputWS", inputWS);
  rebinAlg.initialize();
  rebinAlg.setPropertyValue("Input", "inputWS");
  rebinAlg.setPropertyValue("Result","OutWorkspace");

  //HACK: this is the only way to provide slicing data for centrepiece rebinning code.
  Geometry::MDGeometryDescription *pSlicing = dynamic_cast< Geometry::MDGeometryDescription *>((Mantid::Kernel::Property *)(rebinAlg.getProperty("SlicingData")));
  pSlicing->build_from_geometry(*(inputWS->getGeometry()));


  double r0=0;
   pSlicing->pDimDescription("qx")->cut_min = r0;
pSlicing->pDimDescription("qx")->cut_max = r0+1;
pSlicing->pDimDescription("qy")->cut_min = r0;
pSlicing->pDimDescription("qy")->cut_max = r0+1;
pSlicing->pDimDescription("qz")->cut_min = r0;
pSlicing->pDimDescription("qz")->cut_max = r0+1;
pSlicing->pDimDescription("en")->cut_max = 50;


  rebinAlg.execute();
  this->m_MDWorkspace = boost::dynamic_pointer_cast<MDWorkspace>(AnalysisDataService::Instance().retrieve("OutWorkspace"));

  m_isExecuted = true;
}

void MultiDimensionalDbPresenter::verifyExecution() const
{
  if(!this->m_isExecuted)
  {
    throw std::runtime_error("Cannot get mesh or get variables until rebinning has occured via ::execute()");
  }
}

vtkDataSet* MultiDimensionalDbPresenter::getMesh() const
{
  using namespace Mantid::MDDataObjects;

  //Sanity check. Must run execution sucessfully first.
  verifyExecution();

  const int sizeX = m_MDWorkspace->getXDimension()->getNBins();
  const int sizeY = m_MDWorkspace->getYDimension()->getNBins();
  const int sizeZ = m_MDWorkspace->getZDimension()->getNBins();

  const int imageSize = sizeX * sizeY * sizeZ;
  vtkStructuredGrid* visualDataSet = vtkStructuredGrid::New();
  vtkPoints *points = vtkPoints::New();
  points->Allocate(imageSize);

  //Loop through dimensions
  boost::shared_ptr<const Mantid::MDDataObjects::MDImage> spImage = m_MDWorkspace->get_spMDImage();
  for (int i = 0; i < sizeX; i++)
  {
    for (int j = 0; j < sizeY; j++)
    {
      for (int k = 0; k < sizeZ; k++)
      {
        points->InsertNextPoint(i, j, k);
      }
    }
  }

  //Attach points to dataset.
  visualDataSet->SetPoints(points);
  visualDataSet->SetDimensions(sizeX, sizeY, sizeZ);
  points->Delete();

  vtkFieldData* outputFD = vtkFieldData::New();
  MultiDimensionalDbPresenter::metaDataToFieldData(outputFD, m_MDWorkspace->get_const_MDGeometry().toXMLString() ,"1");
  visualDataSet->SetFieldData(outputFD);
  return visualDataSet;
}

void MultiDimensionalDbPresenter::metaDataToFieldData(vtkFieldData* fieldData, std::string metaData,
    const char* id) const
{
  //clean out existing.
  vtkDataArray* arry = fieldData->GetArray(id);
  if(NULL != arry)
  {
    fieldData->RemoveArray(id);
  }
  //create new.
  vtkCharArray* newArry = vtkCharArray::New();
  newArry->Allocate(metaData.size());
  newArry->SetName(id);
  fieldData->AddArray(newArry);

  for(unsigned int i = 0 ; i < metaData.size(); i++)
  {
    newArry->InsertNextValue(metaData.at(i));
  }
}

int MultiDimensionalDbPresenter::getNumberOfTimesteps() const
{
  verifyExecution();
  return m_MDWorkspace->gettDimension()->getNBins();
}

vtkDataArray* MultiDimensionalDbPresenter::getScalarData(int timeBin) const
{
  using namespace Mantid::MDDataObjects;

  verifyExecution();
  if(timeBin >= getNumberOfTimesteps())
  {
    throw std::range_error("A timestep larger than the range of available timesteps has been requested.");
  }

  boost::shared_ptr<const MDImage> spImage(m_MDWorkspace->get_spMDImage());
  const int sizeX = m_MDWorkspace->getXDimension()->getNBins();
  const int sizeY = m_MDWorkspace->getYDimension()->getNBins();
  const int sizeZ = m_MDWorkspace->getZDimension()->getNBins();

  vtkDoubleArray* scalars = vtkDoubleArray::New();
  scalars->SetName("signal");
  scalars->Allocate((sizeX-1) * (sizeY-1) * (sizeZ-1));

  //Loop through dimensions
  for (int i = 0; i < sizeX-1; i++)
  {
    for (int j = 0; j < sizeY-1; j++)
    {
      for (int k = 0; k < sizeZ-1; k++)
      {
        MD_image_point point = spImage->getPoint(i, j, k, timeBin);
        scalars->InsertNextValue(point.s);
      }
    }
  }

  return scalars;
}

MultiDimensionalDbPresenter::~MultiDimensionalDbPresenter()
{
}

}
}

