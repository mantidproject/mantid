#include "vtkSplatterPlot.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkUnstructuredGridAlgorithm.h"
#include "vtkUnstructuredGrid.h"

#include "MantidMDEvents/LoadMD.h"
#include "MantidNexusCPP/NeXusException.hpp"
#include "MantidVatesAPI/vtkSplatterPlotFactory.h"
#include "MantidVatesAPI/NoThresholdRange.h"
#include "MantidGeometry/MDGeometry/MDGeometryXMLDefinitions.h"
#include "MantidVatesAPI/RebinningCutterXMLDefinitions.h"
#include "MantidVatesAPI/FieldDataToMetadata.h"
#include "MantidVatesAPI/vtkDataSetToWsName.h"
#include "MantidVatesAPI/FilteringUpdateProgressAction.h"
#include "MantidAPI/FrameworkManager.h"


using namespace Mantid::API;
using namespace Mantid::VATES;

vtkCxxRevisionMacro(vtkSplatterPlot, "$Revision: 1.0 $");
vtkStandardNewMacro(vtkSplatterPlot);

vtkSplatterPlot::vtkSplatterPlot()
{
  this->SetNumberOfInputPorts(1);
  this->SetNumberOfOutputPorts(1);
  Mantid::API::FrameworkManager::Instance();
}

vtkSplatterPlot::~vtkSplatterPlot()
{
}

/**
Sets number of points.
@param nPoints : number of points.
*/
void vtkSplatterPlot::SetNumberOfPoints(int nPoints)
{
  if(nPoints >= 0)
  {
    size_t temp = static_cast<size_t>(nPoints);
    if(m_numberPoints != temp)
    {
      m_numberPoints = temp;
      this->Modified();
    }
  }
}


int vtkSplatterPlot::RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *outputVector)
{
  //get the info objects
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  vtkDataSet *output = vtkDataSet::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkDataSet *input = vtkDataSet::SafeDownCast(this->GetInput(0));
  
  std::string wsName = Mantid::VATES::vtkDataSetToWsName::exec(input);

  Workspace_sptr result=AnalysisDataService::Instance().retrieve(wsName);

  std::string scalarName = "signal";
  vtkSplatterPlotFactory vtkGridFactory(ThresholdRange_scptr(new NoThresholdRange), scalarName, m_numberPoints);
  vtkGridFactory.initialize(result);
  
  FilterUpdateProgressAction<vtkSplatterPlot> drawUpdateProgress(this, "Drawing...");
  vtkDataSet* product = vtkGridFactory.create(drawUpdateProgress);
  product->SetFieldData(input->GetFieldData());
  output->ShallowCopy(product);

  return 1;
}

int vtkSplatterPlot::RequestInformation(vtkInformation *request, vtkInformationVector **inputVector, vtkInformationVector *outputVector)
{
  return Superclass::RequestInformation(request, inputVector, outputVector);
}

void vtkSplatterPlot::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

/**
Output the progress information and progress text.
@param : progress
@param : message
*/
void vtkSplatterPlot::updateAlgorithmProgress(double progress, const std::string& message)
{
  this->SetProgress(progress);
  this->SetProgressText(message.c_str());
}


