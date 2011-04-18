#include "vtkEventNexusReader.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkMath.h"
#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkPointData.h"
#include "vtkTransform.h"
#include "vtkFloatArray.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "MantidMDAlgorithms/PlaneImplicitFunction.h"
#include "MantidVatesAPI/MultiDimensionalDbPresenter.h"
#include "MantidNexus/LoadEventNexus.h"
#include "MantidMDEvents/MDEventWorkspace.h"
#include "MantidMDEvents/BinToMDHistoWorkspace.h"
#include "MantidDataHandling/LoadInstrument.h"
#include "MantidMDEvents/OneStepMDEW.h"

vtkCxxRevisionMacro(vtkEventNexusReader, "$Revision: 1.0 $");
vtkStandardNewMacro(vtkEventNexusReader);

vtkEventNexusReader::vtkEventNexusReader() : m_presenter()
{
  this->FileName = NULL;
  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(1);
}

vtkEventNexusReader::~vtkEventNexusReader()
{
  this->SetFileName(0);
}


int vtkEventNexusReader::RequestData(vtkInformation * vtkNotUsed(request), vtkInformationVector ** vtkNotUsed(inputVector), vtkInformationVector *outputVector)
{
  using namespace Mantid::VATES;
  //get the info objects
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  vtkStructuredGrid *output = vtkStructuredGrid::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  int time;
  if (outInfo->Has(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEPS()))
  {
    // usually only one actual step requested
    time = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEPS())[0];
  }

  RebinningXMLGenerator serializer(LocationNotRequired); //Object handles serialization of meta data.
  vtkStructuredGrid* structuredMesh = vtkStructuredGrid::SafeDownCast(m_presenter.getMesh(serializer));
  structuredMesh->GetCellData()->AddArray(m_presenter.getScalarDataFromTime(time, "signal"));

  int subext[6];
  outInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), subext);

  output->SetExtent(subext);
  output->ShallowCopy(structuredMesh);

  return 1;
}

int vtkEventNexusReader::RequestInformation(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **vtkNotUsed(inputVector),
  vtkInformationVector *outputVector)
{
  using namespace Mantid::API;
  using namespace Mantid::MDEvents;
  vtkInformation *outInfo = outputVector->GetInformationObject(0);
 
  const std::string mdEventWsId = "mdEventWsId";
  const std::string histogrammedWsId = "histogrammedWsId";

  // Make sure the existing workspace is erased, otherwise events get added to it.
  AnalysisDataService::Instance().remove("mdEventWsId");

  Mantid::MDEvents::OneStepMDEW alg;
  alg.initialize();
  alg.setPropertyValue("Filename", this->FileName);
  alg.setPropertyValue("OutputWorkspace", mdEventWsId);
  alg.execute();

  //Note: this throws for me (JZ) for some reason, even though the workspace should be in the analysis data service
//  IMDEventWorkspace_sptr eventMDWorkspace = boost::dynamic_pointer_cast<IMDEventWorkspace>(AnalysisDataService::Instance().retrieve(mdEventWsId));

  BinToMDHistoWorkspace hist_alg;
  hist_alg.initialize();
  hist_alg.setPropertyValue("InputWorkspace", mdEventWsId);
  hist_alg.setPropertyValue("DimX", "Qx, -2.0,2.0, 100"); //TODO: provide via Paraview proxy properties
  hist_alg.setPropertyValue("DimY", "Qy, -2.0,2.0, 100"); //TODO: provide via Paraview proxy properties
  hist_alg.setPropertyValue("DimZ", "Qz, -2.0,2.0, 100"); //TODO: provide via Paraview proxy properties
  hist_alg.setPropertyValue("DimT", "NONE,0.0,10.0, 1");  //TODO: provide via Paraview proxy properties
  hist_alg.setPropertyValue("OutputWorkspace", histogrammedWsId);
  m_presenter.execute(hist_alg, histogrammedWsId);

  int wholeExtent[6];

  Mantid::VATES::VecExtents extents = m_presenter.getExtents();
  wholeExtent[0] = extents[0];
  wholeExtent[1] = extents[1];
  wholeExtent[2] = extents[2];
  wholeExtent[3] = extents[3]; 
  wholeExtent[4] = extents[4];
  wholeExtent[5] = extents[5];

  outInfo->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(),
    wholeExtent, 6);

  std::vector<double> timeStepValues = m_presenter.getTimesteps();

  outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_STEPS(), &timeStepValues[0], static_cast<int>(timeStepValues.size()));
  double timeRange[2];
  timeRange[0] = timeStepValues.front();
  timeRange[1] = timeStepValues.back();

  outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_RANGE(), timeRange, 2);
  return 1;
}

void vtkEventNexusReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

int vtkEventNexusReader::CanReadFile(const char* vtkNotUsed(fname))
{
  return 1; //TODO: Apply checks here.
}
