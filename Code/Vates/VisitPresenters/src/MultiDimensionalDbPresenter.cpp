#include <MantidVisitPresenters/MultiDimensionalDbPresenter.h>
#include <MantidGeometry/MDGeometry/MDGeometry.h>
#include <MDDataObjects/IMD_FileFormat.h>
#include <MDDataObjects/MD_FileFormatFactory.h>
#include <MantidMDAlgorithms/CenterpieceRebinning.h>
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

  CenterpieceRebinning rebinAlg;

  MDWorkspace_sptr inputWS = MDWorkspace_sptr(new MDWorkspace(3));

  boost::shared_ptr<IMD_FileFormat> spFile(MD_FileFormatFactory::getFileReader(fileName.c_str()).release());
  inputWS->load_workspace(spFile);
  AnalysisDataService::Instance().add("inputWS", inputWS);
  rebinAlg.initialize();
  rebinAlg.setPropertyValue("Input", "inputWS");
  rebinAlg.setPropertyValue("FileName", fileName);
  rebinAlg.setPropertyValue("Result","OutWorkspace");

  //HACK: this is the only way to provide slicing data for centrepiece rebinning code.
  Geometry::MDGeometryDescription *pSlicing = dynamic_cast< Geometry::MDGeometryDescription *>((Mantid::Kernel::Property *)(rebinAlg.getProperty("SlicingData")));
  pSlicing->build_from_geometry(*(inputWS->getGeometry()));
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

vtkDataArray* MultiDimensionalDbPresenter::getScalarData() const
{
  using namespace Mantid::MDDataObjects;

  verifyExecution();

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
        MD_image_point point = spImage->getPoint(i, j, k);
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

