#include "vtkSQWReader.h"
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
#include "MantidMDAlgorithms/Load_MDWorkspace.h"

vtkCxxRevisionMacro(vtkSQWReader, "$Revision: 1.0 $");
vtkStandardNewMacro(vtkSQWReader);

vtkSQWReader::vtkSQWReader() : m_presenter()
{
  this->FileName = NULL;
  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(1);
}

vtkSQWReader::~vtkSQWReader()
{
  this->SetFileName(0);
}


int vtkSQWReader::RequestData(vtkInformation *request, vtkInformationVector **inputVector, vtkInformationVector *outputVector)
{
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

  vtkStructuredGrid* structuredMesh = vtkStructuredGrid::SafeDownCast(m_presenter.getMesh());
  structuredMesh->GetCellData()->AddArray(m_presenter.getScalarDataFromTime(time, "signal"));

  int subext[6];
  outInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), subext);

  output->SetExtent(subext);
  output->ShallowCopy(structuredMesh);

  return 1;
}

int vtkSQWReader::RequestInformation(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **vtkNotUsed(inputVector),
  vtkInformationVector *outputVector)
{
  vtkInformation *outInfo = outputVector->GetInformationObject(0);


  Mantid::MDAlgorithms::Load_MDWorkspace wsLoaderAlg;
  wsLoaderAlg.initialize();
  std::string wsId = "InputMDWs";
  wsLoaderAlg.setPropertyValue("inFilename", FileName);
  wsLoaderAlg.setPropertyValue("MDWorkspace",wsId);
  m_presenter.execute(wsLoaderAlg, wsId);
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

void vtkSQWReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

int vtkSQWReader::CanReadFile(const char* fname)
{
	return 1; //TODO: Apply checks here.
}
