#include "MantidVatesAPI/MultiDimensionalDbPresenter.h"
#include "MantidVatesAPI/RebinningCutterXMLDefinitions.h"
#include "MantidVatesAPI/RebinningXMLGenerator.h"
#include "MantidVatesAPI/vtkStructuredGridFactory.h"
#include "MantidVatesAPI/MetadataToFieldData.h"
#include <MantidGeometry/MDGeometry/MDGeometry.h>
#include <MDDataObjects/IMD_FileFormat.h>
#include <MDDataObjects/MD_FileFormatFactory.h>
#include <MDDataObjects/MDWorkspace.h>
#include <MantidMDAlgorithms/CenterpieceRebinning.h>
#include "MantidMDAlgorithms/Load_MDWorkspace.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/WorkspaceFactory.h"
#include <vtkFloatArray.h>
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
  this->m_MDWorkspace = inputWS;

  m_isExecuted = true;
}

void MultiDimensionalDbPresenter::verifyExecution() const
{
  if(!this->m_isExecuted)
  {
    throw std::runtime_error("Cannot get mesh or get variables until rebinning has occured via ::execute()");
  }
}

std::string MultiDimensionalDbPresenter::getXAxisName() const
{
  //Sanity check. Must run execution sucessfully first.
  verifyExecution();

  return m_MDWorkspace->getGeometry()->getXDimension()->getDimensionId();
}

std::string MultiDimensionalDbPresenter::getYAxisName() const
{
  //Sanity check. Must run execution sucessfully first.
  verifyExecution();

  return m_MDWorkspace->getGeometry()->getYDimension()->getDimensionId();
}

std::string MultiDimensionalDbPresenter::getZAxisName() const
{
  //Sanity check. Must run execution sucessfully first.
  verifyExecution();

  return m_MDWorkspace->getGeometry()->getZDimension()->getDimensionId();
}

vtkDataSet* MultiDimensionalDbPresenter::getMesh() const
{
  using namespace Mantid::MDDataObjects;

  //Sanity check. Must run execution sucessfully first.
  verifyExecution();

  //Create the mesh
  vtkStructuredGridFactory<MDImage> factory =  vtkStructuredGridFactory<MDImage>::constructAsMeshOnly(m_MDWorkspace->get_spMDImage());
  vtkDataSet* visualDataSet = factory.createMeshOnly();

  vtkFieldData* outputFD = vtkFieldData::New();

  //Serialize metadata
  RebinningXMLGenerator serializer;
  serializer.setWorkspaceName(m_MDWorkspace->getName());
  serializer.setWorkspaceLocation(m_MDWorkspace->getWSLocation());
  serializer.setGeometryXML(m_MDWorkspace->get_const_MDGeometry().toXMLString());
  std::string xmlString = serializer.createXMLString();

  //Add metadata to dataset.

  MetadataToFieldData convert;
  convert(outputFD, xmlString, XMLDefinitions::metaDataId().c_str());
  visualDataSet->SetFieldData(outputFD);
  outputFD->Delete();
  return visualDataSet;
}


int MultiDimensionalDbPresenter::getNumberOfTimesteps() const
{
  verifyExecution();
  return m_MDWorkspace->gettDimension()->getNBins();
}

std::vector<int> MultiDimensionalDbPresenter::getCycles() const
{
  verifyExecution();
  std::vector<int> cycles(m_MDWorkspace->gettDimension()->getNBins());
  for(unsigned int i=0; i < cycles.size(); i++)
    {
      cycles[i] = i;
    }
  return cycles;
}

std::vector<double> MultiDimensionalDbPresenter::getTimesteps() const
{
  using namespace Mantid::Geometry;
  verifyExecution();
  boost::shared_ptr<const IMDDimension> tDimension = m_MDWorkspace->gettDimension();
  const double increment = (tDimension->getMaximum() - tDimension->getMinimum())/tDimension->getNBins();
  std::vector<double> times(tDimension->getNBins());
  for(unsigned int i=0; i < tDimension->getNBins(); i++)
  {
    times[i] = tDimension->getMinimum() + (i*increment);
  }
  return times;
}

vtkDataArray* MultiDimensionalDbPresenter::getScalarDataFromTimeBin(int timeBin, const char* scalarName) const
{
  using namespace Mantid::MDDataObjects;

  verifyExecution();
  if(timeBin >= getNumberOfTimesteps())
  {
    throw std::range_error("A timestep larger than the range of available timesteps has been requested.");
  }

  vtkStructuredGridFactory<MDImage> scalarFactory(m_MDWorkspace->get_spMDImage(), std::string(scalarName), timeBin);
  return scalarFactory.createScalarArray();
}

vtkDataArray* MultiDimensionalDbPresenter::getScalarDataFromTime(double time, const char* scalarName) const
{
  using namespace Mantid::MDDataObjects;

  verifyExecution();
  //Find the timebin corresponding to the time value provided.
  std::vector<double> timeBins = this->getTimesteps();
  double timeBin = timeBins.size() - 1; //Assume it is last.
  for(int i = 0; i < timeBins.size()-1; i++)
  {
    double currentTime = timeBins[i];
    double nextTime = timeBins[i+1];
    if(time >= currentTime && time < nextTime)
    {
      timeBin = i;
      break;
    }
  }

  return getScalarDataFromTimeBin(timeBin, scalarName);
}



MultiDimensionalDbPresenter::~MultiDimensionalDbPresenter()
{
}

}
}

