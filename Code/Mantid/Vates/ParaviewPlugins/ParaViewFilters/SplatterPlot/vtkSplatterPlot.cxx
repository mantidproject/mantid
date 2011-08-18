#include "vtkSplatterPlot.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkUnstructuredGridAlgorithm.h"
#include "vtkUnstructuredGrid.h"

#include "MantidMDEvents/MDEventWorkspace.h"
#include "MantidMDEvents/LoadMDEW.h"
#include "MantidNexus/NeXusException.hpp"
#include "MantidVatesAPI/vtkSplatterPlotFactory.h"
#include "MantidVatesAPI/NoThresholdRange.h"
#include "MantidGeometry/MDGeometry/MDGeometryXMLDefinitions.h"
#include "MantidVatesAPI/RebinningCutterXMLDefinitions.h"
#include "MantidVatesAPI/FieldDataToMetadata.h"

using namespace Mantid::API;
using namespace Mantid::VATES;

vtkCxxRevisionMacro(vtkSplatterPlot, "$Revision: 1.0 $");
vtkStandardNewMacro(vtkSplatterPlot);

vtkSplatterPlot::vtkSplatterPlot()
{
  this->SetNumberOfInputPorts(1);
  this->SetNumberOfOutputPorts(1);
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
  
  std::string wsName = findExistingWorkspaceName(input, XMLDefinitions::metaDataId().c_str());

  Workspace_sptr result=AnalysisDataService::Instance().retrieve(wsName);

  std::string scalarName = "signal";
  vtkSplatterPlotFactory vtkGridFactory(ThresholdRange_scptr(new NoThresholdRange), scalarName, m_numberPoints);
  vtkGridFactory.initialize(result);

  output->ShallowCopy(vtkGridFactory.create());

  //Relay the field data.
  output->SetFieldData(input->GetFieldData());
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

//TODO move or refactor this.
std::string vtkSplatterPlot::findExistingWorkspaceName(vtkDataSet *inputDataSet, const char* id)
{
  using Mantid::Geometry::MDGeometryXMLDefinitions;
  FieldDataToMetadata convert;
  std::string xmlString = convert(inputDataSet->GetFieldData(), id);

  Poco::XML::DOMParser pParser;
  Poco::XML::Document* pDoc = pParser.parseString(xmlString);
  Poco::XML::Element* pRootElem = pDoc->documentElement();
  Poco::XML::Element* wsNameElem = pRootElem->getChildElement(MDGeometryXMLDefinitions::workspaceNameElementName());
  if(wsNameElem == NULL)
  {
    throw std::runtime_error("The element containing the workspace name must be present.");
  }
  return wsNameElem->innerText();
}

